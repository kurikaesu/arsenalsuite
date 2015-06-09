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

#include <qcombobox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlistwidget.h>
#include <qpushbutton.h>
#include <qstringlist.h>

#include "assettype.h"
#include "elementui.h"
#include "employee.h"
#include "usernotifydialog.h"
#include "tasktypecombo.h"
#include "thumbnail.h"

UserNotifyDialog::UserNotifyDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );

	setWindowTitle( "Choose Users" );

	mAvailableList->setSelectionMode( QListWidget::SingleSelection );
	mSelectedList->setSelectionMode( QListWidget::SingleSelection );

	EmployeeList users = Employee::select();
	for( EmployeeIter userit = users.begin(); userit!= users.end(); ++userit ) {
		if( (*userit).disabled() ) continue;
		mAvailable[(*userit).name()] = *userit;
	}

	connect( mAddUser, 		SIGNAL( clicked() ), 		SLOT( addUser() ) );
	connect( mRemoveUser, 	SIGNAL( clicked() ), 		SLOT( removeUser() ) );
	connect( mOKButton, 	SIGNAL( clicked() ), 		SLOT( accept() ) );
	connect( mCancelButton,	SIGNAL( clicked() ), 		SLOT( reject() ) );
	connect( mAvailableList,SIGNAL( currentTextChanged( const QString & ) ), SLOT( setUserInfo( const QString & ) ) );
	connect( mAvailableList,SIGNAL( itemDoubleClicked( QListWidgetItem * ) ), SLOT( addUser() ) );
	connect( mSelectedList,	SIGNAL( currentTextChanged( const QString & ) ), SLOT( setUserInfo( const QString & ) ) );
	connect( mSelectedList, SIGNAL( itemDoubleClicked( QListWidgetItem * ) ), SLOT( removeUser() ) );
}

void UserNotifyDialog::addUser()
{
	QList<QListWidgetItem*> sel = mAvailableList->selectedItems();
	for( int i = 0; i < sel.size(); i++ ) {
		QListWidgetItem * it = sel.at(i);
		mSelected[it->text()] = mAvailable[it->text()];
		mAvailable.remove( it->text() );
		delete it;
	}
	mSelectedList->clear();
	for( QMap<QString,User>::Iterator it = mSelected.begin(); it != mSelected.end(); ++it )
	mSelectedList->addItem( it.key() );
}

void UserNotifyDialog::removeUser()
{
	QList<QListWidgetItem*> sel = mSelectedList->selectedItems();
	for( int i = 0; i < sel.size(); i++ ) {
		QListWidgetItem * it = sel.at(i);
		mAvailable[it->text()] = mSelected[it->text()];
		mSelected.remove( it->text() );
		delete it;
	}
	mAvailableList->clear();
	for( QMap<QString,User>::Iterator it = mAvailable.begin(); it != mAvailable.end(); ++it )
		mAvailableList->addItem( it.key() );
}

void UserNotifyDialog::setMainUserList( const EmployeeList & userList )
{
    mMainUserList = userList;
}

void UserNotifyDialog::setUsers( UserList userList )
{
	mSelectedList->clear();
	for ( UserIter userit = userList.begin(); userit != userList.end(); ++userit){
		mSelectedList->addItem( (*userit).name() );
		mSelected[(*userit).name()] = *userit;
		mAvailable.remove( (*userit).name() );
	}

    refreshList();
}

UserList UserNotifyDialog::userList() const
{
	UserList ret;
	for( QMap<QString,User>::ConstIterator it = mSelected.begin(); it != mSelected.end(); ++it )
		ret += *it;
	return ret;
}

void UserNotifyDialog::setUserInfo( const QString & name )
{
	User u;
	if( mAvailable.contains( name ) )
		u = mAvailable[name];
	else if( mSelected.contains( name ) )
		u = mSelected[name];
	else
		return;
	mSelectedUser = u;
	AssetTypeList ttl = u.roles();
}

void UserNotifyDialog::refreshList()
{
    mAvailableList->clear();

    mAvailable.clear();

    for (EmployeeIter userit = mMainUserList.begin(); userit != mMainUserList.end(); ++userit)
        if (!mSelected.contains( (*userit).name() ) && (*userit).disabled() == 0)
            mAvailable[(*userit).name()] = *userit;

    for (QMap<QString,User>::Iterator it = mAvailable.begin(); it != mAvailable.end(); ++it)
        mAvailableList->addItem( it.key() );
}

