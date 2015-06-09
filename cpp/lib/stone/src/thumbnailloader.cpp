
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of libstone.
 *
 * libstone is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libstone is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libstone; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 */

#include <qfile.h>
#include <qsize.h>
#include <qdir.h>

#include "thumbnailloader.h"
#include "blurqt.h"
#include "path.h"

QString ThumbnailLoader::thumbnailCachePath( const QString & path, const QSize & size )
{
#ifdef Q_OS_WIN
	static const QString temp( "C:/temp/" );
#else
	static const QString temp( "/tmp/" );
#endif // Q_OS_WIN

	// Make sure the thumbnail cache directory exists
	if( !Path( temp + "thumbnailcache" ).mkdir() ) {
		qWarning( "Couldn't create thumbnailcache directory: " + temp + "thumbnailcache" );
		return QString::null;
	}
	
	QString ret( path );
	ret = ret.replace("\\","_");
	ret = ret.replace("/","_");
	ret = ret.replace(":","");
	if( size.isValid() )
		ret += QString("_%1x%2.png").arg( size.width() ).arg( size.height() );
	return temp + "thumbnailcache/" + ret;
}

void ThumbnailLoader::clear( const QString & fileName )
{
	if( mCache )
		mCache->remove( fileName );
	
	QString cacheFile = thumbnailCachePath( fileName );
	Path p( cacheFile );
	QDir d( p.dirPath() );
	QStringList matches = d.entryList( p.fileName() + "*" );
	for( QStringList::Iterator it = matches.begin(); it != matches.end(); ++it )
		QFile::remove( p.dirPath() + *it );
}

QPixmap ThumbnailLoader::load( const QString & p, const QSize & returnSize )
{
	if( !mCache )
		mCache = new PixCache;

	Path path( p );

	ThumbnailSource & ts = (*mCache)[p];
	
	if( ts.contains( returnSize ) )
		return ts.get( returnSize );
	
	QString cachePath = thumbnailCachePath( p, returnSize );
	if( QFile::exists( cachePath ) ) {
		QPixmap pix( cachePath );
		ts.set( returnSize, pix );
		return pix;
	}
	
	QImage img( path.path() );
	
	if( !img.isNull() ){
		QImage scaled = img.scaled( returnSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
		scaled.save( cachePath, "PNG" );
		QPixmap pix( scaled );
		ts.set( returnSize, pix );
		return pix;
	}
	return QPixmap();
}

PixCache * ThumbnailLoader::mCache = 0;

