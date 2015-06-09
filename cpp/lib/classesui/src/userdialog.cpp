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

	connect( mAssignedHostsButton, SIGNAL( clicked() ), SLOT( slotEditHosts() ) );
	connect( mEmployeeGroup, SIGNAL( toggled( bool ) ), SLOT( slotEmpToggle( bool ) ) );
	connect( mEditGroups, SIGNAL( clicked() ), SLOT( slotEditGroups() ) );

	if( !User::hasPerms( "UsrGrp", true ) ) {
		mEditGroups->setText( "View Groups" );
		mEditGroups->setEnabled( User::hasPerms( "UsrGrp" ) && User::hasPerms( "Group" ) );
	}
	
	mUser = Employee();
}

Employee UserDialog::employee() const
{
	CS_ENABLE(mChangeSet);

	Employee e( user() );
	if( !e.isRecord() ){
		e.setElementType( User::type() );
		e.setDateOfTermination( QDate() );
	}
	e.setName( mUserName->text() );
	e.setPassword( mPassword->text() );
	e.setEmail( mEmail->text() );
	e.setFirstName( mFirst->text() );
//	e.setMiddleName( mMiddle->text() );
	e.setLastName( mLast->text() );
	e.setJid( mJid->text() );
	return e;
}

User UserDialog::user() const
{
	CS_ENABLE(mChangeSet);

	mUser.setName( mUserName->text() );
	mUser.setUsr( mUserName->text() );
	mUser.setPassword( mPassword->text() );
	mUser.setThreadNotifyByEmail( mNotifyEmailCheck->isChecked() ? 1 : 0 );
	mUser.setThreadNotifyByJabber( mNotifyJabberCheck->isChecked() ? 1 : 0 );
	mUser.setHomeDir( "/home/netftp/ftpRoot/" + mUserName->text() );
	mUser.setShell( mShellCheck->isChecked() ? "/bin/bash" : "/bin/false" );
	if( !mUser.gid() )
		mUser.setGid( User::nextGID() );
	if( !mUser.uid() )
		mUser.setUid( User::nextUID() );
	return mUser;
}

void UserDialog::setUser( const User & u )
{
	mUserName->setText( u.name() );
	mPassword->setText( u.password() );
	mNotifyEmailCheck->setChecked( u.threadNotifyByEmail() );
	mNotifyJabberCheck->setChecked( u.threadNotifyByJabber() );
	mShellCheck->setChecked( u.shell() == "/bin/bash" );
	slotEmpToggle( Employee( u ).isRecord() );
	
	mPixmapLabel->setPixmap( ElementUi(u).image( mPixmapLabel->size() ) );

	bool canEdit = User::hasPerms( "Usr", true );
	mAdminGroup->setEnabled( canEdit );
	mEmployeeGroup->setEnabled( canEdit );
	mUser = u;

	refreshAssignedHosts();
	
	if( mUser.isRecord() )
		mChangeSet.setTitle( "Edit User" );
}

void UserDialog::refreshAssignedHosts()
{
	CS_ENABLE(mChangeSet);
	mAssignedHostsEdit->setText( mUser.hosts().names().join(",") );
}

void UserDialog::setEmployee( const Employee & e )
{
	mFirst->setText( e.firstName() );
	mLast->setText( e.lastName() );
	mJid->setText( e.jid() );
	mEmail->setText( e.email() );
	setUser( e );
}

void UserDialog::slotEmpToggle( bool emp )
{
	mIsEmp = emp;
	mEmployeeGroup->setChecked( emp );
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
			mUser = Employee();
			u = employee();
		} else {
			mUser = User();
			u = user();
		}
		u.setKeyUsr( u.key(true) );
		u.commit();
	} else {
		if( Employee( mUser ).isRecord() != mIsEmp ) {
			// FIXME
		}
		if( mIsEmp )
			employee().commit();
		else
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




