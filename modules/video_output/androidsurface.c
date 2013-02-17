/*****************************************************************************
 * androidsurface.c: android video output using Surface Flinger
 *****************************************************************************
 * Copyright © 2011 VideoLAN
 *
 * Authors: Ming Hu <tewilove@gmail.com>
 *          Ludovic Fauvet <etix@l0cal.com>
 *          Sébastien Toque <xilasz@gmail.com>
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_vout_display.h>
#include <vlc_picture_pool.h>

#include <dlfcn.h>

#ifndef ANDROID_SYM_S_LOCK
# define ANDROID_SYM_S_LOCK "_ZN7android7Surface4lockEPNS0_11SurfaceInfoEb"
#endif
#ifndef ANDROID_SYM_S_LOCK2
# define ANDROID_SYM_S_LOCK2 "_ZN7android7Surface4lockEPNS0_11SurfaceInfoEPNS_6RegionE"
#endif
#ifndef ANDROID_SYM_S_UNLOCK
# define ANDROID_SYM_S_UNLOCK "_ZN7android7Surface13unlockAndPostEv"
#endif

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
#define CHROMA_TEXT N_("Chroma used")
#define CHROMA_LONGTEXT N_(\
    "Force use of a specific chroma for output. Default is RGB32.")

#define CFG_PREFIX "androidsurface-"

static int  Open (vlc_object_t *);
static void Close(vlc_object_t *);

vlc_module_begin()
    set_category(CAT_VIDEO)
    set_subcategory(SUBCAT_VIDEO_VOUT)
    set_shortname("AndroidSurface")
    set_description(N_("Android Surface video output"))
    set_capability("vout display", 155)
    add_shortcut("androidsurface", "android")
    add_string(CFG_PREFIX "chroma", NULL, CHROMA_TEXT, CHROMA_LONGTEXT, true)
    set_callbacks(Open, Close)
vlc_module_end()

/*****************************************************************************
 * JNI prototypes
 *****************************************************************************/

extern void *jni_LockAndGetAndroidSurface();
extern void  jni_UnlockAndroidSurface();
extern void  jni_SetAndroidSurfaceSize(int width, int height, int sar_num, int sar_den);

// _ZN7android7Surface4lockEPNS0_11SurfaceInfoEb
typedef void (*Surface_lock)(void *, void *, int);
// _ZN7android7Surface4lockEPNS0_11SurfaceInfoEPNS_6RegionE
typedef void (*Surface_lock2)(void *, void *, void *);
// _ZN7android7Surface13unlockAndPostEv
typedef void (*Surface_unlockAndPost)(void *);

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/

static picture_pool_t   *Pool  (vout_display_t *, unsigned);
static void             Display(vout_display_t *, picture_t *, subpicture_t *);
static int              Control(vout_display_t *, int, va_list);

/* */
typedef struct _SurfaceInfo {
    uint32_t    w;
    uint32_t    h;
    uint32_t    s;
    uint32_t    usage;
    uint32_t    format;
    uint32_t*   bits;
    uint32_t    reserved[2];
} SurfaceInfo;

/* */
struct vout_display_sys_t {
    picture_pool_t *pool;
    void *p_library;
    Surface_lock s_lock;
    Surface_lock2 s_lock2;
    Surface_unlockAndPost s_unlockAndPost;

    picture_resource_t resource;

    /* density */
    int i_sar_num;
    int i_sar_den;

    bool b_is_direct;
};

struct picture_sys_t {
    void *surf;
    SurfaceInfo info;
    vout_display_sys_t *sys;
};

static int  AndroidLockSurface(picture_t *);
static void AndroidUnlockSurface(picture_t *);

static vlc_mutex_t single_instance = VLC_STATIC_MUTEX;

static inline void *LoadSurface(const char *psz_lib, vout_display_sys_t *sys)
{
    void *p_library = dlopen(psz_lib, RTLD_NOW);
    if (!p_library)
        return NULL;

    sys->s_lock = (Surface_lock)(dlsym(p_library, ANDROID_SYM_S_LOCK));
    sys->s_lock2 = (Surface_lock2)(dlsym(p_library, ANDROID_SYM_S_LOCK2));
    sys->s_unlockAndPost =
        (Surface_unlockAndPost)(dlsym(p_library, ANDROID_SYM_S_UNLOCK));

    if ((sys->s_lock || sys->s_lock2) && sys->s_unlockAndPost)
        return p_library;

    dlclose(p_library);
    return NULL;
}

static void *InitLibrary(vout_display_sys_t *sys)
{
    static const char *libs[] = {
        "libsurfaceflinger_client.so",
        "libgui.so",
        "libui.so"
    };

    for (size_t i = 0; i < sizeof(libs) / sizeof(*libs); i++) {
        void *lib = LoadSurface(libs[i], sys);
        if (lib)
            return lib;
    }
    return NULL;
}

vout_display_t *g_vd;

static int Open(vlc_object_t *p_this)
{
    vout_display_t *vd = (vout_display_t *)p_this;

    /* */
    if (vlc_mutex_trylock(&single_instance) != 0) {
        msg_Err(vd, "Can't start more than one instance at a time");
        return VLC_EGENERIC;
    }

    /* Allocate structure */
    vout_display_sys_t *sys = (struct vout_display_sys_t*) calloc(1, sizeof(*sys));
    if (!sys) {
        vlc_mutex_unlock(&single_instance);
        return VLC_ENOMEM;
    }

    /* */
    sys->p_library = InitLibrary(sys);
    if (!sys->p_library) {
        free(sys);
        msg_Err(vd, "Could not initialize libui.so/libgui.so/libsurfaceflinger_client.so!");
        vlc_mutex_unlock(&single_instance);
        return VLC_EGENERIC;
    }

    /* Setup chroma */
    video_format_t fmt = vd->fmt;

    char *psz_fcc = var_InheritString(vd, CFG_PREFIX "chroma");
    int i_pictures = 1;
    sys->b_is_direct = false;
    if (fmt.i_chroma == VLC_CODEC_OPAQUE_VBUF) {
        // Keep the opaque format
        i_pictures = 31;
        sys->b_is_direct = true;
    } else if( psz_fcc ) {
        fmt.i_chroma = vlc_fourcc_GetCodecFromString(VIDEO_ES, psz_fcc);
        free(psz_fcc);
    } else
        fmt.i_chroma = VLC_CODEC_RGB32;

    switch(fmt.i_chroma) {
        case VLC_CODEC_YV12:
            /* avoid swscale usage by asking for I420 instead since the
             * vout already has code to swap the buffers */
            fmt.i_chroma = VLC_CODEC_I420;
        case VLC_CODEC_I420:
            break;

        case VLC_CODEC_RGB16:
            fmt.i_bmask = 0x0000001f;
            fmt.i_gmask = 0x000007e0;
            fmt.i_rmask = 0x0000f800;
            break;

        case VLC_CODEC_RGB32:
            fmt.i_rmask  = 0x000000ff;
            fmt.i_gmask  = 0x0000ff00;
            fmt.i_bmask  = 0x00ff0000;
            break;

        default:
            break;
//            return VLC_EGENERIC;
    }
    video_format_FixRgb(&fmt);

    msg_Dbg(vd, "Pixel format %4.4s", (char*)&fmt.i_chroma);
    g_vd = vd;

    /* Create the associated picture */
    picture_t** pictures[31];
    for (int i = 0; i < i_pictures; i++) {
    picture_resource_t *rsc = &sys->resource;
    rsc->p_sys = malloc(sizeof(*rsc->p_sys));
    if (!rsc->p_sys)
        goto enomem;
    rsc->p_sys->sys = sys;

    for (int j = 0; j < PICTURE_PLANE_MAX; j++) {
        rsc->p[j].p_pixels = NULL;
        rsc->p[j].i_pitch = 0;
        rsc->p[j].i_lines = 0;
    }
    picture_t *picture = picture_NewFromResource(&fmt, rsc);
    if (!picture)
        goto enomem;
    pictures[i] = picture;
    }

    /* Wrap it into a picture pool */
    picture_pool_configuration_t pool_cfg;
    memset(&pool_cfg, 0, sizeof(pool_cfg));
    pool_cfg.picture_count = i_pictures;
    pool_cfg.picture       = pictures;
    pool_cfg.lock          = AndroidLockSurface;
    pool_cfg.unlock        = AndroidUnlockSurface;

    sys->pool = picture_pool_NewExtended(&pool_cfg);
    if (!sys->pool) {
//        picture_Release(picture);
        goto enomem;
    }

    /* Setup vout_display */
    vd->sys     = sys;
    vd->fmt     = fmt;
    vd->pool    = Pool;
    vd->display = Display;
    vd->control = Control;
    vd->prepare = NULL;
    vd->manage  = NULL;

    /* Fix initial state */
    vout_display_SendEventFullscreen(vd, false);

    sys->i_sar_num = vd->source.i_sar_num;
    sys->i_sar_den = vd->source.i_sar_den;

    return VLC_SUCCESS;

enomem:
//    free(rsc->p_sys);
    dlclose(sys->p_library);
    free(sys);
    vlc_mutex_unlock(&single_instance);
    return VLC_ENOMEM;
}

static void Close(vlc_object_t *p_this)
{
    vout_display_t *vd = (vout_display_t *)p_this;
    vout_display_sys_t *sys = vd->sys;

    picture_pool_Delete(sys->pool);
    dlclose(sys->p_library);
    free(sys);
    vlc_mutex_unlock(&single_instance);
}

static picture_pool_t *Pool(vout_display_t *vd, unsigned count)
{
    VLC_UNUSED(count);

    return vd->sys->pool;
}

#define ALIGN_16_PIXELS( x ) ( ( ( x ) + 15 ) / 16 * 16 )
static void SetupPictureYV12( SurfaceInfo* p_surfaceInfo, picture_t *p_picture )
{
    /* according to document of android.graphics.ImageFormat.YV12 */
    int i_stride = ALIGN_16_PIXELS( p_surfaceInfo->s );
    int i_c_stride = ALIGN_16_PIXELS( i_stride / 2 );

    p_picture->p->i_pitch = i_stride;

    /* Fill chroma planes for planar YUV */
    for( int n = 1; n < p_picture->i_planes; n++ )
    {
        const plane_t *o = &p_picture->p[n-1];
        plane_t *p = &p_picture->p[n];

        p->p_pixels = o->p_pixels + o->i_lines * o->i_pitch;
        p->i_pitch  = i_c_stride;
        p->i_lines  = p_picture->format.i_height / 2;
    }

    if( vlc_fourcc_AreUVPlanesSwapped( p_picture->format.i_chroma,
                                       VLC_CODEC_YV12 ) ) {
        uint8_t *p_tmp = p_picture->p[1].p_pixels;
        p_picture->p[1].p_pixels = p_picture->p[2].p_pixels;
        p_picture->p[2].p_pixels = p_tmp;
    }
}

static int  AndroidLockSurface(picture_t *picture)
{
    picture_sys_t *picsys = picture->p_sys;
    vout_display_sys_t *sys = picsys->sys;
    SurfaceInfo *info;
    uint32_t sw, sh;
    void *surf;

    if (sys->b_is_direct) {
        if (!picture->p[0].p_pixels) {
            picture->p[0].p_pixels = calloc(1, 50);
//            msg_Dbg(g_vd, "Lock surface allocated %p", picture->p[0].p_pixels);
        } else {
//            msg_Dbg(g_vd, "Lock surface reused %p", picture->p[0].p_pixels);
        }
//        msg_Dbg(g_vd, "Lock surface refcount %d", picture->gc.refcount);
        return VLC_SUCCESS;
    }

    sw = picture->p[0].i_visible_pitch / picture->p[0].i_pixel_pitch;
    sh = picture->p[0].i_visible_lines;

    picsys->surf = surf = jni_LockAndGetAndroidSurface();
    info = &(picsys->info);

    if (unlikely(!surf)) {
        jni_UnlockAndroidSurface();
        return VLC_EGENERIC;
    }

    if (sys->s_lock)
        sys->s_lock(surf, info, 1);
    else
        sys->s_lock2(surf, info, NULL);

    // For RGB (32 or 16) we need to align on 8 or 4 pixels, 16 pixels for YUV
    int align_pixels = (16 / picture->p[0].i_pixel_pitch) - 1;
    uint32_t aligned_width = (sw + align_pixels) & ~align_pixels;

    if (info->w != aligned_width || info->h != sh) {
        // input size doesn't match the surface size -> request a resize
        jni_SetAndroidSurfaceSize(sw, sh, sys->i_sar_num, sys->i_sar_den);
        sys->s_unlockAndPost(surf);
        jni_UnlockAndroidSurface();
        return VLC_EGENERIC;
    }

    picture->p[0].p_pixels = (uint8_t*)info->bits;
    picture->p[0].i_lines = info->h;
    picture->p[0].i_pitch = picture->p[0].i_pixel_pitch * info->s;

    if (info->format == 0x32315659 /*ANDROID_IMAGE_FORMAT_YV12*/)
        SetupPictureYV12(info, picture);

    return VLC_SUCCESS;
}

static void AndroidUnlockSurface(picture_t *picture)
{
    picture_sys_t *picsys = picture->p_sys;
    vout_display_sys_t *sys = picsys->sys;

    if (sys->b_is_direct) {
        void *opaque = ((void**)picture->p[0].p_pixels)[0];
        uint32_t index = ((uint32_t*)picture->p[0].p_pixels)[1];
        void (*display_buffer)(void*,uint32_t,int) = ((void**)picture->p[0].p_pixels)[2];
        int display = ((uint32_t*)picture->p[0].p_pixels)[3];
        msg_Dbg(g_vd, "unlock surface %p, index %d display %d", picture->p[0].p_pixels, index, display);
        if (display == 0 || display == 1) {
//        msg_Dbg(g_vd, "calling display_buffer %p(%p, %d, %d)", display_buffer, opaque, index, display);
        display_buffer(opaque, index, display);
        } else {
//        msg_Dbg(g_vd, "not calling display_buffer, already rendered");
        }
        return;
    }

    if (likely(picsys->surf))
        sys->s_unlockAndPost(picsys->surf);
    jni_UnlockAndroidSurface();
}

static void Display(vout_display_t *vd, picture_t *picture, subpicture_t *subpicture)
{
    picture_sys_t *picsys = picture->p_sys;
    vout_display_sys_t *sys = picsys->sys;
    VLC_UNUSED(vd);
    VLC_UNUSED(subpicture);

    msg_Dbg(g_vd, "Display picture %p, refcount %d", picture, picture->gc.refcount);

    if (sys->b_is_direct) {
//        ((uint32_t*)picture->p[0].p_pixels)[3] = 1;

        void *opaque = ((void**)picture->p[0].p_pixels)[0];
        uint32_t index = ((uint32_t*)picture->p[0].p_pixels)[1];
        void (*display_buffer)(void*,uint32_t,int) = ((void**)picture->p[0].p_pixels)[2];
        if (!((uint32_t*)picture->p[0].p_pixels)[3])
            display_buffer(opaque, index, 1);
        ((uint32_t*)picture->p[0].p_pixels)[3] = 2;
    }

    /* refcount lowers to 0, and pool_cfg.unlock is called */

    picture_Release(picture);
}

static int Control(vout_display_t *vd, int query, va_list args)
{
    VLC_UNUSED(args);

    switch (query) {
    case VOUT_DISPLAY_HIDE_MOUSE:
        return VLC_SUCCESS;

    default:
        msg_Err(vd, "Unknown request in android vout display");

    case VOUT_DISPLAY_CHANGE_FULLSCREEN:
    case VOUT_DISPLAY_CHANGE_WINDOW_STATE:
    case VOUT_DISPLAY_CHANGE_DISPLAY_SIZE:
    case VOUT_DISPLAY_CHANGE_DISPLAY_FILLED:
    case VOUT_DISPLAY_CHANGE_ZOOM:
    case VOUT_DISPLAY_CHANGE_SOURCE_ASPECT:
    case VOUT_DISPLAY_CHANGE_SOURCE_CROP:
    case VOUT_DISPLAY_GET_OPENGL:
        return VLC_EGENERIC;
    }
}
