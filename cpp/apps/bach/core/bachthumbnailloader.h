
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Bach.
 *
 * Bach is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Bach is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Bach; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: bachthumbnailloader.h 9408 2010-03-03 22:35:49Z brobison $
 */

#ifndef BACH_THUMBNAIL_LOADER_H
#define BACH_THUMBNAIL_LOADER_H

#include <qstring.h>
#include <qimage.h>
#include <qrect.h>
#include <qmap.h>
#include <qpixmap.h>

class BachAsset;

#ifdef USE_MEMCACHED
#include "qmemcached.h"
#endif

class ThumbnailLoader
{
public:
	ThumbnailLoader();

        static QString thumbnailCachePath( const QString & cacheDir, const QString & path, const QSize & size );
        static QString thumbnailCachePath( const QString & cacheDir, const QString & path );
        static void setDiskTnSize( const QSize & );

	static void clear( const QString & fileName );
	static QPixmap load( const QString & cacheDir, const QString & fileName, const QSize & retSize, int rotate=0, bool generate=true );
	static QImage loadImage( const BachAsset & );
	static QImage loadFromDisk( const QString & path );

#ifdef USE_MEMCACHED
	static QPixmap loadFromMemCache( const QString & key );
	static void saveToMemCache( const QString & key, const QPixmap & pix );
#endif
#ifdef USE_IMAGE_MAGICK
	static QImage loadImageMagick( const QString & path );
#endif

protected:
#ifdef USE_MEMCACHED
	static QMemCached * mMemCache;
#endif
};

#endif // THUMBNAIL_LOADER_H

