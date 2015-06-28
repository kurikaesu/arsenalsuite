

#ifndef LICENSE_DIALOG_H
#define LICENSE_DIALOG_H

#include <qdialog.h>

#include "classesui.h"
#include "recordsupermodel.h"

#include "ui_licensedialogui.h"

#include "license.h"

class CLASSESUI_EXPORT LicenseDialog : public QDialog, public Ui::LicenseDialogUI
{
Q_OBJECT
public:
	LicenseDialog( QWidget * parent = 0 );

	void refresh();

	virtual void accept();

public slots:
	void slotNewMethod();
	void slotRemoveMethod();
	void slotCurrentChanged( const Record & );

protected:
	License mCurrent;
	RecordSuperModel * mModel;
};

#endif // LICENSE_DIALOG_H

