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

#ifdef HEADER_FILES
#include "projectstorage.h"
#endif

#ifdef CLASS_FUNCTIONS
	/// calling path automatically generates the path if it hasn't
	/// been generated in the past.  Calling the function forces it
	/// to be regenerated, useful for checking to see if the stored
	/// path still matches the template
	QString generatePathFromTemplate( const ProjectStorage & ps = ProjectStorage() );

	/// Returns the path, automatically generating it using the pathtemplate
	/// if the path has not yet been generated and the pathtemplate is valid
	QString path( const ProjectStorage & ps = ProjectStorage() );

	/// Sets the path if the input is valid.  To be valid the path must
	/// a) be an absolute path without a drive-letter '/a/path/to/'
	/// b) contain a drive letter that corrosponds to this pathtrackers
	/// 	projectstorage eg. path - 'G:/A_Project/Scene1/' projectstorage - 'G:'
	/// c) The projectstorage is not set, and the driveletter corrosponds
	/// 	to one of the projectstorage locations for the project this pathtracker belongs to.
	/// Returns true if the path was valid and saved.
	bool setPath( const QString & path );

	bool createPath();
	
	static PathTracker fromPath( const QString & path, bool matchClosest = false );

	static PathTracker fromPath( const ProjectStorage &, const QString & path, bool caseSensitive = false, bool matchClosest = false );
#endif

