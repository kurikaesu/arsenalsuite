/*
 *
 * Copyright 2006 Blur Studio Inc.
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
#include "project.h"
#include "path.h"

ShotGroup Project::findSequence( const QString & name )
{
	// First just try to match the name
	ShotGroupList sgl = ShotGroup::select( "fkeyproject=? AND name ILIKE ?", VarList() << key() << name );
	if( sgl.size() >= 1 )
		return sgl[0];
	return ShotGroup();
}

Shot Project::findShot( const QString & sequenceName, const QString & shotName )
{
	ShotGroup sg = findSequence( sequenceName );
	if( !sg.isRecord() ) return Shot();
	QRegExp shotNumberReg( "\\D+(\\d[\\d\\.]+)" );
	ShotList sl;
	if( shotNumberReg.exactMatch(shotName) ) {
		double shotNumber = shotNumberReg.cap(1).toDouble();
		sl = Shot::select( "fkeyelement=? AND shot=?", VarList() << sg.key() << shotNumber );
	}
	if( sl.size() == 0 )
		sl = Shot::select( "fkeyelement=? AND name=?", VarList() << sg.key() << shotName );
	if( sl.size() >= 1 )
		return sl[0];
	return Shot();
}

Shot Project::findShotFromPath( const QString & shotPath )
{
	Path path(shotPath);
	if( path.level() < 5 ) return Shot();
	return findShot( path[3], path[4] );
}

Project Project::findProjectFromPath( const QString & shotPath )
{
	Path path(shotPath);
	if( path.level() < 2 ) return Project();
	return Project::recordByName( path[1] );
}

ShotList Project::shots()
{
	return Shot::recordsByProject( *this );
}

ShotGroupList Project::sequences()
{
	return ShotGroup::recordsByProject( *this );
}

#endif

