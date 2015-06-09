
#include <qpaintdevice.h>
#include <qpainter.h>

#include <math.h>
#include <typeinfo>

#include "blurqt.h"
#include "scheduleentry.h"
#include "schedulerow.h"
#include "schedulewidget.h"

bool ScheduleEntry::cmp( ScheduleEntry * other ) const
{
	const std::type_info & type = typeid(*this);
	const std::type_info & otype = typeid(*other);
	if( type == otype ) {
		int sk = sortKey();
		int osk = other->sortKey();
		if( sk == osk )
			return this > other;
		return sk > osk;
	}
	return &type > &otype;
}

void ScheduleEntry::setDateEnd( const QDate & de )
{
	QDate ode = dateEnd();
	QDate ds = dateStart();
	if( ode == de ) return;
	while( ode < de ) {
		ode = ode.addDays( 1 );
		addDate( ode );
	}
	while( ode > de ) {
		removeDate( ode );
		ode = ode.addDays( -1 );
	}
}

void ScheduleEntry::setDateStart( const QDate & ds )
{
	QDate ods = dateStart();
	if( ods == ds ) return;
	QDate de = dateEnd();
	while( ods > ds ) {
		ods = ods.addDays( -1 );
		addDate( ods );
	}
	while( ods < ds ) {
		removeDate( ods );
		ods = ods.addDays( 1 );
	}
}

bool ScheduleEntry::canMergeHelper( ScheduleEntry * entry, QDate * newStart, QDate * newEnd )
{
	QDate ods = entry->dateStart(), ds = dateStart();
	QDate rds, rde;
	bool doMerge = false;
	if( ods > ds && dateEnd().addDays(1) == ods ) {
		rds = ds;
		rde = entry->dateEnd();
		doMerge = true;
	} else if( ods < ds && entry->dateEnd().addDays(1) == ds ) {
		rds = ods;
		rde = dateEnd();
		doMerge = true;
	}
	if( newStart ) *newStart = rds;
	if( newEnd ) *newEnd = rde;
	return doMerge;
}

int ScheduleEntry::heightForWidth(int /*width*/, QPaintDevice * device, const ScheduleDisplayOptions &, int columnWidth ) const
{
	return 25 * device->logicalDpiX() / 72;
}

void ScheduleEntry::drawSelectionRect( QPainter * p, const QRect & r, bool drawLeft, bool drawRight )
{
	QRect rect(r);
	p->setBrush( QBrush(Qt::NoBrush) );
	for( int i=0; i<2; i++ ) {
		if( i == 0 )
			p->setPen( Qt::black );
		else {
			QPen pen( Qt::white );
			QVector<qreal> pat;
			int strip_width = 4 * p->device()->logicalDpiX() / 72;
			pat << strip_width << strip_width;
			pen.setDashPattern( pat );
			p->setPen( pen );
			rect.translate( 1, 1 );
		}
		if( drawLeft )  // po.selection.startDate() == po.cellDate ) {
			p->drawLine( rect.x(), rect.y(), rect.x(), rect.bottom() );
		if( drawRight ) // po.selection.endDate() == po.cellDate ) {
			p->drawLine( rect.right() + 1, rect.y(), rect.right() + 1, rect.bottom() );
		p->drawLine( rect.x(), rect.y(), rect.right(), rect.y() );
		p->drawLine( rect.x(), rect.bottom() + 1, rect.right() + 1, rect.bottom() + 1 );
	}
}

int ScheduleEntry::hoursBoxHeightForWidth(int width, QPaintDevice * device, const ScheduleDisplayOptions & displayOptions, int colWidth ) const
{
	QFontMetricsF entryFontMetrics(displayOptions.entryFont,device);
	QFontMetricsF hoursFontMetrics(displayOptions.entryHoursFont,device);
	return entryFontMetrics.height() - entryFontMetrics.descent()			// Main Text
	 +	hoursFontMetrics.height() - hoursFontMetrics.descent()	// Hours
	 +	(4.5 * device->logicalDpiY() / 72.0 ); 									// Hours bar
}

void ScheduleEntry::drawHoursBox( const PaintOptions & po, const QBrush & backgroundBrush, const QColor & borderColor, bool icon )
{
	QDate ed( po.cellDate );
	QPainter * p = po.painter;
	
	int dpix = p->device()->logicalDpiX();
	int dpiy = p->device()->logicalDpiY();
	bool highRes = dpix > 120;

	double hrs = hours( po.cellDate );

	if( mPaintId != po.paintId ) {
		p->setBrush( backgroundBrush );
		p->setPen( borderColor );
		p->drawRect( po.spanRect );
	}

	QRect hr( po.spanRect );
	if( po.cellRect.x() > hr.x() )
		hr.setLeft( po.cellRect.x() );
	if( po.cellRect.right() < hr.right() )
		hr.setRight( po.cellRect.right() );

	qreal innerOffset = dpix / 72.0;
	qreal handleXOffset = (highRes ? 1.0 : 2.0) * dpix / 72.0;
	qreal handleYOffset = 5.0 * dpix / 72.0;

	qreal lhs = innerOffset, rhs = innerOffset;

	// Handles
	p->setPen( borderColor.dark(125) );
	if( po.cellDate == po.startDate ) {
		lhs += handleXOffset;
		p->drawLine( QPointF( hr.x() + handleXOffset, hr.top() + handleYOffset ), QPointF(hr.x() + handleXOffset, hr.bottom() - handleYOffset) );
	}
	
	if( po.cellDate == po.endDate ) {
		rhs += handleXOffset;
		qreal right = hr.right() - handleXOffset + ceil(dpix/72.0);
		p->drawLine( QPointF(right, hr.top() + handleYOffset), QPointF(right, hr.bottom() - handleYOffset) );
	}

	{ // Time bar
		QColor bv( borderColor.value() < 125 ? borderColor.light(125) : borderColor.dark(115) );
		QColor bbv( bv.value() < 125 ? bv.light(115) : bv.dark(110) );
		p->setPen( bbv );
		p->setBrush( bv );
		qreal barTop = hr.bottom() - ((highRes ? 4.0 :3.0) * dpiy / 72.0);
		p->drawRect( QRectF(hr.x() + lhs, barTop, qMin(hrs,8.0) * double(hr.width()-lhs-rhs-1.0) / 8.0, 2.5 * dpix / 72.0) );
		if( hrs > 8.0 ) {
			p->setPen( Qt::red );
			qreal overTimeTop = barTop + dpiy / 72.0;
			p->drawLine( hr.x() + lhs, overTimeTop, (int)(hr.x() + lhs + (hrs - 8.0) * (hr.width()-lhs-rhs-1.0) / 8.0), overTimeTop );
		}
	}

	p->setFont( po.displayOptions->entryFont );

	// Draw the User - Asset label across the top
	// This is only drawn once per span
	if( mPaintId != po.paintId ) {
		QRect b;
		mPaintId = po.paintId;
		// Text
		p->setBrush( QBrush() );
		p->setPen( borderColor.value() > 125 ? Qt::black : Qt::white );
		qreal margin_x = 4.0 * dpix / 72.0;
		int margin_bottom = 8.0 * dpiy / 72.0;
		QStringList txt;
		txt = text(ed);
		p->drawText( po.spanRect.adjusted(margin_x,0,-margin_x,-margin_bottom), Qt::TextSingleLine, txt[0] + " - " + txt[1], &b );
	}
	p->setPen( Qt::black );

	if( icon ) {
		static QPixmap * commentPixmap = 0;
		if( !commentPixmap )
				commentPixmap = new QPixmap( "images/timesheetComment.png" );
		p->drawPixmap( hr.x() + lhs, hr.y() + 10, *commentPixmap );
	}

	// Draw the hours
	QFontMetricsF fm(po.displayOptions->entryFont,p->device());
	qreal leadHeight = fm.height() - fm.descent();// - QFontMetrics(po.displayOptions->entryHoursFont).leading();
	p->setFont( po.displayOptions->entryHoursFont );
	p->setRenderHint( QPainter::TextAntialiasing, true );
	p->drawText( hr.adjusted( 0, leadHeight - 1, 0, 0 ), Qt::AlignHCenter | Qt::AlignTop, QString::number(hrs) + (hrs==1?" Hr":" Hrs") );

	if( po.selection.isValid() )
		drawSelectionRect( p, hr, po.selection.startDate() == po.cellDate, po.selection.endDate() == po.cellDate );
}

