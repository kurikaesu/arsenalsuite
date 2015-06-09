
#ifndef SCHEDULE_PANEL_H
#define SCHEDULE_PANEL_H

#include <qwidget.h>

#include "classesui.h"

class ScheduleWidget;
class ExtTreeView;
class QModelIndex;
class SuperModel;
class RowItemTreeBuilder;

class CLASSESUI_EXPORT SchedulePanel : public QWidget
{
Q_OBJECT
public:
	SchedulePanel( ScheduleWidget * sched, QWidget * parent );
	~SchedulePanel();

	ScheduleWidget * sched() const;

//	int calculateWidth() const;

	// Returns the width
	int print( QPainter * p, int offset, int height );

public slots:
	void highlightChanged( const QRect & );

	void layoutChanged();

	void rowsChanged();

	void scheduleItemExpanded( const QModelIndex & );

	ExtTreeView * tree() const { return mTree; }

	void rowOrderChanged();

	void showMenu( const QPoint & pos, const QModelIndex & underMouse );

protected:

	QRect getButtonRect( int rowHeight );
	
	QRect mHighlightRect;
	int mHighlightStart;
	ExtTreeView * mTree;
	SuperModel * mModel;
	RowItemTreeBuilder * mTreeBuilder;
	ScheduleWidget * mScheduleWidget;
};

#endif // SCHEDULE_PANEL_H
