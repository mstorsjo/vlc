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
import QtQuick 2.11
import QtQuick.Controls 2.4

import "qrc:///utils/" as Utils
import "qrc:///style/"

ListView {
    id: artistListView
    property var onItemClicked

    spacing: 2
    delegate : Utils.ListItem {
        height: VLCStyle.icon_normal
        width: parent.width

        cover: Image {
            id: cover_obj
            fillMode: Image.PreserveAspectFit
            source: model.cover || VLCStyle.noArtCover
        }
        line1: model.name || qsTr("Unknown artist")


        onItemClicked: {
            artistListView.onItemClicked( model.id )
        }

        onPlayClicked: {
            console.log('Clicked on play : '+model.name);
            medialib.addAndPlay( model.id )
        }
        onAddToPlaylistClicked: {
            console.log('Clicked on addToPlaylist : '+model.name);
            medialib.addToPlaylist( model.id );
        }
    }

    ScrollBar.vertical: ScrollBar { }
}
