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
    Action { text: qsTr("&Effects and Filters");    onTriggered: dialogProvider.extendedDialog();   icon.source: "qrc:/menu/settings.svg";    shortcut: "Ctrl+E" }
    Action { text: qsTr("&Track Synchronization");  onTriggered: dialogProvider.synchroDialog();    icon.source: "qrc:/menu/settings.svg";                       }
    Action { text: qsTr("Media &Information") ;     onTriggered: dialogProvider.mediaInfoDialog();  icon.source: "qrc:/menu/info.svg";        shortcut: "Ctrl+I" }
    Action { text: qsTr("&Codec Information") ;     onTriggered: dialogProvider.mediaCodecDialog(); icon.source: "qrc:/menu/info.svg";        shortcut: "Ctrl+J" }
    Action { text: qsTr("Program Guide");           onTriggered: dialogProvider.epgDialog();                                                                     }
    Action { text: qsTr("&Messages");               onTriggered: dialogProvider.messagesDialog();   icon.source: "qrc:/menu/messages.svg";    shortcut: "Ctrl+M" }
    Action { text: qsTr("Plu&gins and extensions"); onTriggered: dialogProvider.pluginDialog();                                                                  }
    MenuSeparator {}
    Action { text: qsTr("&Preferences");            onTriggered: dialogProvider.prefsDialog();      icon.source: "qrc:/menu/preferences.svg"; shortcut: "Ctrl+P" }
}
