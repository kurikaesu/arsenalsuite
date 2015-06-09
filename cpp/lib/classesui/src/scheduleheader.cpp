
#include <qpainter.h>
#include <qevent.h>

#include "scheduleheader.h"
#include "schedulewidget.h"

ScheduleHeader::ScheduleHeader( ScheduleWidget * sw, QWidget * parent )
: QWidget( parent )
, mHighlightStart( -1 )
, mScheduleWidget( sw )
, mLeadHeight( 0 )
, mYearMonthHeight( 35 )
, mWeekDayHeight( 30 )
{
	connect( sw, SIGNAL( highlightChanged( const QRect & ) ), SLOT( highlightChanged( const QRect & ) ) );
}

ScheduleWidget * ScheduleHeader::sched() const
{
	return mScheduleWidget;
}

QString dayName( int day, int width, const QFontMetrics & fm )
{
	QString dn = QDate::longDayName(day);
	if( fm.size(Qt::TextSingleLine,dn).width() <= (width * .9 - 18) )
		return dn;
	dn = QDate::shortDayName( day );
	if( fm.size(Qt::TextSingleLine,dn).width() <= (width * .9 - 18) )
		return dn;
	if( day == 4 )
		return "R";
	return QDate::shortDayName( day ).left( 1 );
}

void ScheduleHeader::setupHeights( int leadHeight, int yearMonthHeight, int weekDayHeight )
{
	mLeadHeight = leadHeight;
	mYearMonthHeight = yearMonthHeight;
	mWeekDayHeight = weekDayHeight;
	resize( width(), sizeHint().height() );
}

void ScheduleHeader::getHeights( int & leadHeight, int & yearMonthHeight, int & weekDayHeight )
{
	leadHeight = mLeadHeight;
	yearMonthHeight = mYearMonthHeight;
	weekDayHeight = mWeekDayHeight;
}

void ScheduleHeader::draw( QPainter * p, const QRegion & region, const QRect & rect, int offset, int width, int height, bool paintBackground )
{
	ScheduleWidget * sw = sched();

	int ew = sw->contentsWidth();

	int dm = sw->displayMode();
	QDate date = sw->date();
	int columns = sw->columnCount();

	QFont regFont = sw->mDisplayOptions.headerMonthYearFont;
	QFont dayFont = sw->mDisplayOptions.headerDayWeekFont;

	// Paint the month header
	p->setFont( regFont );

	if( paintBackground )
		p->fillRect( rect.x(), 1, rect.width(), mLeadHeight + mYearMonthHeight, sw->mHeaderBackground );

	p->setPen( Qt::black );

	if( rect.y() <= 0 )
		p->drawLine( rect.x(), 0, rect.right(), 0 );

	QFontMetrics dayFontMetrics( dayFont, p->device() );
	int monthLeft = 0;
	QDate lastDate = date;
	for( int i=0; i<columns; i++ ) {
		int x = sw->colPos( i );
		int w = sw->colWidth( i );

		while( !sw->showWeekends() && date.dayOfWeek() >= 6 )
			date = date.addDays( 1 );

		bool isLastColumn = (i+1==columns);
		bool isNewMonth = lastDate.month() != date.month();
		if( dm != ScheduleWidget::Month && (isNewMonth || isLastColumn) ) {
			for( int i=0; i<2; i++ ) {
				bool paintLeft = i==0;
				if( i == 0 && !isNewMonth ) continue;
				if( i == 1 && !isLastColumn ) continue;
				int left = qMax(monthLeft,offset);
				int right = qMin(offset+width,x + (paintLeft ? 0 : w));
				int top = mLeadHeight;
				int h = mYearMonthHeight;
				if( right >= offset && left < offset + width ) {
					p->setFont( regFont );
					p->setPen( Qt::black );
					p->drawText( left-offset, top, right - left, h, Qt::AlignCenter, QDate::longMonthName( lastDate.month() ) + ", " + QString::number( lastDate.year() ) );
					p->drawLine( right - offset - 1, top, right - offset - 1, h );
				}
				monthLeft = x;
			}
		}

		if( i + 1 == columns && dm == ScheduleWidget::Month ) {
			int left = qMax(0,offset);
			int right = qMin(offset+width,x+w);
			p->setFont( regFont );
			p->setPen( Qt::black );
			p->drawText( left-offset, mLeadHeight, right - left, mYearMonthHeight, Qt::AlignCenter, QDate::longMonthName( date.month() ) + ", " + QString::number( date.year() ) );
		}
		lastDate = date;

		x -= offset;
		
		if( !region.contains( QRect( x, 0, w, height ) ) ){
			date = date.addDays(1);
			continue;
		}

		int ypos = mLeadHeight + mYearMonthHeight;
		
		QColor col = ( i >= mHighlightRect.x() && i <= mHighlightRect.right() ) ? sw->mHeaderBackgroundHighlight : sw->mHeaderBackground;
		int dow = date.dayOfWeek();
		if( dm != ScheduleWidget::Year && dow > 5)
			col = col.dark( 120 );
		
		if( paintBackground )
			p->fillRect( x, ypos, w, height - ypos, col );

		p->setFont( dayFont );
		p->setPen( Qt::black );
		if( dm == ScheduleWidget::Year ) {
			p->drawText( x, ypos, w, height - ypos, Qt::AlignCenter, QString::number( i + 1 ) );
		} else if( dm == ScheduleWidget::MonthFlat ) {
			int h = (height - ypos) / 2;
			p->drawText( x, ypos, w, h, Qt::AlignCenter, QString::number( date.day() ) );
			// TODO: Scale to width
			p->drawText( x, ypos + h, w, h, Qt::AlignCenter, dayName( date.dayOfWeek(), w, dayFontMetrics ) );
		} else {
			QString title, num;
			title = QDate::longDayName( dow );
			if( dayFontMetrics.boundingRect(title).width() >= w - 2 )
				title = QDate::shortDayName( dow );
			if( dayFontMetrics.boundingRect(title).width() >= w - 2 )
				title = title[0];
			p->drawText( x, ypos + (height-ypos)/2, w, (height-ypos)/2, Qt::AlignCenter, title );
			if( dm == ScheduleWidget::Week ) {
				p->setFont( regFont );
				p->drawText( x, ypos, w, int((height-ypos)*.6), Qt::AlignCenter, QString::number( date.day() ) );
			}
		}

		p->setPen( Qt::gray );
		p->drawLine( x, ypos, x+w, ypos );

		int borderDay = sw->showWeekends() ? 7 : 5;
		if( dow == borderDay || (dm == ScheduleWidget::Year) )
			p->setPen( Qt::black );

		p->drawLine( x+w-1, ypos, x+w-1, height );

		date = date.addDays(1);
	}
	p->setPen( Qt::gray );
	p->drawLine( 0, height-1, ew-1, height-1 );
	p->setPen( Qt::black );
	p->drawLine( ew-1, 0, ew-1, height-1 );
}

void ScheduleHeader::paintEvent( QPaintEvent * pe )
{
	QPainter p( this );
	draw( &p, pe->region(), pe->rect(), sched()->xOffset(), width(), height() );
}
// 
void ScheduleHeader::highlightChanged( const QRect & rect )
{
	if( rect.x() == mHighlightRect.x() && rect.right() == mHighlightRect.right() )
		return;
	QRect damage = rect | mHighlightRect;
	mHighlightRect = rect;
	int x = sched()->colPos( damage.x() );
	int w = sched()->colPos(damage.right()+1) + sched()->colWidth(damage.right()+1);
	update( x-sched()->xOffset(), 0, w, height() );
}

void ScheduleHeader::mousePressEvent( QMouseEvent * me )
{
	ScheduleWidget * sw = sched();
	int cols=sw->columnCount();
	mHighlightStart = cols * me->x() / sw->contentsWidth();
//	sched()->setHighlight( QRect( mHighlightStart, 0, 1, sched()->rowCount() ) );
}

void ScheduleHeader::mouseMoveEvent( QMouseEvent * me )
{
	if( mHighlightStart==-1 )
		return;
	int he = sched()->columnCount() * me->x() / sched()->contentsWidth();
//	sched()->setHighlight( QRect( qMin(he, mHighlightStart), 0, abs(mHighlightStart-he) + 1, sched()->rowCount() ) );
}

void ScheduleHeader::mouseReleaseEvent( QMouseEvent * )
{
	mHighlightStart = -1;
//	sched()->setHighlight( QRect() );
}

int ScheduleHeader::splitHeight() const
{
	return 35;
}

QSize ScheduleHeader::sizeHint() const
{
	return QSize( 20, mLeadHeight + mYearMonthHeight + mWeekDayHeight );
}

QSize ScheduleHeader::minimumSizeHint() const
{
	return sizeHint();
}
