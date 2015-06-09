

/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Arsenal.
 *
 * Arsenal is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Arsenal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Blur; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* $Author$
 * $LastChangedDate: 2007-06-19 04:27:47 +1000 (Tue, 19 Jun 2007) $
 * $Rev: 4632 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/include/imagecache.h $
 */

#ifndef IMAGE_CACHE_H
#define IMAGE_CACHE_H

#include <qobject.h>
#include <qmap.h>
#include <qimage.h>

#include "glwindow.h"

#include "afcommon.h"

class QImage;

struct FREEZER_EXPORT CacheInfo
{
	TexInfo texInfo; // Struct that holds info for OpenGL textures
	QImage image;
	int status;
};

/* ImageCache is a cache that stores either
 * QImage pointers, or TexInfo objects. It
 * also stores the status of an image.  The imagenumber
 * is used a the key to look up a particular image's
 * status, or data. */
class FREEZER_EXPORT ImageCache : public QObject
{
Q_OBJECT
public:
	ImageCache();

	/* Adds image to the cache for num, sets status to ImageLoaded */
	void addImage( int num, const QImage & image );

	/* Adds a texInfo struct to the cache for num, sets status to ImageLoaded */
	void addTexInfo( int num, const TexInfo & );

	/* Retrieves the status for num */
	int status( int num );

	/* Sets the status for num to 'status' */
	void setStatus( int num, int status );

	/* Retrieves the texInfo structure for num
	 * Returns a blank TexInfo struct if num's status!=ImageLoaded */
	const TexInfo & texInfo( int num );

	/* Retrieves the QImage pointer for num */
	QImage qImage( int num );

	/* Clears the cache */
	void clear();

	enum {
		ImageNoInfo,
		ImageLoading,
		ImageNotFound,
		ImageLoaded
	};

signals:
	/* Both of these signals are emitted every time an image is
	 * deleted.  Connect to destroyQImage if the cache is holding
	 * QImage's, or destroyTexInfo, if the cache is holding
	 * texInfo structs */
	void destroyTexInfo( const TexInfo & );
	void frameStatusChange(int fn,int status);


protected:

	QMap<int, CacheInfo> mCacheMap;
};


#endif // IMAGE_CACHE_H


