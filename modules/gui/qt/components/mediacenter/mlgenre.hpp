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

#ifndef MLGENRE_HPP
#define MLGENRE_HPP

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "vlc_common.h"

#include <memory>
#include <QObject>
#include <QString>
#include <QList>
#include <vlc_media_library.h>
#include "mlhelper.hpp"
#include "mlqmltypes.hpp"

class MLGenre : public QObject
{
    Q_OBJECT

    Q_PROPERTY(MLParentId id READ getId CONSTANT)
    Q_PROPERTY(QString name READ getName CONSTANT)
    Q_PROPERTY(unsigned int nbtracks READ getNbTracks CONSTANT)

public:
    MLGenre( const vlc_ml_genre_t *_data, QObject *_parent = nullptr);

    MLParentId getId() const;
    QString getName() const;
    unsigned int getNbTracks() const;

    MLGenre* clone(QObject *parent = nullptr) const;

private:
    MLGenre( const MLGenre& genre, QObject *_parent = nullptr);

    MLParentId m_id;
    QString m_name;
    unsigned int m_nbTracks;
};

#endif
