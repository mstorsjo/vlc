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
import org.videolan.medialib 0.1
import org.videolan.vlc 0.1

import "qrc:///style/"
import "qrc:///qml/"
import "qrc:///utils/" as Utils
import "qrc:///playlist/" as PL

Utils.NavigableFocusScope {
    id: root

    //name and properties of the tab to be initially loaded
    property string view: ""
    property var viewProperties: ({})

    Component {
        id: musicComp
        MCMusicDisplay {}
    }

    Component {
        id: videoComp
        MCVideoDisplay {}
    }

    Component {
        id: networkComp
        MCNetworkDisplay {}
    }

    readonly property var pageModel: [
        {
            displayText: qsTr("Music"),
            pic: "qrc:///sidebar/music.svg",
            name: "music",
            component: musicComp
        }, {
            displayText: qsTr("Video"),
            pic: "qrc:///sidebar/movie.svg",
            name: "video",
            component: videoComp
        }, {
            displayText: qsTr("Network"),
            pic: "qrc:///sidebar/screen.svg",
            name: "network",
            component: networkComp
        }
    ]

    property var tabModel: ListModel {
        id: tabModelid
        Component.onCompleted: {
            pageModel.forEach(function(e) {
                append({
                    displayText: e.displayText,
                    pic: e.pic,
                    name: e.name,
                    selected: (e.name === root.view)
                })
            })
        }
    }

    Row {
        anchors.fill: parent


        Utils.NavigableFocusScope {
            focus: true
            id: medialibId
            width: parent.width * (2. / 3)
            height: parent.height
            onActionRight: playlist.focus = true


            ColumnLayout {
                id: column
                anchors.fill: parent

                Layout.minimumWidth: VLCStyle.minWidthMediacenter
                spacing: 0

                /* Source selection*/
                BannerSources {
                    id: sourcesBanner

                    Layout.preferredHeight: height
                    Layout.minimumHeight: height
                    Layout.maximumHeight: height
                    Layout.fillWidth: true

                    need_toggleView_button: true

                    model: root.tabModel

                    onSelectedIndexChanged: {
                        var name = root.tabModel.get(selectedIndex).name
                        stackView.replace(root.pageModel[selectedIndex].component)
                        history.push(["mc", name], History.Stay)
                        stackView.focus = true
                    }

                    onActionDown: stackView.focus = true

                    onActionLeft: root.actionLeft(index)
                    onActionRight: root.actionRight(index)
                    onActionUp: root.actionUp(index)
                    onActionCancel: root.actionCancel(index)
                }

                Utils.StackViewExt {
                    id: stackView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    focus: true
                    Component.onCompleted: {
                        var found = stackView.loadView(root.pageModel, root.view, root.viewProperties)
                    }
                }
            }

            Connections {
                target: stackView.currentItem
                ignoreUnknownSignals: true

                onActionUp:     sourcesBanner.focus = true
                onActionCancel: sourcesBanner.focus = true

                onActionLeft:   medialibId.actionLeft(index)
                onActionRight:  medialibId.actionRight(index)
                onActionDown:   medialibId.actionDown(index)
            }
        }

        PL.PlaylistListView {
            id: playlist
            focus: false
            width: parent.width / 3
            height: parent.height
            onActionLeft: medialibId.focus = true
        }
    }
}
