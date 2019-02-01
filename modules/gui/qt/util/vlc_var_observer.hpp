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

#ifndef VLC_VAR_OBSERVER_HPP
#define VLC_VAR_OBSERVER_HPP


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_variables.h>
#include <vlc_cxx_helpers.hpp>

#include <QObject>

#include "qt.hpp"

/*
 * type traits to convert VLC_VAR to C++types
 */
template<typename T>
struct VLCVarTypeTraits {
};

template<>
struct VLCVarTypeTraits<QString>
{
    static const int var_type = VLC_VAR_STRING;
    inline static QString fromValue(const vlc_value_t& value) {
        return value.psz_string;
    }
    inline static vlc_value_t toValue(const QString& value) {
        vlc_value_t ret;
        ret.psz_string = strdup(qtu(value));
        return ret;
    }
    inline static void releaseValue(vlc_value_t value) {
        free(value.psz_string);
    }
};

template<>
struct VLCVarTypeTraits<bool>
{
    static const int var_type = VLC_VAR_BOOL;
    inline static bool fromValue(const vlc_value_t& value) {
        return value.b_bool;
    }
    inline static vlc_value_t toValue(bool value) {
        vlc_value_t ret;
        ret.b_bool = value;
        return ret;
    }
    inline static void releaseValue(vlc_value_t) {}
};

template<>
struct VLCVarTypeTraits<int64_t>
{
    static const int var_type = VLC_VAR_INTEGER;
    inline static int64_t fromValue(const vlc_value_t& value) {
        return value.i_int;
    }
    inline static vlc_value_t toValue(int64_t value) {
        vlc_value_t ret;
        ret.i_int = value;
        return ret;
    }
    inline static void releaseValue(vlc_value_t) {}
};

template<>
struct VLCVarTypeTraits<float>
{
    static const int var_type = VLC_VAR_FLOAT;
    inline static float fromValue(const vlc_value_t& value) {
        return value.f_float;
    }
    inline static vlc_value_t toValue(float value) {
        vlc_value_t ret;
        ret.f_float = value;
        return ret;
    }
    inline static void releaseValue(vlc_value_t) {}
};


//Generic observer
template<typename Derived, typename BaseType>
class VLCVarObserver : public QObject
{
public:
    VLCVarObserver(vlc_object_t* object, QString property, QObject* parent)
        : QObject(parent)
        , m_property(property)
    {
       resetObject(object);
       Derived* derived = static_cast<Derived*>(this);
       connect( derived, &Derived::valueChangedInternal, [this](vlc_object_t* object, BaseType val) {
           this->onValueChangedInternal(object, val);
       });
    }

    virtual ~VLCVarObserver()
    {
        if (m_object)
        {
            Derived* derived = static_cast<Derived*>(this);
            var_DelCallback(m_object.get(), qtu(m_property), value_modified, derived);
        }
    }

    ///change the object beeing observed
    void resetObject( vlc_object_t* object )
    {
        Derived* derived = static_cast<Derived*>(this);
        if (m_object)
            var_DelCallback( m_object.get(), qtu(m_property), value_modified, derived );

        m_object.reset( object, true );
        if (m_object)
        {
            int type = var_Type(object, qtu(m_property));
            if (type == 0) //variable not found
            {
                msg_Warn(m_object.get(), "variable %s not found in object", qtu(m_property));
                m_object.reset(nullptr);
                return;
            }
            assert((type & VLC_VAR_CLASS) == VLCVarTypeTraits<BaseType>::var_type);
            vlc_value_t currentvalue;
            if (var_Get(m_object.get(), qtu(m_property), &currentvalue) == VLC_SUCCESS)
            {
                BaseType value = VLCVarTypeTraits<BaseType>::fromValue(currentvalue);
                if (m_value != value)
                {
                    m_value = value;
                    emit derived->valueChanged( m_value );
                }
            }

            var_AddCallback(m_object.get(), qtu(m_property), value_modified, derived);
        }
    }

    BaseType getValue() const
    {
        if (!m_object)
            return BaseType{};
        return m_value;
    }

protected:
    //called by setValue in child classes
    void setValueInternal(BaseType value)
    {
        if (! m_object)
            return;
        vlc_value_t vlcvalue = VLCVarTypeTraits<BaseType>::toValue( value );
        var_Set(m_object.get(), qtu(m_property), vlcvalue);
        VLCVarTypeTraits<BaseType>::releaseValue(vlcvalue);
    }

private:
    //executed on UI thread
    virtual void onValueChangedInternal(vlc_object_t* object, BaseType value)
    {
        if (m_object.get() != object)
            return;
        if (m_value != value) {
            m_value = value;
            Derived* derived = static_cast<Derived*>(this);
            emit derived->valueChanged( m_value );
        }
    }


    //executed on variable thread, this forwards the callback to the UI thread
    static int value_modified( vlc_object_t * object, char const *, vlc_value_t, vlc_value_t newValue, void * data)
    {
        Derived* that = static_cast<Derived*>(data);
        emit that->onValueChangedInternal( object, VLCVarTypeTraits<BaseType>::fromValue( newValue ) );
        return VLC_SUCCESS;
    }

    typedef vlc_shared_data_ptr_type(vlc_object_t, vlc_object_hold, vlc_object_release) ObjectPtr;
    ObjectPtr m_object;
    QString m_property;
    BaseType m_value;
};

//specialisation

class VLCVarBooleanObserver : public VLCVarObserver<VLCVarBooleanObserver, bool>
{
    Q_OBJECT
public:
    Q_PROPERTY(bool value READ getValue WRITE setValue NOTIFY valueChanged)

    VLCVarBooleanObserver(vlc_object_t* object, QString property, QObject* parent = nullptr);

public slots:
    void setValue(bool value);

signals:
    void valueChanged( bool );
    void valueChangedInternal(vlc_object_t *, bool );
};

class VLCVarStringObserver : public VLCVarObserver<VLCVarStringObserver, QString>
{
    Q_OBJECT
public:
    Q_PROPERTY(QString value READ getValue WRITE setValue NOTIFY valueChanged)

    VLCVarStringObserver(vlc_object_t* object, QString property, QObject* parent = nullptr);

public slots:
    void setValue(QString value);

signals:
    void valueChanged( QString );
    void valueChangedInternal( vlc_object_t *, QString );
};

class VLCVarFloatObserver : public VLCVarObserver<VLCVarFloatObserver, float>
{
    Q_OBJECT
public:
    Q_PROPERTY(float value READ getValue WRITE setValue NOTIFY valueChanged)

    VLCVarFloatObserver(vlc_object_t* object, QString property, QObject* parent = nullptr);

public slots:
    void setValue(float value);

signals:
    void valueChanged( float );
    void valueChangedInternal( vlc_object_t *, float );
};


class VLCVarIntObserver : public VLCVarObserver<VLCVarIntObserver, int64_t>
{
    Q_OBJECT
public:
    Q_PROPERTY(int64_t value READ getValue WRITE setValue NOTIFY valueChanged)

    VLCVarIntObserver(vlc_object_t* object, QString property, QObject* parent = nullptr);

public slots:
    void setValue(int64_t value);

signals:
    void valueChanged( int64_t );
    void valueChangedInternal( vlc_object_t *,  int64_t );
};

#endif // VLC_VAR_OBSERVER_HPP
