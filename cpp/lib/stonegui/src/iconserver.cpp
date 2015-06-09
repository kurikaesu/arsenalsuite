 
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
 * $Id: iconserver.cpp 5411 2007-12-18 01:03:08Z brobison $
 */

#include "iconserver.h"
#include "blurqt.h"
#include "iniconfig.h"

QPixmap icon( const QString & name )
{
	return IconServer::instance()->icon( name );
}

IconServer * IconServer::mSelf = 0;

IconServer::IconServer()
: mIcons( 0 )
{
	addSearchPath( "images" );
	addSearchPath( "." );
#ifdef Q_OS_WIN
	addSearchPath( "C:\\Resin\\images" );
	addSearchPath( "X:\\studioMaster\\Resin\\images" );
#else
	addSearchPath( "/usr/share/resin/images" );
#endif // Q_OS_WIN
	IniConfig & cfg = config();
	cfg.pushSection( "Display Prefs" );
	mIconSize = cfg.readSize( "Icon Size", QSize( 16, 16 ) );
	cfg.popSection();
}

void IconServer::addSearchPath( const QString & path )
{
	mPaths += path;
}

QPixmap IconServer::icon( const QString & name )
{

	if( !mIcons )
		mIcons = new IconCache;

	/* Return cached value */
	IconCacheIter it = mIcons->find( name );
	if( it != mIcons->end() )
		return it.value();

	QString fileName = name;
	bool checkExtensions = (
		(fileName.right(4).toLower() != ".png") &&
		(fileName.right(4).toLower() != ".bmp") &&
		(fileName.right(4).toLower() != ".jpg") );

	for( QStringList::Iterator it = mPaths.begin(); it != mPaths.end(); ++it ){
		for( int ext=0; ext <= 3; ext++ ){
			QString testName = name;
			switch( ext ){
				case 0:  break;
				case 1:  testName += ".png"; break;
				case 2:  testName += ".bmp"; break;
				case 3:  testName += ".jpg"; break;
			}
			QImage img( *it + "/" + testName );
			if( !img.isNull() ){
				QPixmap pix = QPixmap::fromImage( img.scaled( mIconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );
				(*mIcons)[name] = pix;
				return pix;
			}
			if( !checkExtensions )
				break;
		}
	}
	return QPixmap();
}

