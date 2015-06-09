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

#include <stdlib.h>

#include <qdir.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qprocess.h>

#include "attachment.h"
#include "asset.h"
#include "blurqt.h"
#include "database.h"
#include "element.h"
#include "freezercore.h"
#include "path.h"
#include "project.h"
#include "rangefiletracker.h"
#include "renderelements.h"
#include "resolution.h"
#include "shot.h"
#include "shotgroup.h"
#include "task.h"
#include "tasktype.h"
#include "thrasher.h"
#include "updatemanager.h"
#include "database.h"
#include "path.h"
#include "versionfiletracker.h"

Thrasher::Thrasher()
{
	connect( Element::table(), SIGNAL( added( RecordList ) ), SLOT( elementsAdded( RecordList ) ) );
	connect( Element::table(), SIGNAL( removed( RecordList ) ), SLOT( elementsRemoved( RecordList ) ) );
	connect( Element::table(), SIGNAL( updated( Record, Record ) ),
		SLOT( elementUpdated( Record, Record ) ) );
		
	connect( Resolution::table(), SIGNAL( added( RecordList ) ),
		SLOT( resolutionsAdded( RecordList ) ) );
	connect( Resolution::table(), SIGNAL( removed( RecordList ) ),
		SLOT( resolutionsRemoved( RecordList ) ) );
	connect( Resolution::table(), SIGNAL( updated( Record, Record ) ),
		SLOT( resolutionUpdated( Record, Record ) ) );

	connect( FileTracker::table(), SIGNAL( added( RecordList ) ),
		SLOT( fileTrackersAdded( RecordList ) ) );
	connect( FileTracker::table(), SIGNAL( removed( RecordList ) ),
		SLOT( fileTrackersRemoved( RecordList ) ) );
	connect( FileTracker::table(), SIGNAL( updated( Record, Record ) ),
		SLOT( fileTrackerUpdated( Record, Record ) ) );
	
	connect( User::table(), SIGNAL( added( RecordList ) ), SLOT( usersAdded( RecordList ) ) );
	
	// Get connected to the database
	FreezerCore::instance();
	// Get connected to update manager
	UpdateManager::instance();
	
	IniConfig & cfg = config();
	cfg.pushSection( "PATHS" );
	mTemplateDir = cfg.readString( "TemplateDir", "templates/" );
	cfg.popSection();
	
	qWarning( "Template Directory: " + mTemplateDir );

	// preload all data into the needed caches so that we get
	// updates for everything we need
	Element::select();
	FileTracker::select();
	Resolution::select();
}

Thrasher::~Thrasher()
{
}

bool copy( const QString & source, const QString & dest, const QString & options )
{
	QString cp("cp -p ");
	if( !options.isEmpty() )
		cp += options + " ";
	cp += source + " " + dest;
	system(cp.utf8());
	return true;
}

QString nrProject( const Element & el )
{
	return el.project().shortName();
}

QString nrName( const Element & el )
{
	return el.parent().relPath().section("/",0,1);
}

QString nrProjectPath( const Element & el )
{
	QString ret;
	Element cur = el;
	do {
		cur = cur.parent();
		if( cur.parent() == cur.project() )
			continue;
		if( cur == cur.project() )
			ret = Project( cur ).shortName() + "/" + ret;
		else
			ret = cur.relPath() + ret;
	} while( cur.elementType() != Project::type() );
	ret = ret.replace( '/', "_" );
	ret = ret.left(ret.length()-1);
	return ret;
}

void renameFilesRecurse( const QString & dir, const QRegExp & search, const QString & replace )
{
	qWarning( dir );
	QDir cur( dir );
	QStringList dirs = cur.entryList( QDir::Dirs );
	for( QStringList::Iterator it = dirs.begin(); it != dirs.end(); ++it ){
		if( *it == "." || *it == ".." )
			continue;
		renameFilesRecurse( dir + "/" +*it, search, replace );
	}

	QStringList files = cur.entryList( QDir::Files );
	files = files.grep( search );
	for( QStringList::Iterator it = files.begin(); it != files.end(); ++it ){
		QString dest( *it );
		dest.replace( search, replace );
		qWarning( "File before: " + *it + "  file after: " + dest );
		if( dest != *it )
			cur.rename( *it, dest );
	}
}

void Thrasher::recurseCopy( const QString & source, QString dest, const Element & el )
{
	if( !QDir(dest).exists() ){
		qWarning("Creating directory " + dest);
		QDir().mkdir(dest);
	}else{
	//	qWarning("Creating directory " + dest + "/" + source.section("/",-1) );
	//	QDir().mkdir(dest + "/" + source.section("/",-1));
	//	dest += "/" + source.section("/",-1);
	}

	QStringList dirs = QDir(source).entryList( QDir::Dirs );
	for( QStringList::Iterator it = dirs.begin(); it != dirs.end(); ++it ){
		if( *it == "." || *it == ".." )
			continue;
		recurseCopy( source + "/" + *it, dest + "/" + *it, el );
	}

	QStringList files = QDir(source).entryList( QDir::Files );
	for( QStringList::Iterator it = files.begin(); it != files.end(); ++it ){
		QString destFile( expandName( *it, el ) );
		if( destFile.isEmpty() )
			continue;
		QRegExp re( "@(.+)@" );
		bool exists = false;
		if( el.isRecord() && re.search( destFile ) >= 0 ) {
			QString keyword = re.cap( 1 );
			destFile.replace( re, "" );
			QString ext = destFile.right( 3 );
			exists = QFile::exists( dest + destFile );
			bool ftexists = FileTracker::fromPath( dest + destFile ).isRecord();
			if( !ftexists && (ext == "max" || ext == "flw") ) {
				VersionFileTracker vft;
				vft.setElement( el );
				vft.setName( keyword );
				vft.setPath( el.path() );
				vft.setFileName( destFile );
				vft.checkForUpdates();
				vft.commit();
				QStringList files = vft.fileNames();
				AttachmentList atl = el.attachments();
				qWarning( "NEW VersionFileTracker, keyword: " + keyword + " filepath: " + dest + "/" + destFile );
				foreach( Attachment at, atl ) {
					if( files.contains( at.fileName() ) ) {
						qWarning( "Removing attachment record( " + QString::number( at.key() ) + " ) because " +
								at.filePath() + " is now tracked by the versionfiletracker " + keyword + "( " + 
								QString::number( vft.key() ) + " )" );
						at.remove();
					}
				}
			}
		} else
			exists = QFile::exists( dest + "/" + destFile );

		if( !exists ) {
			qWarning( "COPY Template File from " + source + "/" + *it + " to " + dest + "/" + destFile );
			copy( source + "/" + *it, dest + "/" + destFile, "" );
		}
	}
}

QString Thrasher::expandName( const QString & input, const Element & el )
{
	QString destFile( input );
	if( el.isRecord() ){
		destFile.replace("[project]", nrProject( el ));
		destFile.replace("[name]", nrName( el ));
		destFile.replace("[project_path]", nrProjectPath( el ));

		if (destFile == "[project_resolution]"){
			ResolutionList resList = Resolution::recordsByProject( Project( el ) );
			for(ResolutionIter it = resList.begin(); it != resList.end(); ++it){
				destFile = input;
				destFile.replace(
					"[project_resolution]", 
					QString::number( (*it).width() ) + "_" + QString::number( (*it).height() )
				);
				recurseCopy( input, destFile, el );
			}
			return QString();
		}
		if (destFile == "[dependencies]"){
			ElementList depList = el.dependencies();
			for(ElementIter it = depList.begin(); it != depList.end(); ++it){
				destFile = input;
				destFile.replace("[dependencies]", (*it).relPath() );
				recurseCopy( input, destFile, el );
			}
			return QString();
		}
	}
	return destFile;
}

void Thrasher::elementsAdded( RecordList rlist )
{
	ElementList list( rlist );

	st_foreach( ElementIter, it, list )
	{
		Element el( *it );
		
		/*******************
			Element Added
		*******************/
		if( !el.project().useFileCreation() )
			continue;

		qWarning( "Element Added( " + QString::number( el.key() ) + " ): " + el.name() );
		
		ElementType et = el.elementType();

		if( et == Project::type() ){
			qWarning( "Creating Project Paths" );
			el.createAllPaths( false );
			recurseCopy( mTemplateDir + "Project/", el.path() );
			recurseCopy( mTemplateDir, el.path() + "templates" );
		}
		else if( et == ShotGroup::type() || et == Shot::type()
			|| et == Asset::type() || et == Task::type()
			|| et == ElementType::assetGroupType() )
		{
			el.createAllPaths( true );

			// Only create Asset Group directories for the top-level asset groups(eg. Characters, Environments...)
			if( et == ElementType::assetGroupType() && el.parent().elementType() != Project::type() )
				continue;
			
			Database::instance()->beginTransaction();
			if( et == Task::type() )
				recurseCopy( el.project().path() + "templates/" + Task( el ).taskType().name().replace(" ","_"),
					el.path(), el );
			el.accountForAllFiles();
			Database::instance()->commitTransaction();
		}
	}
}

void Thrasher::elementsRemoved( RecordList )
{
}

void Thrasher::elementUpdated( Record rcur, Record rold )
{
	Element cur( rcur ), old( rold );
	ElementType et = cur.elementType();

	if( cur.elementType() == Project::type() )
	{
		Project po( old );
		Project pn( cur );
		
		if( !po.useFileCreation() && pn.useFileCreation() ){
			ElementList children = cur.children( ElementTypeList(), true );
			elementsAdded( cur );
			elementsAdded( children );
			return;
		}

		if( !pn.useFileCreation() )
			return;
			
		if( po.shortName() != pn.shortName() ){
			QRegExp re( "^" + po.shortName() );
			renameFilesRecurse( pn.path(), re, pn.shortName() );

			VersionFileTrackerList vftl = VersionFileTracker::select(
				" INNER JOIN element ON element.keyelement = filetracker.fkeyelement WHERE element.fkeyproject=?",
				VarList() += pn.key() );

			st_foreach( VersionFileTrackerIter, it, vftl ) {
				QString fn = (*it).fileName();
				fn.replace( re, pn.shortName() );
				if( fn != (*it).fileName() ){
					(*it).setFileName(fn);
					(*it).commit();
				}
			}
		}
	}

	if( !cur.project().isRecord() || !cur.project().useFileCreation() )
		return;

	if(	  et == ShotGroup::type()
		||et == Shot::type()
		||et == Asset::type()
		||et == Task::type()
		||et == ElementType::assetGroupType()
		||et == Project::type()
	)
	{
		if( cur.path() != old.path() ){
			QString oldpath = old.path();
			QString curpath = cur.path();
			if( QDir().exists( oldpath ) ) {
				qWarning( "Renaming path from " + oldpath + " to " + curpath );
				if( !QDir().rename( oldpath, curpath ) )
					qWarning( "Rename failed" );
			}
			
			oldpath = old.renderOutputPath();
			curpath = cur.renderOutputPath();
			if( QDir().exists( oldpath ) ) {
				qWarning( "Renaming path from " + oldpath + " to " + curpath );
				if( !QDir().rename( oldpath, curpath ) )
					qWarning( "Rename failed" );
			}
			
			oldpath = old.compOutputPath();
			curpath = cur.compOutputPath();
			if( QDir().exists( oldpath ) ) {
				qWarning( "Renaming path from " + oldpath + " to " + curpath );
				if( !QDir().rename( oldpath, curpath ) )
					qWarning( "Rename failed" );
			}
			oldpath = old.path();
			curpath = cur.path();
			ElementList list( cur.children( ElementType(), true ) );
			st_foreach( ElementIter, it, list )
			{
				FileTrackerList ftl( (*it).trackers() );
				st_foreach( FileTrackerIter, it, ftl ) {
					QString pr = Path( (*it).pathRaw() ).path();
					qWarning( "Checking fileversion with path: " + pr );
					qWarning( "does it contain: " + oldpath );
					if( pr.contains( oldpath ) ) {
						pr = pr.replace( oldpath, curpath );
						qWarning( "Renaming to " + pr );
						(*it).setPathRaw( Path::winPath( pr ) );
						(*it).commit();
					}
				}
			}
		}
	}
}

void Thrasher::fileTrackersAdded( RecordList list )
{
	VersionFileTrackerList vftl( list );
	st_foreach( VersionFileTrackerIter, it, vftl )
	{
		VersionFileTracker vft( *it );
		if( vft.isLinked() ){
			QString path = vft.pathRaw();
			QStringList files = vft.oldFileNames();
			files += vft.fileName();
			st_foreach( QStringList::Iterator, fit, files  ) {
				Path p( path + *fit );
				bool exists = p.exists();
				bool symlink = QFileInfo( p.path() ).isSymLink();
				if( exists && !symlink ){
					Path( p.dirPath() + "deleted/" ).mkdir();
					Path::move( p.path(), p.dirPath() + "deleted/" );
					exists = p.exists();
				}
				if( !exists || symlink ){
					if( symlink )
						QFile::remove( p.path() );
					QString link("ln -s " + vft.filePath() + " " + p.path() );
					qWarning( "Creating Link: " + link );
					system( link.utf8() );
				}
			}
		}
	}
}

void Thrasher::fileTrackersRemoved( RecordList )
{
}

void Thrasher::fileTrackerUpdated( Record cur, Record old )
{
	VersionFileTracker vft( cur );
	if( vft.isLinked() ){
		QString path = vft.pathRaw();
		QStringList files = vft.oldFileNames();
		files += vft.fileName();
		st_foreach( QStringList::Iterator, fit, files  ) {
			Path p( path + *fit );
			bool exists = p.exists();
			bool symlink = QFileInfo( p.path() ).isSymLink();
			if( exists && !symlink ){
				Path( p.dirPath() + "deleted/" ).mkdir();
				Path::move( p.path(), p.dirPath() + "deleted/" );
				exists = p.exists();
			}
			if( !exists || symlink ){
				if( symlink )
					QFile::remove( p.path() );
				QString link("ln -s " + vft.filePath() + " " + p.path() );
				qWarning( "Creating Link: " + link );
				system( link.utf8() );
			}
		}
	}
	if( !vft.isLinked() && VersionFileTracker(old).isLinked() ){
		VersionFileTracker ovft( old );
		QString path = ovft.pathRaw();
		QStringList files = ovft.oldFileNames();
		files += ovft.fileName();
		st_foreach( QStringList::Iterator, fit, files  ) {
			Path p( path + *fit );
			bool exists = p.exists();
			bool symlink = QFileInfo( p.path() ).isSymLink();
			if( exists && symlink )
				QFile::remove( p.path() );
		}
	}
}

void Thrasher::resolutionsAdded( RecordList rlist )
{
	ResolutionList list( rlist );
	st_foreach( ResolutionIter, it, list )
	{
		Resolution res( *it );
		
		TaskList tl = res.project().children( Task::type(), true );
		for( TaskIter it = tl.begin(); it != tl.end(); ++it ){
			qWarning( "Creating paths for task: " + (*it).displayPath() );
			(*it).createAllPaths(true);
		}
	}
}

void Thrasher::resolutionsRemoved( RecordList )
{
}

void Thrasher::resolutionUpdated( Record rec, Record old )
{
	Resolution res( rec ), oldres( old );

	// If the resolution changes, the directories must be moved
	if( res.isRecord() && res.dimString() != oldres.dimString() )
	{
		// Get all of the tasks
		TaskList tl = res.project().children( Task::type(), true );
		st_foreach( TaskIter, it, tl )
		{
			Task t( *it );
			VersionFileTrackerList vftl( t.findTrackers( QRegExp( "^max" ) ) );
			st_foreach( VersionFileTrackerIter, vft, vftl ) {
				// Get the render elements for the task
				QStringList relements = RenderElements( renderElementDir( (*vft).filePath() ) ).elementNameList();
				
				// For each render element, rename the resolution directory
				st_foreach( QStringList::Iterator, re, relements ){
					QDir resdir( t.renderOutputPath( *re ) + oldres.relPath() );
					qWarning( "Old - " + t.renderOutputPath( *re ) + oldres.relPath() );
					qWarning( "New - " + t.renderOutputPath( *re, res ) );
					if( resdir.exists() )
						QDir().rename( 
							t.renderOutputPath( *re ) + oldres.relPath(),
							t.renderOutputPath( *re, res )
						);
				}
			}
		}
		
		ShotList sl = res.project().children( Shot::type(), true );
		st_foreach( ShotIter, it, sl )
		{
			// Rename the comp output directory
			if( QDir( (*it).compOutputPath( oldres ) ).exists() ){
				qWarning( "Old - " + (*it).compOutputPath() + oldres.relPath() );
				qWarning( "New - " + (*it).compOutputPath( res ) );
				QDir().rename(
					(*it).compOutputPath() + oldres.relPath(),
					(*it).compOutputPath( res )
				);
			}
		}
	}
}

static QString random_password()
{
	const char * word_dict_path = "/usr/share/dict/words";
	QString ret = "error.error";
	
	QFile f( word_dict_path );
	
	if( !f.exists() || !f.open( IO_ReadOnly ) )
		return ret;
		
	srand( QDateTime::currentDateTime().toTime_t() );
	
	QStringList words = QStringList::split( "\n", f.readAll() ).grep( QRegExp( "^[a-z]....$" ) );
	if( words.isEmpty() )
		return ret;
		
	int index = rand() / (RAND_MAX / words.size());
	if( index >= (int)words.size() || index < 0 )
		index = words.size()-1;
	return words[index] + "." + QString::number( rand() / (RAND_MAX/100) );
}

void Thrasher::usersAdded( RecordList recs )
{
	UserList ul( recs );
	st_foreach( UserIter, it, ul )
	{
		User u( *it );
		if( u.password() == "rfm_generate" ) {
			qWarning( "Generating password for User: " + u.name() );
			u.setPassword( random_password() );
			u.commit();
			qWarning( "Set password to: " + u.password() );
			if( 0 ) {//u.project().isRecord() ) {
				system( QString("perl -MBlur::Model::Usr -e "
			"'my ($usr) = Blur::Model::Usr->retrieve(%1); $usr->new_user_email();'").arg(u.key()).local8Bit() );
			}
		}
	}
}

