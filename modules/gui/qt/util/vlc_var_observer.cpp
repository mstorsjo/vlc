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

#include "vlc_var_observer.hpp"

VLCVarBooleanObserver::VLCVarBooleanObserver(vlc_object_t *object, QString property, QObject *parent)
    : VLCVarObserver<VLCVarBooleanObserver, bool>(object, property, parent)
{
}

void VLCVarBooleanObserver::setValue(bool value)
{
    setValueInternal(value);
}

VLCVarStringObserver::VLCVarStringObserver(vlc_object_t *object, QString property, QObject *parent)
    : VLCVarObserver<VLCVarStringObserver, QString>(object, property, parent)
{
}

void VLCVarStringObserver::setValue(QString value)
{
    setValueInternal(value);
}

VLCVarFloatObserver::VLCVarFloatObserver(vlc_object_t *object, QString property, QObject *parent)
    : VLCVarObserver<VLCVarFloatObserver, float>(object, property, parent)
{
}

void VLCVarFloatObserver::setValue(float value)
{
    setValueInternal(value);
}

VLCVarIntObserver::VLCVarIntObserver(vlc_object_t *object, QString property, QObject *parent)
    : VLCVarObserver<VLCVarIntObserver, int64_t>(object, property, parent)
{
}

void VLCVarIntObserver::setValue(int64_t value)
{
    setValueInternal(value);
}
