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
import QtGraphicalEffects 1.0

import org.videolan.medialib 0.1

import "qrc:///utils/" as Utils
import "qrc:///style/"

Rectangle {
    id: root
    property var artist: null
    color: VLCStyle.colors.bg

    property int contentY: 0
    height: VLCStyle.heightBar_xlarge

    RowLayout {
        anchors.fill: parent
        spacing: VLCStyle.margin_large

        Image {
            id: artistImage
            source: artist.cover || VLCStyle.noArtCover

            Layout.leftMargin: VLCStyle.margin_large
            Layout.preferredWidth: VLCStyle.cover_small
            Layout.preferredHeight: VLCStyle.cover_small

            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: Rectangle {
                    width: artistImage.width
                    height: artistImage.height
                    radius: artistImage.width
                }
            }
        }

        Text {
            id: main_artist
            text: artist.name
            Layout.fillWidth: true
            font.pixelSize: VLCStyle.fontSize_xxxlarge
            font.bold: true
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
            color: VLCStyle.colors.text
        }

    }
    states: [
        State {
            name: "full"
            PropertyChanges {
                target: artistImage
                width: VLCStyle.cover_small
                height: VLCStyle.cover_small
            }
            PropertyChanges {
                target: main_artist
                font.pixelSize: VLCStyle.fontSize_xxxlarge
            }
            when: contentY < VLCStyle.heightBar_large
        },
        State {
            name: "small"
            PropertyChanges {
                target: artistImage
                width: VLCStyle.icon_normal
                height: VLCStyle.icon_normal
            }
            PropertyChanges {
                target: main_artist
                font.pixelSize: VLCStyle.fontSize_large
                anchors.leftMargin: VLCStyle.margin_small
            }
            when: contentY >= VLCStyle.heightBar_large
        }
    ]
}
