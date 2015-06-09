

#ifndef SCHEDULE_ROW_H
#define SCHEDULE_ROW_H

#include <qobject.h>
#include <qlist.h>
#include <qpixmap.h>
#include <qdatetime.h>

#include "classesui.h"

class ScheduleEntry;
class QMenu;
class QPoint;

class CLASSESUI_EXPORT ScheduleRow : public QObject
{
Q_OBJECT
public:
	ScheduleRow( ScheduleRow * parent = 0 )
	: mY( 0 )
	, mHeight( 0 )
	, mExpanded( true )
	, mParent( parent )
	{}

	virtual ~ScheduleRow()
	{}
	virtual QList<ScheduleEntry*> range() = 0;
	virtual QPixmap image() { return QPixmap(); }
	virtual QString name() = 0;
	virtual QDate startDate() { return QDate::currentDate(); }
	virtual QDate endDate() { return QDate::currentDate(); }
	virtual int duration() { return startDate().daysTo( endDate() ); }


	virtual void setStartDate( const QDate & ) {}
	virtual void setEndDate( const QDate & ) {}
	
	virtual void setDateRange( const QDate & start, const QDate & end ) { mStart = start; mEnd = end; }
	virtual QList<ScheduleRow*> children() { return QList<ScheduleRow*>(); }
	ScheduleRow * parent() { return mParent; }

	virtual void populateMenu( QMenu * , const QPoint &, const QDate &, const QDate &, int /* menuId */ ) {}

	enum ChangeType {
		HeightChange=1,
		SpanChange=2,
	};

	bool expanded() const { return mExpanded; }
	void setExpanded( bool exp ) { mExpanded = exp; }
	
	int mY, mHeight;
	bool mExpanded;
signals:
	void change( ScheduleRow *, int );
protected:
	ScheduleRow * mParent;
	QDate mStart, mEnd;
};

class CLASSESUI_EXPORT ScheduleController : public QObject
{
Q_OBJECT
public:
 	virtual QList<ScheduleRow*> dataSources() = 0;

signals:

	void rowsChanged();
};


#endif // SCHEDULE_ROW_H
