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
#include <qfile.h>
#include <qregexp.h>

#include "filetracker.h"
#include "filetrackerdep.h"
#include "blurqt.h"
#include "element.h"
#include "path.h"
#include "versionfiletracker.h"
#include "rangefiletracker.h"
#include "pathtemplate.h"

bool FileTracker::pathMatchesTemplate() const
{
	return filePath() == generatePath();
}

bool FileTracker::fileNameMatchesTemplate() const
{
	return fileName() == generateFileName();
}

bool FileTracker::matchesTemplate() const
{
	return pathMatchesTemplate() && fileNameMatchesTemplate();
}

QString FileTracker::generateFileName() const
{
	return pathTemplate().getFileName( *this );
}

QString FileTracker::generatePath() const
{
	return pathTemplate().getPath( *this );
}

bool FileTracker::doesTrackFile( const QString & path ) const
{
	RangeFileTracker rft( *this );
	VersionFileTracker vft( *this );
	if( rft.isRecord() )
		return rft.doesTrackFile( path );
	else if( vft.isRecord() )
		return vft.doesTrackFile( path );
	return Path( path ).path() == filePath();
}

QString FileTracker::fileName() const
{
	RangeFileTracker rft( *this );
	VersionFileTracker vft( *this );
	if( rft.isRecord() )
		return rft.fileName();
	else if( vft.isRecord() )
		return vft.fileName();
	QString fnr = fileNameRaw();
	if( fnr.isEmpty() ) return generateFileName();
	return fnr;
}

bool FileTracker::setFileName( const QString & fn )
{
	RangeFileTracker rft( *this );
	VersionFileTracker vft( *this );
	if( rft.isRecord() )
		rft.setFileName( fn );
	else if( vft.isRecord() )
		vft.setFileName( fn );
	else
		setFileNameRaw( fn );
	return true;
}

void FileTracker::setFilePath( const QString & filePath )
{
	Path p(filePath);
	setPath( p.dirPath() );
	setFileName( p.fileName() );
}

static int cacheNumber = 1;

QString FileTracker::setCachedPath( const QString & cp ) const
{
	FileTracker ft(*this);
	ft.setMPathCache( cp );
	ft.setMPathCacheNumber( cacheNumber );
	return cp;
}

void FileTracker::invalidatePathCache()
{
	cacheNumber++;
}

QString FileTracker::path() const
{
	if( mPathCacheNumber() == cacheNumber )
		return mPathCache();

	if( !pathRaw().isEmpty() )
		return pathRaw();

	//RangeFileTracker rft( *this );
	VersionFileTracker vft( *this );
	//if( rft.isRecord() )
//		return setCachedPath(rft.path());
	//else
	if( vft.isRecord() )
		return setCachedPath(vft.path());
	return setCachedPath(generatePath());
}

void FileTracker::setPath( const QString & path )
{
	RangeFileTracker rft( *this );
	VersionFileTracker vft( *this );
	if( rft.isRecord() )
		rft.setPath( path );
	else if( vft.isRecord() )
		vft.setPath( path );
	else
		setPathRaw( path );
}

bool FileTracker::createPath()
{
	if( !QDir( path() ).exists() ){
		if( !QDir().mkdir( path() ) ){
			LOG_5( "Create directory failed: " + path() );
			return false;
		}
		return true;
	}
	return true;
}

void FileTracker::checkForUpdates()
{
	RangeFileTracker rft( *this );
	VersionFileTracker vft( *this );
	if( rft.isRecord() )
		return rft.checkForUpdates();
	else if( vft.isRecord() )
		return vft.checkForUpdates();
}

QDateTime FileTracker::updated( bool last ) const
{
	RangeFileTracker rft( *this );
	VersionFileTracker vft( *this );
	if( rft.isRecord() )
		return rft.updated( last );
	else if( vft.isRecord() )
		return vft.updated( last );
	QFileInfo fi( filePath() );
	if( !fi.exists() )
		return QDateTime::currentDateTime();
	return fi.lastModified();
}

QDateTime FileTracker::lastUpdated() const
{
	return updated( true );
}

QDateTime FileTracker::firstUpdated() const
{
	return updated( false );
}

FileTracker FileTracker::fromPath( const QString & path )
{
	//LOG_5( "FileTracker::fromPath: " + path );

	Path p( path ), ftdir( path );
	QString fn = p.fileName();

	if( ftdir.dirName().toLower() == "wip" )
		ftdir = ftdir.dir().parent();
	
	//LOG_5( "Searching for filtrackers by path: " + ftdir.dbDirPath() );
	FileTrackerList fvl( FileTracker::recordsByPath( ftdir.dbDirPath() ) );
	foreach( FileTracker fv, fvl )
	{
		if( fv.doesTrackFile( p.path() ) )
			return fv;
	}
	return FileTracker();
}

FileTrackerList FileTracker::fromDirPath( const QString & dirPath )
{
	Path ftdir( dirPath );
	if( ftdir.dirName().toLower() == "wip" )
		ftdir = ftdir.dir().parent();
	return FileTracker::recordsByPath( ftdir.dbDirPath() );
}

bool FileTracker::needsUpdate() const
{
	FileTrackerList inpts = inputs();
	// Get the oldest file/frame that we track
	QDateTime first = firstUpdated();
	
	// See if it is older than one of our inputs
	foreach( FileTracker ft, inpts )
		if( first < ft.lastUpdated() )
			return true;
	return false;
}

FileTrackerList FileTracker::inputs() const
{
	return FileTrackerDep::recordsByOutput( *this ).inputs();
}

FileTrackerList FileTracker::outputs() const
{
	return FileTrackerDep::recordsByInput( *this ).outputs();
}


#endif // IMP_CLASS_FUNCTIONS

//#endif

