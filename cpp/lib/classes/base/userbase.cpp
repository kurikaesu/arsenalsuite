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

#include <qsqldatabase.h>
#include <qsqlquery.h>
#include <qregexp.h>
#include <qmenu.h>

#include "assettype.h"
#include "blurqt.h"
#include "database.h"
#include "connection.h"

#include "client.h"
#include "group.h"
#include "notificationmethod.h"
#include "notificationroute.h"
#include "permission.h"
#include "project.h"
#include "task.h"
#include "user.h"
#include "userelement.h"
#include "usergroup.h"
#include "userrole.h"
#include "userrole.h"
#include "host.h"

AssetTypeList User::roles() const
{
	return userRoles().assetTypes();
}

void User::addRole( const AssetType & role )
{
	if (!isValid())
		return;

	if( roles().contains( role ) )
		return;
	
	UserRole newRole;
	newRole.setAssetType( role );
	newRole.setUser( *this );
	newRole.commit();
}

void User::removeRole( const AssetType & role )
{
	if (!isValid())
		return;

	userRoles().filter( "fkeyassettype", role.key() ).remove();
}

ElementList User::toolBarElements() const
{
	return userElements().elements();
}

void User::addToolBarElement( const Element & link )
{
	if( userElements().elements().contains( link ) )
		return;

	UserElement ui;
	ui.setElement( link );
	ui.setUser( *this );
	ui.commit();
}

void User::removeToolBarElement( const Element & link )
{
	userElements().filter( "fkeyelement", link.key() ).remove();
}

static QString sUserName;

User User::currentUser()
{
	if( sUserName.isEmpty() )
		sUserName = getUserName();
	return User::recordByUserNameAndDisabled( sUserName, 0 );
}

void User::setCurrentUser( const QString & username )
{
	sUserName = username;
}

bool User::isUserLoggedIn( const QString & username )
{
	User u(User::recordByUserName(username.toLower()));

	if (u.isRecord()) {
		foreach( Host h, u.hosts() )
			if( h.userIsLoggedIn() )
				return true;
	}

	return false;
}

User User::activeByUserName( const QString & un )
{
	return User::recordByUserNameAndDisabled( un, 0 );
}

bool User::relatedElement( const Element & el, bool recurse )
{
	bool re=false;
	re = el.hasUser( *this );
	if( recurse ) {
		ElementList ch = el.children(ElementTypeList(), true);
		foreach( Element e, ch )
			re |= e.hasUser( *this );
	}
	return re;
}

uint User::nextUID()
{
	uint ret = 0;
	QSqlQuery q = Database::current()->connection()->exec( "SELECT MAX(uid) FROM Usr;" );
	if( q.next() )
		ret = qMax( 100, q.value(0).toInt() );
	return ret + 1;
}

uint User::nextGID()
{
	uint ret = 0;
	QSqlQuery q = Database::current()->connection()->exec( "SELECT MAX(gid) FROM Usr;" );
	if( q.next() )
		ret = qMax( 100, q.value(0).toInt() );
	return ret + 1;
}

const QString User::NO_PERMS( "Insufficient Permissions" );

void User::permAction( QAction * action, const QString & key, bool modify )
{
	if( !hasPerms( key, modify ) )
		action->setEnabled( false );
}

bool User::hasPerms( const QString & key, bool modify, const Project &  )
{
	static QMap<QString,int> permCache;
	static uint userCache = 0;
	
	if( userCache != currentUser().key() ) {
		userCache = currentUser().key();
		permCache.clear();
	}
	
	QString classKey = "Arsenal::Perm::" + key;
	QString cacheKey = classKey;
	if( modify )
		cacheKey += "_modify";
	
	int foundKey = 0;
	
	if( permCache.contains( cacheKey ) ) {
		foundKey = permCache[cacheKey];
		if( foundKey == 0 )
			LOG_3( "[Cached] Permission denied for key: " + classKey );
		else
			LOG_3( "[Cached] Permission granted for key: " + cacheKey + " from record: " + QString::number( foundKey ) );
		return foundKey > 0;
	}
	
	PermissionList pl = Permission::select();
	
	bool grantedUser = false;
	bool grantedGroup = false;
	
	GroupList gl = currentUser().userGroups().groups();
	foreach( Permission p, pl ) {
		if (p._class() == classKey)
		{
			if (p.modify() == true || modify == p.modify())
			{
				if (p.user().key() == currentUser().key())
				{
					if (p.enabled())
					{
						grantedUser = true;
						foundKey = p.key();
						break;
					}
					else
					{
						grantedUser = false;
						grantedGroup = false;
						break;
					}
				}
					
				if (gl.find(p.group()) != gl.end())
				{
					if (p.enabled())
					{
						grantedGroup = true;
						foundKey = p.key();
					}
				}
			}
		}
	}
	
	if (grantedUser || grantedGroup)
	{
		permCache[cacheKey] = foundKey;
		LOG_3("Permission granted for key: " + cacheKey + " from record: " + QString::number( foundKey ) );
		return true;
	}
	
	LOG_3( "Permission denied for key: " + cacheKey );
	permCache[cacheKey] = 0;
	return false;
}

NotificationRouteList getDefaultRoutes( const User & user )
{
	return NotificationRoute::select( 
		"fkeyuser=? AND componentMatch IS NULL AND eventMatch IS NULL AND fkeyelement IS NULL AND subjectMatch IS NULL AND messageMatch IS NULL",
		VarList() << user.key() );
}

void User::setDefaultNotificationMethods( NotificationMethodList list )
{
	NotificationRouteList defaultRoutes = getDefaultRoutes( *this );

	if( list.isEmpty() )
		list += NotificationMethod::recordByName( "Email" );
	
	QStringList actions;
	foreach( NotificationMethod method, list )
		actions += QString(actions.isEmpty() ? "default" : "add") + ":" + method.name() + "::" + name();
	NotificationRoute defaultRoute;
	if( defaultRoutes.size() )
		defaultRoute = defaultRoutes[0];
	else
		defaultRoute.setUser( *this );
	defaultRoute.setActions( actions.join(",") );
	defaultRoute.commit();
}

NotificationMethodList User::defaultNotificationMethods() const
{
	NotificationRouteList defaultRoutes = getDefaultRoutes( *this );
	if( defaultRoutes.isEmpty() ) return NotificationMethod::recordByName( "Email" );
	NotificationRoute defaultRoute = defaultRoutes[0];
	QStringList actions = defaultRoute.actions().split(",");
	NotificationMethodList ret;
	foreach( QString action, actions ) {
		NotificationMethod meth = NotificationMethod::recordByName( action.section(':',1,1) );
		if( meth.isRecord() )
			ret += meth;
	}
	return ret;
}

#endif

