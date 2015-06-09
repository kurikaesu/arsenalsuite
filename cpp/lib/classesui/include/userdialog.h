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


#ifndef USER_DIALOG_H
#define USER_DIALOG_H

#include "employee.h"
#include "host.h"
#include "group.h"

#include "classesui.h"

#include "ui_userdialogui.h"

/**
 * Dialog for creating new users
 * can optionally create them as
 * employees
 **/
class CLASSESUI_EXPORT UserDialog : public QDialog, public Ui::UserDialogUI
{
Q_OBJECT
public:
	UserDialog( QWidget * parent=0, ChangeSet changeSet = ChangeSet() );

	ChangeSet changeSet() const { return mChangeSet; }
	
	/**
	 * Returns a User object set to the
	 * current information in the dialog
	 **/
	User user() const;
	Employee employee() const;
	
	/**
	 * Sets the information in the dialog
	 * to the information stored in user
	 **/
	void setUser( const User & );
	void setEmployee( const Employee & );
	
	virtual void accept();
public slots:

	void slotEditHosts();
	
	void slotEmpToggle( bool );

	void slotEditGroups();
	
protected:
	void refreshAssignedHosts();

	ChangeSet mChangeSet;
	mutable User mUser;
	
	GroupList mNewUserGroups;
	bool mIsEmp;
};

#endif // USER_DIALOG_H

