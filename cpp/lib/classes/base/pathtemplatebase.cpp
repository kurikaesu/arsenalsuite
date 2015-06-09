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

#include "pyembed.h"

#include <qregexp.h>

#include "blurqt.h"
#include "pathtemplate.h"
#include "filetracker.h"
#include "pathtracker.h"
#include "versionfiletracker.h"
#include "element.h"
#include "assettype.h"


FileTracker PathTemplate::createTracker( const Element & parent )
{
	FileTracker ret;
	ret.setPathTemplate( *this );
	ret.setName( name() );
	ret.setFileName( getFileName( parent ) );
	ret.setPath( getPath( parent ) );
	return ret;
}

QString runPathTemplateFunction( const PathTemplate & pt, const char * funcName, const Element & el, const FileTracker & ft, const ProjectStorage & projectStorage = ProjectStorage() )
{
	PyObject * func = getCompiledFunction( funcName, pt, pt.pythonCode(), pt.name() );
	if( !func ) return QString();

	VarList args;
	args << qVariantFromValue<Record>(el) << qVariantFromValue<Record>(ft) << qVariantFromValue<Record>(projectStorage);
	return runPythonFunction( func, variantListToTuple( args ) ).toString();
}

QString getFileNameFromPythonCode( const PathTemplate & pt, const Element & el, const FileTracker & ft )
{
	return runPathTemplateFunction( pt, "getFileName", el, ft, ft.projectStorage() );
}

QString getPathFromPythonCode( const PathTemplate & pt, const Element & el, const FileTracker & ft )
{
	return runPathTemplateFunction( pt, "getPath", el, ft, ft.projectStorage() );
}

QString getPathFromPythonCode( const PathTemplate & templ, const Element & el, const ProjectStorage & storage )
{
	return runPathTemplateFunction( templ, "getPath", el, FileTracker(), storage );
}

#undef expr
// This function performs a replacement on a
// single replacement block. The format for a replacement
// block is [KEY[:REG_EXP]+], where key is
//    Name - Name of element
//    ParentPath - Path of parent element
//    AssetType - Type of element
//    Version - Version of VersionFileTracker
//    Iteration - Iteration of VersionFileTracker
//
// REG_EXP is expected to be /SEARCH/REPLACE/.  Any
// '/'  inside the SEARCH or REPLACE portion should
// be escaped \/.  It perfroms the search and replace
// on the value substituted for the flag above.
QString templateReplace( const QString & temp, const Element & el, const FileTracker & ft = FileTracker(), const ProjectStorage & projectStorage = ProjectStorage() )
{
	QString ret;
	QString inner = temp.mid( 1, temp.size() - 2 );
	Element e( el );

	if( inner.isEmpty() ) {
		LOG_5( "PathTemplate::templateReplace: Replacement was empty: " + temp );
		return ret;
	}

	// Split into parts, the first part is the tag
	QStringList parts = inner.split( ":" );
	QString rep = parts[0];

	while( rep.startsWith( "Parent." ) ) {
		e = e.parent();
		rep = rep.mid( 7 );
	}

	QRegExp climb_exp("^climb_to_type\\(([^\\)]+)\\)\\.");
	if( climb_exp.indexIn( rep ) == 0 ) {
		QString type = climb_exp.cap(1);
		AssetType at = AssetType::recordByName( type );
		if( !at.isRecord() ) {
			LOG_5( "PathTemplate::templateReplace: climb_to_type contains invalid type: " + type );
			return ret;
		}
		while( e.isRecord() && e.assetType() != at )
			e = e.parent();
		if( !e.isRecord() ) {
			LOG_5( "PathTempalte::templateReplace: climb_to_type couldn't find anscestor of type: " + type );
			return ret;
		}
		rep = rep.mid( climb_exp.matchedLength() );
	}

	QRegExp climb_task_exp( "^climb_to_istask\\(([^\\)]+)\\)\\.");
	if( climb_task_exp.indexIn( rep ) == 0 ) {
		bool val = climb_task_exp.cap(1).toLower() == "true";
		while( e.isRecord() && e.assetType().isTask() != val )
			e = e.parent();
		if( !e.isRecord() ) {
			LOG_5( "PathTempalte::templateReplace: climb_to_istask couldn't find anscestor with istask of: " + QString(val ? "true" : "false") );
			return ret;
		}
		rep = rep.mid( climb_task_exp.matchedLength() );
	}

	// Lookup the tag value
	if( rep == "Name" )
		ret = e.name();
	else if( rep == "Root" )
		ret = projectStorage.location();
	else if( rep == "Path" && e.isRecord() ) {
		if( e == el && !ft.isRecord() ) {
			LOG_5( "PathTemplate::templateReplace: infinite recursion detected. Setting e to e.parent()" );
			e = e.parent();
		}
		ret = e.path(projectStorage);
	} else if( rep == "AssetType" )
		ret = e.assetType().name();
	
	if( ft.isRecord() ) {
		VersionFileTracker vft( ft );
		if( vft.isRecord() && rep == "Version" ) {
			ret = QString::number( vft.version() );
		} else if( vft.isRecord() && rep == "Iteration" ) {
			ret = QString::number( vft.iteration() );
		}
	}

	// Perform the regexp search and replaces
	for( int i = 1; i<parts.size(); i++ ) {
		// Split the /xxxx/xxxx/ into parts
		QString expr = parts[i];
		if( expr.isEmpty() || expr[0] != '/' || expr.right( 1 ) != "/" ) {
			LOG_5( "PathTemplate::templateReplace: RegExp section is not valid: " + expr );
			continue;
		}

		expr = expr.mid( 1, expr.size()-2 );

		QRegExp cnt( "[^\\\\]?/" );
		int center = cnt.indexIn( expr );

		if( center == -1 ) {
			LOG_5( "PathTemplate::templateReplace: RegExp section couldn't be split into search/replace pieces: " + expr );
			continue;
		}
		QString search( expr.left( center + 1 ) );
		QString replace( expr.mid( center + 2 ) );

		QRegExp re( search );
		if( !re.isValid() ) {
			LOG_5( "PathTemplate::templateReplace: Search expression not valid: " + search );
			continue;
		}

		//LOG_5( "PathTemplate::templateReplace: Replacing expression \"" + search + "\" with \"" + replace + "\"" );
		ret = ret.replace( re, replace );
	}
	
	//LOG_5( "templateReplace: Replacing " + temp + " with: " + ret );
	return ret;
}

QString parseTemplate( const QString & templ, const Element & el, const FileTracker & ft, const ProjectStorage & projectStorage )
{
	//LOG_5( "PathTemplate::parseTemplate: storage: " + projectStorage.location() );
	QString orig( templ );
	QString ret;
	
	bool inside = false;
	int ps = 0;
	for( int i=0; i<orig.size(); i++ ) {
		if( orig[i] == '[' && !inside ) {
			inside = true;
			ps = i;
			continue;
		}
		// If we get a non-escapted ], then close the replacement
		if( inside && orig[i] == ']' && i>ps && (orig[i-1] != '\\') ) {
			inside = false;
			QString rep = templateReplace( orig.mid( ps, i - ps + 1 ), el, ft, projectStorage );
			// If one of the replacements is EMPTY, then the whole template expands to empty
			if( rep.isEmpty() ) return rep;
			ret += rep;
			continue;
		}

		if( !inside )
			ret += orig[i];
	}

	//LOG_5( "PathTemplate::parseTemplate: Template: " + templ + " Result: " + ret );
	return ret;
}

QString PathTemplate::getPath( const Element & element, const ProjectStorage & storage )
{
	//LOG_5( "PathTemplate::getPath: storage: " + storage.location() );
	QString ret = getPathFromPythonCode( *this, element, storage );
	if( ret.isEmpty() )
		ret = parseTemplate( pathTemplate(), element, FileTracker(), storage );
	return ret;
}

QString PathTemplate::getFileName( const FileTracker & ft )
{
	QString ret = getFileNameFromPythonCode( *this, ft.element(), ft );
	if( ret.isEmpty() )
		ret = parseTemplate( fileNameTemplate(), ft.element(), ft, ft.projectStorage() );
	return ret;
}

QString PathTemplate::getPath( const FileTracker & ft )
{
	QString ret = getPathFromPythonCode( *this, ft.element(), ft );
	if( ret.isEmpty() )
		ret = parseTemplate( pathTemplate(), ft.element(), ft, ft.projectStorage() );
	return ret;
}

QString PathTemplate::getPath( const PathTracker & pt, const ProjectStorage & storage )
{
	ProjectStorage ps(storage);
	if( !ps.isRecord() ) ps = pt.projectStorage();
	return getPath( pt.element(), ps );
}


#endif // COMMIT_CODE

