/*
 *
 * Copyright 2007 Blur Studio Inc.
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
#include <qstring.h>

#include "path.h"

#include "element.h"
#include "pathtracker.h"
#include "pathtemplate.h"
#include "project.h"

#ifndef COMMIT_CODE

static ProjectStorage getAnyStorage( const PathTracker & pt, const ProjectStorage & storage )
{
	ProjectStorage ps = storage;
	if( !ps.isRecord() )
		ps = pt.projectStorage();
	if( !ps.isRecord() )
		ps = pt.element().defaultStorage();
	return ps;
}

QString PathTracker::generatePathFromTemplate( const ProjectStorage & ps )
{
	PathTemplate pt = pathTemplate();
	if( pt.isRecord() )
		return pt.getPath( *this, getAnyStorage(*this,ps) );
	return QString();
}

QString PathTracker::path( const ProjectStorage & storage )
{
	QString path = pathRaw();
	if( path.isEmpty() ) {
		QString path = generatePathFromTemplate( storage );
		setPath( path );
		return path;
	}
	ProjectStorage ps = getAnyStorage(*this,storage);
	return ps.location() + path;
}

// Returns true and removes the storage location from the beginning of the
// path if it matches the storage location, else returns false and leaves
// path unchanged.
static bool matchesStorage( const ProjectStorage & ps, QString & path )
{
	QString loc = ps.location();
	if( path.startsWith( ps.location(), Qt::CaseInsensitive ) ) {
		path = path.mid( ps.location().length() );
		return true;
	}
	return false;
}

bool PathTracker::setPath( const QString & p )
{
	QString path(p);

	if( path.startsWith( '/' ) ) {
		setPathRaw( path );
		return true;
	}
		
	ProjectStorage storage = projectStorage();
	if( storage.isRecord() ) {
		if( !matchesStorage( storage, path ) ) {
			LOG_3( "PathTracker::setPath: Path doesn't match the storage location for this pathtracker." );
			return false;
		}
		setPathRaw( path );
		return true;
	}
	// See if it matches a storage in the project
	bool foundMatch = false;
	ProjectStorageList psl = element().project().projectStorages();
	foreach( ProjectStorage ps, psl ) {
		if( matchesStorage( ps, path ) ) {
			foundMatch = true;
			break;
		}
	}
	if( !foundMatch ) {
		LOG_3( "PathTracker::setPath: Path doesn't match any storage in the project" );
		return false;
	}
	setPathRaw( path );
	return true;
}

bool PathTracker::createPath()
{
	return false;
}

PathTracker PathTracker::fromPath( const QString & path, bool matchClosest )
{
	Path p(path);
	QString drive = p.drive();
	do {
		QString abs = p.stripDrive();
		if( !abs.isEmpty() ) {
//			LOG_5( "Element::fromPath: Looking for path: " + abs );
			PathTrackerList ptl = PathTracker::select("lower(path)=?",VarList() << abs.toLower());
			foreach( PathTracker pt, ptl ) {
//				LOG_5( "Element::fromPath: Comparing " + (*it).path() + " to " + p.path() );
				// TODO: This should be case-insensitive on some storages
				if( pt.path().toLower() == p.path().toLower() )
					return pt;
			}
			if( matchClosest )
				p = p.parent();
		} else
			matchClosest = false;
	} while( matchClosest );
	return PathTracker();
}

PathTracker PathTracker::fromPath( const ProjectStorage & ps, const QString & path, bool caseSensitive, bool matchClosest )
{
	Path p(path);
	QString drive = p.drive();
	do {
		QString abs = p.stripDrive();
		if( !abs.isEmpty() ) {
//			LOG_5( "Element::fromPath: Looking for path: " + abs );
			QString sql = QString(caseSensitive ? "path" : "lower(path)") + "=? AND fkeyprojectstorage=?";
			VarList args = VarList() << (caseSensitive ? abs : abs.toLower()) << ps.key();
			PathTrackerList ptl = PathTracker::select(sql,args);
			foreach( PathTracker pt, ptl ) {
//				LOG_5( "Element::fromPath: Comparing " + (*it).path() + " to " + p.path() );
				if( caseSensitive )
					if( pt.path() == p.path() )
						return pt;
				else
					if( pt.path().toLower() == p.path().toLower() )
						return pt;
			}
			if( matchClosest )
				p = p.parent();
		} else
			matchClosest = false;
	} while( matchClosest );
	return PathTracker();
}


#endif // !COMMIT_CODE


