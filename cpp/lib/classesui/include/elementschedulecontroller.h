/*
 *
 * Copyright 2003, 2004 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Resin is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Resin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#ifndef ELEMENT_SCHEDULE_CONTROLLER
#define ELEMENT_SCHEDULE_CONTROLLER

#include <qmap.h>
#include <qlist.h>
#include <qaction.h>

#include "assettype.h"
#include "calendar.h"
#include "calendarcategory.h"
#include "element.h"
#include "timesheet.h"
#include "schedulerow.h"
#include "scheduleentry.h"
#include "schedule.h"
#include "user.h"

class QToolBar;

class CLASSESUI_EXPORT ScheduleAction : public QAction
{
Q_OBJECT
public:
	ScheduleAction( const User & user, const Element & el, const QDate & start, const QDate & end, QObject * parent, QWidget * dialogParent=0 );

	void init();
public slots:
	void slotTriggered();
public:
	User mUser;
	Element mElement;
	AssetType mAssetType;
	QDate mStart, mEnd;
	QWidget * mDialogParent;
};

class CLASSESUI_EXPORT TimeSheetAction : public QAction
{
Q_OBJECT
public:
	TimeSheetAction( const Element & el, const QDate & start, QObject * parent, const QDate & end = QDate() );

	void init();
public slots:
	void slotTriggered();
public:
	Element mElement;
	QDate mStartDate, mEndDate;
};

class CLASSESUI_EXPORT EventAction : public QAction
{
Q_OBJECT
public:
	enum {
		Create,
		Edit,
		Delete
	};
	EventAction( const QDate &, QObject * parent );
	EventAction( const Calendar & event, QObject * parent, int action = Edit );
	
	void init();
public slots:
	void slotTriggered();
public:
	QDate mDate;
	Calendar mEvent;
	int mAction;
};

/* Provide the data for one spot covoring an arbitrary length
 * of time on the calendar.
 */
class CLASSESUI_EXPORT TimeSheetEntry : public ScheduleEntry
{
public:
	TimeSheetEntry( ScheduleRow * , TimeSheetList, bool showUser=false );
	virtual QDate dateStart() const;
	virtual QDate dateEnd() const;
	virtual void setDateStart( const QDate & );
	virtual void setDateEnd( const QDate & );
	virtual void addDate( const QDate & );
	virtual void removeDate( const QDate & );
	virtual void applyChanges( bool commit );
	virtual QColor color() const;
	virtual QStringList text(const QDate &) const;
	virtual QString toolTip( const QDate &, int displayMode ) const;
	virtual int hours( const QDate & ) const;
	virtual void popup( QWidget * parent, const QPoint &, const QDate & date, ScheduleSelection sel );
	virtual int sortKey() const;
	virtual bool cmp( ScheduleEntry * other ) const;
	virtual int heightForWidth( int width, QPaintDevice * device, const ScheduleDisplayOptions & displayOptions, int colWidth ) const;
	virtual void paint( const PaintOptions & );
	virtual int allowResize() const;

	bool merge( ScheduleEntry * entry );

	TimeSheet byDate( const QDate & date ) const;
	mutable TimeSheet mStart, mEnd;
	mutable TimeSheetList mTimeSheets, mAdded, mRemoved;
	mutable int mLastWidth, mLastHeight;
	mutable bool mShowUser;
	int mLeadHeight;
	mutable int mCommentHeight;
};

class CLASSESUI_EXPORT UserScheduleEntry : public ScheduleEntry
{
public:
	UserScheduleEntry( ScheduleRow *, ScheduleList, bool showUser=false );
	virtual QDate dateStart() const;
	virtual QDate dateEnd() const;
	virtual void setDateStart( const QDate & );
	virtual void setDateEnd( const QDate & );
	virtual void addDate( const QDate & );
	virtual void removeDate( const QDate & );
	virtual void applyChanges( bool commit );
	virtual QColor color() const;
	virtual QStringList text( const QDate & ) const;
	virtual QString toolTip( const QDate &, int displayMode ) const;
	virtual int hours( const QDate & ) const;
	virtual void popup( QWidget * parent, const QPoint &, const QDate & date, ScheduleSelection sel );
	virtual int sortKey() const;
	virtual bool cmp( ScheduleEntry * other ) const;
	virtual int heightForWidth( int width, QPaintDevice * device, const ScheduleDisplayOptions & displayOptions, int colWidth ) const;
	virtual void paint( const PaintOptions & );
	virtual int allowResize() const;
	virtual bool allowDrag( ScheduleEntrySelection selection ) const;

	virtual bool canDrop( ScheduleEntrySelection selection, const QDate & startDragDate, const QDate & dropDate, ScheduleRow * row );
	virtual void drop( ScheduleEntrySelection selection, const QDate & startDragDate, const QDate & dropDate, ScheduleRow * row );

	bool merge( ScheduleEntry * entry );

	Schedule byDate( const QDate & date ) const;
	mutable Schedule mStart, mEnd;
	mutable ScheduleList mSchedules, mAdded, mRemoved;
	mutable int mLastWidth, mLastHeight;
	mutable bool mShowUser;
};

class CLASSESUI_EXPORT CalendarEntry : public ScheduleEntry
{
public:
	CalendarEntry( ScheduleRow *, const Calendar & );
	virtual QDate dateStart() const;
	virtual QDate dateEnd() const;
	virtual void setDateStart( const QDate & );
	virtual void setDateEnd( const QDate & );
	virtual void applyChanges( bool commit );
	virtual QColor color() const;
	virtual QString toolTip( const QDate &, int displayMode ) const;
	virtual void popup( QWidget * parent, const QPoint &, const QDate & date, ScheduleSelection sel );
	virtual int sortKey() const;
	virtual bool cmp( ScheduleEntry * other ) const;
	virtual int heightForWidth( int width, QPaintDevice * device, const ScheduleDisplayOptions & displayOptions, int colWidth ) const;
	virtual void paint( const PaintOptions & );

	bool merge( ScheduleEntry * entry );

	mutable int mLastWidth, mLastHeight;
	mutable Calendar mCal;
};

class CLASSESUI_EXPORT TimeSpanEntry : public ScheduleEntry
{
public:
	TimeSpanEntry( ScheduleRow *, const Element & );
	virtual QDate dateStart() const;
	virtual QDate dateEnd() const;
	virtual void setDateStart( const QDate & );
	virtual void setDateEnd( const QDate & );
	virtual void applyChanges( bool commit );
	virtual QColor color() const;
	virtual QString toolTip( const QDate &, int displayMode ) const;
	virtual int sortKey() const;
	virtual int heightForWidth( int width, QPaintDevice * device, const ScheduleDisplayOptions & displayOptions, int colWidth ) const;
	virtual void paint( const PaintOptions & );
	virtual int allowResize() const;

	mutable Element mElement;
};

class ElementScheduleController;

/* Information needed to draw one row on the schedule */
class CLASSESUI_EXPORT ElementScheduleRow : public ScheduleRow
{
Q_OBJECT
public:
	ElementScheduleRow( const Element &, ElementScheduleController *, ScheduleRow * parent = 0 );
	virtual ~ElementScheduleRow();
	virtual QList<ScheduleEntry*> range();
	virtual QPixmap image();
	virtual QString name();
	virtual QList<ScheduleRow*> children();
	virtual void populateMenu( QMenu * menu, const QPoint &, const QDate & dateStart, const QDate &, int menuId );
	Element element() const { return mElement; }

	void setDateRange( const QDate & start, const QDate & end );
	
	virtual QDate startDate();
	virtual QDate endDate();
	//virtual int duration() { return startDate().daysTo( endDate() ); }

	virtual void setStartDate( const QDate & );
	virtual void setEndDate( const QDate & );

	bool mShowGlobalEvents;
public slots:
	void timeSheetsAdded( RecordList );
	void timeSheetUpdated( Record, Record );
	void timeSheetsRemoved( RecordList );
	
	void schedulesAdded( RecordList );
	void scheduleUpdated( Record, Record );
	void schedulesRemoved( RecordList );

	void calendarsAdded( RecordList );
	void calendarUpdated( Record, Record );
	void calendarsRemoved( RecordList );

	void update();
protected:
	void clear();
	void clearTimeSheets();
	void clearSchedules();
	void clearCalendars();

	void updateTimeSheets();
	void updateSchedules();
	void updateCalendars();

	ScheduleList getSchedules( const ElementList &, const QDate & start, const QDate & end );
	TimeSheetList getTimeSheets( const ElementList &, const QDate & start, const QDate & end );

	ElementScheduleController * mController;
	Element mElement;
	
	QList<ScheduleRow*> mChildren;

	/* Filters */
	Project mProject;
	AssetType mAssetType;

	QMap<QDate, QList<TimeSheetEntry*> > mTimeSheetEntries;
	QMap<QDate, QList<UserScheduleEntry*> > mScheduleEntries;
	QMap<QDate, QList<CalendarEntry*> > mCalendarEntries;
	TimeSpanEntry * mTimeSpan;
	bool mNeedsUpdate;
};

/*
 * This row is used to show entries for a certain category for an entire project
 */
class CLASSESUI_EXPORT CategoryGroupingRow : public ElementScheduleRow
{
Q_OBJECT
public:
	CategoryGroupingRow( const Element & element, const QStringList & groups, ElementScheduleController *, ScheduleRow * parent = 0 );
	virtual ~CategoryGroupingRow();
	virtual QList<ScheduleEntry*> range();
	virtual QList<ScheduleRow*> children();
	virtual void populateMenu( QMenu * menu, const QPoint &, const QDate & dateStart, const QDate &, int menuId );
	
	virtual QDate startDate();
	virtual QDate endDate();
	//virtual int duration() { return startDate().daysTo( endDate() ); }

	virtual void setStartDate( const QDate & );
	virtual void setEndDate( const QDate & );

protected:
	void createChildren( QMap<int,ElementList> & byType );

	QStringList mGroupList;
	QList<ScheduleRow*> mChildren;
};

class CLASSESUI_EXPORT GlobalEventsRow : public ScheduleRow
{
Q_OBJECT
public:
	GlobalEventsRow( ElementScheduleController * );
	virtual ~GlobalEventsRow();
	virtual QList<ScheduleEntry*> range();
	virtual QString name();
	virtual void populateMenu( QMenu * menu, const QPoint &, const QDate & date, const QDate &, int menuId );

	void setDateRange( const QDate & start, const QDate & end );
public slots:
	void calendarsAdded( RecordList );
	void calendarUpdated( Record, Record );
	void calendarsRemoved( RecordList );

protected:
	void clear();
	void clearCalendars();

	void updateCalendars();

	ElementScheduleController * mController;
	QMap<QDate, QList<CalendarEntry*> > mCalendarEntries;
	CalendarCategoryList mFilter;
	ProjectList mProjectFilters;
	bool mShowGlobal, mShowMineOnly;
	bool mNeedsUpdate;
};

class CLASSESUI_EXPORT CalendarPlugin
{
public:
	virtual QList<ScheduleEntry*> range( ScheduleRow *, const QDate &, const QDate & ) { return QList<ScheduleEntry*>(); }
	virtual QList<ScheduleRow*> dataSources( ScheduleController * ) { return QList<ScheduleRow*>(); }
	virtual void populateViewMenu( QMenu * ) = 0;
	virtual void populateToolBar( QToolBar * ) = 0;
};

class CLASSESUI_EXPORT ElementScheduleController : public ScheduleController
{
Q_OBJECT
public:
	ElementScheduleController();
	~ElementScheduleController();

	virtual QList<ScheduleRow*> dataSources();

	virtual QList<ScheduleRow*> getRows( ElementList, ScheduleRow * parent = 0 );

	virtual ScheduleRow * getRow( const Element &, ScheduleRow * parent = 0 );

	bool showTimeSheets() const;
	bool showSchedules() const;
	bool showCalendars() const;
	bool showRecursive() const;
	bool showTimeSpans() const;

	ElementList elementList() const { return mElements; }

	CalendarCategoryList calendarCategories() const { return mCalendarCategories; }

	bool showGlobalEvents() const;

	bool showMyEventsOnly() const { return mShowMyEventsOnly; }

	ProjectList projectFilters() const { return mProjectFilters; }

	static void registerPlugin( CalendarPlugin * );
	
	static QList<CalendarPlugin*> plugins();

public slots:

	void setElementList( ElementList );
	
	void setShowTimeSheets( bool st );
	void setShowSchedules( bool ss );

	void setShowCalendars( bool sc );

	void setShowCalendarCategories( CalendarCategoryList ccl );

	void setShowMyEventsOnly( bool );

	void setShowRecursive( bool sr );

	void setShowTimeSpans( bool sts );

	void setShowGlobalEvents( bool );

	void setProjectFilters( ProjectList projectFilter );

protected slots:
	void elementsAdded(RecordList);
	void elementsRemoved(RecordList);
	void elementUpdated(const Record &, const Record &);
protected:
	ElementList mElements;
	QMap<uint,ElementScheduleRow*> mCachedRows;
	QList<ScheduleRow*> mCurRows;
	bool mShowTimeSheets, mShowSchedules, mShowCalendars, mRecursive, mShowTimeSpans, mShowGlobalEvents;
	CalendarCategoryList mCalendarCategories;
	ProjectList mProjectFilters;
	bool mShowMyEventsOnly;
	GlobalEventsRow * mGlobalEventsRow;
	static QList<CalendarPlugin*> mPlugins;
};


#endif // ELEMENT_SCHEDULE_CONTROLLER



