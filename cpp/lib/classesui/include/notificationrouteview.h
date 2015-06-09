
#ifndef NOTIFICATION_ROUTE_VIEW_H
#define NOTIFICATION_ROUTE_VIEW_H

// libclasses
#include "notificationroute.h"
#include "user.h"

// libstonegui
#include "recordtreeview.h"
#include "recordsupermodel.h"

class NotificationRouteModel : public RecordSuperModel
{
Q_OBJECT
public:
	NotificationRouteModel( QObject * parent );

};

class NotificationRouteView : public RecordTreeView
{
Q_OBJECT
public:
	NotificationRouteView( QWidget * parent = 0 );

public slots:
	void setUserFilter( const User & user );
	void addRoute();
	void commit();

protected:
	NotificationRouteModel * mModel;
	User mUserFilter;
};

#endif // NOTIFICATION_ROUTE_VIEW_H

