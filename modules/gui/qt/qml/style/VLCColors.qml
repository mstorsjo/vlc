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

Item {
    id: colors_id

    SystemPalette { id: activePalette; colorGroup: SystemPalette.Active }
    SystemPalette { id: inactivePalette; colorGroup: SystemPalette.Inactive }

    function blendColors( a, b, blend ) {
        return Qt.rgba( a.r * blend + b.r * (1. - blend),
                       a.g * blend + b.g * (1. - blend),
                       a.b * blend + b.b * (1. - blend),
                       a.a * blend + b.a * (1. - blend))
    }

    function setColorAlpha( c, alpha )
    {
        return Qt.rgba(c.r, c.g, c.b, alpha)
    }

    function getBgColor(selected, hovered, focus)
    {
        if (focus)
            return accent
        if ( selected )
            return bgHoverInactive
        else if (hovered)
            return bgHoverInactive
        else
            return "transparent"
    }

    property color text: activePalette.text;
    property color textInactive: inactivePalette.text;

    property color bg: activePalette.base;
    property color bgInactive: inactivePalette.base;

    //for alternate rows
    property color bgAlt: activePalette.alternateBase;
    property color bgAltInactive: inactivePalette.alternateBase;

    property color bgHover: activePalette.highlight;
    property color bgHoverInactive: inactivePalette.highlight;

    property color button: activePalette.button;
    property color buttonText: activePalette.buttonText;
    property color buttonBorder: blendColors(activePalette.button, activePalette.buttonText, 0.8);

    property color textActiveSource: "red";

    property color banner: activePalette.window;
    property color bannerHover: activePalette.highlight;

    //vlc orange
    property color accent: "#FFFF950D";

    property color alert: "red";

    property var colorSchemes: ["system", "day", "night"]

    state: "system"
    states: [
        //other styles are provided for testing purpose
        State {
            name: "day"
            PropertyChanges {
                target: colors_id

                text: "#232627"
                textInactive: "#7f8c8d"

                bg: "#fcfdfc"
                bgInactive: "#fcfdfc"

                bgAlt: "#eff0f1"
                bgAltInactive: "#eff0f1"

                bgHover: "#3daee9"
                bgHoverInactive: "#3daee9"

                button: "#eff0f1";
                buttonText: "#232627";
                buttonBorder: blendColors(activePalette.button, activePalette.buttonText, 0.8);

                textActiveSource: "#ff950d";

                banner: "#eff0f1";
                bannerHover: "#3daee9";

                accent: "#ff950d";
                alert: "#ff0000";
            }
        },
        State {
            name: "night"
            PropertyChanges {
                target: colors_id

                text: "#eff0f1"
                textInactive: "#bdc3c7"
                bg: "#232629"
                bgInactive: "#232629"
                bgAlt: "#31363b"
                bgAltInactive: "#31363b"
                bgHover: "#3daee9"
                bgHoverInactive: "#3daee9"
                button: "#31363b"
                buttonText: "#eff0f1"
                buttonBorder: "#575b5f"
                textActiveSource: "#ff950d"
                banner: "#31363b"
                bannerHover: "#3daee9"
                accent: "#ff950d"
                alert: "#ff0000"
            }
        },
        State {
            name: "system"
            PropertyChanges {
                target: colors_id

                bg: activePalette.base
                bgInactive: inactivePalette.base

                bgAlt: activePalette.alternateBase
                bgAltInactive: inactivePalette.alternateBase

                bgHover: activePalette.highlight
                bgHoverInactive: inactivePalette.highlight

                text: activePalette.text
                textInactive: inactivePalette.text

                button: activePalette.button
                buttonText: activePalette.buttonText
                buttonBorder: blendColors(button, buttonText, 0.8)

                textActiveSource: accent
                banner: activePalette.window
                bannerHover: activePalette.highlight
            }
        }
    ]
}
