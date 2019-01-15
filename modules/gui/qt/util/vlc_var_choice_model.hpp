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

#ifndef VLC_VAR_CHOICE_MODEL_HPP
#define VLC_VAR_CHOICE_MODEL_HPP

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "qt.hpp"
#include <QAbstractListModel>
#include <QMutex>
#include <vlc_cxx_helpers.hpp>

extern "C" {
int VLCVarChoiceModel_on_variable_callback( vlc_object_t * object, char const * , vlc_value_t oldvalue, vlc_value_t newvalue, void * data);
int VLCVarChoiceModel_on_variable_list_callback( vlc_object_t * object, char const * , int action, vlc_value_t* value, void * data);
}

/**
 * @brief The VLCVarChoiceModel class contruct an Abstract List Model from a
 * vlc_var with the VLC_VAR_HASCHOICE flag and a type amongst string, int, float, bool
 *
 * available roles are DisplayRole and CheckStateRole
 */
class VLCVarChoiceModel : public QAbstractListModel
{
    Q_OBJECT
public:
    Q_PROPERTY(bool hasCurrent READ hasCurrent NOTIFY hasCurrentChanged)

    VLCVarChoiceModel(vlc_object_t *p_object, const char* varName, QObject *parent = nullptr);
    ~VLCVarChoiceModel();

    //QAbstractListModel overriden functions
    virtual Qt::ItemFlags flags(const QModelIndex &) const  override;
    QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex & = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    /**
     * @brief resetObject change the observed object.
     * @param object the new object to observe (can be null), the incoming object will be hold.
     * @return true if the object has the observed variable and if the variable has the right type.
     */
    bool resetObject(vlc_object_t *object);

    bool hasCurrent() const;

private slots:
    void onDataUpdated(const vlc_object_t* object, QVariant oldvalue, QVariant newvalue);
    void onListUpdated(const vlc_object_t* object, int action, QVariant newvalue);

signals:
    void dataUpdated(const vlc_object_t* object, QVariant oldvalue, QVariant newvalue, QPrivateSignal);
    void listUpdated(const vlc_object_t* object, int action, QVariant newvalue, QPrivateSignal);
    void hasCurrentChanged( bool );

private:
    int updateData(const vlc_object_t* object, const vlc_value_t& oldvalue, const vlc_value_t& newvalue);
    int updateList(const vlc_object_t* object, int action, const vlc_value_t* p_newvalue);

    QString vlcValToString(const vlc_value_t& value);
    QVariant vlcValToVariant(const vlc_value_t& a);

    typedef vlc_shared_data_ptr_type(vlc_object_t, vlc_object_hold, vlc_object_release) VlcObjectPtr;
    //reference to the observed object. Can only be modified from the UI thread
    VlcObjectPtr m_object;
    int m_type;
    QString m_varname;

    QList< QVariant > m_values;
    QStringList m_titles;
    int m_current = -1;

    friend int VLCVarChoiceModel_on_variable_callback( vlc_object_t * object, char const * , vlc_value_t oldvalue, vlc_value_t newvalue, void * data);
    friend int VLCVarChoiceModel_on_variable_list_callback( vlc_object_t * object, char const * , int action, vlc_value_t* value, void * data);
};

#endif // VLC_VAR_CHOICE_MODEL_HPP
