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
#include <qimage.h>

#include "rangefiletracker.h"
//#include "rangefiletrackerframe.h"
#include "element.h"
#include "blurqt.h"
#include "path.h"
#include "project.h"
#include "projectresolution.h"

QString RangeFileTracker::fileName() const
{
	return makeFramePath( fileNameTemplate(), frameStart() );
}

bool RangeFileTracker::setFileName( const QString &  )
{
	return false;
}

void RangeFileTracker::checkForUpdates()
{
}

bool RangeFileTracker::doesTrackFile( const QString & fn )
{
	Path p( fn );
	if( p.dirPath() != path() ) {
		LOG_5( "RangeFileTracker::doesTrackFile: Paths don't match: " + p.path() + " " + path() );
		return false;
	}

	QRegExp fin( "(\\d+)\\.[^\\.]+$" );
	if( p.fileName().contains( fin ) ) {
		uint in = fin.cap(0).toInt();
		QString mpfn = makeFramePath( fileNameTemplate(), in );
		if( mpfn == p.fileName() )
			return true;
	}

	if( p.fileName() == fileNameTemplate() ) {
		return true;
	} //else LOG_5( "RFTI::vDoesTrackFile - Couldn't find frame number in " + p.fileName() );
	
	return false;
}

QDateTime RangeFileTracker::updated( bool last ) const
{
	QStringList fls = files();
	QString p = path();
	QDateTime ret;
	for( QStringList::Iterator it = fls.begin(); it != fls.end(); ++it )
	{
		QFileInfo fi( p + *it );
		if( fi.exists() ) {
			QDateTime dt = fi.lastModified();
			// If we want the last modified file,
			// then set ret if it is invalid or older
			// than this one
			if( last && (!ret.isValid() || (ret < dt)) )
				ret = dt;
			
			if( !last && (!ret.isValid() || (ret > dt)) )
				ret = dt;
		} else if( !last ) // Return an invalid datetime if there is a missing frame
			return QDateTime();
	}
	return ret;
}

QStringList RangeFileTracker::files() const
{
	QStringList ret;
	QString tem = fileNameTemplate();
	for( int fs = frameStart(), fe = frameEnd(); fs < fe; fs++ )
		ret += makeFramePath( tem, fs );
	return ret;
}

QString RangeFileTracker::sortString( int frame ) const
{
	return QString("%1.%2").arg( frame, 6 ).arg( int(frame*1000.0)%1000 );
}

QString RangeFileTracker::displayNumber( int frame ) const
{
	double sn = frame;
	QString snt;
	snt.sprintf("%04i", (int)sn);
	if( (double)((int)sn) != sn )
		snt += QString::number( sn - (int)sn ).mid(1);
	return snt;
}

QString RangeFileTracker::filePath( int frame ) const
{
	return path() + fileName( frame );
}

QString RangeFileTracker::fileName( int frame ) const
{
	return makeFramePath( fileNameTemplate(), frame );
}

void RangeFileTracker::setFilePath( const QString & filePath )
{
	Path p(filePath);
	setPath(p.dirPath());
	setFileNameTemplate(framePathBaseName(p.fileName()));
	setFileNameRaw(p.fileName());
}

void RangeFileTracker::fillFrames( bool overwrite )
{
	for( int i = frameStart(); i <= frameEnd(); ++i )
	{
		if( overwrite || !QFile::exists( filePath( i ) ) )
			fillFrame( i );
	}
}

void RangeFileTracker::fillFrame( int frame ) const
{
	ProjectResolution r( resolution() );
	Path p( r.fillFrameFilePath() ); 
	
	if( !p.dir().mkdir() ) {
		LOG_5( "Couldn't create templates directory for project" );
		return;
	}
	
	// If there is no template file, create an empty one
	if( !p.fileExists() ) {
		LOG_5( "Creating empty fill frame " + p.fileName() );
		QImage img( r.width(), r.height(), QImage::Format_ARGB32 );
		img.save( p.path(), r.outputFormat().toUpper().toLatin1() );
	}
	
	p.copy( filePath( frame ) );
}

QString RangeFileTracker::timeCode( int frame , int fps )
{
	if(fps == 0)
		return QString("bad FPS");

	int hr = frame / ( fps * 3600 );
	frame -= hr * fps * 3600;

	int mn = frame / ( fps * 60 );
	frame -= mn * fps * 60;

	int sc = frame / fps;
	frame -= sc * fps;

	return QString().sprintf("%02i:%02i:%02i:%02i", hr, mn, sc, frame);
}


void RangeFileTracker::deleteFrame( int frame ) const
{
	QFile::remove( filePath( frame ) );
	LOG_5( "del " + filePath( frame ).replace("/","\\") );
}
/*
RFTFrame RangeFileTracker::firstFrame() const
{
	return RFTFrame( *this, frameStart() );
}

RFTFrame RangeFileTracker::lastFrame() const
{
	return RFTFrame( *this, frameEnd() );
}

RFTFrame RangeFileTracker::frame( int i ) const
{
	return RFTFrame( *this, i );
}

RFTFrame::RFTFrame()
: frame( 0 )
{}

RFTFrame::RFTFrame( const RangeFileTracker & rft, int f )
: tracker( rft )
, frame( f )
{}
	
QString RFTFrame::sortString() const
{
	return tracker.sortString( frame );
}

QString RFTFrame::displayNumber() const
{
	return tracker.displayNumber( frame );
}

QString RFTFrame::filePath() const
{
	return tracker.filePath( frame );
}

QString RFTFrame::fileName() const
{
	return tracker.fileName( frame );
}

void RFTFrame::fillFrame() const
{
	tracker.fillFrame( frame );
}

void RFTFrame::deleteFrame() const
{
	tracker.deleteFrame( frame );
}

bool RFTFrame::exists() const
{
	return QFile::exists( filePath() );
}
	
QString RFTFrame::timeCode( int fps )
{
	return tracker.timeCode( frame, fps );
}

bool RFTFrame::isValid() const
{
	return frame >= tracker.frameStart() && frame <= tracker.frameEnd() && frame < INT_MAX;
}

RFTFrame & RFTFrame::operator++()
{
	frame++;
	return *this;
}
*/
#endif

