/*
 *
 * Copyright 2003, 2004 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Assburner is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Assburner is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef THUMBNAIL_LOADER_H
#define THUMBNAIL_LOADER_H

#include <qstring.h>
#include <qimage.h>
#include <qrect.h>
#include <qmap.h>
#include <q3valuelist.h>
#include <qpixmap.h>

struct ThumbnailEntry
{
	QSize size;
	QPixmap pixmap;
};

struct ThumbnailSource
{
	Q3ValueList<ThumbnailEntry> entries;
	bool contains( const QSize & size ) {
		for( Q3ValueListIterator<ThumbnailEntry> it = entries.begin(); it != entries.end(); ++it )
			if( (*it).size == size )
				return true;
		return false;
	}
	QPixmap get( const QSize & size ) {
		for( Q3ValueListIterator<ThumbnailEntry> it = entries.begin(); it != entries.end(); ++it )
			if( (*it).size == size )
				return (*it).pixmap;
		return QPixmap();
	}
	void set( const QSize & size, const QPixmap & pix ){
		for( Q3ValueListIterator<ThumbnailEntry> it = entries.begin(); it != entries.end(); ++it )
			if( (*it).size == size ) {
				(*it).pixmap = pix;
				return;
			}
		ThumbnailEntry ne;
		ne.size = size;
		ne.pixmap = pix;
		entries += ne;
	}
};

typedef QMap<QString, ThumbnailSource> PixCache;
typedef QMapIterator<QString, ThumbnailSource> PixCacheIter;

class ThumbnailLoader
{
public:

	static QString thumbnailCachePath( const QString & path, const QSize & size = QSize() );
	static void clear( const QString & fileName );
	static QPixmap load( const QString & fileName, const QSize & retSize );

protected:

	static PixCache * mCache;
};


#endif // THUMBNAIL_LOADER_H

