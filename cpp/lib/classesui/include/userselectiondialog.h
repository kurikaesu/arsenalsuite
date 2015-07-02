#ifndef USER_SELECTION_DIALOG_H
#define USER_SELECTION_DIALOG_H

#include <qdialog.h>

#include "classesui.h"
#include "recordsupermodel.h"

#include "ui_userselectiondialogui.h"

#include "user.h"

class CLASSESUI_EXPORT UserSelectionDialog : public QDialog, public Ui::UserSelectionDialogUI
{
Q_OBJECT
public:
	UserSelectionDialog( QWidget * parent = 0, const UserList& available=UserList(), const UserList& selected=UserList() );
	
	virtual void accept();
	
	void setAvailableUsers(const UserList& available);
	void setSelectedUsers(const UserList& selected);
	UserList selectedUsers() const;
public slots:
	void slotRemoveUser();
	void slotAddUser();
protected:
	User mCurrentAvailable;
	User mCurrentSelected;
	RecordSuperModel * mAvailableModel;
	RecordSuperModel * mSelectedModel;	
};

#endif
