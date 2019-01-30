#include "videorenderergl.hpp"
#include <QtQuick/QSGImageNode>
#include <QtQuick/QSGRectangleNode>
#include <QtQuick/QQuickWindow>
#include <vlc_vout_window.h>
#include "main_interface.hpp"

VideoRendererGL::VideoRendererGL(MainInterface* p_mi, QObject* parent)
    : VideoRenderer(parent)
    , m_mainInterface(p_mi)
{
    assert(m_mainInterface);
    m_surfaceProvider = new VideoSurfaceGL(this);
    for (int i = 0; i < 3; i++)
    {
        m_fbo[i] = nullptr;
        m_textures[i] = nullptr;
    }
}

QSharedPointer<QSGTexture> VideoRendererGL::getDisplayTexture()
{
    QMutexLocker lock(&m_lock);
    if (!m_hasTextures)
        return nullptr;
    if (m_updated)
    {
        qSwap(m_displayIdx, m_bufferIdx);
        m_updated = false;
    }
    return m_textures[m_displayIdx];
}

bool VideoRendererGL::make_current_cb(void* data, bool current)
{
    VideoRendererGL* that = static_cast<VideoRendererGL*>(data);
    QMutexLocker lock(&that->m_lock);
    if (!that->m_ctx || !that->m_surface)
    {
        return false;
    }

    if (current)
        return that->m_ctx->makeCurrent(that->m_surface);
    else
        that->m_ctx->doneCurrent();
    return true;
}

void*VideoRendererGL::get_proc_address_cb(void* data, const char* procName)
{
    VideoRendererGL* that = static_cast<VideoRendererGL*>(data);
    return (void*)that->m_ctx->getProcAddress(procName);
}

void VideoRendererGL::swap_cb(void* data)
{
    VideoRendererGL* that = static_cast<VideoRendererGL*>(data);
    {
        QMutexLocker lock(&that->m_lock);
        qSwap(that->m_renderIdx, that->m_bufferIdx);
        that->m_updated = true;
        that->m_hasTextures = true;
    }
    that->m_fbo[that->m_renderIdx]->bind();
    emit that->updated();
}

bool VideoRendererGL::setup_cb(void* data)
{
    VideoRendererGL* that = static_cast<VideoRendererGL*>(data);

    QMutexLocker lock(&that->m_lock);
    that->m_window = that->m_mainInterface->getRootQuickWindow();
    if (! that->m_window)
        return false;

    QOpenGLContext *current = that->m_window->openglContext();

    that->m_ctx = new QOpenGLContext();
    if (!that->m_ctx)
    {
        that->m_window = nullptr;
        return false;
    }
    QSurfaceFormat format = current->format();

    that->m_ctx->setFormat(format);
    that->m_ctx->setShareContext(current);
    that->m_ctx->create();

    that->m_surface = new QOffscreenSurface();
    if (!that->m_surface)
    {
        that->m_window = nullptr;
        delete that->m_ctx;
        that->m_ctx = nullptr;
        return false;
    }
    that->m_surface->setFormat(that->m_ctx->format());
    that->m_surface->create();

    that->m_glViewport = (GlViewportPF)that->m_ctx->getProcAddress("glViewport");

    return true;
}

void VideoRendererGL::cleanup_cb(void* data)
{
    VideoRendererGL* that = static_cast<VideoRendererGL*>(data);

    QMutexLocker lock(&that->m_lock);
    for (int i =0; i < 3; i++)
    {
        if (that->m_fbo[i])
        {
            delete that->m_fbo[i];
            that->m_fbo[i] = nullptr;
        }
        if (that->m_textures[i])
        {
            that->m_textures[i] = nullptr;
        }
    }
    that->m_size = QSize();
    that->m_window = nullptr;
    that->m_hasTextures = false;
}

void VideoRendererGL::resize_cb(void* data, unsigned width, unsigned height)
{
    VideoRendererGL* that = static_cast<VideoRendererGL*>(data);

    QMutexLocker lock(&that->m_lock);
    QSize newsize(width, height);
    if (that->m_size != newsize)
    {
        that->m_size = newsize;
        for (int i =0; i < 3; i++)
        {
            if (that->m_fbo[i])
                delete that->m_fbo[i];
            that->m_fbo[i] = new QOpenGLFramebufferObject(newsize);
            that->m_textures[i] = QSharedPointer<QSGTexture>(that->m_window->createTextureFromId(that->m_fbo[i]->texture(), newsize));
            that->m_hasTextures = false;
        }
        emit that->sizeChanged(newsize);
    }
    that->m_fbo[that->m_renderIdx]->bind();
    //set the initial viewport
    that->m_glViewport(0, 0, width, height);
}

void VideoRendererGL::setupVoutWindow(vout_window_t* voutWindow)
{
    VideoRenderer::setupVoutWindow(voutWindow);
    var_Create( voutWindow, "vout", VLC_VAR_STRING );
    var_Create( voutWindow, "gl", VLC_VAR_STRING );

    var_SetString ( voutWindow, "vout", "gl" );
    var_SetString ( voutWindow, "gl", "vgl");

    var_Create( voutWindow, "vout-cb-opaque", VLC_VAR_ADDRESS );
    var_Create( voutWindow, "vout-cb-setup", VLC_VAR_ADDRESS );
    var_Create( voutWindow, "vout-cb-cleanup", VLC_VAR_ADDRESS );
    var_Create( voutWindow, "vout-cb-update-output", VLC_VAR_ADDRESS );
    var_Create( voutWindow, "vout-cb-swap", VLC_VAR_ADDRESS );
    var_Create( voutWindow, "vout-cb-make-current", VLC_VAR_ADDRESS );
    var_Create( voutWindow, "vout-cb-get-proc-address", VLC_VAR_ADDRESS );

    var_SetAddress( voutWindow, "vout-cb-opaque", this );
    var_SetAddress( voutWindow, "vout-cb-setup", (void*)&VideoRendererGL::setup_cb );
    var_SetAddress( voutWindow, "vout-cb-cleanup", (void*)&VideoRendererGL::cleanup_cb );
    var_SetAddress( voutWindow, "vout-cb-update-output", (void*)&VideoRendererGL::resize_cb );
    var_SetAddress( voutWindow, "vout-cb-swap", (void*)&VideoRendererGL::swap_cb );
    var_SetAddress( voutWindow, "vout-cb-make-current", (void*)&VideoRendererGL::make_current_cb );
    var_SetAddress( voutWindow, "vout-cb-get-proc-address", (void*)&VideoRendererGL::get_proc_address_cb );
}

VideoSurfaceProvider*VideoRendererGL::getVideoSurfaceProvider()
{
    return m_surfaceProvider;
}

////////


VideoSurfaceGL::VideoSurfaceGL(VideoRendererGL* renderer, QObject* parent)
    : VideoSurfaceProvider(parent)
    , m_renderer(renderer)
{
    connect(this, &VideoSurfaceGL::mouseMoved, m_renderer, &VideoRenderer::onMouseMoved);
    connect(this, &VideoSurfaceGL::mousePressed, m_renderer, &VideoRenderer::onMousePressed);
    connect(this, &VideoSurfaceGL::mouseDblClicked, m_renderer, &VideoRenderer::onMouseDoubleClick);
    connect(this, &VideoSurfaceGL::mouseReleased, m_renderer, &VideoRenderer::onMouseReleased);

    connect(this, &VideoSurfaceGL::surfaceSizeChanged, m_renderer, &VideoRenderer::onSurfaceSizeChanged);

    connect(m_renderer, &VideoRendererGL::updated, this, &VideoSurfaceGL::update, Qt::QueuedConnection);
    connect(m_renderer, &VideoRendererGL::sizeChanged, this, &VideoSurfaceGL::sourceSizeChanged, Qt::QueuedConnection);
}

QSGNode* VideoSurfaceGL::updatePaintNode(QQuickItem* item, QSGNode* oldNode, QQuickItem::UpdatePaintNodeData*)
{
    QSGSimpleTextureNode* node = static_cast<QSGSimpleTextureNode*>(oldNode);

    QSharedPointer<QSGTexture> newdisplayTexture = m_renderer->getDisplayTexture();
    if (!newdisplayTexture)
    {
        if (node)
            node->setRect(item->boundingRect());
        return node;
    }
    m_displayTexture = newdisplayTexture;

    if (!node)
    {
        node = new QSGSimpleTextureNode();
        node->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
    }

    node->setTexture(m_displayTexture.data());
    node->setRect(item->boundingRect());
    node->markDirty(QSGNode::DirtyMaterial);
    return node;
}

