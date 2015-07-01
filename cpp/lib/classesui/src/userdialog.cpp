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

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlineedit.h>
#include <qpushbutton.h>

#include "database.h"

#include "host.h"

#include "elementui.h"
#include "groupsdialog.h"
#include "hostselector.h"
#include "userdialog.h"

UserDialog::UserDialog( QWidget * parent, ChangeSet changeSet )
: QDialog( parent )
, mChangeSet( changeSet.isValid() ? changeSet : ChangeSet::create("Create User") )
, mIsEmp( true )
{
	setupUi( this );

	connect( mEditGroups, SIGNAL( clicked() ), SLOT( slotEditGroups() ) );

	if( !User::hasPerms( "UsrGrp", true ) ) {
		mEditGroups->setText( "View Groups" );
		mEditGroups->setEnabled( User::hasPerms( "UsrGrp" ) && User::hasPerms( "Group" ) );
	}
	
	mUser = User();
}

User UserDialog::user() const
{
	CS_ENABLE(mChangeSet);

	mUser.setName( mName->text() );
	mUser.setUsr( mName->text() );
	if( !mUser.gid() )
		mUser.setGid( User::nextGID() );
	if( !mUser.uid() )
		mUser.setUid( User::nextUID() );
	return mUser;
}

void UserDialog::setUser( const User & u )
{
	mName->setText( u.name() );
	mUser = u;
	
	if( mUser.isRecord() )
		mChangeSet.setTitle( "Edit User" );
}

void UserDialog::refreshAssignedHosts()
{
	CS_ENABLE(mChangeSet);
}

void UserDialog::slotEditHosts()
{
	CS_ENABLE(mChangeSet);
	HostSelector * hs = new HostSelector(this);
	hs->setHostList( mUser.hosts() );
	if( hs->exec() == QDialog::Accepted ) {
		HostList newAssignedHosts = hs->hostList();
		HostList clearedHosts = mUser.hosts() - newAssignedHosts;
		newAssignedHosts.setUsers( mUser );
		newAssignedHosts.commit();
		clearedHosts.setUsers( User() );
		clearedHosts.commit();
		refreshAssignedHosts();
	}
	delete hs;
}

void UserDialog::accept()
{
	CS_ENABLE(mChangeSet);
	if( !mUser.isRecord() ) {
		User u;
		if( mIsEmp ) {
			mUser = User();
			u = user();
		} else {
			mUser = User();
			u = user();
		}
		u.setKeyUsr( u.key(true) );
		u.commit();
	} else {
		user().commit();
	}

	mChangeSet.commit();
	QDialog::accept();
}

void UserDialog::slotEditGroups()
{
	GroupsDialog * gd = new GroupsDialog( this, mChangeSet.createChild("Edit User Group Associations") );
	gd->setUser( mUser );
	gd->exec();
	delete gd;
}




