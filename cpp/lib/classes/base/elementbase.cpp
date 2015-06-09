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
#include <qmessagebox.h>
#include <qregexp.h>

#include "database.h"

#include "asset.h"
#include "assetproperty.h"
#include "assettype.h"
#include "assettemplate.h"
#include "assetgroup.h"
#include "blurqt.h"
#include "config.h"
#include "element.h"
#include "elementdep.h"
#include "elementstatus.h"
#include "elementuser.h"
#include "employee.h"
#include "filetracker.h"
#include "filetrackerdep.h"
#include "history.h"
#include "path.h"
#include "pathtemplate.h"
#include "pathtracker.h"
#include "project.h"
#include "projectstatus.h"
#include "projectstorage.h"
#include "schedule.h"
#include "shot.h"
#include "shotgroup.h"
#include "thread.h"
#include "thumbnail.h"
#include "timesheet.h"
#include "user.h"
#include "userelement.h"

bool Element::isTask() const
{
	return assetType().isTask();
}

EmployeeList Element::users( bool recursive ) const
{
	EmployeeList ret = ElementUser::recordsByElement( *this ).users();
	if( recursive ) {
		ElementList kids = children( true );
		foreach( Element e, kids )
			ret += e.users();
	}
	return ret.unique();
}

QStringList Element::userStringList() const
{
	QStringList ret;
	EmployeeList ul = users();
	foreach( Employee emp, ul )
		ret += emp.fullName();
	return ret;
}

ElementUser Element::addUser( const User & user )
{
	if( !isRecord() || !user.isRecord() )
		return ElementUser();

	ElementUserList tul( ElementUser::recordsByElement( *this ) );
	foreach( ElementUser eu, tul )
		if( eu.user() == user )
			return eu;
	
	ElementUser tu;
	tu.setUser( user );
	tu.setElement( *this );
	tu.setActive( 1 );
	tu.commit();
	
	if( !elementStatus().isRecord() || elementStatus().name() == "New" ) {
		setElementStatus( ElementStatus::recordByName( "Assigned" ) );
		commit();
	}
	
	return tu;
}

void Element::removeUser( const User & user )
{
	if( !isRecord() || !user.isRecord() )
		return;
	ElementUser::recordsByElement( *this ).filter( "fkeyUser", user.key() ).remove();
}

void Element::setUsers( EmployeeList ul )
{
	if (!isRecord())
		return;

	EmployeeList toAdd;
	EmployeeList toRemove;

	EmployeeList currentUsers = users();

	foreach( Employee emp, ul )
		if( !currentUsers.contains( emp ) )
			toAdd += emp;

	foreach( Employee eu, currentUsers )
		if( !ul.contains( eu ) )
			toRemove += eu;

	ElementUserList tul = ElementUser::recordsByElement( *this );
	foreach( ElementUser eu, tul )
		if( toRemove.contains( eu.user() ) )
			eu.remove();

	foreach( Employee emp, toAdd )
	{
		ElementUser tu;
		tu.setElement( *this );
		tu.setUser( emp );
		tu.setActive( 1 );
		tu.commit();
	}
}

bool Element::hasUser ( const User & u ) const
{
	if ( !isRecord() || !u.isRecord() )
		return false;
	return elementUsers().filter( "user", u.key() ).size();
}

bool getSceneRange( const QString & str, float * start, float * end )
{
	QString lwr = str.toLower();
	*end = 0;
	*start = 0;
	if( lwr.indexOf( "sce" ) >= 0 ){
		int pos = lwr.indexOf("sce");
		QString scer = lwr.mid(pos);
		LOG_5( scer );
		scer = scer.replace( "sce", "" );
		LOG_5( scer );
		scer = scer.replace( "ne", "" );
		LOG_5( scer );
		bool suc;
		float f = scer.toFloat();
		*start = f;
		if( scer.indexOf("-") >= 0 ){
			scer = scer.section("-",1,1);
			f = scer.toFloat(&suc);
			*end = f;
		}
		return true;
	}
	return false;
}

Element Element::createFromTemplate( const AssetTemplate & at, RecordList & created )
{
	LOG_5( "Expanding Asset Template: " + at.name() );
	
	Element e = at.element().copy();
	// Store the asset template, in case we need to
	// modify all assets created with a template later
	e.setAssetTemplate( at );
	e.commit();
	at.element().copyTemplateRecurse( e, created );
	return e;
}

void Element::copyTemplateRecurse( const Element & parent, RecordList & created )
{
	ElementList kids = children();
	ElementList com;

	typedef QPair<Element,Element> ElementPair;
	
	QList<ElementPair> copies;
	foreach( Element e,  kids ) {
//		LOG_3( "Creating copy of: " + (*it).name() );
		if( e.assetTemplate().isRecord() ) {
			Element tmp = e.createFromTemplate( e.assetTemplate(), created );
			tmp.setParent( parent );
			com += tmp;
		} else {
			Element c( e.copy() );
			c.setParent( parent );
			copies += qMakePair<Element,Element>(e,c);
			com += c;
		}
	}
	
	com.commit();
	
	foreach( ElementPair it, copies )
		it.first.copyTemplateRecurse( it.second, created );
	
	created += com;
	
	// Copy the file trackers from the template
	FileTrackerList ftl = trackers(false);
	FileTrackerList fcom;
	foreach( FileTracker fto, ftl ) {
		FileTracker ft = fto.copy();
		ft.setElement( parent );
		// Point to the proper storage location for this project
		ft.setProjectStorage( storageByName( fto.storageName() ) );
		fcom += ft;
	}
	fcom.commit();

	// Copy the path trackers from the template
	PathTrackerList paths = pathTrackers(), ptcom;
	foreach( PathTracker pto, paths ) {
		PathTracker pt = pto.copy();
		pt.setElement( parent );
		// Point to the proper storage location for this project
		pt.setProjectStorage( storageByName( pto.storageName() ) );
		ptcom += pt;
	}
	ptcom.commit();

	created += fcom;
	created += ptcom;
}

void Element::createFileDependancies( const Element & /*dependant*/, bool /*recursive*/ )
{
	/*FileTrackerDepList toCommit;
	FileTrackerList ftl = trackers( recursive );
	FileTrackerList other = dependant.trackers();
	st_foreach( FileTrackerIter, it, ftl ) {
		PathTemplate pt = (*it).pathTemplate();
		PathTemplateList deps = PathTemplateDep::recordsByPathTemplate( pt ).pathTemplateDep();
		st_foreach( FileTrackerIter, dep_it, other ) {
			PathTemplate dep_pt = (*it).pathTemplate();
			if( deps.contains( dep_pt ) ) {
				FileTrackerDep ftd;
				ftd.setInput( *it );
				ftd.setOutput( *dep_it );
				toCommit += ftd;
			}
		}
	}
	toCommit.commit();*/
}

FileTrackerList Element::trackers( bool recursive ) const
{
	FileTrackerList ret;
	ret += FileTracker::recordsByElement( *this );
	if( recursive ) {
		ElementList el( children( ElementType(), true ) );
		foreach( Element e, el )
			ret += e.trackers( false );
	}
	return ret;
}

FileTracker Element::tracker( const QString & key) const
{
	FileTrackerList trs( trackers(false) );
	foreach( FileTracker ft, trs )
		if( ft.name() == key )
			return ft;
	return FileTracker();
}

FileTrackerList Element::findTrackers( const QRegExp & re, bool recurse ) const
{
	FileTrackerList trs( trackers(recurse) ), ret;
	foreach( FileTracker ft, trs )
		if( ft.name().contains( re ) )
			ret += ft;
	return ret;
}

/******************************************
	User Display Names
******************************************/
QString Element::displayPath() const
{
	QString sep( " . " );
	if( elementType() == Project::type() )
		return name();
	if( parent().isRecord() )
		return parent().displayPath() + sep + displayName();
	return displayName();
}

QString Element::displayName( bool needContext ) const
{
	if( Employee( *this ).isRecord() )
		return Employee( *this ).fullName();
	QString ret;
	if( needContext && isTask() && parent().isRecord() )
		ret = parent().displayName( true ) + ". ";
	if( elementType() == Shot::type() ) {
		if( needContext && !isTask() )
			ret += parent().displayName( true ) + " ";
	}
	return ret + name();
}

QString Element::prettyPath() const {
	Element par = parent();
	bool ap = (par.isRecord()) && (par != *this);
	return QString(ap ? par.prettyPath() : "") + "/" + name();
}

/******************************************
	Related Users
******************************************/

UserList usersByAssetType( Element e, const AssetType & at )
{
	UserList ret;
	while( e.isRecord() )
	{
		ElementList ch = e.children();
		foreach( Element e, ch )
			if( e.assetType() == at )
				ret += e.users();
		e = e.parent();
	}
	return ret;
}

UserList Element::coordinators() const
{
	return usersByAssetType( *this, AssetType::recordByName( "Coordinator" ) );
}

UserList Element::producers() const
{
	return usersByAssetType( *this, AssetType::recordByName( "Production" ) );
}

UserList Element::supervisors() const
{
	return usersByAssetType( *this, AssetType::recordByName( "Supervise" ) );
}

/*****************************************************
	Path functions
*****************************************************/
QString Element::uri() const
{
	QString ret;
	if( parent().isRecord() )
		ret = parent().uri() + ".";
	ret += name().replace( QRegExp("[\\.\\s-]"), "_" );
	return ret;
}

static QString uriToRegEx( const QString & uri )
{
	QString ret(uri);
	return ret.replace( "_", ".?" );
}

Element Element::childFromUri( const QString & uri )
{
	LOG_5( "Current: " + this->uri() + " relative: " + uri );
	QStringList parts = uri.split( "." );
	Element e = *this;
	for( int i=0; i<parts.size(); i++ ) {
		QString part = parts[i];
		if( part.isEmpty() )
			continue;
		ElementList found = Element::select( "fkeyelement=? AND name ~* ?", VarList() << e.key() << uriToRegEx( part ) );
		if( found.isEmpty() ) {
			LOG_5( "Unable to find " + part + " at pos " + QString::number(i) + " in uri: " + uri );
			return Element();
		}
		if( found.size() > 1 )
			LOG_5( "Found more than 1 match for uri: " + part + " at pos " + QString::number(i) + " in uri: " + uri );
		e = found[0];
	}
	return e;
}

Element Element::fromUri( const QString & uri )
{
	QString firstPart = uri;
	QString relativeUri;
	int i=0;
	do {
		i = firstPart.indexOf( "." );
		if( i > 0 ) {
			relativeUri = firstPart.mid(i+1);
			firstPart = firstPart.left(i);
			break;
		} else if( i == 0 )
			firstPart = firstPart.mid(1);
	} while( i >= 0 );
	LOG_5( "Got first part: " + firstPart + " from uri: " + uri );
	ProjectList found = Project::select( "name ~* ?", VarList() << uriToRegEx( firstPart ) );
	if( found.isEmpty() ) {
		LOG_5( "Unable to find Project from part: " + firstPart );
		return Element();
	}
	if( found.size() > 1 ) {
		LOG_5( "Found multiple projects matching uri part: " + firstPart );
		ProjectList production = found.filter( "fkeyprojectstatus", ProjectStatus::recordByName( "Production" ).key() );
		if( production.size() == 1 ) {
			LOG_5( "Returning the only production project found" );
			return production[0];
		}
		if( production.size() )
			found = production;
	}
	Project root = found[0];
	if( !relativeUri.isEmpty() )
		return root.childFromUri( relativeUri );
	return root;
}

/// Returns the ProjectStorage record for this project with storageName
ProjectStorage Element::storageByName( const QString & storageName ) const
{
	// Retrieve the projectstorage record for this project by name
	return ProjectStorage::recordByProjectAndName( project(), storageName );
}

ProjectStorage Element::defaultStorage() const
{
	// Only one project storage record should be marked default=true per project
	ProjectStorageList ptl = project().projectStorages().filter( "default", true );
	if( ptl.size() )
		return ptl[0];
	return ProjectStorage();
}

ProjectStorage Element::storageByDriveLetter( const QString & driveLetter ) const
{
	if( driveLetter[0].isLetter() ) {
		ProjectStorageList ptl = project().projectStorages().filter( "location", QString(driveLetter[0]) + ":" );
		if( !ptl.isEmpty() ) return ptl[0];
	}
	return ProjectStorage();
}

PathTracker Element::pathTracker( const ProjectStorage & ps )
{
	ProjectStorage storage = ps;
	if( !storage.isRecord() ) storage = defaultStorage();
	PathTrackerList trackers = pathTrackers();
	if( storage.isRecord() )
		trackers = trackers.filter( "fkeyprojectstorage", storage.key() );
	return trackers.isEmpty() ? PathTracker() : trackers[0];
}

static int cacheNumber = 1;

QString Element::setCachedPath( const QString & cp ) const
{
	Element e(*this);
	e.setValue( "mPathCache", cp );
	e.setValue( "mPathCacheNumber", cacheNumber );
	return cp;
}

void Element::invalidatePathCache()
{
	cacheNumber++;
}

QString Element::path( const ProjectStorage & projectStorage )
{
	if( !isRecord() ) return QString();

	ProjectStorage ps = projectStorage;

	// If an empty project storage record is passed, use the default
	if( !ps.isRecord() )
		ps = defaultStorage();

	PathTrackerList ptl = pathTrackers();

	// Find a path tracker with the given storage
	if( ps.isRecord() ) {
		PathTrackerList storageSpecific = ptl.filter( "fkeyprojectstorage", ps.key() );
		if( storageSpecific.size() ) {
		//	LOG_5( "Returning path from storage specific path tracker" );
			return storageSpecific[0].path();
		}
	}

	// Find non-storage specific
	PathTrackerList nonStorage = ptl.filter( "fkeyprojectstorage", QVariant() );
	if( nonStorage.size() ) {
		//LOG_5( "Returning path from storage independent path tracker" );
		return nonStorage[0].path(ps);
	}

	PathTemplate templ = pathTemplate();
	// If the template can generate a valid path for this storage
	// then automatically create a path tracker, so that this asset
	// can be looked up via the path
	if( templ.isRecord() ) {
		if( !templ.getPath( *this, ps ).isEmpty() ) {
			PathTracker pt;
			pt.setPathTemplate( templ );
			pt.setElement( *this );
			pt.setProjectStorage( ps );
			// Generates the path from the template and saves it
			QString path = pt.path();
			LOG_5( "Creating new pathtracker: " + path );
			pt.commit();
			return path;
		}
		// If we have a path template that doesnt generate a path
		// for this storage, then return an empty path to indicate such
		return QString();
	}

	// Else return whatever is left
	if( ptl.size() ) return ptl[0].path(ps);

	// Give reasonable defaults if no path trackers are defined
	if( Project(*this).isRecord() )
		return ps.location() + "/";

	QString ret = parent().path(ps);
	if( !ret.isEmpty() )
		ret += name().replace(" ","_") + "/";
	return ret;
}

QString Element::path( const QString & storageName )
{
	return path(storageByName(storageName));
}

bool Element::setPath( const QString & path, const ProjectStorage & storage )
{
	ProjectStorage ps = storage;
	if( !ps.isRecord() )
		ps = defaultStorage();
	PathTracker pt = pathTracker( ps );
	// Create a new one, if the path is valid
	if( !pt.isRecord() ) {
		pt.setProjectStorage( ps );
		pt.setElement( *this );
	}
	if( pt.setPath( path ) ) {
		pt.commit();
		return true;
	}
	return false;
}

bool Element::setPath( const QString & path, const QString & storageName )
{
	return setPath( path, storageByName(storageName) );
}

Element Element::fromPath( const QString & path, bool matchClosest )
{
	return PathTracker::fromPath( path, matchClosest ).element();
}

QString Element::elementPath() const
{
	return "/Element=" + QString::number( key() );
}

Element Element::fromElementPath( const QString & path )
{
	static QString last;
	static int cache;
	if( last==path )
		return Element(cache);

	Element element;
	QString lower = path.toLower();
	if( lower.section('=', 0, 0)=="/element" ){
		bool isNum;
		uint key = lower.section('=', 1, 1).toUInt(&isNum);
		if( !isNum )
			return Element();
		element = Element( key );
	}
	cache = element.key();
	return element;
}
/*
bool Element::import( const QString & _path )
{
	Element e = fromDir( Path(_path).dirPath() );
	return e.importRecurse( _path, 1 );
}

bool Element::importRecurse( const QString & _path, int curLevel, RecordList * created ) const
{
	QString next = Path(_path).chopLevel( curLevel + 1 ).path();
	ElementList cl = children();
	st_foreach( ElementIter, it, cl ) {
		if( (*it).path().lower() == next )
			return (*it).importRecurse( _path, curLevel+1 );
	}

	bool isFile = Path(next).fileExists();
	bool isDir = Path(next).dirExists();
	QString name = isFile ? Path(next).fileName() : Path(next).dirName();

	if( elementType() == Project::type() ) {
		if( isDir ) {
			// Shots dir
			if( name == "01_Shots" && children( ShotGroup::type() ).isEmpty() ) {
				ShotGroup sg;
				sg.setParent( *this );
				sg.setName( "Shots" );
				sg.setProject( *this );
				sg.setElementType( ShotGroup::type() );
				sg.commit();
				if( created )
					*created += sg;
			}

			{
				// Assets dirs
				AssetTypeList atl = AssetType::select();
				st_foreach( AssetTypeIter, it, atl )
					if( name.find( (*it).name(), 0, false ) >= 0 ) {
						Element g;
						g.setProject( *this );
						g.setParent( *this );
						g.setName( (*it).name() );
						g.setElementType( ElementType::assetGroupType() );
						g.commit();
						if( created )
							*created += g;
					}
			}
		}
	} else if( elementType() == ShotGroup::type() ) {
		if( isDir ) {
			QRegExp scre( "[Ss][cC]\\d\\d\\d" );
			if( scre.exactMatch( name ) ) {
				ShotGroup sg;
				sg.setParent( *this );
				sg.setProject( project() );
				sg.setElementType( ShotGroup::type() );
				sg.setName( name );
				sg.commit();
				if( created )
					*created += sg;
			}
			QRegExp sre( "([Ss]_?)([\\d\\.]+)" );
			if( sre.exactMatch( name ) ) {
				double shotNumber = sre.cap(2).toDouble();
				Shot s;
				s.setParent( *this );
				s.setProject( project() );
				s.setElementType( Shot::type() );
				s.setName( sre.cap(1) );
				s.setShotNumber( shotNumber );
				s.commit();
				if( created )
					*created += s;
			}
		}
	} else if( elementType() == Shot::type() || elementType() == Asset::type() ) {
		if( isDir ) {
			TaskTypeList ttl = TaskType::select();
			st_foreach( TaskTypeIter, it, ttl )
				if( (*it).name() == name ) {
					Task t;
					t.setParent( *this );
					t.setProject( project() );
					t.setElementType( Task::type() );
					t.setTaskType( *it );
					t.setName( name );
					t.commit();
					if( created )
						*created += t;
				}
		}
	} else if( elementType() == Task::type() ) {
		if( isFile ) {
			
		}
	} else if( elementType() == ElementType::assetGroupType() ) {
		if( isDir ) {
			Asset a;
			a.setParent( *this );
			a.setProject( project() );
			a.setName( name );
			a.setAssetType( AssetGroup( *this ).assetType() );
			a.commit();
			if( created )
				*created += a;
		}
	}
	// Couldn't find a child that owns this directory
	return false;
}

Element Element::fromDir( const QString & dir )
{
	// First find the project
	ProjectList pl = Project::select( "filepath=substring(? for (char_length(filepath)))", VarList() += dir );
	if( pl.size() != 1 ) return Element();
	
	Project p( pl[0] );
	Element ret = p;
	while( 1 ) {
		ElementList el = ret.children();
		st_foreach( ElementIter, it, el ) {
			if( dir.find( (*it).path() ) == 0 ) {
				ret = *it;
				continue;
			}
		}
		break;
	}
	return ret;	
}

*/

// Trys to create the directory that this element will 
// store it's files in
bool Element::createPath( bool createParents )
{
	if( parent().isRecord() )
	{
		QDir pd( parent().path() );
		if( !pd.exists() && (!createParents || !parent().createPath( createParents )) ) {
			LOG_5( "Couldn't create parent directory: " + pd.path() );
			return false;
		}
	}
	if( !QDir( path() ).exists() ){
		if( !QDir().mkdir( path() ) ){
			LOG_5( "Create directory failed: " + path() );
			return false;
		}
		return true;
	}
	return true;
}

// Creates the path and all the filetracker paths
bool Element::createAllPaths( bool createParents )
{
//	if( !project().useFileCreation() )
//		return;

	createPath( createParents );
	FileTrackerList files = trackers(false);
	foreach( FileTracker ft, files )
		ft.createPath();
	ElementList kids = children();
	foreach( Element e, kids )
		e.createAllPaths( false );

//	createRenderOutputPath();
//	createCompOutputPath();
	//createDailyOutputPath();

//	if( Asset( *this ).isRecord() )
//		QDir().mkdir( path() + "_MASTER" );

	return true;
}

QString Element::driveToUnix( const QString & drive )
{
	QStringList tbl = Config::recordByName( "driveToUnixPath" ).value().split(',');
	tbl = tbl.filter( drive );
	if( tbl.size() == 1 )
		return tbl[0].mid( 2 );
	return QString::null;
}

void Element::move( const Element & parent )
{
	setParent( parent );
	commit();
}

void Element::dropped( QWidget * /*parent*/, const Record & r, Qt::DropAction /*da*/ )
{
	if( r == *this ) return;
	bool startedTrans = false;
	do {
		Element el(r);
		if( el.isRecord() ) {
			Database::current()->beginTransaction( name() + " dropped on " + el.name() );
			startedTrans = true;
			// Dropping Users
			if( elementType()==User::type() && el.isTask() ) {
				Element(el).addUser( User( *this ) );
				break;
			}
			
			if( isTask() && el.elementType()==User::type() ) {
				addUser( User( el ) );
				break;
			}

			/* Dependancies, disable until we have popup menu with options
			if( elementType() == Shot::type() && el.elementType() == Asset::type() )
				setDependencies( dependencies() += el );
			if( elementType() == Asset::type() && el.elementType() == Shot::type() )
				setDependants( dependants() += el );
			*/
			
			bool isAbove = false;
			Element check = *this;
			while( check.isRecord() ){
				if( check == el ) {
					isAbove = true;
					break;
				}
				check = check.parent();
			}

			if( !isAbove )
				el.move( *this );
		}
		FileTracker ft(r);
		if( ft.isRecord() ) {
			ft.setElement( *this );
			ft.commit();
		}
	} while( 0 );
	if( startedTrans )
		Database::current()->commitTransaction();
}

void Element::setDependencies( ElementList el )
{
	Database * db = table()->database();
	QMap<uint, bool> emap;
	foreach( Element e, el )
		emap[e.key()] = true;
	ElementDepList deps = ElementDep::recordsByElement( *this );
	ElementDepList to_remove, to_commit;
	foreach( ElementDep dep, deps )
	{
		if( emap.contains( dep.elementDep().key() ) )
			emap.remove( dep.elementDep().key() );
  	else
			to_remove += dep;
	}
	
	for( QMap<uint,bool>::Iterator it = emap.begin(); it != emap.end(); ++it ){
		ElementDep ed;
		ed.setElement( *this );
		ed.setElementDep( Element( it.key() ) );
		to_commit += ed;
	}
	
	// Keep it all in one real transaction
	db->beginTransaction();
	
	// Set the title and remove
	db->beginTransaction( "Remove Dependenc" + QString(to_remove.size() > 1 ? "ies" : "y") );
	to_remove.remove();
	db->commitTransaction();
	
	// Set the title and commit
	db->beginTransaction( "Added Dependenc" + QString(to_commit.size() > 1 ? "ies" : "y") );
	to_commit.commit();
	db->commitTransaction();
	
	db->commitTransaction();
}

void Element::setDependants( ElementList el )
{
	Database * db = table()->database();
	QMap<uint, bool> emap;
	foreach( Element e, el )
		emap[e.key()] = true;
	ElementDepList deps = ElementDep::recordsByElementDep( *this );
	ElementDepList to_remove, to_commit;
	foreach( ElementDep ed, deps )
	{
		if( emap.contains( ed.element().key() ) )
			emap.remove( ed.element().key() );
  		else
			to_remove += ed;
	}
	for( QMap<uint,bool>::Iterator it = emap.begin(); it != emap.end(); ++it ){
		ElementDep ed;
		ed.setElement( Element( it.key() ) );
		ed.setElementDep( *this );
		to_commit += ed;
	}
	
	// Keep it all in one real transaction
	db->beginTransaction();
	
	// Set the title and remove
	db->beginTransaction( "Remove Dependenc" + QString(to_remove.size() > 1 ? "ies" : "y") );
	to_remove.remove();
	db->commitTransaction();
	
	// Set the title and commit
	db->beginTransaction( "Added Dependenc" + QString(emap.size() > 1 ? "ies" : "y") );
	to_commit.commit();
	db->commitTransaction();
	
	db->commitTransaction();
}

ElementList Element::dependencies() const
{
	ElementDepList edl = ElementDep::recordsByElement( *this );
	ElementList ret;
	foreach( ElementDep dep, edl )
		ret += dep.elementDep();
	return ret;
}

ElementList Element::dependants() const
{
	ElementDepList edl = ElementDep::recordsByElementDep( *this );
	ElementList ret;
	foreach( ElementDep dep, edl )
		ret += dep.element();
	return ret;
}

ElementList Element::children( const ElementType & elementType, bool recurse ) const
{
	ElementTypeList etl;
	if( elementType.isRecord() )
		etl += elementType;
	return children( etl, recurse );
}

ElementList Element::children( ElementTypeList elementTypes, bool recurse ) const
{
	ElementList ret;
	if( !isRecord() )
		return ret;

	QMap<uint, bool> elementTypeMap;

	bool checkType = !elementTypes.isEmpty();
	if( checkType )
		foreach( ElementType et, elementTypes )
			elementTypeMap[et.key()] = true;

	RecordList imps = Element::recordsByParent( key() );

	if( !recurse && !checkType )
		return ElementList( imps );

	while( imps.size() ){
		RecordIter it = imps.begin();
		Record r = *it;
		RecordImp * imp = r.imp();
		imps.remove( it );
		if( recurse )
			imps += Element::recordsByParent( imp->key() );
		if( !checkType || elementTypeMap.contains( imp->getValue("fkeyElementType").toUInt() ) )
			ret += r;
	}
	return ret;
}

ElementList Element::children( const AssetType assetType, bool recursive ) const
{
	if( Project(*this).isRecord() && recursive ) {
		return Element::select( "fkeyproject=? AND fkeyassettype=?", VarList() << key() << assetType.key() );
	}
	ElementList ret = Element::select( "fkeyassettype=? AND fkeyelement=?", VarList() << assetType.key() << key() );
	if( recursive ) {
		ElementList children(ret);
		foreach( Element e, children )
			ret += e.children( assetType, true );
	}
	return ret;
}

ElementList Element::children( bool recursive ) const
{
	ElementList ret = Element::recordsByParent( *this );
	if( recursive ) {
		ElementList kids(ret);
		foreach( Element e, kids )
			ret += e.children(true);
	}
	return ret;
}

Element Element::ancestorByAssetType( const AssetType & at )
{
	Element e = parent();
	while( e.isRecord() ) {
		if( e.assetType() == at )
			return e;
		e = e.parent();
	}
	return e;
}

/*
void Element::accountForAllFiles() const
{
	AttachmentList al = Attachment::recordsByElement( *this );
	AttachmentList to_remove, to_add;
	
	Database::instance()->beginTransaction();
	
	FileTrackerList ft = trackers();
	st_foreach( FileTrackerIter, it, ft )
		(*it).checkForUpdates();
	
	QMap<QString, Attachment> accounted;
	st_foreach( AttachmentIter, it, al ) {
		QString fn( (*it).fileName() );
		if( accounted.contains( fn ) && accounted[fn] != *it )
			to_remove += *it;
		else
			accounted[fn] = *it;
	}
	
	QStringList att_dirs;
	att_dirs += path();
	if( QDir( path() + "/wip/" ).exists() )
		att_dirs += path() + "/wip/";
	else if( QDir( path() + "/WIP/" ).exists() )
		att_dirs += path() + "/WIP/";
	//att_dirs += path() + "_Attachments/";

	st_foreach( QStringList::Iterator, ad, att_dirs )
	{
		QStringList files = QDir(*ad).entryList( QDir::Files );
		st_foreach( QStringList::Iterator, it, files ){
			bool acc = false;
			QString at_path( *ad + *it );
			
			if( *it == "thumbnail.png" )
				acc = true;
			else {
				st_foreach( FileTrackerIter, ft_it, ft )
					if( (*ft_it).doesTrackFile( at_path ) ) {
						if( accounted.contains( *it ) ) {
							to_remove += accounted[*it];
							accounted.remove( *it );
						}
						acc = true;
					}
			}
			
			if( !acc && accounted.contains( *it ) )
				acc = true;
		}
	}

	to_add.commit();
	to_remove.remove();
	Database::instance()->commitTransaction();
}*/


QStringList Element::propertyNames() const
{
	return Element(*this).assetProperties().names();
}

static QVariant assetPropertyToValue( const AssetProperty & ap )
{
	QVariant v( ap.value() );
	if( v.convert( (QVariant::Type)ap.type() ) )
		return v;
	return QVariant();
}

QVariant Element::getProperty( const QString & propertyName, const QVariant & defaultValue, bool storeDefault ) const
{
	AssetProperty ap = AssetProperty::recordByElementAndName( *this, propertyName );
	if( ap.isRecord() ) {
		QVariant v = assetPropertyToValue(ap);
		if( !v.isNull() )
			return v;
	}
	if( defaultValue.isValid() && storeDefault )
		Element(*this).setProperty( propertyName, defaultValue );
	return defaultValue;
}

QVariant Element::setProperty( const QString & propertyName, const QVariant & value )
{
	AssetProperty ap = AssetProperty::recordByElementAndName( *this, propertyName );
	QVariant oldValue = assetPropertyToValue(ap);
	ap.setElement( *this );
	ap.setValue( value.toString() );
	ap.setType( value.type() );
	ap.setName( propertyName );
	ap.commit();
	return oldValue;
}


/*
void Element::checkCacheValid() const
{
	bool val = false;
	uint cv = getValue( "mCacheValid" ).toUInt( &val );
	if( !val || !cv  )
		calculateCache();
}

float Element::calcDaysBid() const
{
	checkCacheValid();
	return getValue( "mDaysBidCache" ).toDouble();
}

float Element::calcDaysScheduled() const
{
	checkCacheValid();
	return getValue( "mDaysScheduledCache" ).toDouble();
}

float Element::calcDaysEstimated() const
{
	checkCacheValid();
	return getValue( "mDaysEstimatedCache" ).toDouble();
}

float Element::calcDaysSpent( bool eightHour ) const
{
	checkCacheValid();
	return eightHour ? getValue( "mDaysSpentCache8" ).toDouble() : getValue( "mDaysSpentCache" ).toDouble();
}

void Element::calculateCache() const
{
	double daysBidCache = 0.0;
	double daysScheduledCache = 0.0;
	double daysEstimatedCache = 0.0;
	double daysSpentCache8 = 0.0;
	double daysSpentCache = 0.0;

	bool isTask = assetType().isTask();
	if( isTask ){
		float ds = daysSpent();
		daysSpentCache8 = (ds > 8) ? 8 : ds;
		daysSpentCache = ds;
	}

	// Use timesheets for daysSpent
	TimeSheetList tsl = TimeSheet::recordsByElement( *this );
	st_foreach( TimeSheetIter, it, tsl ) {
		daysSpentCache += (*it).scheduledHour() / 8.0;
		daysSpentCache8 += QMIN( (*it).scheduledHour(), (float)8 ) / 8.0;
	}

	// Use Schedule for daysScheduled
	ScheduleList sl = Schedule::recordsByElement( *this );
	st_foreach( ScheduleIter, it, sl )
		daysScheduledCache += (*it).hours();
	daysScheduledCache /= 8.0;

	ElementList tasks = children();
	for( ElementIter it = tasks.begin(); it!=tasks.end(); ++it ){
		daysBidCache += (*it).calcDaysBid();
		daysEstimatedCache += (*it).calcDaysEstimated();
		daysScheduledCache += (*it).calcDaysScheduled();
		daysSpentCache8 += (*it).calcDaysSpent( true );
		daysSpentCache += (*it).calcDaysSpent( false );
	}

	if( daysBidCache < daysBid() )
		daysBidCache = daysBid();
	if( daysEstimatedCache < daysEstimated() )
		daysEstimatedCache = daysEstimated();

	Element e( *this );
	e.setValue( "mDaysBidCache", daysBidCache );
	e.setValue( "mDaysScheduledCache", daysScheduledCache );
	e.setValue( "mDaysEstimatedCache", daysEstimatedCache );
	e.setValue( "mDaysSpentCache8", daysSpentCache8 );
	e.setValue( "mDaysSpentCache", daysSpentCache );
	e.setValue( "mCacheValid", 1 );
}

void Element::invalidateCache()
{
	setValue( "mCacheValid", 0 );
	Element par = parent();
	if( par.isRecord() )
		par.invalidateCache();
}
*/

/*

static void updateParentDates( const Element & el )
{
	ElementList toCommit;
	Element parent = el.parent();
	QDate start = el.startDate();
	QDate complete = el.dateComplete();
	while( parent.isRecord() ) {
		if( parent.startDate().isNull() || parent.startDate() > start )
			parent.setStartDate( start );
		if( parent.dateComplete().isNull() || parent.dateComplete() < complete )
			parent.setDateComplete( complete );
		start = parent.startDate();
		complete = parent.dateComplete();
		toCommit += parent;
		parent = parent.parent();
	}
	toCommit.commit();
}

static void updatePaths( const Element & e )
{
	Element el(e);
	ProjectStorageList psl = el.project().projectStorages();
	foreach( ProjectStorage ps, psl )
		el.path( ps );
}

void ElementSchema::postUpdate( const Record & updated, const Record & r )
{
	updateParentDates( updated );
	//updatePaths( updated );
	TableSchema::postUpdate( updated, r );
}

void ElementSchema::postInsert( RecordList rl )
{
	ElementList list(rl);
	foreach( Element e, list ) {
		// Update the start/complete dates
		updateParentDates( e );
		// Generate the pathtrackers from the templates
		//updatePaths( e );
	}
	TableSchema::postInsert( rl );
}

void setupDates( Element e )
{
	QDate estStart = e.startDate();
	if( !estStart.isValid() ) {
		estStart = QDate::currentDate();
		if( e.parent().isRecord() && e.parent().startDate() > estStart )
			estStart = e.parent().startDate();
		e.setStartDate( estStart );
	}
	if( !e.dateComplete().isValid() || e.dateComplete() < estStart )
		e.setDateComplete( estStart.addDays( qMax<int>(0,e.daysEstimated()-1) ) );;
}

void ElementSchema::preInsert( RecordList rl )
{
	foreach( Record r, rl )
		setupDates( r );
	TableSchema::preInsert(rl);
}

void ElementSchema::preUpdate( const Record & r, const Record & b )
{
	setupDates( r );
	TableSchema::preUpdate(r,b);
}
*/
#endif


