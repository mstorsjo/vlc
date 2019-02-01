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
import QtQml.Models 2.2

import org.videolan.medialib 0.1

import "qrc:///utils/" as Utils
import "qrc:///style/"

Utils.ListItem {
    width: root.width
    height: VLCStyle.icon_normal

    focus: true

    color: VLCStyle.colors.getBgColor(element.DelegateModel.inSelected, this.hovered, this.activeFocus)

    cover: Image {
        id: cover_obj
        fillMode: Image.PreserveAspectFit
        source: "qrc:///type/file-asym.svg"
    }
    line1: model.name || qsTr("Unknown share")
    line2: model.mrl

    onItemClicked : {
        delegateModel.updateSelection( modifier, view.currentIndex, index )
        view.currentIndex = index
        this.forceActiveFocus()
    }
    onItemDoubleClicked: {
        medialib.addAndPlay( model.mrl )
    }
    onPlayClicked: {
        medialib.addAndPlay( model.mrl )
    }
    onAddToPlaylistClicked: {
        medialib.addToPlaylist( model.mrl );
    }
}
