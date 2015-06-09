
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
 * $Id: iconserver.h 5411 2007-12-18 01:03:08Z brobison $
 */

#ifndef ICON_SERVER_H
#define ICON_SERVER_H

#include <qimage.h>
#include <qmap.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qpixmap.h>

#include "stonegui.h"

typedef QMap<QString, QPixmap> IconCache;
typedef QMap<QString, QPixmap>::Iterator IconCacheIter;

/** \brief Looks up the icon with 'name' */
STONEGUI_EXPORT QPixmap icon( const QString & name );

class STONEGUI_EXPORT IconServer
{
public:
	IconServer();

	// Adds a path to be used when searching for an icon
	void addSearchPath( const QString & );

	// Searches for an icon with file name 'iconName' in
	// the search paths that were set using addSearchPath
	QPixmap icon( const QString & iconName );

	static IconServer * instance() {
		if( !mSelf ) mSelf = new IconServer();
		return mSelf;
	}
	
protected:

	static IconServer * mSelf;
	
	QSize mIconSize;
	QStringList mPaths;
	IconCache * mIcons;
};

#endif // ICON_SERVER_H

