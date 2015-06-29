

#ifndef JOBTYPE_EDIT_DIALOG_H
#define JOBTYPE_EDIT_DIALOG_H

#include <qdialog.h>

#include "classesui.h"
#include "recordsupermodel.h"

#include "ui_jobtypeeditdialogui.h"

#include "jobtype.h"

class CLASSESUI_EXPORT JobTypeEditDialog : public QDialog, public Ui::JobTypeEditDialogUI
{
Q_OBJECT
public:
	JobTypeEditDialog( QWidget * parent = 0 );

	void refresh();

	virtual void accept();

public slots:
	void slotNewMethod();
	void slotRemoveMethod();
	void slotCurrentChanged( const Record & );

protected:
	JobType mCurrent;
	RecordSuperModel * mModel;
};

#endif // JOBTYPE_EDIT_DIALOG_H

