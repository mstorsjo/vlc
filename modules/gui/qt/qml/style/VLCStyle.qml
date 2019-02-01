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
pragma Singleton
import QtQuick 2.11

Item {
    id: vlc_style

    property double scale: 1.0

    TextMetrics { id: fontMetrics_xxsmall; font.pixelSize: 6 * scale;  text: "lq"}
    TextMetrics { id: fontMetrics_xsmall;  font.pixelSize: 8 * scale;  text: "lq"}
    TextMetrics { id: fontMetrics_small;   font.pixelSize: 10 * scale; text: "lq"}
    TextMetrics { id: fontMetrics_normal;  font.pixelSize: 12 * scale; text: "lq"}
    TextMetrics { id: fontMetrics_large;   font.pixelSize: 14 * scale; text: "lq"}
    TextMetrics { id: fontMetrics_xlarge;  font.pixelSize: 16 * scale; text: "lq"}
    TextMetrics { id: fontMetrics_xxlarge;  font.pixelSize: 20 * scale; text: "lq"}
    TextMetrics { id: fontMetrics_xxxlarge;  font.pixelSize: 30 * scale; text: "lq"}

    property VLCColors colors: VLCColors {}

    // Sizes
    property double margin_xxxsmall: 2 * scale;
    property double margin_xxsmall: 4 * scale;
    property double margin_xsmall: 8 * scale;
    property double margin_small: 12 * scale;
    property double margin_normal: 16 * scale;
    property double margin_large: 24 * scale;
    property double margin_xlarge: 32 * scale;

    property int fontSize_xsmall: fontMetrics_xxsmall.font.pixelSize
    property int fontSize_small:  fontMetrics_small.font.pixelSize
    property int fontSize_normal: fontMetrics_normal.font.pixelSize
    property int fontSize_large:  fontMetrics_large.font.pixelSize
    property int fontSize_xlarge: fontMetrics_xlarge.font.pixelSize
    property int fontSize_xxlarge: fontMetrics_xxlarge.font.pixelSize
    property int fontSize_xxxlarge: fontMetrics_xxxlarge.font.pixelSize

    property int fontHeight_xsmall: Math.ceil(fontMetrics_xxsmall.height)
    property int fontHeight_small:  Math.ceil(fontMetrics_small.height)
    property int fontHeight_normal: Math.ceil(fontMetrics_normal.height)
    property int fontHeight_large:  Math.ceil(fontMetrics_large.height)
    property int fontHeight_xlarge: Math.ceil(fontMetrics_xlarge.height)
    property int fontHeight_xxlarge: Math.ceil(fontMetrics_xxlarge.height)
    property int fontHeight_xxxlarge: Math.ceil(fontMetrics_xxxlarge.height)


    property int heightAlbumCover_xsmall: 32 * scale;
    property int heightAlbumCover_small: 64 * scale;
    property int heightAlbumCover_normal: 128 * scale;
    property int heightAlbumCover_large: 255 * scale;
    property int heightAlbumCover_xlarge: 512 * scale;

    property int icon_xsmall: 8 * scale;
    property int icon_small: 16 * scale;
    property int icon_normal: 32 * scale;
    property int icon_large: 64 * scale;
    property int icon_xlarge: 128 * scale;

    property int cover_xxsmall: 32 * scale;
    property int cover_xsmall: 64 * scale;
    property int cover_small: 96 * scale;
    property int cover_normal: 128 * scale;
    property int cover_large: 160 * scale;
    property int cover_xlarge: 192 * scale;

    property int heightBar_xsmall: 8 * scale;
    property int heightBar_small: 16 * scale;
    property int heightBar_normal: 32 * scale;
    property int heightBar_large: 64 * scale;
    property int heightBar_xlarge: 128 * scale;
    property int heightBar_xxlarge: 256 * scale;

    property int minWidthMediacenter: 500 * scale;
    property int maxWidthPlaylist: 400 * scale;
    property int defaultWidthPlaylist: 300 * scale;
    property int closedWidthPlaylist: 20 * scale;

    property int widthSearchInput: 200 * scale;
    property int widthSortBox: 150 * scale;


    //timings
    property int delayToolTipAppear: 500;
    property int timingPlaylistClose: 1000;
    property int timingPlaylistOpen: 1000;
    property int timingGridExpandOpen: 200;
    property int timingListExpandOpen: 200;

    //default arts
    property url noArtCover: "qrc:///noart.png";


}
