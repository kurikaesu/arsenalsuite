
#include "user.h"

#include "notificationrouteview.h"

NotificationRouteModel::NotificationRouteModel( QObject * parent )
: RecordSuperModel( parent )
{
	RecordDataTranslator * rdt = new RecordDataTranslator(treeBuilder());
	setHeaderLabels( QStringList() << "Event" << "Component" << "User" << "Subject" << "Message" << "Actions" << "Priority" );
	rdt->setRecordColumnList( QStringList() << "eventMatch" << "componentMatch" << "user" << "subjectMatch" << "messageMatch" << "actions" << "priority", true /*defaultEditable*/ );
	
	listen( NotificationRoute::table() );
}

NotificationRouteView::NotificationRouteView( QWidget * parent )
: RecordTreeView( parent )
{
	mModel = new NotificationRouteModel( this );
	setModel( mModel );
	/// Set all columns to auto resize
	setColumnAutoResize( -1, true );
	
}

void NotificationRouteView::setUserFilter( const User & user )
{
	mUserFilter = user;
	mModel->setRootList( NotificationRoute::recordsByUser( user ) );
}

void NotificationRouteView::addRoute()
{
	mModel->append( NotificationRoute() );
}

void NotificationRouteView::commit()
{
	mModel->rootList().commit();
}
