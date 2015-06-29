

#ifndef SERVICE_EDIT_DIALOG_H
#define SERVICE_EDIT_DIALOG_H

#include <qdialog.h>

#include "classesui.h"
#include "recordsupermodel.h"

#include "ui_serviceeditdialogui.h"

#include "service.h"

class CLASSESUI_EXPORT ServiceEditDialog : public QDialog, public Ui::ServiceEditDialogUI
{
Q_OBJECT
public:
	ServiceEditDialog( QWidget * parent = 0 );

	void refresh();

	virtual void accept();

public slots:
	void slotNewMethod();
	void slotRemoveMethod();
	void slotCurrentChanged( const Record & );

protected:
	Service mCurrent;
	RecordSuperModel * mModel;
};

#endif // SERVICE_EDIT_DIALOG_H

