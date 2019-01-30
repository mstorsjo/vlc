/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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
#ifndef VIDEORENDERERGL_HPP
#define VIDEORENDERERGL_HPP

#include <QtQuick/QQuickItem>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOffscreenSurface>
#include <QMutex>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QSGRectangleNode>
#include <components/qml_main_context.hpp>
#include "qt.hpp"
#include "main_interface.hpp"
#include "videosurface.hpp"
#include "videorenderer.hpp"

class VideoSurfaceGL;

class VideoRendererGL : public VideoRenderer
{
    Q_OBJECT
public:
    VideoRendererGL(MainInterface* p_mi,  QObject *parent = nullptr);

    QSharedPointer<QSGTexture> getDisplayTexture();

    void setupVoutWindow(vout_window_t* window) override;

    VideoSurfaceProvider* getVideoSurfaceProvider() override;

private:
    //openGL callbacks
    static bool make_current_cb(void* data, bool current);
    static void* get_proc_address_cb(void* data, const char* procName);
    static void swap_cb(void* data);
    static bool setup_cb(void* data);
    static void cleanup_cb(void* data);
    static void resize_cb(void* data, unsigned width, unsigned height);

signals:
    void updated();
    void sizeChanged(QSize);

private:
    QMutex  m_lock;

    QOpenGLContext* m_ctx = nullptr;
    QOffscreenSurface* m_surface = nullptr;

    MainInterface* m_mainInterface = nullptr;
    QQuickWindow* m_window = nullptr;
    QSize m_size;

    QSharedPointer<QSGTexture> m_textures[3] ;
    QOpenGLFramebufferObject* m_fbo[3];

    bool m_hasTextures = false;
    bool m_updated = false;
    int m_renderIdx = 0;
    int m_bufferIdx = 0;
    int m_displayIdx = 0;

    VideoSurfaceGL* m_surfaceProvider = nullptr;

    typedef void (*GlViewportPF)(GLint, GLint, GLsizei, GLsizei);
    GlViewportPF m_glViewport = nullptr;
};

class VideoSurfaceGL : public VideoSurfaceProvider
{
public:
    VideoSurfaceGL(VideoRendererGL* renderer, QObject* parent = nullptr);

private:
    QSGNode *updatePaintNode(QQuickItem* item, QSGNode *, QQuickItem::UpdatePaintNodeData *) override;

private:
    VideoRendererGL* m_renderer = nullptr;
    QSharedPointer<QSGTexture> m_displayTexture;
};



#endif // VIDEORENDERERGL_HPP
