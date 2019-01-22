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
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import org.videolan.medialib 0.1
import org.videolan.vlc 0.1

import "qrc:///utils/" as Utils
import "qrc:///style/"

import "qrc:///mediacenter/" as MC
import "qrc:///player/" as Player
import "qrc:///about/" as AB
import "qrc:///dialogs/" as DG

Rectangle {
    id: root
    color: "transparent"

    Component {
        id: medialibComp
        FocusScope {
            focus: true
            property alias viewProperties: mainview.viewProperties
            property alias view: mainview.view
            Rectangle {
                color: VLCStyle.colors.bg
                anchors.fill: parent
                MC.MCMainDisplay {
                    id: mainview
                    focus: true
                    anchors.fill: parent
                }
            }
        }
    }

    Component {
        id: audioplayerComp
        Player.Player {
            focus: true
        }
    }

    Component {
        id: aboutComp
        AB.About {
            focus: true
            onActionCancel: {
                console.log("onActionCancel")
                history.pop(History.Go)
            }
        }
    }

    readonly property var pageModel: [
        { name: "about", component: aboutComp },
        { name: "mc", component: medialibComp },
    ]

    function loadCurrentHistoryView() {
        var current = history.current
        if ( !current || !current.view ) {
            console.warn("unable to load requested view, undefined")
            return
        }
        stackView.loadView(root.pageModel, current.view, current.viewProperties)
    }

    Connections {
        target: history
        onCurrentChanged: loadCurrentHistoryView()
    }

    Component.onCompleted: {
        //set the initial view
        history.push(["mc", "music", "albums"], History.Go)
    }

    Utils.StackViewExt {
        id: stackView
        anchors.fill: parent
        focus: true
        property int prevPlayerState: PlayerControler.PLAYING_STATE_STOPPED

        Connections {
            target: player
            onPlayingStateChanged: {
                if (state == PlayerControler.PLAYING_STATE_STOPPED )
                    loadCurrentHistoryView()
                else if (stackView.prevPlayerState == PlayerControler.PLAYING_STATE_STOPPED)
                    stackView.replace(audioplayerComp)
                stackView.prevPlayerState = state
            }
        }
    }

    Utils.ScanProgressBar {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }

    DG.Dialogs {
        anchors.fill: parent
        bgContent: root
        onRestoreFocus: {
            stackView.focus = true
        }
    }
}
