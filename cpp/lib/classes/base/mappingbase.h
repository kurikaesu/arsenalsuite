

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
class JobType;
#endif

#ifdef CLASS_FUNCTIONS

/// Maps this mapping. This function can be used without a database connection as long as
/// Mapping.host() is cached in the Host key index.
bool map( bool forceUnmount, QString * errorMsg );

/// Blur specific function that tells if a path meets the criteria to be synced to the mirrors
static bool isMirrorSyncPath( const QString & path);

/// Returns true if the path will be available for jobs with jobType.  Checks isMirrorSyncPath where applicable
static bool isMappedPath( const JobType & jobType, const QString & path );

#endif

