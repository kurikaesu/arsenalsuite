
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
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/src/imagecache.cpp $
 */

#include "imagecache.h"


ImageCache::ImageCache()
{}


void ImageCache::addImage( int num, const QImage & img )
{
	CacheInfo & ci = mCacheMap[num];
	ci.image = img;
	ci.status = ImageLoaded;
	emit frameStatusChange(num,ImageLoaded);
}

void ImageCache::addTexInfo( int num, const TexInfo & texInfo )
{
	CacheInfo & ci = mCacheMap[num];
	ci.texInfo = texInfo;
	ci.status = ImageLoaded;
	emit frameStatusChange(num,ImageLoaded);
}

void ImageCache::setStatus( int num, int status )
{
	mCacheMap[num].status = status;
	emit frameStatusChange(num,status);
}

int ImageCache::status( int num )
{
	return mCacheMap.contains(num) ? mCacheMap[num].status : ImageNoInfo;
}

const TexInfo & ImageCache::texInfo( int num )
{
	static TexInfo _st;
	return (status(num)==ImageLoaded) ? mCacheMap[num].texInfo : _st;
}

QImage ImageCache::qImage( int num )
{
	return (status(num)==ImageLoaded) ? mCacheMap[num].image : QImage();
}

void ImageCache::clear()
{
	QMap<int, CacheInfo>::Iterator it = mCacheMap.begin();
	CacheInfo logo;
	bool lf=false;

	for(; it!=mCacheMap.end(); ++it ){
		if( it.key()==-1 ){
			logo = it.value();
			lf = true;
		}else{
			emit destroyTexInfo( it.value().texInfo );
		}
	}

	mCacheMap.clear();

	if( lf )
		mCacheMap[-1] = logo;
}

