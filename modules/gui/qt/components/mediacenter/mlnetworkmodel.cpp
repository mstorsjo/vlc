/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include "mlnetworkmodel.hpp"

#include "mlhelper.hpp"

namespace {

enum Role {
    NETWORK_NAME = Qt::UserRole + 1,
    NETWORK_MRL,
    NETWORK_INDEXED,
    NETWORK_CANINDEX,
    NETWORK_TYPE,
    NETWORK_PROTOCOL,
};

}

MLNetworkModel::MLNetworkModel( QObject* parent )
    : QAbstractListModel( parent )
    , m_ml( nullptr )
    , m_input( nullptr, &input_Close )
{
}

QVariant MLNetworkModel::data( const QModelIndex& index, int role ) const
{
    if (!m_ctx)
        return {};
    auto idx = index.row();
    if ( idx < 0 || (size_t)idx >= m_items.size() )
        return {};
    const auto& item = m_items[idx];
    switch ( role )
    {
        case NETWORK_NAME:
            return item.name;
        case NETWORK_MRL:
            return item.mainMrl;
        case NETWORK_INDEXED:
            return item.indexed;
        case NETWORK_CANINDEX:
            return item.canBeIndexed;
        case NETWORK_TYPE:
            return item.type;
        case NETWORK_PROTOCOL:
            return item.protocol;
        default:
            return {};
    }
}

QHash<int, QByteArray> MLNetworkModel::roleNames() const
{
    return {
        { NETWORK_NAME, "name" },
        { NETWORK_MRL, "mrl" },
        { NETWORK_INDEXED, "indexed" },
        { NETWORK_CANINDEX, "can_index" },
        { NETWORK_TYPE, "type" },
        { NETWORK_PROTOCOL, "protocol" },
    };
}

int MLNetworkModel::rowCount(const QModelIndex& parent) const
{
    if ( parent.isValid() )
        return 0;
    assert( m_items.size() < INT32_MAX );
    return static_cast<int>( m_items.size() );
}

Qt::ItemFlags MLNetworkModel::flags( const QModelIndex& idx ) const
{
    return QAbstractListModel::flags( idx ) | Qt::ItemIsEditable;
}

bool MLNetworkModel::setData( const QModelIndex& idx, const QVariant& value, int role )
{
    if (!m_ctx)
        return false;

    if ( role != NETWORK_INDEXED )
        return false;
    auto ml = vlc_ml_instance_get( m_ctx->getIntf() );
    assert( ml != nullptr );
    auto enabled = value.toBool();
    assert( m_items[idx.row()].indexed != enabled );
    int res;
    if ( enabled )
        res = vlc_ml_add_folder( ml, qtu( m_items[idx.row()].mainMrl.toString( QUrl::None ) ) );
    else
        res = vlc_ml_remove_folder( ml, qtu( m_items[idx.row()].mainMrl.toString( QUrl::None ) ) );
    m_items[idx.row()].indexed = enabled;
    emit dataChanged(idx, idx, { NETWORK_INDEXED });
    return res == VLC_SUCCESS;
}

void MLNetworkModel::setContext(QmlMainContext* ctx, QUrl parentMrl)
{

    assert(!m_ctx);
    if (ctx) {
        m_ctx = ctx;
        m_parentMrl = parentMrl;
        m_ml = vlc_ml_instance_get( m_ctx->getIntf() );
        if ( m_parentMrl.isEmpty() )
            initializeDeviceDiscovery();
        else
            initializeFolderDiscovery();
    }
}

bool MLNetworkModel::initializeDeviceDiscovery()
{
    static const services_discovery_callbacks cbs = {
        .item_added = &MLNetworkModel::onItemAdded,
        .item_removed = &MLNetworkModel::onItemRemoved,
    };
    services_discovery_owner_t owner = {
        .cbs = &cbs,
        .sys = this
    };
    char** sdLongNames;
    int* categories;
    auto releaser = [](char** ptr) {
        for ( auto i = 0u; ptr[i] != nullptr; ++i )
            free( ptr[i] );
        free( ptr );
    };
    auto sdNames = vlc_sd_GetNames( VLC_OBJECT( m_ctx->getIntf() ), &sdLongNames, &categories );
    if ( sdNames == nullptr )
        return false;
    auto sdNamesPtr = vlc::wrap_carray( sdNames, releaser );
    auto sdLongNamesPtr = vlc::wrap_carray( sdLongNames, releaser );
    auto categoriesPtr = vlc::wrap_carray( categories );
    for ( auto i = 0u; sdNames[i] != nullptr; ++i )
    {
        if ( categories[i] != SD_CAT_LAN )
            continue;
        SdPtr sd{ vlc_sd_Create( VLC_OBJECT( m_ctx->getIntf() ), sdNames[i], &owner ),
                  &vlc_sd_Destroy };
        if ( sd == nullptr )
            continue;
        m_sds.push_back( std::move( sd ) );
    }
    return m_sds.size() > 0;
}

bool MLNetworkModel::initializeFolderDiscovery()
{
    std::unique_ptr<input_item_t, decltype(&input_item_Release)> inputItem{
        input_item_New( qtu( m_parentMrl.toString( QUrl::None ) ), NULL ),
        &input_item_Release
    };
    inputItem->i_preparse_depth = 1;
    if ( inputItem == nullptr )
        return false;
    m_input.reset( input_CreatePreparser( VLC_OBJECT( m_ctx->getIntf() ),
                                          &MLNetworkModel::onInputEvent,
                                          this, inputItem.get() ) );
    if ( m_input == nullptr )
        return false;

    Item item;
    if ( m_parentMrl.path().isEmpty() || m_parentMrl.path() == "/" )
    {
        item.name = QString::fromUtf8(u8"⮤"); //arrow up ^_
        item.mainMrl = QUrl{};
        item.mrls = {QUrl{}};
        item.protocol = m_parentMrl.scheme();
        item.indexed = false;
        item.type = TYPE_DIR;
        item.canBeIndexed = false;
    }
    else
    {
        item.name = QString::fromUtf8(u8"⮤"); //arrow up ^_
        item.mainMrl = m_parentMrl.resolved(QUrl(".."));
        item.mrls = {m_parentMrl.resolved(QUrl(".."))};
        item.protocol = m_parentMrl.scheme();
        item.indexed = false;
        item.type = TYPE_DIR;
        item.canBeIndexed = false;
    }
    beginInsertRows( {}, 0, 0 );
    m_items.push_back(item);
    endInsertRows();

    input_Start( m_input.get() );
    return true;
}

void MLNetworkModel::onItemAdded( input_item_t* parent, input_item_t* p_item,
                                  const char* )
{
    assert( parent == nullptr );

    Item item;
    item.mainMrl = QUrl::fromEncoded( QByteArray{ p_item->psz_uri }.append( '/' ) );
    item.name = qfu(p_item->psz_name);
    item.mrls.push_back( item.mainMrl );
    item.indexed = false;
    item.canBeIndexed = canBeIndexed( item.mainMrl );
    item.type = TYPE_SHARE;
    item.protocol = item.mainMrl.scheme();

    callAsync([this, item = std::move( item )]() mutable {
        auto it = std::find_if( begin( m_items ), end( m_items ), [&item](const Item& i) {
            return QString::compare(item.name , i.name, Qt::CaseInsensitive ) == 0 &&
                    item.mainMrl.scheme() == i.mainMrl.scheme();
        });
        if ( it != end( m_items ) )
        {
            (*it).mrls.push_back( item.mainMrl );
            filterMainMrl( ( *it ), std::distance( begin( m_items ), it ) );
            return;
        }
        if ( item.canBeIndexed == true )
        {
            if ( vlc_ml_is_indexed( m_ml, qtu( item.mainMrl.toString( QUrl::None ) ),
                                    &item.indexed ) != VLC_SUCCESS )
                item.indexed = false;
        }
        beginInsertRows( {}, m_items.size(), m_items.size() );
        m_items.push_back( std::move( item ) );
        endInsertRows();
    });
}

void MLNetworkModel::onItemRemoved( input_item_t* p_item )
{
    input_item_Hold( p_item );
    callAsync([this, p_item]() {
        QUrl itemUri = QUrl::fromEncoded(p_item->psz_uri);
        auto it = std::find_if( begin( m_items ), end( m_items ), [p_item, itemUri](const Item& i) {
            return QString::compare( qfu(p_item->psz_name), i.name, Qt::CaseInsensitive ) == 0 &&
                itemUri.scheme() == i.mainMrl.scheme();
        });
        if ( it == end( m_items ) )
        {
            input_item_Release( p_item );
            return;
        }
        auto mrlIt = std::find_if( begin( (*it).mrls ), end( (*it).mrls),
                                   [itemUri]( const QUrl& mrl ) {
            return mrl == itemUri;
        });
        input_item_Release( p_item );
        if ( mrlIt == end( (*it).mrls ) )
            return;
        (*it).mrls.erase( mrlIt );
        if ( (*it).mrls.empty() == false )
            return;
        auto idx = std::distance( begin( m_items ), it );
        beginRemoveRows({}, idx, idx );
        m_items.erase( it );
        endRemoveRows();
    });
}

void MLNetworkModel::onInputEvent( input_thread_t*, const vlc_input_event* event )
{
    if ( event->type != INPUT_EVENT_SUBITEMS )
        return;

    std::vector<Item> items;
    for ( auto i = 0; i < event->subitems->i_children; ++i )
    {
        auto it = event->subitems->pp_children[i]->p_item;
        Item item;
        item.name = it->psz_name;
        item.protocol = "";
        item.indexed = false;
        item.type = (it->i_type == ITEM_TYPE_DIRECTORY || it->i_type == ITEM_TYPE_NODE) ?
                TYPE_DIR : TYPE_FILE;
        item.mainMrl = item.type == TYPE_DIR ?
                    QUrl::fromEncoded(QByteArray(it->psz_uri).append('/')) :
                    QUrl::fromEncoded(it->psz_uri);

        item.canBeIndexed = canBeIndexed( item.mainMrl );

        if ( item.canBeIndexed == true )
        {
            if ( vlc_ml_is_indexed( m_ml, qtu( item.mainMrl.toString( QUrl::None ) ),
                                    &item.indexed ) != VLC_SUCCESS )
                item.indexed = false;
        }
        items.push_back( std::move( item ) );
    }
    callAsync([this, items = std::move(items)]() {
        beginInsertRows( {}, m_items.size(), m_items.size() + items.size() - 1 );
        std::move( begin( items ), end( items ), std::back_inserter( m_items ) );
        endInsertRows();
    });
}

void MLNetworkModel::onItemAdded( services_discovery_t* sd, input_item_t* parent,
                                  input_item_t* p_item, const char* psz_cat )
{
    MLNetworkModel* self = static_cast<MLNetworkModel*>( sd->owner.sys );
    self->onItemAdded( parent, p_item, psz_cat );
}

void MLNetworkModel::onItemRemoved( services_discovery_t* sd,
                                    input_item_t* p_item )
{
    MLNetworkModel* self = static_cast<MLNetworkModel*>( sd->owner.sys );
    self->onItemRemoved( p_item );
}

void MLNetworkModel::onInputEvent( input_thread_t* input,
                                   const vlc_input_event* event, void* data )
{
    MLNetworkModel* self = static_cast<MLNetworkModel*>( data );
    self->onInputEvent( input, event );
}

bool MLNetworkModel::canBeIndexed(const QUrl& url)
{
    return url.scheme() == "smb" || url.scheme() == "ftp";
}

void MLNetworkModel::filterMainMrl( MLNetworkModel::Item& item , size_t itemIndex )
{
    assert( item.mrls.empty() == false );
    if ( item.mrls.size() == 1 )
        return;

    //maybe we should rather use QHostAddress, but this adds a dependency uppon QNetwork that we don't require at the moment
    //https://stackoverflow.com/a/17871737/148173
    QRegExp ipv4("((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])");
    QRegExp ipv6("(([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|fe80:(:[0-9a-fA-F]{0,4}){0,4}%[0-9a-zA-Z]{1,}|::(ffff(:0{1,4}){0,1}:){0,1}((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}:((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9]))");

    // We're looking for the mrl which is a (netbios) name, not an IP
    for ( const auto& mrl : item.mrls )
    {
        if (mrl.isEmpty() == true || mrl.scheme() == "")
            continue;

        QString host = mrl.host();
        if (ipv4.exactMatch(host) || ipv6.exactMatch(host))
            continue;

        item.mainMrl = mrl;
        item.canBeIndexed = canBeIndexed( mrl  );
        auto idx = index( static_cast<int>( itemIndex ), 0 );
        emit dataChanged( idx, idx, { NETWORK_MRL, NETWORK_CANINDEX } );
        return;
    }
    // If we can't get a cannonical name, don't attempt to index this as we
    // would fail to get a unique associated device in the medialibrary
    item.canBeIndexed = false;
    auto idx = index( static_cast<int>( itemIndex ), 0 );
    emit dataChanged( idx, idx, { NETWORK_CANINDEX } );
}

