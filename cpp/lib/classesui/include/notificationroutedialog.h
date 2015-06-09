

#ifndef NOTIFICATION_ROUTE_DIALOG_H
#define NOTIFICATION_ROUTE_DIALOG_H

#include <qdialog.h>

#include "ui_notificationroutedialogui.h"

class NotificationRouteDialog : public QDialog, public Ui::NotificationRouteDialogUI
{
Q_OBJECT
public:
	NotificationRouteDialog( QWidget * parent = 0 );

public slots:
	void slotUserFilterChanged( const Record & );
	void slotAddRoute();
	void slotRemoveRoute();
	void routeSelectionChanged( RecordList );
protected:
	void accept();
};

#endif // NOTIFICATION_ROUTE_DIALOG_H

