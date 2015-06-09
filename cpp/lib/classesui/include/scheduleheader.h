

#ifndef SCHEDULE_HEADER_H
#define SCHEDULE_HEADER_H

#include <qwidget.h>

#include "classesui.h"

class ScheduleWidget;

class CLASSESUI_EXPORT ScheduleHeader : public QWidget
{
Q_OBJECT
public:
	ScheduleHeader( ScheduleWidget * sched, QWidget * parent );
	ScheduleWidget * sched() const;

	int splitHeight() const;

	void draw( QPainter * p, const QRegion &, const QRect & rect, int offset, int width, int height, bool paintBackground = true );

	void setupHeights( int leadHeight, int yearMonthHeight, int weekDayHeight );
	void getHeights( int & leadHeight, int & yearMonthHeight, int & weekDayHeight );

	virtual QSize sizeHint() const;
	virtual QSize minimumSizeHint() const;

public slots:
	void highlightChanged( const QRect & );

protected:
	virtual void paintEvent( QPaintEvent * );
	virtual void mousePressEvent( QMouseEvent * me );
	virtual void mouseMoveEvent( QMouseEvent * me );
	virtual void mouseReleaseEvent( QMouseEvent * me );

	QRect mHighlightRect;
	int mHighlightStart;
	ScheduleWidget * mScheduleWidget;
	int mLeadHeight, mYearMonthHeight, mWeekDayHeight;
};

#endif // SCHEDULE_HEADER_H

