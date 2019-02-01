/*****************************************************************************
 * medialib.cpp: medialibrary module
 *****************************************************************************
 * Copyright © 2008-2018 VLC authors, VideoLAN and VideoLabs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_url.h>
#include <vlc_media_library.h>
#include "medialibrary.h"
#include "fs/fs.h"

#include <medialibrary/IMedia.h>
#include <medialibrary/IAlbumTrack.h>
#include <medialibrary/IAlbum.h>
#include <medialibrary/IArtist.h>
#include <medialibrary/IGenre.h>
#include <medialibrary/IMetadata.h>
#include <medialibrary/IShow.h>
#include <medialibrary/IPlaylist.h>

#include <sstream>
#include <initializer_list>

class Logger : public medialibrary::ILogger
{
public:
    Logger( vlc_object_t* obj ) : m_obj( obj ) {}

private:
    virtual void Error( const std::string& msg ) override
    {
        msg_Err( m_obj, "%s", msg.c_str() );
    }
    virtual void Warning( const std::string& msg ) override
    {
        msg_Warn( m_obj, "%s", msg.c_str() );
    }
    virtual void Info( const std::string& msg ) override
    {
        msg_Dbg( m_obj, "%s", msg.c_str() );
    }
    virtual void Debug( const std::string& msg ) override
    {
        msg_Dbg( m_obj, "%s", msg.c_str() );
    }

private:
    vlc_object_t* m_obj;
};

namespace
{

void assignToEvent( vlc_ml_event_t* ev, vlc_ml_media_t* m )    { ev->modification.p_media    = m; }
void assignToEvent( vlc_ml_event_t* ev, vlc_ml_artist_t* a )   { ev->modification.p_artist   = a; }
void assignToEvent( vlc_ml_event_t* ev, vlc_ml_album_t* a )    { ev->modification.p_album    = a; }
void assignToEvent( vlc_ml_event_t* ev, vlc_ml_genre_t* g )    { ev->modification.p_genre    = g; }
void assignToEvent( vlc_ml_event_t* ev, vlc_ml_playlist_t* p ) { ev->modification.p_playlist = p; }

template <typename To, typename From>
void wrapEntityEventCallback( vlc_medialibrary_module_t* ml,
                              const std::vector<From>& entities, vlc_ml_event_type evType )
{
    vlc_ml_event_t ev;
    ev.i_type = evType;
    for ( const auto& e : entities )
    {
        auto val = vlc::wrap_cptr<To>( static_cast<To*>( malloc( sizeof( To ) ) ),
                                       static_cast<void(*)(To*)>( vlc_ml_release ) );
        if ( unlikely( val == nullptr ) )
            return;
        if ( Convert( e.get(), *val ) == false )
            return;
        assignToEvent( &ev, val.get() );
        ml->cbs->pf_send_event( ml, &ev );
    }
}

void wrapEntityDeletedEventCallback( vlc_medialibrary_module_t* ml,
                                     const std::vector<int64_t>& ids, vlc_ml_event_type evType )
{
    vlc_ml_event_t ev;
    ev.i_type = evType;
    for ( const auto& id : ids )
    {
        ev.deletion.i_entity_id = id;
        ml->cbs->pf_send_event( ml, &ev );
    }
}

} // end of anonymous namespace

void MediaLibrary::onMediaAdded( std::vector<medialibrary::MediaPtr> media )
{
    wrapEntityEventCallback<vlc_ml_media_t>( m_vlc_ml, media, VLC_ML_EVENT_MEDIA_ADDED );
}

void MediaLibrary::onMediaModified( std::vector<medialibrary::MediaPtr> media )
{
    wrapEntityEventCallback<vlc_ml_media_t>( m_vlc_ml, media, VLC_ML_EVENT_MEDIA_UPDATED );
}

void MediaLibrary::onMediaDeleted( std::vector<int64_t> mediaIds )
{
    wrapEntityDeletedEventCallback( m_vlc_ml, mediaIds, VLC_ML_EVENT_MEDIA_DELETED );
}

void MediaLibrary::onArtistsAdded( std::vector<medialibrary::ArtistPtr> artists )
{
    wrapEntityEventCallback<vlc_ml_artist_t>( m_vlc_ml, artists, VLC_ML_EVENT_ARTIST_ADDED );
}

void MediaLibrary::onArtistsModified( std::vector<medialibrary::ArtistPtr> artists )
{
    wrapEntityEventCallback<vlc_ml_artist_t>( m_vlc_ml, artists, VLC_ML_EVENT_ARTIST_UPDATED );
}

void MediaLibrary::onArtistsDeleted( std::vector<int64_t> artistIds )
{
    wrapEntityDeletedEventCallback( m_vlc_ml, artistIds, VLC_ML_EVENT_ARTIST_DELETED );
}

void MediaLibrary::onAlbumsAdded( std::vector<medialibrary::AlbumPtr> albums )
{
    wrapEntityEventCallback<vlc_ml_album_t>( m_vlc_ml, albums, VLC_ML_EVENT_ALBUM_ADDED );
}

void MediaLibrary::onAlbumsModified( std::vector<medialibrary::AlbumPtr> albums )
{
    wrapEntityEventCallback<vlc_ml_album_t>( m_vlc_ml, albums, VLC_ML_EVENT_ALBUM_UPDATED );
}

void MediaLibrary::onAlbumsDeleted( std::vector<int64_t> albumIds )
{
    wrapEntityDeletedEventCallback( m_vlc_ml, albumIds, VLC_ML_EVENT_ALBUM_DELETED );
}

void MediaLibrary::onPlaylistsAdded( std::vector<medialibrary::PlaylistPtr> playlists )
{
    wrapEntityEventCallback<vlc_ml_playlist_t>( m_vlc_ml, playlists, VLC_ML_EVENT_PLAYLIST_ADDED );
}

void MediaLibrary::onPlaylistsModified( std::vector<medialibrary::PlaylistPtr> playlists )
{
    wrapEntityEventCallback<vlc_ml_playlist_t>( m_vlc_ml, playlists, VLC_ML_EVENT_PLAYLIST_UPDATED );
}

void MediaLibrary::onPlaylistsDeleted( std::vector<int64_t> playlistIds )
{
    wrapEntityDeletedEventCallback( m_vlc_ml, playlistIds, VLC_ML_EVENT_PLAYLIST_DELETED );
}

void MediaLibrary::onGenresAdded( std::vector<medialibrary::GenrePtr> genres )
{
    wrapEntityEventCallback<vlc_ml_genre_t>( m_vlc_ml, genres, VLC_ML_EVENT_GENRE_ADDED );
}

void MediaLibrary::onGenresModified( std::vector<medialibrary::GenrePtr> genres )
{
    wrapEntityEventCallback<vlc_ml_genre_t>( m_vlc_ml, genres, VLC_ML_EVENT_GENRE_UPDATED );
}

void MediaLibrary::onGenresDeleted( std::vector<int64_t> genreIds )
{
    wrapEntityDeletedEventCallback( m_vlc_ml, genreIds, VLC_ML_EVENT_GENRE_DELETED );
}

void MediaLibrary::onDiscoveryStarted( const std::string& entryPoint )
{
    vlc_ml_event_t ev;
    ev.i_type = VLC_ML_EVENT_DISCOVERY_STARTED;
    ev.discovery_started.psz_entry_point = entryPoint.c_str();
    m_vlc_ml->cbs->pf_send_event( m_vlc_ml, &ev );
}

void MediaLibrary::onDiscoveryProgress( const std::string& entryPoint )
{
    vlc_ml_event_t ev;
    ev.i_type = VLC_ML_EVENT_DISCOVERY_PROGRESS;
    ev.discovery_progress.psz_entry_point = entryPoint.c_str();
    m_vlc_ml->cbs->pf_send_event( m_vlc_ml, &ev );
}

void MediaLibrary::onDiscoveryCompleted( const std::string& entryPoint, bool success )
{
    vlc_ml_event_t ev;
    ev.i_type = VLC_ML_EVENT_DISCOVERY_COMPLETED;
    ev.discovery_completed.psz_entry_point = entryPoint.c_str();
    ev.discovery_completed.b_success = success;
    m_vlc_ml->cbs->pf_send_event( m_vlc_ml, &ev );
}

void MediaLibrary::onReloadStarted( const std::string& entryPoint )
{
    vlc_ml_event_t ev;
    ev.i_type = VLC_ML_EVENT_RELOAD_STARTED;
    ev.reload_started.psz_entry_point = entryPoint.c_str();
    m_vlc_ml->cbs->pf_send_event( m_vlc_ml, &ev );
}

void MediaLibrary::onReloadCompleted( const std::string& entryPoint, bool success )
{
    vlc_ml_event_t ev;
    ev.i_type = VLC_ML_EVENT_RELOAD_COMPLETED;
    ev.reload_completed.psz_entry_point = entryPoint.c_str();
    ev.reload_completed.b_success = success;
    m_vlc_ml->cbs->pf_send_event( m_vlc_ml, &ev );
}

void MediaLibrary::onEntryPointRemoved( const std::string& entryPoint, bool success )
{
    vlc_ml_event_t ev;
    ev.i_type = VLC_ML_EVENT_ENTRY_POINT_REMOVED;
    ev.entry_point_removed.psz_entry_point = entryPoint.c_str();
    ev.entry_point_removed.b_success = success;
    m_vlc_ml->cbs->pf_send_event( m_vlc_ml, &ev );
}

void MediaLibrary::onEntryPointBanned( const std::string& entryPoint, bool success )
{
    vlc_ml_event_t ev;
    ev.i_type = VLC_ML_EVENT_ENTRY_POINT_BANNED;
    ev.entry_point_banned.psz_entry_point = entryPoint.c_str();
    ev.entry_point_banned.b_success = success;
    m_vlc_ml->cbs->pf_send_event( m_vlc_ml, &ev );
}

void MediaLibrary::onEntryPointUnbanned( const std::string& entryPoint, bool success )
{
    vlc_ml_event_t ev;
    ev.i_type = VLC_ML_EVENT_ENTRY_POINT_UNBANNED;
    ev.entry_point_unbanned.psz_entry_point = entryPoint.c_str();
    ev.entry_point_unbanned.b_success = success;
    m_vlc_ml->cbs->pf_send_event( m_vlc_ml, &ev );
}

void MediaLibrary::onParsingStatsUpdated( uint32_t progress )
{
    vlc_ml_event_t ev;
    ev.i_type = VLC_ML_EVENT_PARSING_PROGRESS_UPDATED;
    ev.parsing_progress.i_percent = progress;
    m_vlc_ml->cbs->pf_send_event( m_vlc_ml, &ev );
}

void MediaLibrary::onBackgroundTasksIdleChanged( bool idle )
{
    vlc_ml_event_t ev;
    ev.i_type = VLC_ML_EVENT_BACKGROUND_IDLE_CHANGED;
    ev.background_idle_changed.b_idle = idle;
    m_vlc_ml->cbs->pf_send_event( m_vlc_ml, &ev );
}

void MediaLibrary::onMediaThumbnailReady( medialibrary::MediaPtr media, bool success )
{
    vlc_ml_event_t ev;
    ev.i_type = VLC_ML_EVENT_MEDIA_THUMBNAIL_GENERATED;
    ev.media_thumbnail_generated.b_success = success;
    auto mPtr = vlc::wrap_cptr<vlc_ml_media_t>(
                static_cast<vlc_ml_media_t*>( malloc( sizeof( vlc_ml_media_t ) ) ),
                vlc_ml_media_release );
    if ( unlikely( mPtr == nullptr ) )
        return;
    ev.media_thumbnail_generated.p_media = mPtr.get();
    if ( Convert( media.get(), *mPtr ) == false )
        return;
    m_vlc_ml->cbs->pf_send_event( m_vlc_ml, &ev );
}

MediaLibrary::MediaLibrary( vlc_medialibrary_module_t* ml )
    : m_vlc_ml( ml )
{
}

bool MediaLibrary::Start()
{
    if ( m_ml != nullptr )
        return true;

    std::unique_ptr<medialibrary::IMediaLibrary> ml( NewMediaLibrary() );

    m_logger.reset( new Logger( VLC_OBJECT( m_vlc_ml ) ) );
    ml->setVerbosity( medialibrary::LogLevel::Info );
    ml->setLogger( m_logger.get() );

    auto userDir = vlc::wrap_cptr( config_GetUserDir( VLC_USERDATA_DIR ) );
    std::string mlDir = std::string{ userDir.get() } + "/ml/";
    auto thumbnailsDir = mlDir + "thumbnails/";

    auto initStatus = ml->initialize( mlDir + "ml.db", thumbnailsDir, this );
    switch ( initStatus )
    {
        case medialibrary::InitializeResult::AlreadyInitialized:
            msg_Info( m_vlc_ml, "MediaLibrary was already initialized" );
            return true;
        case medialibrary::InitializeResult::Failed:
            msg_Err( m_vlc_ml, "Medialibrary failed to initialize" );
            return false;
        case medialibrary::InitializeResult::DbReset:
            msg_Info( m_vlc_ml, "Database was reset" );
            break;
        case medialibrary::InitializeResult::Success:
            msg_Dbg( m_vlc_ml, "MediaLibrary successfully initialized" );
            break;
    }

    ml->addParserService( std::make_shared<MetadataExtractor>( VLC_OBJECT( m_vlc_ml ) ) );
    try
    {
        ml->addThumbnailer( std::make_shared<Thumbnailer>(
                                m_vlc_ml, std::move( thumbnailsDir ) ) );
    }
    catch ( const std::runtime_error& ex )
    {
        msg_Err( m_vlc_ml, "Failed to provide a thumbnailer module to the "
                 "medialib: %s", ex.what() );
        return false;
    }

    auto networkFs = std::make_shared<vlc::medialibrary::SDFileSystemFactory>( VLC_OBJECT( m_vlc_ml ), "smb://");
    ml->addNetworkFileSystemFactory( networkFs );
    // Disabled by default for now
    ml->setDiscoverNetworkEnabled( false );

    if ( ml->start() == false )
    {
        msg_Err( m_vlc_ml, "Failed to start the MediaLibrary" );
        return false;
    }

    // Reload entry points we already know about, and then add potential new ones.
    // Doing it the other way around would cause the initial scan to be performed
    // twice, as we start discovering the new folders, then reload them.
    ml->reload();

    auto folders = vlc::wrap_cptr( var_InheritString( m_vlc_ml, "ml-folders" ) );
    if ( folders != nullptr && strlen( folders.get() ) > 0 )
    {
        std::istringstream ss( folders.get() );
        std::string folder;
        while ( std::getline( ss, folder, ';' ) )
            ml->discover( folder );
    }
    else
    {
        std::string varValue;
        for( auto&& target : { VLC_VIDEOS_DIR, VLC_MUSIC_DIR } )
        {
            auto folder = vlc::wrap_cptr( config_GetUserDir( target ) );
            if( folder == nullptr )
                continue;
            auto folderMrl = vlc::wrap_cptr( vlc_path2uri( folder.get(), nullptr ) );
            ml->discover( folderMrl.get() );
            varValue += std::string{ ";" } + folderMrl.get();
        }
        if ( varValue.empty() == false )
            config_PutPsz( "ml-folders", varValue.c_str()+1 ); /* skip initial ';' */
    }
    m_ml = std::move( ml );
    return true;
}

int MediaLibrary::Control( int query, va_list args )
{
    if ( Start() == false )
        return VLC_EGENERIC;

    switch ( query )
    {
        case VLC_ML_ADD_FOLDER:
        case VLC_ML_REMOVE_FOLDER:
        case VLC_ML_BAN_FOLDER:
        case VLC_ML_UNBAN_FOLDER:
        {
            const char* mrl = va_arg( args, const char* );
            switch( query )
            {
                case VLC_ML_ADD_FOLDER:
                    m_ml->discover( mrl );
                    break;
                case VLC_ML_REMOVE_FOLDER:
                    m_ml->removeEntryPoint( mrl );
                    break;
                case VLC_ML_BAN_FOLDER:
                    m_ml->banFolder( mrl );
                    break;
                case VLC_ML_UNBAN_FOLDER:
                    m_ml->unbanFolder( mrl );
                    break;
            }
            break;
        }
        case VLC_ML_LIST_FOLDERS:
        {
            auto entryPoints = m_ml->entryPoints()->all();
            auto res = ml_convert_list<vlc_ml_entry_point_list_t,
                                         vlc_ml_entry_point_t>( entryPoints );
            *(va_arg( args, vlc_ml_entry_point_list_t**) ) = res;
            break;
        }
        case VLC_ML_IS_INDEXED:
        {
            auto mrl = va_arg( args, const char* );
            auto res = va_arg( args, bool* );
            *res = m_ml->isIndexed( mrl );
            break;
        }
        case VLC_ML_RELOAD_FOLDER:
        {
            auto mrl = va_arg( args, const char* );
            if ( mrl == nullptr )
                m_ml->reload();
            else
                m_ml->reload( mrl );
            break;
        }
        case VLC_ML_PAUSE_BACKGROUND:
            m_ml->pauseBackgroundOperations();
            break;
        case VLC_ML_RESUME_BACKGROUND:
            m_ml->resumeBackgroundOperations();
            break;
        case VLC_ML_CLEAR_HISTORY:
            m_ml->clearHistory();
            break;
        case VLC_ML_NEW_EXTERNAL_MEDIA:
        {
            auto mrl = va_arg( args, const char* );
            auto media = m_ml->addExternalMedia( mrl );
            if ( media == nullptr )
                return VLC_EGENERIC;
            *va_arg( args, vlc_ml_media_t**) = CreateAndConvert<vlc_ml_media_t>( media.get() );
            return VLC_SUCCESS;
        }
        case VLC_ML_NEW_STREAM:
        {
            auto mrl = va_arg( args, const char* );
            auto media = m_ml->addStream( mrl );
            if ( media == nullptr )
                return VLC_EGENERIC;
            *va_arg( args, vlc_ml_media_t**) = CreateAndConvert<vlc_ml_media_t>( media.get() );
            return VLC_SUCCESS;
        }
        case VLC_ML_MEDIA_INCREASE_PLAY_COUNT:
        case VLC_ML_MEDIA_GET_MEDIA_PLAYBACK_PREF:
        case VLC_ML_MEDIA_SET_MEDIA_PLAYBACK_PREF:
        case VLC_ML_MEDIA_SET_THUMBNAIL:
        case VLC_ML_MEDIA_GENERATE_THUMBNAIL:
        case VLC_ML_MEDIA_ADD_EXTERNAL_MRL:
            return controlMedia( query, args );
        default:
            return VLC_EGENERIC;
    }
    return VLC_SUCCESS;
}

int MediaLibrary::List( int listQuery, const vlc_ml_query_params_t* params, va_list args )
{
    if ( Start() == false )
        return VLC_EGENERIC;

    medialibrary::QueryParameters p{};
    medialibrary::QueryParameters* paramsPtr = nullptr;
    uint32_t nbItems = 0;
    uint32_t offset = 0;
    const char* psz_pattern = nullptr;
    if ( params )
    {
        p.desc = params->b_desc;
        p.sort = sortingCriteria( params->i_sort );
        nbItems = params->i_nbResults;
        offset = params->i_offset;
        psz_pattern = params->psz_pattern;
        paramsPtr = &p;
    }
    switch ( listQuery )
    {
        case VLC_ML_LIST_MEDIA_OF:
        case VLC_ML_COUNT_MEDIA_OF:
        case VLC_ML_LIST_ARTISTS_OF:
        case VLC_ML_COUNT_ARTISTS_OF:
        case VLC_ML_LIST_ALBUMS_OF:
        case VLC_ML_COUNT_ALBUMS_OF:
        {
            auto parentType = va_arg( args, int );
            listQuery = filterListChildrenQuery( listQuery, parentType );
        }
        default:
            break;
    }
    switch( listQuery )
    {
        case VLC_ML_LIST_ALBUM_TRACKS:
        case VLC_ML_COUNT_ALBUM_TRACKS:
        case VLC_ML_LIST_ALBUM_ARTISTS:
        case VLC_ML_COUNT_ALBUM_ARTISTS:
            return listAlbums( listQuery, paramsPtr, psz_pattern, nbItems, offset, args );

        case VLC_ML_LIST_ARTIST_ALBUMS:
        case VLC_ML_COUNT_ARTIST_ALBUMS:
        case VLC_ML_LIST_ARTIST_TRACKS:
        case VLC_ML_COUNT_ARTIST_TRACKS:
            return listArtists( listQuery, paramsPtr, psz_pattern, nbItems, offset, args );

        case VLC_ML_LIST_VIDEOS:
        {
            medialibrary::Query<medialibrary::IMedia> query;
            if ( psz_pattern != nullptr )
                query = m_ml->searchVideo( psz_pattern, paramsPtr );
            else
                query = m_ml->videoFiles( paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            auto res = ml_convert_list<vlc_ml_media_list_t, vlc_ml_media_t>(
                        query->items( nbItems, offset ) );
            *va_arg( args, vlc_ml_media_list_t**) = res;
            break;
        }
        case VLC_ML_COUNT_VIDEOS:
        {
            medialibrary::Query<medialibrary::IMedia> query;
            if ( psz_pattern != nullptr )
                query = m_ml->searchVideo( psz_pattern, paramsPtr );
            else
                query = m_ml->videoFiles( paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            *va_arg( args, size_t* ) = query->count();
            break;
        }
        case VLC_ML_LIST_AUDIOS:
        {
            medialibrary::Query<medialibrary::IMedia> query;
            if ( psz_pattern != nullptr )
                query = m_ml->searchAudio( psz_pattern, paramsPtr );
            else
                query = m_ml->audioFiles( paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            auto res = ml_convert_list<vlc_ml_media_list_t, vlc_ml_media_t>(
                        query->items( nbItems, offset ) );
            *va_arg( args, vlc_ml_media_list_t**) = res;
            break;
        }
        case VLC_ML_COUNT_AUDIOS:
        {
            medialibrary::Query<medialibrary::IMedia> query;
            if ( psz_pattern != nullptr )
                query = m_ml->searchAudio( psz_pattern, paramsPtr );
            else
                query = m_ml->audioFiles( paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            *va_arg( args, size_t* ) = query->count();
            break;
        }
        case VLC_ML_LIST_ALBUMS:
        {
            medialibrary::Query<medialibrary::IAlbum> query;
            if ( psz_pattern != nullptr )
                query = m_ml->searchAlbums( psz_pattern, paramsPtr );
            else
                query = m_ml->albums( paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            auto res = ml_convert_list<vlc_ml_album_list_t, vlc_ml_album_t>(
                        query->items( nbItems, offset ) );
            *va_arg( args, vlc_ml_album_list_t**) = res;
            break;
        }
        case VLC_ML_COUNT_ALBUMS:
        {
            medialibrary::Query<medialibrary::IAlbum> query;
            if ( psz_pattern != nullptr )
                query = m_ml->searchAlbums( psz_pattern, paramsPtr );
            else
                query = m_ml->albums( paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            *va_arg( args, size_t* ) = query->count();
            break;
        }
        case VLC_ML_LIST_GENRES:
        {
            medialibrary::Query<medialibrary::IGenre> query;
            if ( psz_pattern != nullptr )
                query = m_ml->searchGenre( psz_pattern, paramsPtr );
            else
                query = m_ml->genres( paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            auto res = ml_convert_list<vlc_ml_genre_list_t,vlc_ml_genre_t>(
                        query->items( nbItems, offset ) );
            *va_arg( args, vlc_ml_genre_list_t**) = res;
            break;
        }
        case VLC_ML_COUNT_GENRES:
        {
            medialibrary::Query<medialibrary::IGenre> query;
            if ( psz_pattern != nullptr )
                query = m_ml->searchGenre( psz_pattern, paramsPtr );
            else
                query = m_ml->genres( paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            *va_arg( args, size_t* ) = query->count();
            break;
        }
        case VLC_ML_LIST_ARTISTS:
        {
            medialibrary::Query<medialibrary::IArtist> query;
            bool includeAll = va_arg( args, int ) != 0;
            if ( psz_pattern != nullptr )
                query = m_ml->searchArtists( psz_pattern, paramsPtr );
            else
                query = m_ml->artists( includeAll, paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            auto res = ml_convert_list<vlc_ml_artist_list_t, vlc_ml_artist_t>(
                        query->items( nbItems, offset ) );
            *va_arg( args, vlc_ml_artist_list_t**) = res;
            break;
        }
        case VLC_ML_COUNT_ARTISTS:
        {
            medialibrary::Query<medialibrary::IArtist> query;
            bool includeAll = va_arg( args, int ) != 0;
            if ( psz_pattern != nullptr )
                query = m_ml->searchArtists( psz_pattern, paramsPtr );
            else
                query = m_ml->artists( includeAll, paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            *va_arg( args, size_t* ) = query->count();
            break;
        }
        case VLC_ML_LIST_GENRE_ARTISTS:
        case VLC_ML_COUNT_GENRE_ARTISTS:
        case VLC_ML_LIST_GENRE_TRACKS:
        case VLC_ML_COUNT_GENRE_TRACKS:
        case VLC_ML_LIST_GENRE_ALBUMS:
        case VLC_ML_COUNT_GENRE_ALBUMS:
            return listGenre( listQuery, paramsPtr, psz_pattern, nbItems, offset, args );

        case VLC_ML_LIST_MEDIA_LABELS:
        case VLC_ML_COUNT_MEDIA_LABELS:
        {
            auto media = m_ml->media( va_arg( args, int64_t ) );
            if ( media == nullptr )
                return VLC_EGENERIC;
            auto query = media->labels();
            if ( query == nullptr )
                return VLC_EGENERIC;
            switch ( listQuery )
            {
                case VLC_ML_LIST_MEDIA_LABELS:
                    *va_arg( args, vlc_ml_label_list_t**) =
                            ml_convert_list<vlc_ml_label_list_t, vlc_ml_label_t>(
                                query->items( nbItems, offset ) );
                    return VLC_SUCCESS;
                case VLC_ML_COUNT_MEDIA_LABELS:
                    *va_arg( args, size_t* ) = query->count();
                    return VLC_SUCCESS;
                default:
                    vlc_assert_unreachable();
            }
        }
        case VLC_ML_LIST_SHOWS:
        {
            medialibrary::Query<medialibrary::IShow> query;
            if ( psz_pattern != nullptr )
                query = m_ml->searchShows( psz_pattern, paramsPtr );
            else
                query = m_ml->shows( paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            *va_arg( args, vlc_ml_show_list_t** ) =
                    ml_convert_list<vlc_ml_show_list_t, vlc_ml_show_t>(
                        query->items( nbItems, offset ) );
            return VLC_SUCCESS;
        }
        case VLC_ML_COUNT_SHOWS:
        {
            auto query = m_ml->shows( paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            *va_arg( args, int64_t* ) = query->count();
            return VLC_SUCCESS;
        }
        case VLC_ML_LIST_SHOW_EPISODES:
        case VLC_ML_COUNT_SHOW_EPISODES:
        {
            auto show = m_ml->show( va_arg( args, int64_t ) );
            if ( show == nullptr )
                 return VLC_EGENERIC;
            medialibrary::Query<medialibrary::IMedia> query;
            if ( psz_pattern != nullptr )
                query = show->searchEpisodes( psz_pattern, paramsPtr );
            else
                query = show->episodes( paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            switch ( listQuery )
            {
                case VLC_ML_LIST_SHOW_EPISODES:
                    *va_arg( args, vlc_ml_media_list_t**) =
                            ml_convert_list<vlc_ml_media_list_t, vlc_ml_media_t>(
                                query->items( nbItems, offset ) );
                    return VLC_SUCCESS;
                case VLC_ML_COUNT_SHOW_EPISODES:
                    *va_arg( args, int64_t* ) = query->count();
                    return VLC_SUCCESS;
                default:
                    vlc_assert_unreachable();
            }
        }
        case VLC_ML_LIST_PLAYLIST_MEDIA:
        case VLC_ML_COUNT_PLAYLIST_MEDIA:
        case VLC_ML_LIST_PLAYLISTS:
        case VLC_ML_COUNT_PLAYLISTS:
            return listPlaylist( listQuery, paramsPtr, psz_pattern, nbItems, offset, args );
        case VLC_ML_LIST_HISTORY:
        {
            auto query = m_ml->history();
            if ( query == nullptr )
                return VLC_EGENERIC;
            *va_arg( args, vlc_ml_media_list_t**) =
                    ml_convert_list<vlc_ml_media_list_t, vlc_ml_media_t>(
                        query->items( nbItems, offset ) );
            return VLC_SUCCESS;
        }
        case VLC_ML_LIST_STREAM_HISTORY:
        {
            auto query = m_ml->streamHistory();
            if ( query == nullptr )
                return VLC_EGENERIC;
            *va_arg( args, vlc_ml_media_list_t**) =
                    ml_convert_list<vlc_ml_media_list_t, vlc_ml_media_t>(
                        query->items( nbItems, offset ) );
            return VLC_SUCCESS;
        }
    }
    return VLC_SUCCESS;
}

void* MediaLibrary::Get( int query, int64_t id )
{
    if ( Start() == false )
        return nullptr;

    switch ( query )
    {
        case VLC_ML_GET_MEDIA:
        {
            auto media = m_ml->media( id );
            return CreateAndConvert<vlc_ml_media_t>( media.get() );
        }
        case VLC_ML_GET_INPUT_ITEM:
        {
            auto media = m_ml->media( id );
            return MediaToInputItem( media.get() );
        }
        case VLC_ML_GET_ALBUM:
        {
            auto album = m_ml->album( id );
            return CreateAndConvert<vlc_ml_album_t>( album.get() );
        }
        case VLC_ML_GET_ARTIST:
        {
            auto artist = m_ml->artist( id );
            return CreateAndConvert<vlc_ml_artist_t>( artist.get() );
        }
        case VLC_ML_GET_GENRE:
        {
            auto genre = m_ml->genre( id );
            return CreateAndConvert<vlc_ml_genre_t>( genre.get() );
        }
        case VLC_ML_GET_SHOW:
        {
            auto show = m_ml->show( id );
            return CreateAndConvert<vlc_ml_show_t>( show.get() );
        }
        case VLC_ML_GET_PLAYLIST:
        {
            auto playlist = m_ml->playlist( id );
            return CreateAndConvert<vlc_ml_playlist_t>( playlist.get() );
        }
        default:
            vlc_assert_unreachable();

    }
    return nullptr;
}

medialibrary::IMedia::MetadataType MediaLibrary::metadataType( int meta )
{
    switch ( meta )
    {
        case VLC_ML_PLAYBACK_PREF_RATING:
            return medialibrary::IMedia::MetadataType::Rating;
        case VLC_ML_PLAYBACK_PREF_PROGRESS:
            return medialibrary::IMedia::MetadataType::Progress;
        case VLC_ML_PLAYBACK_PREF_SPEED:
            return medialibrary::IMedia::MetadataType::Speed;
        case VLC_ML_PLAYBACK_PREF_TITLE:
            return medialibrary::IMedia::MetadataType::Title;
        case VLC_ML_PLAYBACK_PREF_CHAPTER:
            return medialibrary::IMedia::MetadataType::Chapter;
        case VLC_ML_PLAYBACK_PREF_PROGRAM:
            return medialibrary::IMedia::MetadataType::Program;
        case VLC_ML_PLAYBACK_PREF_SEEN:
            return medialibrary::IMedia::MetadataType::Seen;
        case VLC_ML_PLAYBACK_PREF_VIDEO_TRACK:
            return medialibrary::IMedia::MetadataType::VideoTrack;
        case VLC_ML_PLAYBACK_PREF_ASPECT_RATIO:
            return medialibrary::IMedia::MetadataType::AspectRatio;
        case VLC_ML_PLAYBACK_PREF_ZOOM:
            return medialibrary::IMedia::MetadataType::Zoom;
        case VLC_ML_PLAYBACK_PREF_CROP:
            return medialibrary::IMedia::MetadataType::Crop;
        case VLC_ML_PLAYBACK_PREF_DEINTERLACE:
            return medialibrary::IMedia::MetadataType::Deinterlace;
        case VLC_ML_PLAYBACK_PREF_VIDEO_FILTER:
            return medialibrary::IMedia::MetadataType::VideoFilter;
        case VLC_ML_PLAYBACK_PREF_AUDIO_TRACK:
            return medialibrary::IMedia::MetadataType::AudioTrack;
        case VLC_ML_PLAYBACK_PREF_GAIN:
            return medialibrary::IMedia::MetadataType::Gain;
        case VLC_ML_PLAYBACK_PREF_AUDIO_DELAY:
            return medialibrary::IMedia::MetadataType::AudioDelay;
        case VLC_ML_PLAYBACK_PREF_SUBTITLE_TRACK:
            return medialibrary::IMedia::MetadataType::SubtitleTrack;
        case VLC_ML_PLAYBACK_PREF_SUBTITLE_DELAY:
            return medialibrary::IMedia::MetadataType::SubtitleDelay;
        case VLC_ML_PLAYBACK_PREF_APP_SPECIFIC:
            return medialibrary::IMedia::MetadataType::ApplicationSpecific;
        default:
            vlc_assert_unreachable();
    }
}

medialibrary::SortingCriteria MediaLibrary::sortingCriteria(int sort)
{
    switch ( sort )
    {
        case VLC_ML_SORTING_DEFAULT:
            return medialibrary::SortingCriteria::Default;
        case VLC_ML_SORTING_ALPHA:
            return medialibrary::SortingCriteria::Alpha;
        case VLC_ML_SORTING_DURATION:
            return medialibrary::SortingCriteria::Duration;
        case VLC_ML_SORTING_INSERTIONDATE:
            return medialibrary::SortingCriteria::InsertionDate;
        case VLC_ML_SORTING_LASTMODIFICATIONDATE:
            return medialibrary::SortingCriteria::LastModificationDate;
        case VLC_ML_SORTING_RELEASEDATE:
            return medialibrary::SortingCriteria::ReleaseDate;
        case VLC_ML_SORTING_FILESIZE:
            return medialibrary::SortingCriteria::FileSize;
        case VLC_ML_SORTING_ARTIST:
            return medialibrary::SortingCriteria::Artist;
        case VLC_ML_SORTING_PLAYCOUNT:
            return medialibrary::SortingCriteria::PlayCount;
        case VLC_ML_SORTING_ALBUM:
            return medialibrary::SortingCriteria::Album;
        case VLC_ML_SORTING_FILENAME:
            return medialibrary::SortingCriteria::Filename;
        case VLC_ML_SORTING_TRACKNUMBER:
            return medialibrary::SortingCriteria::TrackNumber;
        default:
            vlc_assert_unreachable();
    }
}

int MediaLibrary::getMeta( const medialibrary::IMedia& media, int meta, char** result )
{
    auto& md = media.metadata( metadataType( meta ) );
    if ( md.isSet() == false )
    {
        *result = nullptr;
        return VLC_SUCCESS;
    }
    *result = strdup( md.str().c_str() );
    if ( *result == nullptr )
        return VLC_ENOMEM;
    return VLC_SUCCESS;
}

int MediaLibrary::setMeta( medialibrary::IMedia& media, int meta, const char* value )
{
    bool res;
    if ( value == nullptr )
        res = media.unsetMetadata( metadataType( meta ) );
    else
        res = media.setMetadata( metadataType( meta ), value );
    if ( res == false )
        return VLC_EGENERIC;
    return VLC_SUCCESS;
}

int MediaLibrary::controlMedia( int query, va_list args )
{
    auto mediaId = va_arg( args, int64_t );
    auto m = m_ml->media( mediaId );
    if ( m == nullptr )
        return VLC_EGENERIC;
    switch( query )
    {
        case VLC_ML_MEDIA_INCREASE_PLAY_COUNT:
            if ( m->increasePlayCount() == false )
                return VLC_EGENERIC;
            return VLC_SUCCESS;
        case VLC_ML_MEDIA_GET_MEDIA_PLAYBACK_PREF:
        {
            auto meta = va_arg( args, int );
            auto res = va_arg( args, char** );
            return getMeta( *m, meta, res );
        }
        case VLC_ML_MEDIA_SET_MEDIA_PLAYBACK_PREF:
        {
            auto meta = va_arg( args, int );
            auto value = va_arg( args, const char* );
            return setMeta( *m, meta, value );
        }
        case VLC_ML_MEDIA_SET_THUMBNAIL:
        {
            auto mrl = va_arg( args, const char* );
            m->setThumbnail( mrl );
            return VLC_SUCCESS;
        }
        case VLC_ML_MEDIA_GENERATE_THUMBNAIL:
        {
            auto res = m_ml->requestThumbnail( m );
            return res == true ? VLC_SUCCESS : VLC_EGENERIC;
        }
        case VLC_ML_MEDIA_ADD_EXTERNAL_MRL:
        {
            auto mrl = va_arg( args, const char* );
            auto type = va_arg( args, int );
            medialibrary::IFile::Type mlType;
            switch ( type )
            {
                case VLC_ML_FILE_TYPE_UNKNOWN:
                // The type can't be main since this is added to an existing media
                // which must already have a file
                case VLC_ML_FILE_TYPE_MAIN:
                case VLC_ML_FILE_TYPE_PLAYLIST:
                    return VLC_EGENERIC;
                case VLC_ML_FILE_TYPE_PART:
                    mlType = medialibrary::IFile::Type::Part;
                    break;
                case VLC_ML_FILE_TYPE_SOUNDTRACK:
                    mlType = medialibrary::IFile::Type::Soundtrack;
                    break;
                case VLC_ML_FILE_TYPE_SUBTITLE:
                    mlType = medialibrary::IFile::Type::Subtitles;
                    break;
                default:
                    vlc_assert_unreachable();
            }
            if ( m->addExternalMrl( mrl, mlType ) == nullptr )
                return VLC_EGENERIC;
            return VLC_SUCCESS;
        }
        default:
            vlc_assert_unreachable();
    }
}

int MediaLibrary::filterListChildrenQuery( int query, int parentType )
{
    switch( query )
    {
        case VLC_ML_LIST_MEDIA_OF:
            switch ( parentType )
            {
                case VLC_ML_PARENT_ALBUM:
                    return VLC_ML_LIST_ALBUM_TRACKS;
                case VLC_ML_PARENT_ARTIST:
                    return VLC_ML_LIST_ALBUM_TRACKS;
                case VLC_ML_PARENT_SHOW:
                    return VLC_ML_LIST_SHOW_EPISODES;
                case VLC_ML_PARENT_GENRE:
                    return VLC_ML_LIST_GENRE_TRACKS;
                case VLC_ML_PARENT_PLAYLIST:
                    return VLC_ML_LIST_PLAYLIST_MEDIA;
                default:
                    vlc_assert_unreachable();
            }
        case VLC_ML_COUNT_MEDIA_OF:
            switch ( parentType )
            {
                case VLC_ML_PARENT_ALBUM:
                    return VLC_ML_COUNT_ALBUM_TRACKS;
                case VLC_ML_PARENT_ARTIST:
                    return VLC_ML_COUNT_ALBUM_TRACKS;
                case VLC_ML_PARENT_SHOW:
                    return VLC_ML_COUNT_SHOW_EPISODES;
                case VLC_ML_PARENT_GENRE:
                    return VLC_ML_COUNT_GENRE_TRACKS;
                case VLC_ML_PARENT_PLAYLIST:
                    return VLC_ML_COUNT_PLAYLIST_MEDIA;
                default:
                    vlc_assert_unreachable();
            }
        case VLC_ML_LIST_ALBUMS_OF:
            switch ( parentType )
            {
                case VLC_ML_PARENT_ARTIST:
                    return VLC_ML_LIST_ARTIST_ALBUMS;
                case VLC_ML_PARENT_GENRE:
                    return VLC_ML_LIST_GENRE_ALBUMS;
                default:
                    vlc_assert_unreachable();
            }
        case VLC_ML_COUNT_ALBUMS_OF:
            switch ( parentType )
            {
                case VLC_ML_PARENT_ARTIST:
                    return VLC_ML_COUNT_ARTIST_ALBUMS;
                case VLC_ML_PARENT_GENRE:
                    return VLC_ML_COUNT_GENRE_ALBUMS;
                default:
                    vlc_assert_unreachable();
            }
        case VLC_ML_LIST_ARTISTS_OF:
            switch ( parentType )
            {
                case VLC_ML_PARENT_ALBUM:
                    return VLC_ML_LIST_ALBUM_ARTISTS;
                case VLC_ML_PARENT_GENRE:
                    return VLC_ML_LIST_GENRE_ARTISTS;
                default:
                    vlc_assert_unreachable();
            }
        case VLC_ML_COUNT_ARTISTS_OF:
            switch ( parentType )
            {
                case VLC_ML_PARENT_ALBUM:
                    return VLC_ML_COUNT_ALBUM_ARTISTS;
                case VLC_ML_PARENT_GENRE:
                    return VLC_ML_COUNT_GENRE_ARTISTS;
                default:
                    vlc_assert_unreachable();
            }
        default:
            vlc_assert_unreachable();
    }
}

int MediaLibrary::listAlbums( int listQuery, const medialibrary::QueryParameters* paramsPtr,
                              const char* pattern, uint32_t nbItems, uint32_t offset, va_list args )
{
    auto album = m_ml->album( va_arg( args, int64_t ) );
    if ( album == nullptr )
        return VLC_EGENERIC;
    switch ( listQuery )
    {
        case VLC_ML_LIST_ALBUM_TRACKS:
        case VLC_ML_COUNT_ALBUM_TRACKS:
        {
            medialibrary::Query<medialibrary::IMedia> query;
            if ( pattern != nullptr )
                query = album->searchTracks( pattern, paramsPtr );
            else
                query = album->tracks( paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            switch ( listQuery )
            {
                case VLC_ML_LIST_ALBUM_TRACKS:
                    *va_arg( args, vlc_ml_media_list_t**) =
                            ml_convert_list<vlc_ml_media_list_t, vlc_ml_media_t>(
                                query->items( nbItems, offset ) );
                    return VLC_SUCCESS;
                case VLC_ML_COUNT_ALBUM_TRACKS:
                    *va_arg( args, size_t* ) = query->count();
                    return VLC_SUCCESS;
                default:
                    vlc_assert_unreachable();
            }
        }
        case VLC_ML_LIST_ALBUM_ARTISTS:
        case VLC_ML_COUNT_ALBUM_ARTISTS:
        {
            auto query = album->artists( paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            switch ( listQuery )
            {
                case VLC_ML_LIST_ALBUM_ARTISTS:
                    *va_arg( args, vlc_ml_artist_list_t**) =
                            ml_convert_list<vlc_ml_artist_list_t, vlc_ml_artist_t>(
                                query->items( nbItems, offset ) );
                    return VLC_SUCCESS;
                case VLC_ML_COUNT_ALBUM_ARTISTS:
                    *va_arg( args, size_t* ) = query->count();
                    return VLC_SUCCESS;
                default:
                    vlc_assert_unreachable();
            }
        }
        default:
            vlc_assert_unreachable();
    }
}

int MediaLibrary::listArtists( int listQuery, const medialibrary::QueryParameters* paramsPtr,
                               const char* pattern, uint32_t nbItems, uint32_t offset,
                               va_list args )
{
    auto artist = m_ml->artist( va_arg( args, int64_t ) );
    if ( artist == nullptr )
        return VLC_EGENERIC;
    switch( listQuery )
    {
        case VLC_ML_LIST_ARTIST_ALBUMS:
        case VLC_ML_COUNT_ARTIST_ALBUMS:
        {
            medialibrary::Query<medialibrary::IAlbum> query;
            if ( pattern != nullptr )
                query = artist->searchAlbums( pattern, paramsPtr );
            else
                query = artist->albums( paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            switch ( listQuery )
            {
                case VLC_ML_LIST_ARTIST_ALBUMS:
                    *va_arg( args, vlc_ml_album_list_t**) =
                            ml_convert_list<vlc_ml_album_list_t, vlc_ml_album_t>(
                                query->items( nbItems, offset ) );
                    return VLC_SUCCESS;
                case VLC_ML_COUNT_ARTIST_ALBUMS:
                    *va_arg( args, size_t* ) = query->count();
                    return VLC_SUCCESS;
                default:
                    vlc_assert_unreachable();
            }
        }
        case VLC_ML_LIST_ARTIST_TRACKS:
        case VLC_ML_COUNT_ARTIST_TRACKS:
        {
            medialibrary::Query<medialibrary::IMedia> query;
            if ( pattern != nullptr )
                query = artist->searchTracks( pattern, paramsPtr );
            else
                query = artist->tracks( paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            switch ( listQuery )
            {
                case VLC_ML_LIST_ARTIST_TRACKS:
                    *va_arg( args, vlc_ml_media_list_t**) =
                            ml_convert_list<vlc_ml_media_list_t, vlc_ml_media_t>(
                                query->items( nbItems, offset ) );
                    return VLC_SUCCESS;
                case VLC_ML_COUNT_ARTIST_TRACKS:
                    *va_arg( args, size_t* ) = query->count();
                    return VLC_SUCCESS;
                default:
                    vlc_assert_unreachable();
            }
        }
        default:
            vlc_assert_unreachable();
    }
}

int MediaLibrary::listGenre( int listQuery, const medialibrary::QueryParameters* paramsPtr,
                             const char* pattern, uint32_t nbItems, uint32_t offset, va_list args )
{
    auto genre = m_ml->genre( va_arg( args, int64_t ) );
    if ( genre == nullptr )
        return VLC_EGENERIC;
    switch( listQuery )
    {
        case VLC_ML_LIST_GENRE_ARTISTS:
        case VLC_ML_COUNT_GENRE_ARTISTS:
        {
            medialibrary::Query<medialibrary::IArtist> query;
            if ( pattern != nullptr )
                query = genre->searchArtists( pattern, paramsPtr );
            else
                query = genre->artists( paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            switch ( listQuery )
            {
                case VLC_ML_LIST_GENRE_ARTISTS:
                    *va_arg( args, vlc_ml_artist_list_t**) =
                            ml_convert_list<vlc_ml_artist_list_t, vlc_ml_artist_t>(
                                    query->items( nbItems, offset ) );
                    return VLC_SUCCESS;
                case VLC_ML_COUNT_GENRE_ARTISTS:
                    *va_arg( args, size_t* ) = query->count();
                    return VLC_SUCCESS;
                default:
                    vlc_assert_unreachable();
            }
        }
        case VLC_ML_LIST_GENRE_TRACKS:
        case VLC_ML_COUNT_GENRE_TRACKS:
        {
            medialibrary::Query<medialibrary::IMedia> query;
            if ( pattern != nullptr )
                query = genre->searchTracks( pattern, paramsPtr );
            else
                query = genre->tracks( paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            switch ( listQuery )
            {
                case VLC_ML_LIST_GENRE_TRACKS:
                    *va_arg( args, vlc_ml_media_list_t**) =
                            ml_convert_list<vlc_ml_media_list_t, vlc_ml_media_t>(
                                query->items( nbItems, offset ) );
                    return VLC_SUCCESS;
                case VLC_ML_COUNT_GENRE_TRACKS:
                    *va_arg( args, size_t*) = query->count();
                    return VLC_SUCCESS;
                default:
                    vlc_assert_unreachable();
            }
        }
        case VLC_ML_LIST_GENRE_ALBUMS:
        case VLC_ML_COUNT_GENRE_ALBUMS:
        {
            medialibrary::Query<medialibrary::IAlbum> query;
            if ( pattern != nullptr )
                query = genre->searchAlbums( pattern, paramsPtr );
            else
                query = genre->albums( paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            switch ( listQuery )
            {
                case VLC_ML_LIST_GENRE_ALBUMS:
                    *va_arg( args, vlc_ml_album_list_t**) =
                            ml_convert_list<vlc_ml_album_list_t, vlc_ml_album_t>(
                                query->items( nbItems, offset ) );
                    return VLC_SUCCESS;
                case VLC_ML_COUNT_GENRE_ALBUMS:
                    *va_arg( args, size_t* ) = query->count();
                    return VLC_SUCCESS;
                default:
                    vlc_assert_unreachable();
            }
        }
        default:
            vlc_assert_unreachable();
    }
}

int MediaLibrary::listPlaylist( int listQuery, const medialibrary::QueryParameters* paramsPtr,
                                const char* pattern, uint32_t nbItems, uint32_t offset, va_list args )
{
    switch( listQuery )
    {
        case VLC_ML_LIST_PLAYLISTS:
        case VLC_ML_COUNT_PLAYLISTS:
        {
            medialibrary::Query<medialibrary::IPlaylist> query;
            if ( pattern != nullptr )
                query = m_ml->searchPlaylists( pattern, paramsPtr );
            else
                query = m_ml->playlists( paramsPtr );
            if ( query == nullptr )
                return VLC_EGENERIC;
            switch ( listQuery )
            {
                case VLC_ML_LIST_PLAYLISTS:
                    *va_arg( args, vlc_ml_playlist_list_t** ) =
                            ml_convert_list<vlc_ml_playlist_list_t, vlc_ml_playlist_t>(
                                query->items( nbItems, offset ) );
                    return VLC_SUCCESS;
                case VLC_ML_COUNT_PLAYLISTS:
                    *va_arg( args, size_t* ) = query->count();
                    return VLC_SUCCESS;
                default:
                    vlc_assert_unreachable();
            }
        }
        case VLC_ML_LIST_PLAYLIST_MEDIA:
        case VLC_ML_COUNT_PLAYLIST_MEDIA:
        {
            auto playlist = m_ml->playlist( va_arg( args, int64_t ) );
            if ( playlist == nullptr )
                return VLC_EGENERIC;
            medialibrary::Query<medialibrary::IMedia> query;
            if ( pattern != nullptr )
                query = playlist->searchMedia( pattern, paramsPtr );
            else
                query = playlist->media();
            if ( query == nullptr )
                return VLC_EGENERIC;
            switch ( listQuery )
            {
                case VLC_ML_LIST_PLAYLIST_MEDIA:
                    *va_arg( args, vlc_ml_media_list_t**) =
                            ml_convert_list<vlc_ml_media_list_t, vlc_ml_media_t>(
                                query->items( nbItems, offset ) );
                    return VLC_SUCCESS;
                case VLC_ML_COUNT_PLAYLIST_MEDIA:
                    *va_arg( args, size_t* ) = query->count();
                    return VLC_SUCCESS;
                default:
                    vlc_assert_unreachable();
            }
        }
        default:
            vlc_assert_unreachable();
    }
}

static void* Get( vlc_medialibrary_module_t* module, int query, int64_t id )
{
    auto ml = static_cast<MediaLibrary*>( module->p_sys );
    return ml->Get( query, id );
}

static int List( vlc_medialibrary_module_t* module, int query,
                   const vlc_ml_query_params_t* params, va_list args )
{
    auto ml = static_cast<MediaLibrary*>( module->p_sys );
    return ml->List( query, params, args );
}

static int Control( vlc_medialibrary_module_t* module, int query, va_list args )
{
    auto ml = static_cast<MediaLibrary*>( module->p_sys );
    return ml->Control( query, args );
}

static int Open( vlc_object_t* obj )
{
    auto* p_ml = reinterpret_cast<vlc_medialibrary_module_t*>( obj );

    try
    {
        p_ml->p_sys = new MediaLibrary( p_ml );
    }
    catch ( const std::exception& ex )
    {
        msg_Err( obj, "Failed to instantiate/initialize medialibrary: %s", ex.what() );
        return VLC_EGENERIC;
    }
    p_ml->pf_control = Control;
    p_ml->pf_get = Get;
    p_ml->pf_list = List;
    return VLC_SUCCESS;
}

static void Close( vlc_medialibrary_module_t* module )
{
    MediaLibrary* p_ml = static_cast<MediaLibrary*>( module->p_sys );
    delete p_ml;
}

#define ML_FOLDER_TEXT _( "Folders discovered by the media library" )
#define ML_FOLDER_LONGTEXT _( "Semicolon separated list of folders to discover " \
                              "media from" )

vlc_module_begin()
    set_shortname(N_("media library"))
    set_description(N_( "Organize your media" ))
    set_category(CAT_ADVANCED)
    set_subcategory(SUBCAT_ADVANCED_MISC)
    set_capability("medialibrary", 100)
    set_callbacks(Open, Close)
    add_string( "ml-folders", nullptr, ML_FOLDER_TEXT, ML_FOLDER_LONGTEXT, false )
vlc_module_end()
