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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "player_controler.hpp"
#include "player_controler_p.hpp"
#include "recents.hpp"

#include <vlc_actions.h>           /* ACTION_ID */
#include <vlc_url.h>            /* vlc_uri_decode */
#include <vlc_strings.h>        /* vlc_strfinput */
#include <vlc_aout.h>           /* audio_output_t */
#include <vlc_es.h>
#include <vlc_cxx_helpers.hpp>
#include <vlc_vout.h>

#include <QApplication>
#include <QFile>
#include <QDir>
#include <QSignalMapper>
#include <QMessageBox>

#include <assert.h>

//PlayerControler private implementation

PlayerControlerPrivate::~PlayerControlerPrivate()
{
    vlc_player_locker locker{m_player}; //this also locks the player
    vlc_player_vout_RemoveListener( m_player, m_player_vout_listener );
    vlc_player_aout_RemoveListener( m_player, m_player_aout_listener );
    vlc_player_RemoveListener( m_player, m_player_listener );
}

void PlayerControlerPrivate::UpdateName(input_item_t* media)
{
    Q_Q(PlayerControler);
    /* Update text, name and nowplaying */
    QString name;
    if (! media)
        return;

    /* Try to get the nowplaying */
    char *format = var_InheritString( p_intf, "input-title-format" );
    char *formatted = NULL;
    if (format != NULL)
    {
        formatted = vlc_strfinput( NULL, media, format );
        free( format );
        if( formatted != NULL )
        {
            name = qfu(formatted);
            free( formatted );
        }
    }

    /* If we have Nothing */
    if( name.simplified().isEmpty() )
    {
        char *uri = input_item_GetURI( media );
        char *file = uri ? strrchr( uri, '/' ) : NULL;
        if( file != NULL )
        {
            vlc_uri_decode( ++file );
            name = qfu(file);
        }
        else
            name = qfu(uri);
        free( uri );
    }

    name = name.trimmed();

    if( m_name != name )
    {
        emit q->nameChanged( name );
        m_name = name;
    }
}


void PlayerControlerPrivate::UpdateArt(input_item_t *p_item)
{
    Q_Q(PlayerControler);
    if (! p_item)
        return;

    QString url = PlayerControler::decodeArtURL( p_item );

    /* the art hasn't changed, no need to update */
    if(m_artUrl == url)
        return;

    /* Update Art meta */
    m_artUrl = url;
    emit q->artChanged( m_artUrl );
}

void PlayerControlerPrivate::UpdateStats( const input_stats_t& stats )
{
    Q_Q(PlayerControler);
    emit q->statisticsUpdated( stats );
}

void PlayerControlerPrivate::UpdateProgram(enum vlc_player_list_action action, const struct vlc_player_program* prgm)
{
    Q_Q(PlayerControler);
    m_programList.updatePrograms( action, prgm );
    emit q->isEncryptedChanged( prgm->scrambled );
}

void PlayerControlerPrivate::UpdateTrackSelection(vlc_es_id_t *trackid, bool selected)
{
    if (trackid == NULL)
        return;
    es_format_category_e cat = vlc_es_id_GetCat(trackid);
    TrackListModel* tracklist;
    switch (cat) {
    case VIDEO_ES: tracklist = &m_videoTracks; break;
    case AUDIO_ES: tracklist = &m_audioTracks; break;
    case SPU_ES: tracklist = &m_subtitleTracks; break;
    default: return;
    }
    tracklist->updateTrackSelection(trackid, selected);
}

void PlayerControlerPrivate::UpdateMeta( input_item_t *p_item )
{
    Q_Q(PlayerControler);
    emit q->currentMetaChanged( p_item  );
}

void PlayerControlerPrivate::UpdateInfo( input_item_t *p_item )
{
    Q_Q(PlayerControler);
    emit q->infoChanged( p_item );
}

void PlayerControlerPrivate::UpdateVouts(vout_thread_t **vouts, size_t i_vouts)
{
    Q_Q(PlayerControler);
    bool hadVideo = m_hasVideo;
    m_hasVideo = i_vouts > 0;

    vout_thread_t* main_vout = nullptr;
    if (m_hasVideo)
        main_vout = vouts[0];

    m_zoom.resetObject( VLC_OBJECT(main_vout) );
    m_aspectRatio.resetObject( VLC_OBJECT(main_vout) );
    m_crop.resetObject( VLC_OBJECT(main_vout) );
    m_deinterlace.resetObject( VLC_OBJECT(main_vout) );
    m_deinterlaceMode.resetObject( VLC_OBJECT(main_vout) );
    m_autoscale.resetObject( VLC_OBJECT(main_vout) );

    emit q->voutListChanged(vouts, i_vouts);
    if( hadVideo != m_hasVideo )
        emit q->hasVideoOutputChanged(m_hasVideo);
}


/*****************************************************************************
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
extern "C" {

//player callbacks

static  void on_player_current_media_changed(vlc_player_t *, input_item_t *new_media, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_current_media_changed");
    if ( new_media )
        input_item_Hold( new_media );
    that->callAsync([=] () {
        PlayerControler* q = that->q_func();
        that->UpdateName( new_media );
        that->UpdateArt( new_media );
        that->UpdateMeta( new_media );
        emit q->inputChanged( new_media != nullptr );
        if ( new_media) {
            input_item_Release( new_media );
        }
    });
}

static void on_player_state_changed(vlc_player_t *, enum vlc_player_state state, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_state_changed");

    that->callAsync([=] () {
        PlayerControler* q = that->q_func();
        that->m_playing_status = static_cast<PlayerControler::PlayingState>(state);
        switch ( state ) {
        case VLC_PLAYER_STATE_STARTED:
            msg_Info( that->p_intf, "on_player_state_changed VLC_PLAYER_STATE_STARTED");
            break;
        case VLC_PLAYER_STATE_PLAYING:
        {
            msg_Info( that->p_intf, "on_player_state_changed VLC_PLAYER_STATE_PLAYING");
            PlayerControler::AoutPtr aout = q->getAout();
            that->m_audioStereoMode.resetObject(VLC_OBJECT(aout.get()));
            that->m_audioVisualization.resetObject(VLC_OBJECT(aout.get()));
            break;
        }
        case VLC_PLAYER_STATE_PAUSED:
            msg_Info( that->p_intf, "on_player_state_changed VLC_PLAYER_STATE_PAUSED");
            break;
        case VLC_PLAYER_STATE_STOPPING:
            msg_Info( that->p_intf, "on_player_state_changed VLC_PLAYER_STATE_STOPPING");
            break;
        case VLC_PLAYER_STATE_STOPPED:
        {
            msg_Info( that->p_intf, "on_player_state_changed VLC_PLAYER_STATE_STOPPED");

            that->m_audioStereoMode.resetObject(nullptr);
            that->m_audioVisualization.resetObject(nullptr);

            /* reset the state on stop */
            emit q->positionUpdated( -1.0, 0 ,0 );
            emit q->rateChanged( 1.0f );
            emit q->nameChanged( "" );
            emit q->hasChaptersChanged( false );
            emit q->hasTitlesChanged( false );
            emit q->hasMenuChanged( false );

            emit q->teletextAvailableChanged( false );
            emit q->ABLoopStateChanged(PlayerControler::ABLOOP_STATE_NONE);
            emit q->ABLoopAChanged(VLC_TICK_INVALID);
            emit q->ABLoopBChanged(VLC_TICK_INVALID);
            emit q->hasVideoOutputChanged( false );
            emit q->voutListChanged( NULL, 0 );

            /* Reset all InfoPanels but stats */
            emit q->artChanged( NULL );
            emit q->artChanged( "" );
            emit q->infoChanged( NULL );
            emit q->currentMetaChanged( (input_item_t *)NULL );

            emit q->isEncryptedChanged( false );
            emit q->recordingChanged( false );

            emit q->bufferingChanged( 0.0 );

            break;
        }
        }
        emit q->playingStateChanged( that->m_playing_status );
    });
}

void on_player_error_changed(vlc_player_t *, enum vlc_player_error , void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_error_changed");
}

static void on_player_buffering(vlc_player_t *, float new_buffering, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_buffering");
    that->callAsync([=](){
        that->m_buffering = new_buffering;
        emit that->q_func()->bufferingChanged( new_buffering );
    });
}

static void on_player_rate_changed(vlc_player_t *, float new_rate, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_rate_changed %f", new_rate);
    that->callAsync([=](){
        that->m_rate = new_rate;
        emit that->q_func()->rateChanged( new_rate );
    });
}

static void on_player_capabilities_changed(vlc_player_t *, int new_caps, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_capabilities_changed");
    that->callAsync([=](){
        PlayerControler* q = that->q_func();
        that->m_capabilities = new_caps;
        emit q->seekableChanged( (new_caps & VLC_INPUT_CAPABILITIES_SEEKABLE) != 0 );
        emit q->rewindableChanged( (new_caps & VLC_INPUT_CAPABILITIES_REWINDABLE) != 0 );
        emit q->pausableChanged( (new_caps & VLC_INPUT_CAPABILITIES_PAUSEABLE) != 0 );
        emit q->recordableChanged( (new_caps & VLC_INPUT_CAPABILITIES_RECORDABLE) != 0 );
        emit q->rateChangableChanged( (new_caps & VLC_INPUT_CAPABILITIES_CHANGE_RATE) != 0 );
    });

    //FIXME other events?
}

static void on_player_position_changed(vlc_player_t *player, vlc_tick_t time, float pos, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    vlc_tick_t length =  vlc_player_GetLength( player );
    //msg_Info( that->p_intf, "on_player_position_changed time:%" PRIu64 " pos:%f lenght:%" PRIu64, time, pos, length);
    that->callAsync([=] () {
        PlayerControler* q = that->q_func();
        that->m_position = pos;
        emit q->positionChanged(pos);
        that->m_time = time;
        emit q->timeChanged(time);
        emit that->q_func()->positionUpdated(pos, time, SEC_FROM_VLC_TICK(length) );
    });
}

static void on_player_length_changed(vlc_player_t *player, vlc_tick_t new_length, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    vlc_tick_t time = vlc_player_GetTime( player );
    float pos = vlc_player_GetPosition( player );
    that->callAsync([=] () {
        PlayerControler* q = that->q_func();
        that->m_length = new_length;
        emit q->lengthChanged(new_length);
        emit that->q_func()->positionUpdated( pos, time, SEC_FROM_VLC_TICK(new_length) );
    });

}

static void on_player_track_list_changed(vlc_player_t *, enum vlc_player_list_action action, const struct vlc_player_track *track, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    struct vlc_player_track* newTrack =  vlc_player_track_Dup(track);
    msg_Info( that->p_intf, "on_player_track_list_changed");
    that->callAsync([=] () {
        switch (newTrack->fmt.i_cat) {
        case VIDEO_ES:
            msg_Info( that->p_intf, "on_player_track_list_changed (video)");
            that->m_videoTracks.updateTracks( action, newTrack );
            break;
        case AUDIO_ES:
            msg_Info( that->p_intf, "on_player_track_list_changed (audio)");
            that->m_audioTracks.updateTracks( action, newTrack );
            break;
        case SPU_ES:
            msg_Info( that->p_intf, "on_player_track_list_changed (spu)");
            that->m_subtitleTracks.updateTracks( action, newTrack );
            break;
        default:
            //we don't handle other kind of tracks
            msg_Info( that->p_intf, "on_player_track_list_changed (other)");
            break;
        }
        vlc_player_track_Delete(newTrack);
    });
}

static void on_player_track_selection_changed(vlc_player_t *, vlc_es_id_t * unselected, vlc_es_id_t *selected, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_track_selection_changed");

    vlc_es_id_t* new_unselected = nullptr;
    vlc_es_id_t* new_selected = nullptr;
    if (unselected)
        new_unselected = vlc_es_id_Hold(unselected);
    if (selected)
        new_selected = vlc_es_id_Hold(selected);

    that->callAsync([=] () {
        if (new_unselected)
        {
            that->UpdateTrackSelection( new_unselected, false );
            vlc_es_id_Release(new_unselected);
        }
        if (new_selected)
        {
            that->UpdateTrackSelection( new_selected, true );
            vlc_es_id_Release(new_selected);
        }
    });
}


static void on_player_program_list_changed(vlc_player_t *, enum vlc_player_list_action action, const struct vlc_player_program *new_prgm, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_program_list_changed");
    struct vlc_player_program* prgm = vlc_player_program_Dup(new_prgm);
    that->callAsync([=] (){
        that->UpdateProgram(action, prgm);
        vlc_player_program_Delete(prgm);
    });
}

static void on_player_program_selection_changed(vlc_player_t *, int unselected, int selected, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_program_selection_changed");

    that->callAsync([=] (){
        that->m_programList.updateProgramSelection(unselected, false);
        that->m_programList.updateProgramSelection(selected, true);
    });
}

static void on_player_titles_changed(vlc_player_t *, struct vlc_player_title_list *titles, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_title_array_changed");

    if (titles)
        vlc_player_title_list_Hold(titles);

    that->callAsync([=] (){
        that->m_chapterList.resetTitle(nullptr);
        that->m_titleList.resetTitles(titles);

        if (titles)
        {
            size_t nbTitles = vlc_player_title_list_GetCount(titles);
            for( size_t i = 0; i < nbTitles; i++)
            {
                const vlc_player_title* title = vlc_player_title_list_GetAt(titles, i);
                if( (title ->flags & INPUT_TITLE_MENU) != 0 )
                {
                    that->m_hasMenu = true;
                    break;
                }
            }
            that->m_hasTitles = nbTitles != 0;
            emit that->q_func()->hasTitlesChanged(that->m_hasTitles);
            emit that->q_func()->hasMenuChanged(that->m_hasMenu);
            vlc_player_title_list_Release(titles);
        }
    });
}

static void on_player_title_selection_changed(vlc_player_t *,
                                              const struct vlc_player_title *new_title, size_t new_idx, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_title_selection_changed");

    bool hasChapter = (new_title != nullptr && new_title->chapter_count != 0);
    that->callAsync([=] (){
        that->m_chapterList.resetTitle(new_title);
        that->m_titleList.setCurrent(new_idx);
        that->m_hasChapters  = hasChapter;
        emit that->q_func()->hasChaptersChanged( hasChapter );
    });
}

static void on_player_chapter_selection_changed(vlc_player_t *,
                                                const struct vlc_player_title *new_title, size_t title_idx,
                                                const struct vlc_player_chapter *chapter, size_t chapter_idx,
                                                void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_chapter_selection_changed");
    VLC_UNUSED(new_title);
    VLC_UNUSED(title_idx);
    VLC_UNUSED(chapter);
    that->callAsync([=] (){
        that->m_chapterList.setCurrent(chapter_idx);
    });
}


static void on_player_teletext_menu_changed(vlc_player_t *, bool has_teletext_menu, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_teletext_menu_changed, %s", has_teletext_menu ? "available" : "unavailable" );
    that->callAsync([=] () {
        that->m_teletextAvailable = has_teletext_menu;
        emit that->q_func()->teletextAvailableChanged(has_teletext_menu);
    });
}

static void on_player_teletext_enabled_changed(vlc_player_t *, bool enabled, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_teletext_enabled_changed %s", enabled ? "enabled" : "disabled");
    that->callAsync([=] () {
        that->m_teletextEnabled = enabled;
        emit that->q_func()->teletextEnabledChanged(enabled);
    });
}

static void on_player_teletext_page_changed(vlc_player_t *, unsigned new_page, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_teletext_page_changed %u", new_page);
    that->callAsync([=] () {
        that->m_teletextPage = new_page;
        emit that->q_func()->teletextPageChanged(new_page);
    });
}

static void on_player_teletext_transparency_changed(vlc_player_t *, bool enabled, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_teletext_transparency_changed %s", enabled ? "enabled" : "disabled");
    that->callAsync([=] () {
        that->m_teletextTransparent = enabled;
        emit that->q_func()->teletextTransparencyChanged(enabled);
    });
}

static void on_player_audio_delay_changed(vlc_player_t *, vlc_tick_t new_delay,
                               void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_audio_delay_changed");
    that->callAsync([=] (){
        that->m_audioDelay = new_delay;
        emit that->q_func()->audioDelayChanged( new_delay );
    });
}

static void on_player_subtitle_delay_changed(vlc_player_t *, vlc_tick_t new_delay,
                                  void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_subtitle_delay_changed");
    that->callAsync([=] (){
        that->m_subtitleDelay = new_delay;
        emit that->q_func()->subtitleDelayChanged( new_delay );
    });
}

static void on_player_associated_subs_fps_changed(vlc_player_t *, float subs_fps, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_associated_subs_fps_changed");
    that->callAsync([=] (){
        that->m_subtitleFPS = subs_fps;
        emit that->q_func()->subtitleFPSChanged( subs_fps );
    });
}

void on_player_renderer_changed(vlc_player_t *, vlc_renderer_item_t *new_item, void *data)
{
    VLC_UNUSED(new_item);
    VLC_UNUSED(data);
}


static void on_player_record_changed(vlc_player_t *, bool recording, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_record_changed");
    that->callAsync([=] (){
        that->m_recording = recording;
        emit that->q_func()->recordingChanged( recording );
    });
}

static void on_player_signal_changed(vlc_player_t *, float quality, float strength, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    VLC_UNUSED(quality);
    VLC_UNUSED(strength);
    msg_Info( that->p_intf, "on_player_signal_changed");
}

static void on_player_stats_changed(vlc_player_t *, const struct input_stats_t *stats, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    struct input_stats_t stats_tmp = *stats;
    that->callAsync([=] () {
        that->m_stats = stats_tmp;
        emit that->q_func()->statisticsUpdated( that->m_stats );
    });
}

static void on_player_atobloop_changed(vlc_player_t *, enum vlc_player_abloop state, vlc_tick_t time, float, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_atobloop_changed");
    that->callAsync([=] (){
        PlayerControler* q = that->q_func();
        switch (state) {
        case VLC_PLAYER_ABLOOP_NONE:
            that->m_ABLoopA = VLC_TICK_INVALID;
            that->m_ABLoopB = VLC_TICK_INVALID;
            emit q->ABLoopAChanged(that->m_ABLoopA);
            emit q->ABLoopBChanged(that->m_ABLoopB);
            break;
        case VLC_PLAYER_ABLOOP_A:
            that->m_ABLoopA = time;
            emit q->ABLoopAChanged(that->m_ABLoopA);
            break;
        case VLC_PLAYER_ABLOOP_B:
            that->m_ABLoopB = time;
            emit q->ABLoopBChanged(that->m_ABLoopB);
            break;
        }
        that->m_ABLoopState = static_cast<PlayerControler::ABLoopState>(state);
        emit q->ABLoopStateChanged(that->m_ABLoopState);
    });
}

static void on_player_media_stopped_action_changed(vlc_player_t *, enum vlc_player_media_stopped_action new_action, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    that->callAsync([=] () {
        that->m_mediaStopAction = static_cast<PlayerControler::MediaStopAction>(new_action);
        emit that->q_func()->mediaStopActionChanged(that->m_mediaStopAction);
    });
}

static void on_player_item_meta_changed(vlc_player_t *, input_item_t *item, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_item_meta_changed");

    input_item_Hold(item);
    //call on object thread
    that->callAsync([=] () {
        that->UpdateName(item);
        that->UpdateArt(item);
        that->UpdateMeta(item);
        input_item_Release(item);
    });
}

static void on_player_item_epg_changed(vlc_player_t *, input_item_t *item, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_item_epg_changed");
    VLC_UNUSED(item);
    emit that->q_func()->epgChanged();
}

static void on_player_subitems_changed(vlc_player_t *, input_item_t *, input_item_node_t *subitems, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_subitems_changed");
    VLC_UNUSED(subitems);
}


static void on_player_vout_list_changed(vlc_player_t *player, enum vlc_player_list_action action, vout_thread_t *vout, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_vout_list_changed");

    VLC_UNUSED(action);
    VLC_UNUSED(vout);

    //player is locked within callbacks*
    size_t i_vout = 0;
    vout_thread_t **vouts = vlc_player_vout_HoldAll(player, &i_vout);

    //call on object thread
    that->callAsync([=] () {
        that->UpdateVouts(vouts, i_vout);

        for (size_t i = 0; i < i_vout; i++)
            vlc_object_release( vouts[i] );
        free(vouts);
    });
}

//player vout callbacks

static void on_player_vout_fullscreen_changed(vlc_player_t *, vout_thread_t* vout, bool is_fullscreen, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_vout_fullscreen_changed %s", is_fullscreen ? "fullscreen" : "windowed");
    if (vout)
        vlc_object_hold(vout);
    that->callAsync([=] () {
        PlayerControler* q = that->q_func();
        const PlayerControler::VoutPtrList voutList = q->getVouts();
        if (vout == NULL  //property sets for all vout
            || (voutList.size() == 1 && vout == voutList[0].get()) ) //on the only vout
        {
            that->m_fullscreen = is_fullscreen;
            emit q->fullscreenChanged(is_fullscreen);
        }
        if (vout)
            vlc_object_release(vout);
    });
}

static void on_player_vout_wallpaper_mode_changed(vlc_player_t *, vout_thread_t* vout,  bool enabled, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_vout_wallpaper_mode_changed");
    if (vout)
        vlc_object_hold(vout);
    that->callAsync([=] () {
        PlayerControler* q = that->q_func();
        const PlayerControler::VoutPtrList voutList = q->getVouts();
        if (vout == NULL  //property sets for all vout
            || (voutList.size() == 1 && vout == voutList[0].get()) ) //on the only vout
        {
            that->m_wallpaperMode = enabled;
            emit q->wallpaperModeChanged(enabled);
        }
        if (vout)
            vlc_object_release(vout);
    });
}

//player aout callbacks

static void on_player_aout_volume_changed(vlc_player_t *, float volume, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_aout_volume_changed");
    that->callAsync([=](){
        that->m_volume = volume;
        emit that->q_func()->volumeChanged( volume );
    });
}

static void on_player_aout_mute_changed(vlc_player_t *, bool muted, void *data)
{
    PlayerControlerPrivate* that = static_cast<PlayerControlerPrivate*>(data);
    msg_Info( that->p_intf, "on_player_aout_mute_changed");
    that->callAsync([=](){
        that->m_muted = muted;
        emit that->q_func()->soundMuteChanged(muted);
    });
}

} //extern "C"

static const struct vlc_player_cbs player_cbs = {
    on_player_current_media_changed,
    on_player_state_changed,
    on_player_error_changed,
    on_player_buffering,
    on_player_rate_changed,
    on_player_capabilities_changed,
    on_player_position_changed,
    on_player_length_changed,
    on_player_track_list_changed,
    on_player_track_selection_changed,
    on_player_program_list_changed,
    on_player_program_selection_changed,
    on_player_titles_changed,
    on_player_title_selection_changed,
    on_player_chapter_selection_changed,
    on_player_teletext_menu_changed,
    on_player_teletext_enabled_changed,
    on_player_teletext_page_changed,
    on_player_teletext_transparency_changed,
    on_player_audio_delay_changed,
    on_player_subtitle_delay_changed,
    on_player_associated_subs_fps_changed,
    on_player_renderer_changed,
    on_player_record_changed,
    on_player_signal_changed,
    on_player_stats_changed,
    on_player_atobloop_changed,
    on_player_media_stopped_action_changed,
    on_player_item_meta_changed,
    on_player_item_epg_changed,
    on_player_subitems_changed,
    on_player_vout_list_changed,
};

static const struct vlc_player_vout_cbs player_vout_cbs = {
    on_player_vout_fullscreen_changed,
    on_player_vout_wallpaper_mode_changed
};

static const struct vlc_player_aout_cbs player_aout_cbs = {
    on_player_aout_volume_changed,
    on_player_aout_mute_changed,
};

PlayerControlerPrivate::PlayerControlerPrivate(PlayerControler *playerControler, intf_thread_t *p_intf)
    : q_ptr(playerControler)
    , p_intf(p_intf)
    , m_player(p_intf->p_sys->p_player)
    , m_videoTracks(m_player)
    , m_audioTracks(m_player)
    , m_subtitleTracks(m_player)
    , m_titleList(m_player)
    , m_chapterList(m_player)
    , m_programList(m_player)
    , m_zoom(nullptr, "zoom")
    , m_aspectRatio(nullptr, "aspect-ratio")
    , m_crop(nullptr, "crop")
    , m_deinterlace(nullptr, "deinterlace")
    , m_deinterlaceMode(nullptr, "deinterlace-mode")
    , m_autoscale(nullptr, "autoscale")
    , m_audioStereoMode(nullptr, "stereo-mode")
    , m_audioVisualization(nullptr, "visual")
{
    {
        vlc_player_locker locker{m_player};
        m_player_listener = vlc_player_AddListener( m_player, &player_cbs, this );
        m_player_aout_listener = vlc_player_aout_AddListener( m_player, &player_aout_cbs, this );
        m_player_vout_listener = vlc_player_vout_AddListener( m_player, &player_vout_cbs, this );
    }

    QObject::connect( &m_autoscale, &VLCVarBooleanObserver::valueChanged, q_ptr, &PlayerControler::autoscaleChanged );
    QObject::connect( &m_audioVisualization, &VLCVarChoiceModel::hasCurrentChanged, q_ptr, &PlayerControler::hasAudioVisualizationChanged );
}


/*****************************************************************************
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

PlayerControler::PlayerControler( intf_thread_t *_p_intf )
    : QObject(NULL)
    , d_ptr( new PlayerControlerPrivate(this, _p_intf) )
{
    /* Audio Menu */
    menusAudioMapper = new QSignalMapper(this);
    CONNECT( menusAudioMapper, mapped(const QString&), this, menusUpdateAudio(const QString&) );
}

PlayerControler::~PlayerControler()
{
}

// PLAYBACK

input_item_t *PlayerControler::getInput()
{
    Q_D(PlayerControler);
    vlc_player_locker locker{ d->m_player };
    return vlc_player_GetCurrentMedia( d->m_player );
}

bool PlayerControler::hasInput() const
{
    Q_D(const PlayerControler);
    vlc_player_locker locker{ d->m_player };
    return vlc_player_IsStarted( d->m_player );
}

void PlayerControler::reverse()
{
    Q_D(PlayerControler);
    msg_Info( d->p_intf, "reverse");
    vlc_player_locker lock{ d->m_player };
    if ( vlc_player_CanChangeRate( d->m_player ) )
    {
        float f_rate_ = vlc_player_GetRate( d->m_player );
        vlc_player_ChangeRate( d->m_player, -f_rate_ );
    }
}

void PlayerControler::setRate( float new_rate )
{
    Q_D(PlayerControler);
    msg_Info( d->p_intf, "setRate %f", new_rate);
    vlc_player_locker lock{ d->m_player };
    if ( vlc_player_CanChangeRate( d->m_player ) )
        vlc_player_ChangeRate( d->m_player, new_rate );
}

void PlayerControler::setMediaStopAction(PlayerControler::MediaStopAction action)
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    vlc_player_SetMediaStoppedAction( d->m_player, static_cast<vlc_player_media_stopped_action>(action) );
}

void PlayerControler::slower()
{
    Q_D(PlayerControler);
    msg_Info( d->p_intf, "slower");
    vlc_player_locker lock{ d->m_player };
    if ( vlc_player_CanChangeRate( d->m_player ) )
        vlc_player_DecrementRate( d->m_player );
}

void PlayerControler::faster()
{
    Q_D(PlayerControler);
    msg_Info( d->p_intf, "faster");
    vlc_player_locker lock{ d->m_player };
    if ( vlc_player_CanChangeRate( d->m_player ) )
        vlc_player_IncrementRate( d->m_player );
}

void PlayerControler::littlefaster()
{
    Q_D(PlayerControler);
    msg_Info( d->p_intf, "littlefaster");
    var_SetInteger( d->p_intf->obj.libvlc, "key-action", ACTIONID_RATE_FASTER_FINE );
}

void PlayerControler::littleslower()
{
    Q_D(PlayerControler);
    msg_Info( d->p_intf, "littleslower");
    var_SetInteger( d->p_intf->obj.libvlc, "key-action", ACTIONID_RATE_SLOWER_FINE );
}

void PlayerControler::normalRate()
{
    Q_D(PlayerControler);
    msg_Info( d->p_intf, "normalRate");
    vlc_player_locker lock{ d->m_player };
    if ( vlc_player_CanChangeRate( d->m_player ) )
        vlc_player_ChangeRate( d->m_player, 1.0f );
}


void PlayerControler::setTime(VLCTick new_time)
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    return vlc_player_SetTime( d->m_player, new_time );
}

void PlayerControler::setPosition(float position)
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    return vlc_player_SetPosition( d->m_player, position );
}

void PlayerControler::jumpFwd()
{
    Q_D(PlayerControler);
    msg_Info( d->p_intf, "jumpFwd");
    int i_interval = var_InheritInteger( d->p_intf, "short-jump-size" );
    {
        vlc_player_locker lock{ d->m_player };
        vlc_player_JumpTime( d->m_player, vlc_tick_from_sec( i_interval ) );
    }
}

void PlayerControler::jumpBwd()
{
    Q_D(PlayerControler);
    msg_Info( d->p_intf, "jumpBwd");
    int i_interval = var_InheritInteger( d->p_intf, "short-jump-size" );
    {
        vlc_player_locker lock{ d->m_player };
        vlc_player_JumpTime( d->m_player, vlc_tick_from_sec( -i_interval ) );
    }
}

void PlayerControler::jumpToTime(VLCTick i_time)
{
    Q_D(PlayerControler);
    msg_Info( d->p_intf, "jumpToTime");
    vlc_player_locker lock{ d->m_player };
    vlc_player_JumpTime( d->m_player, vlc_tick_from_sec( i_time ) );
}

void PlayerControler::jumpToPos( float new_pos )
{
    Q_D(PlayerControler);
    {
        vlc_player_locker lock{ d->m_player };
        if( vlc_player_IsStarted( d->m_player ) )
            vlc_player_SetPosition( d->m_player, new_pos );
    }
    emit seekRequested( new_pos );
}

void PlayerControler::frameNext()
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    vlc_player_NextVideoFrame( d->m_player );
}

//TRACKS

void PlayerControler::setAudioDelay(VLCTick delay)
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    vlc_player_SetAudioDelay( d->m_player, delay, VLC_PLAYER_WHENCE_ABSOLUTE );
}

void PlayerControler::setSubtitleDelay(VLCTick delay)
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    vlc_player_SetSubtitleDelay( d->m_player, delay, VLC_PLAYER_WHENCE_ABSOLUTE );
}

void PlayerControler::setSubtitleFPS(float fps)
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    vlc_player_SetAssociatedSubsFPS( d->m_player, fps );
}

//TITLE/CHAPTER/MENU

void PlayerControler::sectionPrev()
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    if( vlc_player_IsStarted( d->m_player ) )
    {
        if (vlc_player_GetSelectedChapter( d->m_player ) != NULL)
            vlc_player_SelectPrevChapter( d->m_player );
        else
            vlc_player_SelectPrevTitle( d->m_player );
    }
}

void PlayerControler::sectionNext()
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    if( vlc_player_IsStarted( d->m_player ) )
    {
        if (vlc_player_GetSelectedChapter( d->m_player ) != NULL)
            vlc_player_SelectNextChapter( d->m_player );
        else
            vlc_player_SelectNextTitle( d->m_player );
    }
}

void PlayerControler::sectionMenu()
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    if( vlc_player_IsStarted( d->m_player ) )
        vlc_player_Navigate( d->m_player, VLC_PLAYER_NAV_MENU );
}

void PlayerControler::chapterNext()
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    if (vlc_player_IsStarted(d->m_player ))
        vlc_player_SelectNextChapter( d->m_player );
}

void PlayerControler::chapterPrev()
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    if (vlc_player_IsStarted(d->m_player ))
        vlc_player_SelectPrevChapter( d->m_player );
}

void PlayerControler::titleNext()
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    if (vlc_player_IsStarted(d->m_player ))
        vlc_player_SelectNextTitle( d->m_player );
}

void PlayerControler::titlePrev()
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    if (vlc_player_IsStarted(d->m_player ))
        vlc_player_SelectPrevTitle( d->m_player );
}

//PROGRAMS

void PlayerControler::changeProgram( int program )
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    if( vlc_player_IsStarted( d->m_player ) )
        vlc_player_SelectProgram( d->m_player, program );
}

//TELETEXT


void PlayerControler::enableTeletext( bool enable )
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    if (vlc_player_IsStarted(d->m_player ))
        vlc_player_SetTeletextEnabled( d->m_player, enable );
}

void PlayerControler::setTeletextPage(int page)
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    if (vlc_player_IsTeletextEnabled( d->m_player ))
        vlc_player_SelectTeletextPage( d->m_player, page );
}

void PlayerControler::setTeletextTransparency( bool transparent )
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    if (vlc_player_IsTeletextEnabled( d->m_player ))
        vlc_player_SetTeletextTransparency( d->m_player, transparent );
}

//VOUT PROPERTIES

PlayerControler::VoutPtrList PlayerControler::getVouts() const
{
    Q_D(const PlayerControler);
    vout_thread_t **pp_vout;
    VoutPtrList VoutList;
    size_t i_vout;
    {
        vlc_player_locker lock{ d->m_player };
        if( !vlc_player_IsStarted( d->m_player ) )
            return VoutPtrList{};
        i_vout = 0;
        pp_vout = vlc_player_vout_HoldAll( d->m_player, &i_vout );
        if ( i_vout <= 0 )
            return VoutPtrList{};
    }
    VoutList.reserve( i_vout );
    for( size_t i = 0; i < i_vout; i++ )
    {
        assert( pp_vout[i] );
        //pass ownership
        VoutList.append(VoutPtr(pp_vout[i], false));
    }
    free( pp_vout );

    return VoutList;
}

PlayerControler::VoutPtr PlayerControler::getVout()
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    size_t count = 0;
    vout_thread_t** vouts = vlc_player_vout_HoldAll( d->m_player, &count );
    if( count == 0 || vouts == NULL )
        return VoutPtr{};
    //add a reference
    VoutPtr first_vout{vouts[0], true};
    for( size_t i = 0; i < count; i++ )
        vlc_object_release( vouts[i]);
    free( vouts );
    return first_vout;
}

void PlayerControler::setFullscreen( bool new_val )
{
    Q_D(PlayerControler);
    msg_Info(d->p_intf, "setFullscreen %s", new_val? "fullscreen" : "windowed");
    vlc_player_locker lock{ d->m_player };
    vlc_player_vout_SetFullscreen( d->m_player, new_val );
}

void PlayerControler::toggleFullscreen()
{
    Q_D(PlayerControler);
    setFullscreen( ! d->m_fullscreen );
}

void PlayerControler::setWallpaperMode( bool new_val )
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    vlc_player_vout_SetWallpaperModeEnabled( d->m_player, new_val );
}

bool PlayerControler::getAutoscale( ) const
{
    Q_D(const PlayerControler);
    return d->m_autoscale.getValue();
}

void PlayerControler::setAutoscale( bool new_val )
{
    Q_D(PlayerControler);
    d->m_autoscale.setValue( new_val );
}

//AOUT PROPERTIES

PlayerControler::AoutPtr PlayerControler::getAout()
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    return AoutPtr( vlc_player_aout_Hold( d->m_player ), false );
}

void PlayerControler::setVolume(float volume)
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    vlc_player_aout_SetVolume( d->m_player, volume );
}

void PlayerControler::setVolumeUp()
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    vlc_player_aout_IncrementVolume( d->m_player, 1, NULL );
}

void PlayerControler::setVolumeDown()
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    vlc_player_aout_DecrementVolume( d->m_player, 1, NULL );
}

void PlayerControler::setMuted(bool muted)
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    vlc_player_aout_Mute( d->m_player, muted );
}

void PlayerControler::toggleMuted()
{
    Q_D(PlayerControler);
    setMuted( !d->m_muted );
}

bool PlayerControler::hasAudioVisualization() const
{
    Q_D(const PlayerControler);
    return d->m_audioVisualization.hasCurrent();
}


void PlayerControler::menusUpdateAudio( const QString& data )
{
    AoutPtr aout = getAout();
    if( aout )
        aout_DeviceSet( aout.get(), qtu(data) );
}

//MISC

void PlayerControler::setABloopState(ABLoopState state)
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    vlc_player_SetAtoBLoop( d->m_player, static_cast<vlc_player_abloop>(state));
}

void PlayerControler::toggleABloopState()
{
    Q_D(PlayerControler);
    switch (d->m_ABLoopState) {
    case ABLOOP_STATE_NONE:
        setABloopState(ABLOOP_STATE_A);
        break;
    case ABLOOP_STATE_A:
        setABloopState(ABLOOP_STATE_B);
        break;
    case ABLOOP_STATE_B:
        setABloopState(ABLOOP_STATE_NONE);
        break;
    }
}

void PlayerControler::toggleRecord()
{
    Q_D(PlayerControler);
    setRecording(!d->m_recording);
}

void PlayerControler::setRecording( bool recording )
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    vlc_player_SetRecordingEnabled( d->m_player, recording );
}

void PlayerControler::snapshot()
{
    VoutPtr vout = getVout();
    if (vout)
        var_TriggerCallback(vout.get(), "video-snapshot");
}


//OTHER


/* Playlist Control functions */

void PlayerControler::requestArtUpdate( input_item_t *p_item, bool b_forced )
{
    Q_D(PlayerControler);
    bool b_current_item = false;
    if ( !p_item )
    {
        /* default to current item */
        vlc_player_locker lock{ d->m_player };
        if ( vlc_player_IsStarted( d->m_player ) )
        {
            p_item = vlc_player_GetCurrentMedia( d->m_player );
            b_current_item = true;
        }
    }

    if ( p_item )
    {
        /* check if it has already been enqueued */
        if ( p_item->p_meta && !b_forced )
        {
            int status = vlc_meta_GetStatus( p_item->p_meta );
            if ( status & ( ITEM_ART_NOTFOUND|ITEM_ART_FETCHED ) )
                return;
        }
        libvlc_ArtRequest( d->p_intf->obj.libvlc, p_item,
                           (b_forced) ? META_REQUEST_OPTION_SCOPE_ANY
                                      : META_REQUEST_OPTION_NONE,
                           NULL, NULL );
        /* No input will signal the cover art to update,
             * let's do it ourself */
        if ( b_current_item )
            d->UpdateArt( p_item );
        else
            emit artChanged( p_item );
    }
}

const QString PlayerControler::decodeArtURL( input_item_t *p_item )
{
    assert( p_item );

    char *psz_art = input_item_GetArtURL( p_item );
    if( psz_art )
    {
        char *psz = vlc_uri2path( psz_art );
        free( psz_art );
        psz_art = psz;
    }

#if 0
    /* Taglib seems to define a attachment://, It won't work yet */
    url = url.replace( "attachment://", "" );
#endif

    QString path = qfu( psz_art ? psz_art : "" );
    free( psz_art );
    return path;
}

void PlayerControler::setArt( input_item_t *p_item, QString fileUrl )
{
    Q_D(PlayerControler);
    if( hasInput() )
    {
        char *psz_cachedir = config_GetUserDir( VLC_CACHE_DIR );
        QString old_url = decodeArtURL( p_item );
        old_url = QDir( old_url ).canonicalPath();

        if( old_url.startsWith( QString::fromUtf8( psz_cachedir ) ) )
            QFile( old_url ).remove(); /* Purge cached artwork */

        free( psz_cachedir );

        input_item_SetArtURL( p_item , fileUrl.toUtf8().constData() );
        d->UpdateArt( p_item );
    }
}

int PlayerControler::AddAssociatedMedia(es_format_category_e cat, const QString &uri, bool select, bool notify, bool check_ext)
{
    Q_D(PlayerControler);
    vlc_player_locker lock{ d->m_player };
    return vlc_player_AddAssociatedMedia( d->m_player, cat, qtu(uri), select, notify, check_ext );
}

#define QABSTRACTLIST_GETTER( type, fun, var ) \
    type* PlayerControler::fun() \
    { \
        Q_D(PlayerControler); \
        return &d->var; \
    }


QABSTRACTLIST_GETTER( TrackListModel, getVideoTracks, m_videoTracks)
QABSTRACTLIST_GETTER( TrackListModel, getAudioTracks, m_audioTracks)
QABSTRACTLIST_GETTER( TrackListModel, getSubtitleTracks, m_subtitleTracks)
QABSTRACTLIST_GETTER( TitleListModel, getTitles, m_titleList)
QABSTRACTLIST_GETTER( ChapterListModel,getChapters, m_chapterList)
QABSTRACTLIST_GETTER( ProgramListModel, getPrograms, m_programList)
QABSTRACTLIST_GETTER( VLCVarChoiceModel, getZoom, m_zoom)
QABSTRACTLIST_GETTER( VLCVarChoiceModel, getAspectRatio, m_aspectRatio)
QABSTRACTLIST_GETTER( VLCVarChoiceModel, getCrop, m_crop)
QABSTRACTLIST_GETTER( VLCVarChoiceModel, getDeinterlace, m_deinterlace)
QABSTRACTLIST_GETTER( VLCVarChoiceModel, getDeinterlaceMode, m_deinterlaceMode)
QABSTRACTLIST_GETTER( VLCVarChoiceModel, getAudioStereoMode, m_audioStereoMode)
QABSTRACTLIST_GETTER( VLCVarChoiceModel, getAudioVisualizations, m_audioVisualization)


#undef QABSTRACTLIST_GETTER

#define PRIMITIVETYPE_GETTER( type, fun, var ) \
    type PlayerControler::fun() const \
    { \
        Q_D(const PlayerControler); \
        return d->var; \
    }


PRIMITIVETYPE_GETTER(PlayerControler::PlayingState, getPlayingState, m_playing_status)
PRIMITIVETYPE_GETTER(QString, getName, m_name)
PRIMITIVETYPE_GETTER(VLCTick, getTime, m_time)
PRIMITIVETYPE_GETTER(float, getPosition, m_position)
PRIMITIVETYPE_GETTER(VLCTick, getLength, m_length)
PRIMITIVETYPE_GETTER(VLCTick, getAudioDelay, m_audioDelay)
PRIMITIVETYPE_GETTER(VLCTick, getSubtitleDelay, m_subtitleDelay)
PRIMITIVETYPE_GETTER(bool, isSeekable, m_capabilities & VLC_INPUT_CAPABILITIES_SEEKABLE)
PRIMITIVETYPE_GETTER(bool, isRewindable, m_capabilities & VLC_INPUT_CAPABILITIES_REWINDABLE)
PRIMITIVETYPE_GETTER(bool, isPausable, m_capabilities & VLC_INPUT_CAPABILITIES_PAUSEABLE)
PRIMITIVETYPE_GETTER(bool, isRecordable, m_capabilities & VLC_INPUT_CAPABILITIES_RECORDABLE)
PRIMITIVETYPE_GETTER(bool, isRateChangable, m_capabilities & VLC_INPUT_CAPABILITIES_CHANGE_RATE)
PRIMITIVETYPE_GETTER(float, getSubtitleFPS, m_subtitleFPS)
PRIMITIVETYPE_GETTER(bool, hasVideoOutput, m_hasVideo)
PRIMITIVETYPE_GETTER(float, getBuffering, m_buffering)
PRIMITIVETYPE_GETTER(PlayerControler::MediaStopAction, getMediaStopAction, m_mediaStopAction)
PRIMITIVETYPE_GETTER(float, getVolume, m_volume)
PRIMITIVETYPE_GETTER(bool, isMuted, m_muted)
PRIMITIVETYPE_GETTER(bool, isFullscreen, m_fullscreen)
PRIMITIVETYPE_GETTER(bool, getWallpaperMode, m_wallpaperMode)
PRIMITIVETYPE_GETTER(float, getRate, m_rate)
PRIMITIVETYPE_GETTER(bool, hasTitles, m_hasTitles)
PRIMITIVETYPE_GETTER(bool, hasChapters, m_hasChapters)
PRIMITIVETYPE_GETTER(bool, hasMenu, m_hasMenu)
PRIMITIVETYPE_GETTER(bool, isEncrypted, m_encrypted)
PRIMITIVETYPE_GETTER(bool, isRecording, m_recording)
PRIMITIVETYPE_GETTER(PlayerControler::ABLoopState, getABloopState, m_ABLoopState)
PRIMITIVETYPE_GETTER(VLCTick, getABLoopA, m_ABLoopA)
PRIMITIVETYPE_GETTER(VLCTick, getABLoopB, m_ABLoopB)
PRIMITIVETYPE_GETTER(bool, isTeletextEnabled, m_teletextEnabled)
PRIMITIVETYPE_GETTER(bool, isTeletextAvailable, m_teletextAvailable)
PRIMITIVETYPE_GETTER(int, getTeletextPage, m_teletextPage)
PRIMITIVETYPE_GETTER(bool, getTeletextTransparency, m_teletextTransparent)

#undef PRIMITIVETYPE_GETTER
