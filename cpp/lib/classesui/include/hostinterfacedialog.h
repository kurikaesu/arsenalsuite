

#ifndef HOST_INTERFACE_DIALOG_H
#define HOST_INTERFACE_DIALOG_H

#include <qdialog.h>

#include "classesui.h"

#include "host.h"
#include "recordsupermodel.h"
#include "ui_hostinterfacedialogui.h"

class CLASSESUI_EXPORT HostInterfaceDialog : public QDialog, public Ui::HostInterfaceDialogUI
{
Q_OBJECT
public:
	HostInterfaceDialog( QWidget * parent = 0 );

	Host host() const { return mHost; }
public slots:
	void setHost( const Host & host );

protected slots:
	void showMenu();

protected:
	
	RecordSuperModel * mModel;
	Host mHost;
};

#endif // HOST_INTERFACE_DIALOG_H

