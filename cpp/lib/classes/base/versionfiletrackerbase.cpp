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

#include "element.h"
#include "blurqt.h"
#include "path.h"
#include "project.h"
#include "rangefiletracker.h"
#include "versionfiletracker.h"
#include "filetrackerdep.h"

QString VersionFileTracker::fileName() const
{
	if( isLinked() ) return versionFileTracker().fileName();
	QString fnr = fileNameRaw();
	return fnr.isEmpty() ? generateFileName() : fnr;
}

bool VersionFileTracker::setFileName( const QString & fileName )
{
	if( isLinked() ) return versionFileTracker().setFileName( fileName );

	if( fileName.isEmpty() )
		return false;
	
	int ver, ite;
	if( parseVersion( fileName, &ver, &ite ) )
	{
		setVersion( ver );
		setIteration( ite );
	}

	QString old = fileNameRaw();

	if( fileNameTemplate().isEmpty() )
		setFileNameTemplate( fileName.section( QRegExp( "_[vV][0-9]+_[0-9]+.*" ), 0, 0 ) );
	setFileNameRaw( fileName );

	// Update oldFileNames	
	QStringList ofn( oldFileNames() );
	if( old != fileName && !ofn.contains( old ) && !old.isEmpty() )
		ofn += old;
	ofn.removeAll( fileName );
	setOldFileNames( ofn );
	
	// Copy to master if autoMaster
	if( autoFinal() )
		copyToFinal();
	
	// Create any new render element directories
	ElementList linked( elements() );
	foreach( Element e, linked )
		e.createAllPaths( true );
	
	return true;
}

QString VersionFileTracker::path() const
{
	if( isLinked() ) return versionFileTracker().path();
	QString pr = pathRaw();
	return pr.isEmpty() ? generatePath() : pr;
}

void VersionFileTracker::setPath( const QString & path )
{
	if( isLinked() )
		versionFileTracker().setPath( path );
	else
		setPathRaw( Path(path).dbDirPath() );
}

void VersionFileTracker::checkForUpdates()
{
	QString baseName = fileNameTemplate();
	QStringList ofn = oldFileNames();
	
	QStringList targets;
	targets += path();
	if( QDir( path() + "/wip/" ).exists() )
		targets += path() + "/wip/";
	else if( QDir( path() + "/WIP/" ).exists() )
		targets += path() + "/WIP/";
		
	QString max;
	int maxVersion=-1, maxIteration=-1;
	
	foreach( QString target, targets )
	{
		Path p( target );
		
		QDir d( p.path() );
		d.setSorting( QDir::Time );
		
		QFileInfoList list = d.entryInfoList(QStringList()<<"*", QDir::Files);
		for( int i=0; i<list.size(); i++ ) {
			QFileInfo fi = list.at(i);
			int ver, ite;
			if( parseVersion( fi.fileName(), &ver, &ite ) ){
				if( ver > maxVersion || (ver==maxVersion && ite>maxIteration) ){
					maxVersion = ver;
					maxIteration = ite;
					max = fi.fileName();
				} else if( !ofn.contains( fi.fileName() ) )
					ofn += fi.fileName();
			}
		}
	}
	
	if( !max.isEmpty() && max != fileName() )
		setFileName( max );
	setOldFileNames( ofn );
	if( isRecord() )
		commit();
}

bool VersionFileTracker::doesTrackFile( const QString & fn )
{
	Path p( fn );
//	LOG_5( vft.path() + " compared to " + p.dbDirPath() );
	
//	LOG_5( "TLD: " + p.dirName().toLower() );
	
	bool ret = false;
	if( path() == p.dirPath() )
		ret = true;
	else if( p.dirName().toLower() == "wip" ) { 
		//LOG_5( "Found wip, checking for parent path: " + p.dir().parent().dirPath() + " compared to " + path() );
		if( p.dir().parent().dirPath() == path() )
			ret = true;
	}
	
	if( ret ) ret = parseVersion( p.fileName() );
	
	//LOG_5( fn + " is " + QString(!ret ? "not " : "") + "tracked by filetracker: " + name() + " in path: " + path() );
	 
	return ret;
}

QStringList VersionFileTracker::fileNames() const
{
	return (oldFileNames() += fileName());
}

QStringList VersionFileTracker::oldFileNames() const
{
	VersionFileTracker fv( versionFileTracker() );
	if( fv.isRecord() )
		return fv.oldFileNames();
	return oldFileNamesSingle().split("||");
}

void VersionFileTracker::setOldFileNames( const QStringList & ofn )
{
	setOldFileNamesSingle( ofn.join("||") );
}

bool VersionFileTracker::isLinked() const
{
	return versionFileTracker().isRecord();
}

bool VersionFileTracker::copyToFinal( QString * errorMsg ) const
{
	QString fp = filePath();
	QString finPath = finalPath();
	QString finFilePath = finalFilePath();
	
	// Ensure that we have a file to copy
	if( !QFile::exists( fp ) ){
		if( errorMsg )
			*errorMsg = "Couldn't Find File to copy: " + fp;
		return false;
	}

	// Ensure that we have a place to copy it
	if( !QDir( finPath ).exists() ){
		if( !QDir().mkdir( finPath ) ){
			if( errorMsg )
				*errorMsg = "Couldn't Find or Create FINAL directory: " + finPath;
			return false;
		}
	}

	// Copy the file
	QFile::remove( finFilePath );
	Path::copy( fp, finFilePath );
	return true;
}

QString VersionFileTracker::finalPath() const
{
	return path() + "FINAL/";
}

QString VersionFileTracker::finalFilePath() const
{
	return finalPath() + "/" + getFinalFileName();
}

QString VersionFileTracker::getFinalFileName() const
{
	return fileNameTemplate() + "_FINAL.max";
}
	
bool VersionFileTracker::parseVersion( const QString & fileName, int * version, int * iteration )
{
	// All the variables to this function will be redirected to the linked
	// version, so there is no need to redirect this function
	QString fnt( fileNameTemplate() );
	if( !fnt.isEmpty() && fileName.left( fnt.length() ) != fnt ) {
		LOG_5( "VersionFileTracker::parseVersion: fileName doesn't match template: " + fileName + " " + fnt );
		return false;
	}
	QString exp(".*" + fnt + "_[vV](\\d+)_(\\d+)\\.*");
	QRegExp getvals( exp );
	if( fileName.contains( getvals ) ){
		if( version ) *version = getvals.cap( 1 ).toInt();
		if( iteration ) *iteration = getvals.cap( 2 ).toInt();
		return true;
	}
	LOG_5( "Couldn't parse version/iterations from filename: " + fileName );
	return false;
}

QString VersionFileTracker::assembleVersion( int version, int iteration )
{
	QRegExp re( "^.*_[vV](\\d+)_(\\d+)(.*)" );
	return fileNameTemplate() + "_v" + QString::number( version ) + "_" + QString::number( iteration )
			+ (fileName().contains( re ) ? re.cap( 2 ) : QString::null);
}

uint VersionFileTracker::version() const
{
	if( isLinked() ) return versionFileTracker().version();
	return versionRaw();
}

void VersionFileTracker::setVersion( uint version )
{
	if( isLinked() ) versionFileTracker().setVersion( version );
	else setVersionRaw( version );
}

uint VersionFileTracker::iteration() const
{
	if( isLinked() ) return versionFileTracker().iteration();
	return iterationRaw();
}

void VersionFileTracker::setIteration( uint iteration )
{
	if( isLinked() ) versionFileTracker().setIteration( iteration );
	else setIterationRaw( iteration );
}

bool VersionFileTracker::autoFinal() const
{
	if( isLinked() ) return versionFileTracker().autoFinal();
	return autoFinalRaw();
}

void VersionFileTracker::setAutoFinal( bool autoFinal )
{
	if( isLinked() ) versionFileTracker().setAutoFinal( autoFinal );
	else setAutoFinalRaw( autoFinal );
}

QString VersionFileTracker::fileNameTemplate() const
{
	if( isLinked() ) return versionFileTracker().fileNameTemplate();
	return fileNameTemplateRaw();
}

void VersionFileTracker::setFileNameTemplate( const QString & fnt )
{
	if( isLinked() ) versionFileTracker().setFileNameTemplate( fnt );
	else setFileNameTemplateRaw( fnt );
}

ElementList VersionFileTracker::elements() const
{
	if( !isRecord() ) return ElementList();
	FileTrackerList fvl( VersionFileTracker::recordsByVersionFileTracker( *this ) );
	ElementList els = fvl.elements();
	els += elements();
	ElementList ret;
	foreach( Element e, els )
		if( e.isRecord() )
			ret += e;
	return ret;
}
	
ElementList VersionFileTracker::elementsFromPath( const QString & path )
{
	return VersionFileTracker( FileTracker::fromPath( path ) ).elements();
}

#endif

