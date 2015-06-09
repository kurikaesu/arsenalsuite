
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
 * $Id: bachthumbnailloader.cpp 9408 2010-03-03 22:35:49Z brobison $
 */

#include <qfile.h>
#include <qsize.h>
#include <qdir.h>
#include <qprocess.h>
#include <qbytearray.h>
#include <qpixmap.h>
#include <qpixmapcache.h>
#include <qbuffer.h>
#include <qdebug.h>

#include "bachthumbnailloader.h"
#include "blurqt.h"
#include "path.h"
#include "imagesequenceprovider.h"

#ifdef USE_MEMCACHED
#include "qmemcached.h"
#endif

#ifdef USE_IMAGE_MAGICK
#include <Magick++.h>
#endif

#include "bachmainwindow.h"

static QPixmap sNoImage = 0;
static QSize sTnSize;

QString ThumbnailLoader::thumbnailCachePath( const QString & cacheDir, const QString & path )
{
        return cacheDir + path + "_raw.png";
}

QString ThumbnailLoader::thumbnailCachePath( const QString & cacheDir, const QString & path, const QSize & size )
{
	QString ret( path );
	if( size.isValid() )
		ret += QString("_%1x%2.png").arg( size.width() ).arg( size.height() );
	//LOG_3( QString("checking cache for %1").arg(ret) );
	return cacheDir + ret;
}

void ThumbnailLoader::clear( const QString & /*fileName*/ )
{
/*
	QString cacheFile = thumbnailCachePath( fileName );
	Path p( cacheFile );
	QDir d( p.dirPath() );
	QStringList matches = d.entryList( QStringList(p.fileName() + "*") );
	foreach( QString s, matches )
		QFile::remove( p.dirPath() + s );
*/
}

void ThumbnailLoader::setDiskTnSize( const QSize & size )
{
    sTnSize = size;
}

QPixmap ThumbnailLoader::load( const QString & cacheDir, const QString & p, const QSize & returnSize, int rotate, bool genThumb )
{
        QString cachePath = thumbnailCachePath( cacheDir, p, sTnSize );
        QTransform trannie;
        if( rotate != 0 )
            trannie.rotate(rotate);

	// first try local RAM cache
        QPixmap pix;
        if( QPixmapCache::find( cachePath, pix ) ) {
            return pix.scaled( returnSize, Qt::KeepAspectRatio, Qt::FastTransformation ).transformed(trannie);
        }

#ifdef USE_MEMCACHED
	// next try memcached
	QPixmap fromMemCache = ThumbnailLoader::loadFromMemCache( cachePath );
	if( !fromMemCache.isNull() )
		return fromMemCache;
#endif

	// then try disk cache
	if( QFile::exists( cachePath ) ) {
		QPixmap pix( cachePath );
		QPixmapCache::insert( cachePath, pix );
#ifdef USE_MEMCACHED
		ThumbnailLoader::saveToMemCache(cachePath, pix);
#endif
        return pix.scaled( returnSize, Qt::KeepAspectRatio, Qt::FastTransformation ).transformed(trannie);
	}

    QSize noSize( qMin(128, returnSize.width()), qMin(128,returnSize.width()) );
    if( !sNoImage || sNoImage.size() != noSize )
        sNoImage = QPixmap("images/noimage.png").scaled( noSize, Qt::KeepAspectRatio, Qt::FastTransformation );
    return sNoImage;
}

QImage ThumbnailLoader::loadImage( const BachAsset & asset )
{
    QString cacheDir = BachMainWindow::cacheRoot();
    QString p = asset.path();
    QSize returnSize = BachMainWindow::tnSize();
    int rotate = asset.tnRotate();

    QString cachePath = thumbnailCachePath( cacheDir, p, QSize(256,256) );

    QTransform trannie;
    if( rotate != 0 )
        trannie.rotate(rotate);

#ifdef USE_MEMCACHED
	// next try memcached
	QImage fromMemCache = ThumbnailLoader::loadFromMemCache( cachePath );
	if( !fromMemCache.isNull() )
		return fromMemCache;
#endif

	// then try disk cache
	if( QFile::exists( cachePath ) ) {
		QImage pix( cachePath );
#ifdef USE_MEMCACHED
		ThumbnailLoader::saveToMemCache(cachePath, pix);
#endif
        return pix.scaled( returnSize, Qt::KeepAspectRatio, Qt::SmoothTransformation ).transformed(trannie);
	}

/*
    QSize noSize( qMin(128, returnSize.width()), qMin(128,returnSize.width()) );
    if( !sNoImage || sNoImage.size() != noSize )
        sNoImage = QImage("images/noimage.png").scaled( noSize, Qt::KeepAspectRatio, Qt::FastTransformation );
    return QImage::fromPixmap(sNoImage);
    */
    return QImage();
}

#ifdef USE_MEMCACHED
QPixmap ThumbnailLoader::loadFromMemCache( const QString & key )
{
	if( !mMemCache ) {
		LOG_3("init memcache");
		mMemCache = new QMemCached;
		mMemCache->addServer("memcache01");
		if( mMemCache->rc() != MEMCACHED_SUCCESS ) {
			LOG_3("could not connect to memcached server");
			return QPixmap();
		}
	}

	//LOG_3("trying to find memcache record:"+key);
	QByteArray rawPix = mMemCache->get(key);
	if( mMemCache->rc() == MEMCACHED_NOTFOUND || mMemCache->rc() != MEMCACHED_SUCCESS ) {
		return QPixmap();
	}

	//LOG_3("found memcached record, WOOOOT");
	QPixmap pix;
	pix.loadFromData(QByteArray::fromBase64(rawPix), "PNG");
	//if( !pix.isNull() )
		//LOG_3("and it's valid, DOUBLE WOOOOT");
	return pix;
}

void ThumbnailLoader::saveToMemCache( const QString & key, const QPixmap & pix )
{
	if( !mMemCache ) {
		LOG_3("init memcache");
		mMemCache = new QMemCached;
		mMemCache->addServer("memcache01");
		if( mMemCache->rc() != MEMCACHED_SUCCESS ) {
			LOG_3("could not connect to memcached server");
			return;
		}
	}
	//LOG_3("trying to save memcache record");
  QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	pix.save(&buffer, "PNG");
	mMemCache->set(key, bytes.toBase64());
	/*
	if( mMemCache->rc() == MEMCACHED_SUCCESS )
		LOG_3("thumbnail stored in memcached");
	else
		LOG_3("memcached store failed!");
		*/
}
#endif

QImage ThumbnailLoader::loadFromDisk( const QString & path )
{
	QImage img( path );
#ifdef USE_IMAGE_MAGICK
	if( img.isNull() ) {
		//LOG_5( "QImage loading failed, trying ImageMagick" );
		img = loadImageMagick(path);
	}
#endif
	return img;
}

#ifdef USE_IMAGE_MAGICK
QImage ThumbnailLoader::loadImageMagick( const QString & path )
{
	Magick::Image img;
	QImage ret;
	try {
		img.read(path.toStdString());

		ret = QImage(img.columns(), img.rows(), QImage::Format_RGB32);
		const Magick::PixelPacket *pixels;
		Magick::ColorRGB rgb;
		for (int y = 0; y < ret.height(); y++) {
			pixels = img.getConstPixels(0, y, ret.width(), 1);
			for (int x = 0; x < ret.width(); x++) {
				rgb = (*(pixels + x));
				ret.setPixel(x, y, QColor((int) (255 * rgb.red())
					, (int) (255 * rgb.green())
					, (int) (255 * rgb.blue())).rgb());
			}
		}
	} catch(...) {
		return ret;
	}
  return ret;
}
#endif

#ifdef USE_MEMCACHED
QMemCached * ThumbnailLoader::mMemCache = 0;
#endif

