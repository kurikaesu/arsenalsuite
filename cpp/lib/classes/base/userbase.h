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
class QAction;
class NotificationMethodList;
#include "project.h"
#endif

#ifdef CLASS_FUNCTIONS

	static User currentUser();
	static void setCurrentUser( const QString & );

    static bool isUserLoggedIn( const QString & );
	
	static User activeByUserName( const QString & );

	static User setupProjectUser( Project p, Client c, bool ftp, bool web );
	
	typedef QMap<uint,int> PermMap;
	typedef QMapIterator<uint,int> PermIter;
	PermMap mProjectPerms;
	
	static bool hasPerms( const QString &, bool modify = false, const Project & project = Project() );
	static void permAction( QAction *, const QString &, bool modify = false );

	static const QString NO_PERMS;
		
	/* Roles */
	AssetTypeList roles() const;
	void addRole( const AssetType & );
	void removeRole( const AssetType & );

	/* ToolBar Elements */
	ElementList toolBarElements() const;
	void addToolBarElement( const Element & );
	void removeToolBarElement( const Element & );

	bool relatedElement( const Element &, bool recurse=false );

	static uint nextUID();
	static uint nextGID();
	
	void setDefaultNotificationMethods( NotificationMethodList );
	NotificationMethodList defaultNotificationMethods() const;
#endif

