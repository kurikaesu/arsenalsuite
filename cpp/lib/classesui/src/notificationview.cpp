
#include "notification.h"

#include "notificationview.h"

NotificationModel::NotificationModel( QObject * parent )
: RecordSuperModel( parent )
{
	RecordDataTranslator * rdt = new RecordDataTranslator(treeBuilder());
	rdt->setRecordColumnList( QStringList() << "created" << "subject" << "component" << "event" << "routed" );
	setHeaderLabels( QStringList() << "Creation Date" << "Subject" << "Component" << "Event" << "Routed" );
}

NotificationView::NotificationView( QWidget * parent )
: QTreeView( parent )
, mLimit( 500 )
{
	mModel = new NotificationModel( this );
	setModel( mModel );
	setSortingEnabled( true );
}

static void addRegExMatch( QString & query, const QString & column, const QString & value, VarList & args )
{
	if( !value.isEmpty() ) {
		if( !query.isEmpty() )
			query += " AND ";
		query += column + " ~ ?";
		args += value;
	}
}

void NotificationView::refresh()
{
	QString query;
	VarList args;
	addRegExMatch( query, "component", mComponentFilter, args );
	addRegExMatch( query, "method", mMethodFilter, args );
	addRegExMatch( query, "subject", mSubjectFilter, args );
	if( !query.isEmpty() )
		query += " AND ";
	query += " created >= ? AND created <= ? ORDER BY created DESC LIMIT ?";
	args << mTimeStart << mTimeEnd << mLimit;
	NotificationList results = Notification::select( query, args );
	mModel->setRootList( results );
}

void NotificationView::setComponentFilter( const QString & componentFilter )
{
	mComponentFilter = componentFilter;
}

void NotificationView::setMethodFilter( const QString & methodFilter )
{
	mMethodFilter = methodFilter;
}

void NotificationView::setSubjectFilter( const QString & subjectFilter )
{
	mSubjectFilter = subjectFilter;
}

void NotificationView::setTimeRange( QDateTime startTime, QDateTime endTime )
{
	mTimeStart = startTime;
	mTimeEnd = endTime;
}

void NotificationView::setStartDateTime( const QDateTime & dt )
{
	mTimeStart = dt;
}

void NotificationView::setEndDateTime( const QDateTime & dt )
{
	mTimeEnd = dt;
}

void NotificationView::setLimit( int limit )
{
	mLimit = limit;
}

