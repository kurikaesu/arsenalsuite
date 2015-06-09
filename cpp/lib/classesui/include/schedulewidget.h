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


#ifndef SCHEDULE_WIDGET_H
#define SCHEDULE_WIDGET_H

#include <QScrollArea>

#include <vector>

#include <qmap.h>
#include <qwidget.h>
#include <qdatetime.h>
#include <qcolor.h>
#include <qpixmap.h>
#include <qlist.h>
#include <qrect.h>
#include <qscrollbar.h>

#include "classesui.h"

#define ENABLE_SW_DEBUG 0

#if ENABLE_SW_DEBUG
#define SW_DEBUG(...) LOG_3(__VA_ARGS__)
#else
#define SW_DEBUG(...)
#endif // ENABLE_SW_DEBUG

class QLabel;
class QPushButton;
class QSplitter;
class QTreeWidget;
class QTreeWidgetItem;
class QModelIndex;
class QMenu;

class ExtTreeView;

// Data classes
class ScheduleController;
class ScheduleRow;
class ScheduleEntry;

// Sub-widgets
class ScheduleHeader;
class SchedulePanel;

// Internal widgets
class ScrollArea;
class ScheduleCanvas;

#include "scheduleselection.h"

class CLASSESUI_EXPORT ScheduleDisplayOptions
{
public:
	QFont headerMonthYearFont, headerDayWeekFont;
	QFont panelFont;
	QFont cellDayFont;
	QFont entryFont, entryHoursFont;
};

class CLASSESUI_EXPORT ScheduleWidget : public QWidget
{
Q_OBJECT
public:
	ScheduleWidget( ScheduleController * sc, QWidget * parent );
	~ScheduleWidget();

	// Date Related Functions
	// First Date displayed on the calendar
	QDate date() const;
	// Last Date displayed on the calendar
	QDate endDate() const;

	QDate dateAtCell( int row, int col );
	int dateColumn( const QDate & date );
	int dateCell( const QDate & date, ScheduleRow * sr = 0, int * row=0, int * col=0 );

	//
	// Display Options Related Functions
	//
	enum {
		Week,
		Month,
		MonthFlat,
		Year
	};

	int displayMode() const;
	bool showWeekends() const;
	int weekStartDay() const;
	int zoom() const;

	//
	// Layout Related functions
	//
	int rowCount() const;
	int columnCount() const;
	int cellCount() const;

	int colPos( int col ) const;
	int colWidth( int col = 0 ) const;

	int rowPos( int row ) const;
	int rowHeight( int row ) const;

	// Pixels
	uint offset() const;
	uint xOffset() const;
	int contentsWidth() const;
	int contentsHeight() const;

	bool pointToCell( const QPoint & pos, int * row=0, int * col=0, QPoint * celPos=0 );

	bool isCellValid( int row, int col );

	// Takes a logical rect(eg. [0,0] - [4,4], 16 squares) and returns the pixel coverage rect
	// TODO: Remove these functions once the new selections stuff is in place
	QRect posToPixels( const QRect & ) const;

	//
	// Selection Related Functions
	//
	ScheduleSelection selection();
	void setSelection( const ScheduleSelection & );

	ScheduleSelection current();
	bool moveCurrent( int cell, ScheduleEntry * entry, Qt::KeyboardModifiers );

	enum {
		SingleSelect,
		ExtendedSelect
	};

	int selectionMode() const;

		// Header and Panel widgets
	ScheduleHeader * header() const { return mHeader; }
	SchedulePanel * panel() const { return mPanel; }
	ScrollArea * scrollArea() const { return mScrollView; }

	// Data
	QList<ScheduleRow*> rows() const;
	QList<ScheduleEntry*> cellEntries( int row, int col );
	QList<ScheduleRow*> scheduleRowsForRow( int row );
	ScheduleEntry * entryAtPos( const QPoint & pos, QRect * rect=0 );

	ScheduleDisplayOptions mDisplayOptions;

	struct PrintOptions
	{
		QString leftHeaderText, middleHeaderText, rightHeaderText;
		QString leftFooterText, middleFooterText, rightFooterText;
		int horizontalPages, verticalPages;
		bool avoidSplitRows;
		bool printHeader, printPanel;
		bool panelOnEveryPage, headerOnEveryPage;
		ScheduleDisplayOptions * displayOptions;
		QFont headerFooterFont;
	};

public slots:

	void slotRowsChanged();

	void setSelectionMode( int sm );
	void setDisplayMode( int mode = Week );
	void setShowWeekends( bool sw );
	void setWeekStartDay( int wsd );
	void setZoom( int zoom );

	void setDate( const QDate & date );
	void setEndDate( const QDate & endDate );

	void back();
	void forward();
	void yearBack();
	void yearForward();
	void today();

	void contentsMoved( int x, int y );

	// Uses the viewport width for the desired width if -1
	void updateColumnPositions( int desiredWidth = -1 );

	// Returns true if the row positions have changed
	bool layoutCells(int rowstart=0,int rowend=-1, QPaintDevice * paintDevice = 0, int desiredHeight = -1 );

	void slotRowChanged( ScheduleRow * row, int change );

	void print( QPrinter * printer, PrintOptions & po );

signals:
	// Emitted when the highlighted rectangle changes
	void highlightChanged( const QRect & );

	void rowsChanged();
	
	void layoutChanged();
	
public:

	void setRows( QList<ScheduleRow*> rows );

	void addCellEntries( ScheduleEntry * entry, const QDate & startDate, const QDate & endDate );
	void removeCellEntries( ScheduleEntry * entry, const QDate & startDate, const QDate & endDate );

	class ResizeEntry {
	public:
		ResizeEntry( bool rightSide, int row, int col, ScheduleEntry * entry )
		: mRightSide( rightSide ), mRowStart( row ), mColStart( col ), mCurRow( row ), mCurCol( col )
		, mEntry( entry )
		{}
		bool mRightSide;
		int mRowStart, mColStart, mCurRow, mCurCol;
		ScheduleEntry * mEntry;
	};

	ResizeEntry * mResizeEntry;

	// Widget colors
	QColor mCellBackground;
	QColor mCellWeekendBackground;
	QColor mCellBackgroundHighlight;
	QColor mCellBorder;
	QColor mCellBorderHighlight;
	QColor mHeaderBackground;
	QColor mHeaderBackgroundHighlight;
	QColor mPanelBackground;
	QColor mPanelBackgroundHighlight;

	// Widget sizes
	int mWeekStartDay;
	int mCellBorderWidth;
	int mCellBorderWidthHighlight;
	int mCellLeadHeight;
	int mCellMinHeight;
	int mEntryBorderWidth;
	int mEntryBorderWidthHighlight;
	int mEntryHeight;
	int mMonthCellSize;
	uint mPanelWidth;

	bool mShowingPopup;

	int mRowCount;
	int mColumnCount;
	int mCellCount;

	// Row/column layout
	std::vector<int> mRowPositions, mRowHeights;
	std::vector<int> mColumnPositions;

	// Cell entries, they keep track of their own layout
	// through their mLayouts map.
	QVector<QList<ScheduleEntry*> > mCellEntries;

	struct RowEntry {
		RowEntry(ScheduleEntry * e, int cs, int ce) : entry(e), colStart(cs), colEnd(ce) {}
		bool operator<( const RowEntry & other ) const;
		bool operator==(const RowEntry & other ) const;
		ScheduleEntry * entry;
		int colStart, colEnd;
	};

	QVector<QList<RowEntry> > mRowEntries;

	// Sometimes when updating the cell entries
	// we will get a signal that something has been
	// updated, we must avoid updating when we are
	// already in the process of updating
	bool mUpdatingCellEntries;
protected:
	virtual bool eventFilter( QObject *, QEvent * );
	void resizePanel();

	//
	// Updates the rowCount, columnCount, and cellCount based
	// on current ScheduleRow's and view mode
	//
	void updateDimensions();

	void updateCellEntries();

	// Vertical Scroll Position
	int mContentsYOffset, mContentsXOffset;

	// Week, Month, MonthFlat, Year
	int mDisplayMode;

	int mSelectionMode;
	ScheduleSelection mSelection;

	// This must only be one cell OR one entry(in one cell) in mCurrent
	ScheduleSelection mCurrent;
 
	// Date of first cell, and date navigated to
	QDate mDate, mOrigDate, mEndDate;

	// The data sources
	// The controller provides the rows
	ScheduleController * mController;
	QList<ScheduleRow*> mRows;

	// Header and Panel widgets
	ScheduleHeader * mHeader;
	SchedulePanel * mPanel;

	ScrollArea * mScrollView;
	ScheduleCanvas * mCanvas;

	// Container widgets
	QWidget * mHeaderCanvasContainer;
	QWidget * mPanelContainer;
	
	QSplitter * mSplitter;
	
	// Date Label and navigation
	QLabel * mDateLabel;
	QPushButton * mForward, * mBack, * mYearForward, * mYearBack, * mTodayButton;

	bool mHideWeekends;
	
	int mZoom; // Percentage
};


/*
 *  This class is the actual widget inside the scroll view
 *  that is painted on and recieves the input events.
 *  It uses it's ScheduleWidget * to access all the layout
 *  info it needs.
 */
class CLASSESUI_EXPORT ScheduleCanvas : public QWidget
{
Q_OBJECT
public:
	ScheduleCanvas( QWidget * parent, ScheduleWidget * sw );

	ScheduleWidget * sched();

	ScheduleEntry * findHandle( const QPoint &, bool * right = 0 );

	QSize sizeHint() const;

	void paint( QPainter * p, const QRect & rect, bool paintBackground = true );

protected:
	virtual void paintEvent( QPaintEvent * pe );
	virtual void mousePressEvent( QMouseEvent * );
	virtual void mouseMoveEvent( QMouseEvent * );
	virtual void mouseReleaseEvent( QMouseEvent * );
	virtual void mouseDoubleClickEvent( QMouseEvent * );
	virtual bool eventFilter( QObject *, QEvent * );
	virtual void resizeEvent( QResizeEvent * );
	ScheduleWidget * mSchedulePopup;
	ScheduleWidget * mSched;
	bool mDragging;
	QPoint mDragStartPosition;
	QPoint mDragPosition;
	QDate mDragStartDate;
	QRect mLastDragRect;
};

class CLASSESUI_EXPORT ScrollArea : public QScrollArea
{
Q_OBJECT
public:
	ScrollArea( QWidget * parent ) : QScrollArea( parent ) {}
signals:
	void contentsMoving( int, int );
protected:
	void scrollContentsBy( int x, int y );
};


/*
 * Inline Functions
 */
inline int ScheduleWidget::rowCount() const
{ return mRowCount; }

inline int ScheduleWidget::columnCount() const
{ return mColumnCount; }

inline int ScheduleWidget::cellCount() const 
{ return rowCount() * columnCount(); }

inline uint ScheduleWidget::offset() const
{ return mContentsYOffset; }

inline uint ScheduleWidget::xOffset() const
{ return mContentsXOffset; }

inline int ScheduleWidget::contentsWidth() const
{ return mCanvas->width(); }

inline int ScheduleWidget::contentsHeight() const
{ return mCanvas->height(); }

#endif // SCHEDULE_WIDGET_H

