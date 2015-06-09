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

#include <qcolor.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qstringlist.h>

#include "elementtype.h"
#include "history.h"
#include "record.h"
#include "projectstorage.h"
#include "filetracker.h"

class AssetTemplate;
class AttachmentList;
class ElementImp;
class ElementManager;
class ElementUser;
class Employee;
class EmployeeList;
class FileVersion;
class UserList;
class UserImp;
class PathTracker;
class QRegExp;
class QWidget;
#endif

#ifdef CLASS_FUNCTIONS

	bool isTask() const;

	// Assigned user stuff
	EmployeeList users( bool recursive = false ) const;

	QStringList userStringList() const;

	ElementUser addUser( const User & );

	void removeUser( const User & );

	void setUsers( EmployeeList );

	bool hasUser ( const User & ) const;

//	static Element recordByElementAndName( const Element &, const QString & );

//	float daysSpent() const;

	// Template Stuff
	static Element createFromTemplate( const AssetTemplate &, RecordList & created );
	void copyTemplateRecurse( const Element &, RecordList & created );
	void createFileDependancies( const Element & dependant, bool recursive );

	// File Tracker Stuff
	FileTrackerList trackers( bool recursive = true ) const;
	FileTrackerList findTrackers( const QRegExp &, bool recursive = true ) const;
	FileTracker tracker( const QString & key ) const;
	
	QString displayName( bool needContext = false ) const;
	QString displayPath() const;
	QString prettyPath() const;

	/// Returns the path to this asset made up from the names of each ancestor joined by .
	/// Characters ' ', '-', and '.' are replaced by '_' in the asset names
	/// Example "Transformers.Scene.Sc001.S0012_00.Scene_Assembly"
	QString uri() const;
	
	/// Returns the descendant relative to this asset using the uri notation above
	/// to describe the relative location.
	Element childFromUri( const QString & dotPath );
	
	/// Returns the asset described by the uri notation.  The first name must be a project.
	static Element fromUri( const QString & dotPath );
	
	/***************************************************************
		Path functions
	****************************************************************/
	/// Returns the ProjectStorage record for this project with storageName
	ProjectStorage storageByName( const QString & storageName ) const;
	ProjectStorage defaultStorage() const;
	ProjectStorage storageByDriveLetter( const QString & driveLetter ) const;

	/// Returns the pathtracker for the given storage, if one exists
	PathTracker pathTracker( const ProjectStorage & ps = ProjectStorage() );

	/// Returns the full path to the element, given the storage location
	/// Examples(Assuming asset is a shot):
	///   asset.path() - Uses the default project storage('animation'), returns 'G:/Project/01_Shots/Sc001/S0001.00/'
	///   asset.path('renderOutput') - returns 'S:/Project/Render/Sc001/S0001.00/'
	///   asset.path('compOutput') - returns 'Q:/Project/Comp/Sc001/S0001.00/'
	QString path( const ProjectStorage & ps = ProjectStorage() );
	QString path( const QString & storageName );

	/// Sets the path, returns true if successful.
	/// The drive letter of the path much match the given storage(default storage if not given)
	bool setPath( const QString & path, const ProjectStorage & storage = ProjectStorage() );
	bool setPath( const QString & path, const QString & storageName );

	/// Returns the asset that owns path
	/// If no asset directly owns path, and matchClosest is true, then
	/// then first asset to own a root directory of path will be returned
	static Element fromPath( const QString & path, bool matchClosest = false );

	QString setCachedPath( const QString & ) const;
	static void invalidatePathCache();

	// Returns a string that is a unique identifier for
	// any element in the database
	// format: "/Element=12456" where 12456 is the primary
	// key for the element
	QString elementPath() const;
	static Element fromElementPath( const QString & );
	
//	static bool import( const QString & fileName );

//	bool importRecurse( const QString & fileName, int level, RecordList * created = 0 ) const;

//	static Element fromDir( const QString & dir );

	// Trys to create the directory that this element will 
	// store it's files in
	bool createPath( bool createParents );

	// Creates the path and all the filetracker paths
	bool createAllPaths( bool createParents );

	// Uses the Config "driveToUnixPath" to convert
	static QString driveToUnix( const QString & drive );

	/*********************************************************
		Drag and drop
	**********************************************************/
	void dropped( QWidget * window, const Record &, Qt::DropAction da );
	//void copy( const Element & copyParent );
	void move( const Element & newParent );

	/**********************************************************
		Relations
	***********************************************************/
	/* Returns all of the elements that this element
	 * depends on */
	ElementList dependencies() const;
	ElementList dependants() const;

	void setDependencies( ElementList elist );
	void setDependants( ElementList elist );
	
	/* Returns the list of this element's
	 * children in the element tree */
	ElementList children( const ElementType & elementType, bool recursive=false) const;
	ElementList children( ElementTypeList elementTypes, bool recursive=false) const;
	
	/// Returns all children of assetType
	/// Search the entire hierarchy under this asset if recursive is true
	/// If this asset is a project and recursive is true then it will 
	/// search the entire project with a single SELECT(FAST!!!)
	ElementList children( bool recursive = false ) const;
	ElementList children( const AssetType assetType, bool recurssive=false ) const;
	
	// Returns the first anscestor up the hierarchy that is of type at
	Element ancestorByAssetType( const AssetType & at );
	
	/* Administration Users */
	UserList coordinators() const;
	UserList producers() const;
	UserList supervisors() const;
	

	/// Generated function assetPropertys() is also available returning a list of AssetProperty Objects.
	/// This function returns all the properties associated with this asset.
	QStringList propertyNames() const;

	/// Returns the property associated with this asset named propertyName.
	/// If the property doesn't exist the defaultValue is returned.
	/// If the property doesn't exist and storeDefault is true, then the property
	/// is stored using setProperty.
	QVariant getProperty( const QString & propertyName, const QVariant & defaultValue = QVariant(), bool storeDefault = false ) const;
	
	/// Stores value as a property associated with this asset named propertyName, can later be
	/// retrieved using getProperty.
	QVariant setProperty( const QString & propertyName, const QVariant & value );
	
/*
	// Scheduling functions
	float calcDaysBid() const;
	float calcDaysScheduled() const;
	float calcDaysEstimated() const;
	float calcDaysSpent( bool EightHour=true ) const;
	
	// Caching
	void calculateCache() const;
	void invalidateCache();
	void checkCacheValid() const;
*/
#endif

#ifdef TABLE_FUNCTIONS
/*
	void preInsert( RecordList );
	void preUpdate( const Record &, const Record & );
	void postUpdate( const Record &, const Record & );
	void postInsert( RecordList );
*/
#endif

