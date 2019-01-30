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
#include "videorendererdummy.hpp"
#include <QtQuick/QQuickWindow>

VideoSurfaceProviderDummy::VideoSurfaceProviderDummy(QObject* parent)
    : VideoSurfaceProvider(parent)
{
}

QSGNode*VideoSurfaceProviderDummy::updatePaintNode(QQuickItem* item, QSGNode* oldNode, QQuickItem::UpdatePaintNodeData*)
{
    QSGRectangleNode* node = static_cast<QSGRectangleNode*>(oldNode);

    if (!node)
    {
        node = item->window()->createRectangleNode();
        node->setColor(Qt::black);
    }
    node->setRect(item->boundingRect());
    return node;
}

VideoRendererDummy::VideoRendererDummy(MainInterface* p_mi, QObject* parent)
    : VideoRenderer(parent)
    , m_surfaceProvider(new VideoSurfaceProviderDummy(this))
{
}

VideoSurfaceProvider*VideoRendererDummy::getVideoSurfaceProvider()
{
    return m_surfaceProvider;
}
