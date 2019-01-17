#ifndef VIDEORENDERER_HPP
#define VIDEORENDERER_HPP

#include <QObject>
#include <QMutex>
#include "qt.hpp"
#include "vlc_vout_window.h"
#include "videosurface.hpp"

class VideoRenderer : public QObject
{
    Q_OBJECT
public:
    VideoRenderer(QObject* parent = nullptr);
    virtual ~VideoRenderer() {}

    virtual void setupVoutWindow(vout_window_t* window);
    virtual void enableVideo(unsigned width, unsigned height, bool fullscreen);
    virtual void disableVideo();

    virtual VideoSurfaceProvider* getVideoSurfaceProvider() = 0;

public slots:
    void onMousePressed( int vlcButton );
    void onMouseReleased( int vlcButton );
    void onMouseDoubleClick( int vlcButton );
    void onMouseMoved( float x, float y );
    void onSurfaceSizeChanged(QSizeF size);

protected:
    QMutex m_voutlock;
    vout_window_t* m_voutWindow = nullptr;
    bool m_hasVideo = false;

};

#endif // VIDEORENDERER_HPP
