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

#ifdef HEADER_FILES

#include <qstringlist.h>
class ElementList;
#endif

#ifdef CLASS_FUNCTIONS

	bool pathMatchesTemplate() const;
	bool fileNameMatchesTemplate() const;
	bool matchesTemplate() const;

	bool doesTrackFile( const QString & filePath ) const;
	
	// Generates the filename from the path template
	QString generateFileName() const;

	QString fileName() const;
	bool setFileName( const QString & fn );

	// Generates the path from the path template
	QString generatePath() const;

	QString path() const;
	void setPath( const QString & path );

	bool createPath();

	QString setCachedPath( const QString & path ) const;
	static void invalidatePathCache();

	void checkForUpdates();
	
	QString filePath() const { return path() + fileName(); }
	void setFilePath( const QString & filePath );

	QDateTime updated( bool last = true ) const;
	QDateTime firstUpdated() const;
	QDateTime lastUpdated() const;
	
	// Returns all FileTrackers that have files in 'dir'
	static FileTrackerList fromDirPath( const QString & dir );
	
	// Returns the FileTracker that tracks 'file'
	static FileTracker fromPath( const QString & file );
	
	bool needsUpdate() const;
	
	FileTrackerList inputs() const;
	FileTrackerList outputs() const;
	
#endif

