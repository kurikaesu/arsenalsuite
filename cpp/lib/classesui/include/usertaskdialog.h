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

#ifndef USER_TASK_H
#define USER_TASK_H

#include <qdialog.h>

#include "element.h"
#include "user.h"
#include "assettype.h"
#include "ui_userroleui.h"
#include "ui_usertaskui.h"

class CLASSESUI_EXPORT UserTaskDialog : public QDialog, public Ui::UserTaskUI
{
Q_OBJECT
public:
	UserTaskDialog( QWidget * parent=0 );

	void setUsers( UserList );
	UserList userList() const;

public slots:

	void addUser();
	void removeUser();
	void slotRoleComboChange();
	void setRole( const AssetType & );

	void setUserInfo( const QString & );
	void editUserRoleList();

	
protected:

	AssetType mDefaultAssetType;
	AssetType mAssetTypeFilter;
	
	QMap <QString,User> mAvailable;
	QMap <QString,User> mSelected;
	QMap <QString,AssetType> mAssetTypeMap;

	User mSelectedUser;

	QStringList mRoles;
};

#endif // USER_TASK_H

