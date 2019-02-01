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

#ifndef QVLC_INPUT_MANAGER_P_H_
#define QVLC_INPUT_MANAGER_P_H_

#include "player_controler.hpp"
#include "util/vlc_var_observer.hpp"
#include "util/input_models.hpp"
#include "util/vlc_var_choice_model.hpp"

class PlayerControlerPrivate {
    Q_DISABLE_COPY(PlayerControlerPrivate)
public:
    Q_DECLARE_PUBLIC(PlayerControler)
    PlayerControler * const q_ptr;

public:
    PlayerControlerPrivate(PlayerControler* playercontroler, intf_thread_t* p_intf);
    PlayerControlerPrivate() = delete;
    ~PlayerControlerPrivate();

    void UpdateName( input_item_t *p_item );
    void UpdateArt( input_item_t *p_item );
    void UpdateMeta( input_item_t *p_item );
    void UpdateInfo( input_item_t *p_item );
    void UpdateStats( const input_stats_t& stats );
    void UpdateProgram(vlc_player_list_action action, const vlc_player_program *prgm);
    void UpdateVouts(vout_thread_t **vouts, size_t i_vouts);
    void UpdateTrackSelection(vlc_es_id_t *trackid, bool selected);

    ///call function @a fun on object thread
    template <typename Fun>
    void callAsync(Fun&& fun)
    {
        Q_Q(PlayerControler);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
        QMetaObject::invokeMethod(q, std::forward<Fun>(fun), Qt::QueuedConnection, nullptr);
#else
        QObject src;
        QObject::connect(&src, &QObject::destroyed, q, std::forward<Fun>(fun), Qt::QueuedConnection);
#endif
    }

public slots:
    void menusUpdateAudio( const QString& );
    void AtoBLoop( float, VLCTick, int );


public:
    intf_thread_t           *p_intf;
    vlc_player_t            *m_player;

    //callbacks
    vlc_player_listener_id* m_player_listener = nullptr;
    vlc_player_aout_listener_id* m_player_aout_listener = nullptr;
    vlc_player_vout_listener_id* m_player_vout_listener = nullptr;

    //playback
    PlayerControler::PlayingState m_playing_status = PlayerControler::PLAYING_STATE_STOPPED;
    QString         m_name;
    float           m_buffering = 0.f;
    float           m_rate = 1.f;
    PlayerControler::MediaStopAction m_mediaStopAction = PlayerControler::MEDIA_STOPPED_CONTINUE;

    VLCTick      m_time = 0;
    float           m_position = 0.f;
    VLCTick      m_length= 0;

    int             m_capabilities = 0;

    //tracks
    TrackListModel m_videoTracks;
    TrackListModel m_audioTracks;
    TrackListModel m_subtitleTracks;

    VLCTick      m_audioDelay = 0;
    VLCTick      m_subtitleDelay = 0;
    float           m_subtitleFPS = 1.0;

    //title/chapters/menu
    TitleListModel m_titleList;
    ChapterListModel m_chapterList;
    bool m_hasTitles = false;
    bool m_hasChapters = false;
    bool m_hasMenu = false;

    //programs
    ProgramListModel m_programList;
    bool m_encrypted = false;

    //teletext
    bool m_teletextEnabled = false;
    bool m_teletextAvailable = false;
    int m_teletextPage = false;
    bool m_teletextTransparent = false;

    //vout properties
    VLCVarChoiceModel m_zoom;
    VLCVarChoiceModel m_aspectRatio;
    VLCVarChoiceModel m_crop;
    VLCVarChoiceModel m_deinterlace;
    VLCVarChoiceModel m_deinterlaceMode;
    VLCVarBooleanObserver m_autoscale;
    bool            m_hasVideo = false;
    bool            m_fullscreen = false;
    bool            m_wallpaperMode = false;

    //aout properties
    VLCVarChoiceModel m_audioStereoMode;
    float           m_volume = 0.f;
    bool            m_muted = false;
    VLCVarChoiceModel m_audioVisualization;

    //misc
    bool            m_recording = false;
    PlayerControler::ABLoopState m_ABLoopState = PlayerControler::ABLOOP_STATE_NONE;
    VLCTick m_ABLoopA = VLC_TICK_INVALID;
    VLCTick m_ABLoopB = VLC_TICK_INVALID;

    //others
    QString         m_artUrl;
    struct input_stats_t m_stats;

};

#endif /* QVLC_INPUT_MANAGER_P_H_ */
