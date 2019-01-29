#ifndef VLC_QT_VIDEORENDERERWL_HPP
#define VLC_QT_VIDEORENDERERWL_HPP

#include <inttypes.h>
#include <QtQuick/QQuickItem>
#include <QMutex>
#include <QtQuick/QSGRectangleNode>
#include <components/qml_main_context.hpp>
#include "qt.hpp"

#include "videorenderer.hpp"

class MainInterface;
class VideoSurfaceWayland;

struct wl_compositor;
struct wl_subcompositor;
struct wl_subsurface;
struct wl_registry;

class VideoRendererWayland: public VideoRenderer
{
    Q_OBJECT
public:
    VideoRendererWayland(MainInterface* p_mi,  QObject *parent = nullptr);
    ~VideoRendererWayland() override;

    void enableVideo(unsigned int width, unsigned int height, bool fullscreen);
    void disableVideo();

    VideoSurfaceProvider *getVideoSurfaceProvider() override;

signals:
    void updated();

private:
    /* wayland registry callbacks */
    static void registry_global( void *, wl_registry *, uint32_t,
                                 const char *, uint32_t );
    static void registry_global_remove( void *, wl_registry *, uint32_t );

    MainInterface *m_mainInterface;
    VideoSurfaceWayland *m_surfaceProvider = nullptr;
    wl_compositor *m_compositor = nullptr;
    wl_subcompositor *m_subcompositor = nullptr;
    wl_subsurface *m_subsurface = nullptr;
    wl_surface *m_surface = nullptr;
};

class VideoSurfaceWayland : public VideoSurfaceProvider
{
    Q_OBJECT
public:
    VideoSurfaceWayland(VideoRendererWayland* renderer, QObject* parent = nullptr);

private:
    QSGNode *updatePaintNode(QQuickItem* item, QSGNode *, QQuickItem::UpdatePaintNodeData *) override;

private:
    VideoRendererWayland* m_renderer = nullptr;
};

#endif // VLC_QT_VIDEORENDERERWL_HPP
