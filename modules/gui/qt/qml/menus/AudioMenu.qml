/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
import QtQuick 2.11
import QtQuick.Controls 2.4

import "qrc:///utils/" as Utils

Utils.MenuExt {
    CheckableModelSubMenu {
        title: qsTr("Audio &Track");
        enabled: player.isPlaying
        model: player.audioTracks
    }

    /* FIXME port to QML
    Menu {
        title: qsTr("Audio &Device")
    }
    */

    CheckableModelSubMenu {
        title: qsTr("&Stereo Mode");
        enabled: player.isPlaying
        model: player.audioStereoMode
    }

    MenuSeparator { }

    CheckableModelSubMenu {
        title: qsTr("&Visualizations");
        enabled: player.isPlaying
        model: player.audioVisualization
    }

    MenuSeparator { }

    Action { text: qsTr("Increase Volume"); enabled: player.isPlaying; onTriggered: player.setVolumeUp();   icon.source: "qrc:/toolbar/volume-high.svg";  }
    Action { text: qsTr("Decrease Volume"); enabled: player.isPlaying; onTriggered: player.setVolumeDown(); icon.source: "qrc:/toolbar/volume-low.svg";   }
    Action { text: qsTr("Mute");            enabled: player.isPlaying; onTriggered: player.toggleMuted();   icon.source: "qrc:/toolbar/volume-muted.svg"; }
}
