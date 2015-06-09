

#ifndef HOST_DIALOG_H
#define HOST_DIALOG_H

#include "ui_hostdialogui.h"

#include "classesui.h"

#include "recordproxy.h"
#include "host.h"

class CLASSESUI_EXPORT HostDialog : public QDialog, public Ui::HostDialogUI
{
Q_OBJECT
public:
	HostDialog( QWidget * parent=0 );

	void setHost( const Host & h );
	Host host();
	virtual void accept();
	virtual void reject();

public slots:
	void slotEditServices();
	void slotEditInterfaces();

protected:
	RecordProxy * mProxy;
	Host mHost;
};

#endif // HOST_DIALOG_H

