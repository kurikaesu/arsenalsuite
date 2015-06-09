
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of libstone.
 *
 * libstone is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libstone is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libstone; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: recordtreeview.h 13596 2012-09-17 23:43:49Z newellm $
 */

#ifndef RECORD_TREE_VIEW_H
#define RECORD_TREE_VIEW_H

#include <qtreeview.h>
#include <qbitarray.h>
#include <qscrollarea.h>

#include "recorddelegate.h"
#include "recordsupermodel.h"
#include "stonegui.h"

class QWidget;
class QGridLayout;
class RecordFilterWidget;

struct ColumnStruct {
	const char * name;
	const char * iniName;
	int defaultSize;
	int defaultPos;
	bool defaultHidden;
	bool filterEnabled;
};

class BusyWidget;

class STONEGUI_EXPORT ExtTreeView : public QTreeView
{
Q_OBJECT
public:
	ExtTreeView( QWidget * parent, ExtDelegate * = 0 );

	void setModel( QAbstractItemModel * model );

	QModelIndexList selectedRows();
	
	bool columnAutoResize( int col ) const;

	/// If col is -1, then all columns are set to autoResize
	void setColumnAutoResize( int col, bool autoResize );

	bool showBranches() const;
	void setShowBranches( bool showBranches );

	int sizeHintForColumn( int column, int * height ) const;
	int sizeHintForColumn ( int column ) const;
	QSize allContentsSizeHint();

	void setShowGrid( bool showGrid );
	bool showGrid() const;

	void setGridColors( const QColor & gridColor, const QColor & gridHighlightColor );
	void getGridColors( QColor & gridColor, QColor & gridHighlightColor );

	static int State_ShowGrid;

	void setupTreeView( IniConfig & ini, const ColumnStruct columns [] );
	void setupRecordFilterWidget( IniConfig & ini, const ColumnStruct columns [] );
	void saveTreeView( IniConfig & ini, const ColumnStruct columns [] );

	void setupTreeView( const QString & group, const QString & key, const ColumnStruct columns [] );
	void saveTreeView( const QString & group, const QString & key, const ColumnStruct columns [] );

	void enableFilterWidget(bool);
	
	RecordFilterWidget * mRecordFilterWidget;

	BusyWidget * busyWidget( bool autoCreate = true );
	
	void setPropagateGroupSelection( bool pgs );
	bool propagateGroupSelection() const;
	
public slots:
	void expandRecursive( const QModelIndex & index = QModelIndex(), int levels = -1 );
	void addFilterLayout();
	void sortBySelection();

signals:
	void aboutToShowHeaderMenu( QMenu * );
	void showMenu( const QPoint & pos, const QModelIndex & underMouse );
	void columnVisibilityChanged( int column, bool visible );

protected slots:
	void scheduleResizeAutoColumns();
	void resizeAutoColumns();
	virtual void slotCustomContextMenuRequested( const QPoint & );
    virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
	void slotSectionResized();
	
	void slotGrouperChanged( ModelGrouper * grouper );
	void slotGroupingChanged();
	void slotGroupCreated( const QModelIndex & );
	void slotGroupEmptied( const QModelIndex & );
	void slotGroupPopulated( const QModelIndex & );
	
protected:
	void saveColumns( IniConfig & ini, const ColumnStruct columns [] );
	void setupColumns( IniConfig & ini, const ColumnStruct columns [] );
	// Overridden to prevent stupid GIANT pixmap with the drag
	virtual void startDrag ( Qt::DropActions supportedActions );
	void drawBranches ( QPainter * painter, const QRect & rect, const QModelIndex & index ) const;
	QStyleOptionViewItem viewOptions() const;

	void showHeaderMenu( const QPoint &, int section = -1 );
	void doAutoColumnConnections();
	bool eventFilter( QObject *, QEvent * );

	ExtDelegate * mDelegate;
	bool mColumnAutoResize, mShowBranches, mAutoResizeScheduled;
	QBitArray mAutoResizeColumns;
	bool mShowGrid;
	QColor mGridColor, mGridColorHighlight;

	friend class RecordFilterWidget;

	void resizeEvent(QResizeEvent *event);

	BusyWidget * mBusyWidget;
	bool mHeaderClickIsResize;
	
	// Loaded from config file, so we can restore the expanded/collapsed state of each group
	int mLastGroupColumn;
	QMap<QString,bool> mGroupExpandState;
	bool mPropagateGroupSelection;
};

class STONEGUI_EXPORT RecordTreeView : public ExtTreeView
{
Q_OBJECT
public:
	RecordTreeView( QWidget * parent );
	void setModel( QAbstractItemModel * model );

	RecordSuperModel * model() const { return dynamic_cast<RecordSuperModel*>(ExtTreeView::model()); }

	RecordDelegate * delegate() const { return (RecordDelegate*)mDelegate; }
	
	QModelIndex findIndex( const Record & r, bool recursive = true, const QModelIndex & parent = QModelIndex(), bool loadChildren = true );
	QModelIndexList findIndexes( RecordList, bool recursive = true, const QModelIndex & parent = QModelIndex(), bool loadChildren = true );
	Record getRecord(const QModelIndex & i);

	Record current();

	RecordList selection();

	using QTreeView::scrollTo;
public slots:
	void setSelection( const RecordList & rl );
	void setCurrent( const Record & r );

	void scrollTo( const Record & );
	void scrollTo( RecordList );
signals:
	void currentChanged( const Record & );
	void selectionChanged( RecordList );
	void clicked( const Record & );
	void showMenu( const QPoint &, const Record & underMouse, RecordList selected );

protected slots:
	void slotCurrentChanged( const QModelIndex & i, const QModelIndex & );
	void slotSelectionChanged( const QItemSelection & sel, const QItemSelection & );
	void slotClicked( const QModelIndex & );

protected:
	void slotCustomContextMenuRequested( const QPoint & );
};


#endif // RECORD_TREE_VIEW_H

