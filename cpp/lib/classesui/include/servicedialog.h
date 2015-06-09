

#ifndef SERVICE_DIALOG_H
#define SERVICE_DIALOG_H

#include <qmap.h>

#include "classesui.h"
#include "recordsupermodel.h"

#include "ui_servicedialogui.h"

#include "host.h"
#include "service.h"

class CLASSESUI_EXPORT ServiceDialog : public QDialog, public Ui::ServiceDialogUI
{
Q_OBJECT
public:
	ServiceDialog( QWidget * parent=0 );

	Host host() const;
	void setHost( const Host & );

	virtual void accept();
	virtual void reject();
	
public slots:
	void slotNewService();
	void slotRemoveService();
	
	void refresh();
	
protected:
	Host mHost;
	ServiceList mServices;
	RecordSuperModel * mServiceModel;
};

#endif // SERVICE_DIALOG_H
