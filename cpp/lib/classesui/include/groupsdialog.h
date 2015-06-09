

#ifndef GROUPS_DIALOG_H
#define GROUPS_DIALOG_H

#include <qmap.h>

#include "classesui.h"

#include "ui_groupsdialogui.h"
#include "user.h"
#include "usergroup.h"
#include "group.h"

class CLASSESUI_EXPORT GroupsDialog : public QDialog, public Ui::GroupsDialogUI
{
Q_OBJECT
public:
	GroupsDialog( QWidget * parent, ChangeSet changeSet );
	
	void setUser( const User & );
	User user();
	
	GroupList checkedGroups();
	void setCheckedGroups( GroupList );
	
	ChangeSet changeSet() const { return mChangeSet; }
	
public slots:
	void reset();
	void newGroup();
	void deleteGroup();

	void groupsAdded(RecordList);
	void groupsRemoved(RecordList);
	void groupUpdated(Record,Record);
	
	void userGroupsAdded(RecordList);
	void userGroupsRemoved(RecordList);

protected:
	virtual void accept();

	ChangeSetNotifier * mGroupNotifier, * mUserGroupNotifier;
	ChangeSet mChangeSet;
	User mUser;
};

#endif // GROUPS_DIALOG_H

