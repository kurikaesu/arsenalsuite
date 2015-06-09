

#ifndef PERMS_DIALOG_H
#define PERMS_DIALOG_H

#include <qdialog.h>

#include "classesui.h"
#include "recordsupermodel.h"

#include "ui_permsdialogui.h"

#include "permission.h"

class CLASSESUI_EXPORT PermsDialog : public QDialog, public Ui::PermsDialogUI
{
Q_OBJECT
public:
	PermsDialog( QWidget * parent = 0 );

	void refresh();

	virtual void accept();

public slots:
	void slotNewMethod();
	void slotRemoveMethod();
	void slotCurrentChanged( const Record & );

protected:
	Permission mCurrent;
	RecordSuperModel * mModel;
};

#endif // PERMS_DIALOG_H

