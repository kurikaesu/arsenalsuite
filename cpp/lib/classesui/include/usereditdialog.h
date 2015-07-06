

#ifndef USER_EDIT_DIALOG_H
#define USER_EDIT_DIALOG_H

#include <qdialog.h>

#include "classesui.h"
#include "recordsupermodel.h"

#include "ui_usereditdialogui.h"

#include "user.h"

class CLASSESUI_EXPORT UserEditDialog : public QDialog, public Ui::UserEditDialogUI
{
Q_OBJECT
public:
	UserEditDialog( QWidget * parent = 0 );

	void refresh();

	virtual void accept();

public slots:
	void slotNewMethod();
	void slotEditMethod();
	void slotRemoveMethod();
	void slotCurrentChanged( const Record & );

protected:
	User mCurrent;
	RecordSuperModel * mModel;
};

#endif // USER_EDIT_DIALOG_H

