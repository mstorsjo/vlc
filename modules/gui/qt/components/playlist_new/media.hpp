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

#ifndef VLC_QT_MEDIA_HPP_
#define VLC_QT_MEDIA_HPP_

#include <vlc_cxx_helpers.hpp>
#include <vlc_common.h>
#include <vlc_input_item.h>
#include <QString>

namespace vlc {
  namespace playlist {

using InputItemPtr = vlc_shared_data_ptr_type(input_item_t,
                                              input_item_Hold,
                                              input_item_Release);

class Media
{
public:
    Media(input_item_t *media = nullptr)
    {
        if (media)
        {
            /* the media must be unique in the playlist */
            ptr.reset(input_item_Copy(media), false);
            if (!ptr)
                throw std::bad_alloc();
        }
    }

    Media(QString uri, QString name)
    {
        auto uUri = uri.toUtf8();
        auto uName = name.toUtf8();
        const char *rawUri = uUri.isNull() ? nullptr : uUri.constData();
        const char *rawName = uName.isNull() ? nullptr : uName.constData();
        ptr.reset(input_item_New(rawUri, rawName), false);
        if (!ptr)
            throw std::bad_alloc();
    }

    operator bool() const
    {
        return ptr;
    }

    input_item_t *raw() const
    {
        return ptr.get();
    }

private:
    InputItemPtr ptr;
};

  } // namespace playlist
} // namespace vlc

#endif
