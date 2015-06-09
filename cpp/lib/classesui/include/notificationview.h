
#ifndef NOTIFICATION_VIEW_H
#define NOTIFICATION_VIEW_H

#include <qdatetime.h>
#include <qtreeview.h>

#include "recordsupermodel.h"

class NotificationModel : public RecordSuperModel
{
Q_OBJECT
public:
	NotificationModel( QObject * parent );
};

class NotificationView : public QTreeView
{
Q_OBJECT
public:
	NotificationView( QWidget * parent );

public slots:
	
	void refresh();

	void setComponentFilter( const QString & componentFilter );
	void setMethodFilter( const QString & methodFilter );
	void setSubjectFilter( const QString & subjectFilter );
	void setTimeRange( QDateTime startTime, QDateTime endTime );
	void setStartDateTime( const QDateTime & );
	void setEndDateTime( const QDateTime & );
	void setLimit( int limit );

protected:
	NotificationModel * mModel;
	QString mComponentFilter, mMethodFilter, mSubjectFilter;
	QDateTime mTimeStart, mTimeEnd;
	int mLimit;
};

#endif // NOTIFICATION_VIEW_H

