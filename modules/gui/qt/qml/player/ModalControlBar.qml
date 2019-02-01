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
import QtQuick.Layouts 1.3

import org.videolan.vlc 0.1

import "qrc:///style/"
import "qrc:///utils/" as Utils

Utils.NavigableFocusScope {
    id: root
    signal showPlaylist();

    property bool showPlaylistButton: false

    property bool forceNoAutoHide: false
    property bool noAutoHide: state !== "control" || forceNoAutoHide

    Component {
        id: controlbarComp_id
        ControlBar {
            focus: true

            showPlaylistButton: root.showPlaylistButton

            onShowTrackBar: root.state = "tracks"
            onShowPlaylist: root.showPlaylist()

            onActionUp: root.actionUp(index)
            onActionDown: root.actionDown(index)
            onActionLeft: root.actionLeft(index)
            onActionRight: root.actionRight(index)
            onActionCancel: root.actionCancel(index)
        }
    }

    Component {
        id: trackbarComp_id
        TrackSelector {
            focus: true
            onActionCancel:  root.state = "control"

            onActionUp: root.actionUp(index)
            onActionDown: root.actionDown(index)
            onActionLeft: root.actionLeft(index)
            onActionRight: root.actionRight(index)
        }
    }

    Utils.StackViewExt {
        id: stack_id
        initialItem: controlbarComp_id
        anchors.fill: parent
        focus: true
    }

    state: "control"
    onStateChanged: {
        if (state === "tracks")
            stack_id.replace(trackbarComp_id)
        else if (state === "control")
            stack_id.replace(controlbarComp_id)
    }
}
