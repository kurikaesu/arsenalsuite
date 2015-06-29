

#ifndef PROJECTS_EDIT_DIALOG_H
#define PROJECTS_EDIT_DIALOG_H

#include <qdialog.h>

#include "classesui.h"
#include "recordsupermodel.h"

#include "ui_projectseditdialogui.h"

#include "project.h"

class CLASSESUI_EXPORT ProjectsEditDialog : public QDialog, public Ui::ProjectsEditDialogUI
{
Q_OBJECT
public:
	ProjectsEditDialog( QWidget * parent = 0 );

	void refresh();

	virtual void accept();

public slots:
	void slotNewMethod();
	void slotRemoveMethod();
	void slotCurrentChanged( const Record & );

protected:
	Project mCurrent;
	RecordSuperModel * mModel;
};

#endif // PERMS_DIALOG_H

