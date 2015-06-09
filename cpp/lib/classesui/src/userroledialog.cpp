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

#include "userrole.h"
#include "userroledialog.h"

UserRoleDialog::UserRoleDialog( const User & user, QWidget * parent )
: QDialog( parent )
, mUser( user )
{
	mUserRoleUI = new Ui::UserRoleUI;
	QWidget * w = new QWidget( this );
	mUserRoleUI->setupUi(w);
	QHBoxLayout * hbl = new QHBoxLayout( this );
	hbl->addWidget( w );
	mAssetTypes = AssetType::select();

	for ( AssetTypeIter ttit = mAssetTypes.begin(); ttit != mAssetTypes.end(); ++ttit )
	{
		QString name = (*ttit).name();
		mAssetTypeMap[ name ] = *ttit;
	}

	//setRoles( UserRole::recordsByUser( user ) );

	balance();

	connect( mUserRoleUI->mAddButton,		SIGNAL( clicked() ),					SLOT( addRole() ) );
	connect( mUserRoleUI->mRemoveButton, 	SIGNAL( clicked() ),					SLOT( removeRole() ) );
	connect( mUserRoleUI->mOKButton,		SIGNAL( clicked() ),					SLOT( accept() ) );
	connect( mUserRoleUI->mCancelButton,	SIGNAL( clicked() ),					SLOT( reject() ) );
	connect( mUserRoleUI->mAvailableList,	SIGNAL( selected( const QString & ) ),	SLOT( addRole() ) );
	connect( mUserRoleUI->mSelectedList,	SIGNAL( selected( const QString & ) ),	SLOT( removeRole() ) );

	mHasEditPerms = (mUser == User::currentUser() || User::hasPerms( "UserRole", true ) );
	
	mUserRoleUI->mAddButton->setEnabled( mHasEditPerms );
	mUserRoleUI->mRemoveButton->setEnabled( mHasEditPerms );
	mUserRoleUI->mOKButton->setEnabled( mHasEditPerms );
	
	mUserRoleUI->mAvailableList->setSelectionMode( QListWidget::ExtendedSelection );
	mUserRoleUI->mSelectedList->setSelectionMode( QListWidget::ExtendedSelection );
	
	setRoles( user.roles() );
	resize(640,480);
	setWindowTitle( user.displayName() + "'s Roles" );
}

void UserRoleDialog::addRole()
{
	if( !mHasEditPerms ) return;
	
	QList<QListWidgetItem*> sel = mUserRoleUI->mAvailableList->selectedItems();
	for( int i = 0; i < sel.size(); i++ ) {
		QListWidgetItem * cur = sel.at(i);
		mSelected += cur->text();
		mAvailable.removeAll( cur->text() );
		delete cur;
	}
	mSelected.sort();
	mUserRoleUI->mSelectedList->clear();
	mUserRoleUI->mSelectedList->addItems( mSelected );
}

void UserRoleDialog::removeRole()
{
	if( !mHasEditPerms ) return;

	QList<QListWidgetItem*> sel = mUserRoleUI->mSelectedList->selectedItems();
	for( int i = 0; i < sel.size(); i++ ) {
		QListWidgetItem * cur = sel.at(i);
		mAvailable += cur->text();
		mSelected.removeAll( cur->text() );
		delete cur;
	}
	mAvailable.sort();
	mUserRoleUI->mAvailableList->clear();
	mUserRoleUI->mAvailableList->addItems( mAvailable );
}

void UserRoleDialog::setRoles( AssetTypeList roleList )
{
	for( AssetTypeIter roleit = roleList.begin(); roleit!= roleList.end(); ++roleit )
		mSelected += (*roleit).name();
	balance();
}

void UserRoleDialog::balance ()
{
	mUserRoleUI->mAvailableList->clear();
	mUserRoleUI->mSelectedList->clear();
	
	QStringList sorted;
	QStringList sortedSelected;
	for( AssetTypeIter roleit = mAssetTypes.begin(); roleit!= mAssetTypes.end(); ++roleit ) {
		if( !mSelected.contains( (*roleit).name() ) )
			sorted += (*roleit).name();
		else
			sortedSelected += (*roleit).name();
	}
	
	sorted.sort();
	sortedSelected.sort();
	
	mUserRoleUI->mAvailableList->addItems( sorted );
	mUserRoleUI->mSelectedList->addItems( sortedSelected );
	
	mAvailable = sorted;
	mSelected = sortedSelected;
}


QStringList UserRoleDialog::roleList() const
{
	return mSelected;
}

void UserRoleDialog::accept()
{
	if( mHasEditPerms ) {
		AssetTypeList atl;
		for( QStringList::Iterator sit = mSelected.begin(); sit != mSelected.end(); ++sit )
			atl += mAssetTypeMap[ *sit ];
		AssetTypeList roles = mUser.roles();
		for (AssetTypeIter ttit = roles.begin(); ttit != roles.end(); ++ttit)
			if ( !atl.contains(*ttit) )
				mUser.removeRole(*ttit);
	
		for (AssetTypeIter ttit = atl.begin(); ttit != atl.end(); ++ttit)
			if ( !roles.contains(*ttit) )
				mUser.addRole(*ttit);
	}
	QDialog::accept();
}

