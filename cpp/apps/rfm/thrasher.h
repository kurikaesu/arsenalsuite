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

#ifndef THRASHER_H
#define THRASHER_H

#include <qobject.h>

#include "element.h"
#include "filetracker.h"
#include "user.h"

class Record;

class Thrasher : public QObject
{
Q_OBJECT
public:
	Thrasher();
	~Thrasher();
	void recurseCopy( const QString & source, QString dest, const Element & el = Element() );
	QString expandName( const QString & input, const Element & el );
	QString unixToWindows( const QString & file );

public slots:
	void elementsAdded( RecordList );
	void elementsRemoved( RecordList );
	void elementUpdated( Record , Record  );

	void resolutionsAdded( RecordList );
	void resolutionsRemoved( RecordList );
	void resolutionUpdated( Record , Record );

	void fileTrackersAdded( RecordList );
	void fileTrackersRemoved( RecordList );
	void fileTrackerUpdated( Record , Record );

	void usersAdded( RecordList );

protected:
	QString mTemplateDir;
};


#endif // TRASHER_H

