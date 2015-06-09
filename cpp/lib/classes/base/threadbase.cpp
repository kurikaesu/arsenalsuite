/*
 *
 * Copyright 2003, 2004 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Resin is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Resin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef COMMIT_CODE

#include <qdir.h>

#include "blurqt.h"
#include "path.h"

#include "thread.h"
#include "element.h"
#include "project.h"
#include "config.h"

QString Thread::attachmentsPath() const
{
	if( !isRecord() )
		return QString::null;

	QString ap;
#ifdef Q_OS_WIN
    ap = Config::getString("attachmentPathWin");
	if( element().isRecord() )
		ap = element().project().wipDrive() + "/Attachments/";
#else
    ap = Config::getString("attachmentPathUnix", "/mnt/animation/Attachments");
#endif
	ap += QString::number( key() ) + "/";
	if( !Stone::Path( ap ).mkdir( 1 ) ) {
		LOG_5( "Couldn't create path: " + ap );
		return QString::null;
	}
	return ap;
}

QStringList Thread::attachmentFiles() const
{
	QString ap( attachmentsPath() );
	if( ap.isEmpty() ) return QStringList();
	return QDir( ap ).entryList( QDir::Files );
}


#endif

