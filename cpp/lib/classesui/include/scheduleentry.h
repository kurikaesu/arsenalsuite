

#ifndef SCHEDULE_ENTRY_H
#define SCHEDULE_ENTRY_H

#include <qdatetime.h>
#include <qcolor.h>
#include <qrect.h>
#include <qmap.h>

#include "classesui.h"

#include "scheduleselection.h"

class ScheduleRow;
class QWidget;
class QPainter;
class QFontMetrics;
class QPaintDevice;
class ScheduleDisplayOptions;

class CLASSESUI_EXPORT RowLayout
{
public:
	int mPosInRow, mColStart, mColEnd;
	QRect mRect;
};

class CLASSESUI_EXPORT ScheduleEntry
{
public:
	ScheduleEntry( ScheduleRow * row ) : mRow( row ), mRef( 1 ), mPaintId( 0 ) {}
	virtual ~ScheduleEntry(){}
	virtual QDate dateStart() const = 0;
	virtual QDate dateEnd() const = 0;

	// This function returns true if the two
	// entries are continuous, but not overlapping
	// If newStart and newEnd are valid pointers
	// they will be set to the post-merge start and end
	bool canMergeHelper(ScheduleEntry*, QDate*newStart, QDate*newEnd);

	// The default implementation of these two function
	// will call dateStart and dateEnd to figure
	// out which dates need added and removed
	// then it will call addDate or remove date
	// for each.
	virtual void setDateStart( const QDate & );
	virtual void setDateEnd( const QDate & );

	virtual void addDate( const QDate & ) {}
	virtual void removeDate( const QDate & ) {}

	// If commit is false, the changes are reverted
	virtual void applyChanges( bool commit=true )=0;
	virtual QStringList text( const QDate & ) const { return QStringList(); }
	virtual QColor color() const = 0;
	virtual QString toolTip( const QDate & date, int displayMode ) const = 0;
	virtual void popup( QWidget * /*parent*/, const QPoint &, const QDate &, ScheduleSelection sel ) { }
	virtual bool merge( ScheduleEntry * ) { return false; }
	virtual int sortKey() const { return 1; };
	virtual bool cmp( ScheduleEntry * other ) const;
	virtual int hours( const QDate & ) const { return 0; }
	
	enum {
		ResizeLeft=1,
		ResizeRight=2
	};
	virtual int allowResize() const { return 0; }

	virtual bool allowDrag( ScheduleEntrySelection /*selection*/ ) const { return false; }

	virtual bool canDrop( ScheduleEntrySelection /*selection*/, const QDate & /*startDragDate*/, const QDate & /*dropDate*/, ScheduleRow * /*row*/ ) { return false; }
	virtual void drop( ScheduleEntrySelection /*selection*/, const QDate & /*startDragDate*/, const QDate & /*dropDate*/, ScheduleRow * /*row*/ ) {}

	// This is currently per-span, but passes the column width also.  If in the future
	// we change it so that column width's vary, then we'll have to change this function
	virtual int heightForWidth(int /*width*/, QPaintDevice * device, const ScheduleDisplayOptions & displayOptions, int columnWidth ) const;

	struct PaintOptions {
		QPainter * painter;
		QRect spanRect, cellRect;
		QDate startDate, endDate, cellDate;
		int startColumn, endColumn, column;
		QColor backgroundColor;
		int paintId;
		ScheduleEntrySelection selection;
		ScheduleDisplayOptions * displayOptions;
	};

	int hoursBoxHeightForWidth(int width, QPaintDevice * device, const ScheduleDisplayOptions & displayOptions, int colWidth ) const;
 
	void drawHoursBox( const PaintOptions &, const QBrush & backgroundBrush, const QColor & borderColor, bool icon = false );

	void drawSelectionRect( QPainter * p, const QRect & rect, bool drawLeft, bool drawRight );

	virtual void paint( const PaintOptions & ){}

	ScheduleRow * row() const { return mRow; }
	QMap<uint,RowLayout> mLayouts;
	ScheduleRow * mRow;
	int mRef;
	int mPaintId;
	void ref() { mRef++; }
	void deref() { if( --mRef == 0 ) delete this; }
};

#endif // SCHEDULE_ENTRY_H

