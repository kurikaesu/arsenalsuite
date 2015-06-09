
#include "employee.h"

#include "notificationroutedialog.h"

NotificationRouteDialog::NotificationRouteDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	connect( mUserFilterCombo, SIGNAL(currentChanged( const Record & )), SLOT(slotUserFilterChanged( const Record & )) );
	connect( mAddButton, SIGNAL( clicked() ), SLOT( slotAddRoute() ) );
	connect( mRemoveButton, SIGNAL( clicked() ), SLOT( slotRemoveRoute() ) );
	connect( mNotificationRouteView, SIGNAL( selectionChanged( RecordList ) ), SLOT( routeSelectionChanged( RecordList ) ) );

	EmployeeList emps = Employee::select( "dateoftermination IS NULL" ).sorted( "name" );
	Employee none;
	none.setName( "Global Routes" );
	emps.insert( emps.begin(), none );
	mUserFilterCombo->setItems( emps  );
	mUserFilterCombo->setColumn( "name" );
	mUserFilterCombo->setCurrentIndex( 0 );

}

void NotificationRouteDialog::slotUserFilterChanged( const Record & r )
{
	mNotificationRouteView->setUserFilter( r );
}

void NotificationRouteDialog::slotAddRoute()
{
	mNotificationRouteView->addRoute();
}

void NotificationRouteDialog::slotRemoveRoute()
{
	RecordList sel = mNotificationRouteView->selection();
	sel.remove();
	mNotificationRouteView->model()->remove(sel);
}

void NotificationRouteDialog::routeSelectionChanged( RecordList rl )
{
	mRemoveButton->setEnabled( bool(rl.size()) );
}

void NotificationRouteDialog::accept()
{
	mNotificationRouteView->commit();
}
