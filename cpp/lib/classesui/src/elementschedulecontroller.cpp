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

#include <typeinfo>

#include <qdatetime.h>
#include <qmenu.h>
#include <qpainter.h>

#include "blurqt.h"
#include "database.h"

#include "calendar.h"
#include "elementstatus.h"
#include "employee.h"
#include "elementuser.h"
#include "projectstatus.h"
#include "shot.h"
#include "thumbnail.h"
#include "timesheet.h"
#include "user.h"

#include "addnotedialog.h"
#include "elementui.h"
#include "elementschedulecontroller.h"
#include "eventdialog.h"
#include "resinerror.h"
#include "scheduledialog.h"
#include "schedulewidget.h"
#include "timeentrydialog.h"
#include "usertaskdialog.h"

ScheduleAction::ScheduleAction( const User & user, const Element & el, const QDate & start, const QDate & end, QObject * parent, QWidget * dialogParent )
: QAction( parent )
, mUser( user )
, mElement( el )
, mStart( start )
, mEnd( end )
, mDialogParent( dialogParent )
{
	init();
}


void ScheduleAction::init()
{
	setText( mUser.isRecord() && mElement.isRecord() ? mUser.displayName() : "Schedule User" );
	connect( this, SIGNAL( triggered() ), SLOT( slotTriggered() ) );
}

void ScheduleAction::slotTriggered()
{
	ScheduleDialog * sd = new ScheduleDialog( mDialogParent ? mDialogParent : qobject_cast<QWidget*>(parent()) );
	sd->setElement( mElement );
	sd->setEmployee( mUser );
	sd->setDateRange( mStart, mEnd );
	sd->exec();
	delete sd;
}

TimeSheetAction::TimeSheetAction( const Element & el, const QDate & start, QObject * parent, const QDate & end )
: QAction( parent )
, mElement( el )
, mStartDate( start )
, mEndDate( end )
{
	init();
}

void TimeSheetAction::init()
{
	setText( mElement.displayPath() );
	connect( this, SIGNAL( triggered() ), SLOT( slotTriggered() ) );
}

void TimeSheetAction::slotTriggered()
{
	TimeEntryDialog::recordTimeSheet( ::qobject_cast<QWidget*>(parent()), mElement.isRecord() ? ElementList(mElement) : ElementList(), mStartDate, AssetType(), mEndDate );
}

EventAction::EventAction( const QDate & date, QObject * parent )
: QAction( parent )
, mDate( date )
, mAction(Create)
{
	init();
}

EventAction::EventAction( const Calendar & event, QObject * parent, int action )
: QAction( parent )
, mEvent(event)
, mAction(action)
{
	init();
}

void EventAction::init()
{
	connect( this, SIGNAL( triggered() ), SLOT( slotTriggered() ) );
	switch( mAction ) {
		case Create:
			setText( "Create Event" );
			break;
		case Edit:
			setText( "Edit Event" );
			break;
		case Delete:
			setText( "Delete Event" );
			break;
	}
}

void EventAction::slotTriggered()
{
	switch( mAction ) {
		case Create:
		{
			EventDialog * eventDialog = new EventDialog( ::qobject_cast<QWidget*>(parent()) );
			eventDialog->mWhenDateTimeEdit->setDateTime( QDateTime(mDate) );
			eventDialog->exec();
			delete eventDialog;
			break;
		}
		case Edit:
		{
			EventDialog * eventDialog = new EventDialog( ::qobject_cast<QWidget*>(parent()) );
			eventDialog->setEvent( mEvent );
			eventDialog->exec();
			delete eventDialog;
			break;
		}
		case Delete:
		{
			ResinError::deleteConfirmation( ::qobject_cast<QWidget*>(parent()) );
			break;
		}
	}
}

TimeSheetEntry::TimeSheetEntry( ScheduleRow * sr, TimeSheetList tsl, bool showUser )
: ScheduleEntry( sr )
, mTimeSheets( tsl )
, mLastWidth(0)
, mLastHeight(0)
, mShowUser( showUser )
, mCommentHeight( 0 )
{
	foreach( TimeSheet ts, tsl ) {
		if( !mStart.isRecord() || mStart.dateTime().date() > ts.dateTime().date() )
			mStart = ts;
		if( !mEnd.isRecord() || mEnd.dateTime().date() < ts.dateTime().date() )
			mEnd = ts;
	}
}

QDate TimeSheetEntry::dateStart() const
{
	return mStart.dateTime().date();
}

QDate TimeSheetEntry::dateEnd() const
{
	return mEnd.dateTime().date();
}

void TimeSheetEntry::setDateStart( const QDate & ds )
{
	ScheduleEntry::setDateStart( ds );
	mStart = byDate( ds );
}

void TimeSheetEntry::setDateEnd( const QDate & de )
{
	ScheduleEntry::setDateEnd( de );
	mEnd = byDate( de );
}

void TimeSheetEntry::addDate( const QDate & date )
{
	bool found = false;
	TimeSheet new_ts;
	foreach( TimeSheet tsi, mRemoved )
		if( tsi.dateTime().date() == date ) {
			found = true;
			new_ts = tsi;
			break;
		}
	if( !found ) {
		int closest = 0;
		foreach( TimeSheet ts, mTimeSheets ) {
			int dist = abs( ts.dateTime().date().daysTo( date ) );
			if( !closest || closest > dist ) {
				new_ts = ts.copy();
				closest = dist;
			}
		}
		new_ts.setDateTime( QDateTime( date ) );
	}
	if( new_ts.isRecord() )
		mTimeSheets += new_ts;
	else
		mAdded += new_ts;
}

void TimeSheetEntry::removeDate( const QDate & date )
{
	TimeSheet ts = byDate( date );
	if( ts.isRecord() ) {
		mTimeSheets -= ts;
		mRemoved += ts;
	} else
		mAdded -= ts;
}

bool TimeSheetEntry::merge( ScheduleEntry * entry )
{
	TimeSheetEntry * tse = dynamic_cast<TimeSheetEntry*>(entry);
	if( !tse ) return false;
	if( mTimeSheets.isEmpty() ) return false;
	TimeSheet ts(mTimeSheets[0]),tso(tse->mTimeSheets[0]);
	if( tso.element() != ts.element() ) return false;
	if( tso.user() != ts.user() ) return false;
	if( tso.assetType() != ts.assetType() ) return false;
	QDate rds, rde;
	if( !canMergeHelper( entry, &rds, &rde ) ) return false;
	mAdded += tse->mAdded;
	mRemoved += tse->mRemoved;
	mTimeSheets += tse->mTimeSheets;
	mStart = byDate( rds );
	mEnd = byDate( rde );
	return true;
}

void TimeSheetEntry::applyChanges( bool commit )
{
	if( commit ) {
		Database::current()->beginTransaction( "Record Time Sheet(s)" );
		mAdded.commit();
		Database::current()->commitTransaction();
		mTimeSheets += mAdded;
		mAdded.clear();
		Database::current()->beginTransaction( "Delete Time Sheet(s)" );
		mRemoved.remove();
		Database::current()->commitTransaction();
		mRemoved.clear();
	} else {
		mAdded.clear();
		mTimeSheets += mRemoved;
		mRemoved.clear();
	}
}

int TimeSheetEntry::hours( const QDate & date ) const
{
	return (int)byDate( date ).scheduledHour();
}

QColor TimeSheetEntry::color() const
{
	AssetType at =  byDate( dateStart() ).assetType();
	QColor c;
	c.setNamedColor( at.color() );
	return c.isValid() ? c : QColor( 166, 198, 184 );
}

QStringList TimeSheetEntry::text(const QDate & date) const
{
	QStringList ret;
	TimeSheet cur = byDate( date );
	User u = cur.user();
	ret += u.name();
	if( cur.element().isRecord() )
		ret += cur.element().displayName(true) + " - " + cur.assetType().name();
	else
		ret += cur.project().displayName() + " - " + cur.assetType().name();
	float hrs = cur.scheduledHour();
	ret += QString::number( hrs ) + " Hour" + ((hrs>1) ? QString("s") : QString::null);
	return ret;
}

QString TimeSheetEntry::toolTip( const QDate & date, int displayMode ) const
{
	QString ret;
	if( displayMode == ScheduleWidget::Year ) {
		QDate d = dateStart(), end = dateEnd();
		TimeSheet first = byDate( d );
		if( first.element().isRecord() )
			ret = first.element().prettyPath();
		else
			ret = first.project().name() + first.assetType().name();
		ret += "\n" + first.user().displayName();
		while( d <= end ) {
			ret += "\n" + d.toString() + " - " + QString::number( hours( d ) );
			d = d.addDays( 1 );
		}
	} else {
		ret = text(date).join("\n");
		QString c = byDate(date).comment();
		if( !c.isEmpty() )
			ret += "\n" + c;
	}
	return ret;
}

TimeSheet TimeSheetEntry::byDate( const QDate & date ) const
{
	foreach( TimeSheet ts, (mTimeSheets + mAdded) )
		if( ts.dateTime().date() == date )
			return ts;
	return TimeSheet();
}

void TimeSheetEntry::popup( QWidget * parent, const QPoint & pos, const QDate & date, ScheduleSelection sel )
{
	TimeSheet cur = byDate( date );
	if( !(User::currentUser() == cur.user() && User::hasPerms( "TimeSheet", true)) && !User::hasPerms( "OtherPeoplesTimeSheets", true) ) return;

	QMenu * p = new QMenu( parent );
	QAction * edit = p->addAction( "Edit TimeSheet" );
	QMenu * hours = p->addMenu( "Change Hours" );
	QAction * del = p->addAction( "Delete TimeSheet Entry" );
	QAction * debug = p->addAction( "Debug Dump" );
	for( int i=1; i<=24; i++ ) {
		QAction * tmp = hours->addAction( QString::number( i ) );
		tmp->setCheckable( true );
		if( i == cur.scheduledHour() )
			tmp->setChecked( true );
	}
	QAction * res = p->exec( pos );
	if( !res ) {
		delete p;
		return;
	}
	if( res == del ) {
		Database::current()->beginTransaction( "Delete Time Sheet(s)" );
		cur.remove();
		Database::current()->commitTransaction();
	} else if( res->parentWidget() == hours ) {
		bool val;
		int sh = res->text().toInt( &val );
		if( val ) {
			Database::current()->beginTransaction( "Change Time Sheet Hours" );
			cur.setScheduledHour( sh );
			cur.commit();
			Database::current()->commitTransaction();
		}
	} else if( res == edit ) {
		TimeEntryDialog * ted = new TimeEntryDialog( parent );
		ted->setTimeSheet( cur );
		ted->exec();
		delete ted;
	} else if( res == debug ) {
		foreach( TimeSheet ts, (mTimeSheets + mAdded) )
			LOG_5( "TimeSheet: dateStart: " + dateStart().toString() + " dateEnd: " + dateEnd().toString() );
	}
	delete p;
}

int TimeSheetEntry::sortKey() const
{
	return byDate(dateStart()).getValue("fkeytimesheetcategory").toInt();
}

bool TimeSheetEntry::cmp( ScheduleEntry * other ) const
{
	if( typeid(TimeSheetEntry) != typeid(*other) ) return this->ScheduleEntry::cmp(other);
	int sk = sortKey();
	int osk = other->sortKey();
	if( sk != osk ) return sk > osk;
	return byDate(dateStart()).key() > ((TimeSheetEntry*)other)->byDate(other->dateStart()).key();
}

static bool EnableInlineComments = false;

int TimeSheetEntry::heightForWidth( int width, QPaintDevice * device, const ScheduleDisplayOptions & displayOptions, int colWidth ) const
{
	return hoursBoxHeightForWidth(width,device,displayOptions,colWidth);
}

void TimeSheetEntry::paint( const PaintOptions & po )
{
	QColor bg = color();
	drawHoursBox(po, bg, bg.dark( 115 ), !byDate(po.cellDate).comment().isEmpty() );
}

int TimeSheetEntry::allowResize() const
{
	if( (User::hasPerms( "TimeSheet", true ) && byDate(dateStart()).user() == User::currentUser()) || User::hasPerms( "OtherPeoplesTimeSheets", true ) )
		return ResizeLeft | ResizeRight;
	return 0;
}

UserScheduleEntry::UserScheduleEntry( ScheduleRow * sr, ScheduleList sl, bool showUser )
: ScheduleEntry( sr )
, mSchedules( sl )
, mLastWidth(0)
, mLastHeight(0)
, mShowUser( showUser )
{
	foreach( Schedule s, sl ) {
		if( !mStart.isRecord() || mStart.date() > s.date() )
			mStart = s;
		if( !mEnd.isRecord() || mEnd.date() < s.date() )
			mEnd = s;
	}
}

QDate UserScheduleEntry::dateStart() const
{
	return mStart.date();
}

QDate UserScheduleEntry::dateEnd() const
{
	return mEnd.date();
}

void UserScheduleEntry::setDateStart( const QDate & ds )
{
	ScheduleEntry::setDateStart( ds );
	mStart = byDate( ds );
}

void UserScheduleEntry::setDateEnd( const QDate & de )
{
	ScheduleEntry::setDateEnd( de );
	mEnd = byDate( de );
}

void UserScheduleEntry::addDate( const QDate & date )
{
	bool found = false;
	Schedule s;
	foreach( Schedule si, mRemoved )
		if( si.date() == date ) {
			found = true;
			s = si;
			break;
		}
	if( !found ) {
		int closest = 0;
		foreach( Schedule si, mSchedules ) {
			int dist = abs( si.date().daysTo( date ) );
			if( !closest || closest > dist ) {
				s = si.copy();
				closest = dist;
			}
		}
		s.setDate( date );
	}
	if( s.isRecord() )
		mSchedules += s;
	else
		mAdded += s;
}

void UserScheduleEntry::removeDate( const QDate & date )
{
	Schedule s = byDate( date );
	if( s.isRecord() ) {
		mSchedules -= s;
		mRemoved += s;
	} else
		mAdded -= s;
}

bool UserScheduleEntry::merge( ScheduleEntry * entry )
{
	UserScheduleEntry * tse = dynamic_cast<UserScheduleEntry*>(entry);
	if( !tse ) return false;
	if( tse->mSchedules[0].element() != mSchedules[0].element() ) return false;
	if( tse->mSchedules[0].user() != mSchedules[0].user() ) return false;
	QDate rds, rde;
	if( !ScheduleEntry::canMergeHelper( entry, &rds, &rde ) ) return false;
	mAdded += tse->mAdded;
	mRemoved += tse->mRemoved;
	mSchedules += tse->mSchedules;
	mStart = byDate( rds );
	mEnd = byDate( rde );
	return true;
}

void UserScheduleEntry::applyChanges( bool commit )
{
	// This causes the row to get deref'd, so well keep a ref until the function is finished
	ref();
	if( commit ) {
		Database::current()->beginTransaction( "Create Schedule(s)" );
		mAdded.commit();
		Database::current()->commitTransaction();
		mSchedules += mAdded;
		mAdded.clear();
		Database::current()->beginTransaction( "Delete Schedule(s)" );
		mRemoved.remove();
		Database::current()->commitTransaction();
		mRemoved.clear();
	} else {
		mAdded.clear();
		mSchedules += mRemoved;
		mRemoved.clear();
	}
	deref();
}

int UserScheduleEntry::hours( const QDate & date ) const
{
	return byDate( date ).duration().hours();
}

QColor UserScheduleEntry::color() const
{
	AssetType at =  byDate( dateStart() ).element().assetType();
	QColor c;
	c.setNamedColor( at.color() );
	return c.isValid() ? c.light(115) : QColor( 200, 200, 100 );
}

QStringList UserScheduleEntry::text( const QDate & date ) const
{
	QStringList ret;
	Schedule cur = byDate( date );
	ret += cur.user().name();
	ret += cur.element().displayName(); 
	return ret;
}

QString UserScheduleEntry::toolTip( const QDate & date, int displayMode ) const
{
	QString ret;
	if( displayMode == ScheduleWidget::Year ) {
		QDate d = dateStart(), end = dateEnd();
		Schedule first = byDate( d );
		ret = first.element().prettyPath();
		ret += "\n" + first.user().displayName();
		while( d <= end ) {
			ret += "\n" + d.toString() + " - " + QString::number( hours( d ) );
			d = d.addDays( 1 );
		}
	} else {
		ret = text(date).join("\n");
		ret += "\n" + date.toString() + " - " + QString::number( byDate(date).duration().hours() ) + " hours";
		ret += "\n" + dateStart().toString() + " to " + dateEnd().toString();
	}
	return ret;
}

Schedule UserScheduleEntry::byDate( const QDate & date ) const
{
	ScheduleList toLook( mSchedules + mAdded );
	foreach( Schedule s, mSchedules )
		if( s.date() == date )
			return s;
	foreach( Schedule s, mAdded )
		if( s.date() == date )
			return s;
	return Schedule();
}

void UserScheduleEntry::popup( QWidget * parent, const QPoint & pos, const QDate & date, ScheduleSelection sel )
{
	bool allSchedules = true;
	bool isUser = true;
	ScheduleList schedules;
	// Check the selection to make sure they are all UserScheduleEntries
	// and see if the user is assigned to all of them.
	foreach( ScheduleEntrySelection es, sel.mEntrySelections ) {
		if( typeid(UserScheduleEntry) == typeid(*es.mEntry) ) {
			UserScheduleEntry * e = dynamic_cast<UserScheduleEntry*>(es.mEntry);
			QDate date = es.startDate();
			do {
				Schedule s = e->byDate(date);
				isUser &= s.user() == User::currentUser();
				schedules += s;
				date = date.addDays(1);
			} while( date <= es.endDate() );
		} else {
			allSchedules = false;
			break;
		}
	}

	if( !allSchedules ) return;

	int count = schedules.size();
	bool hasSchedulePerms = User::hasPerms( "Schedule", true );
	Schedule cur = byDate( date );
	QMenu * p = new QMenu( parent ), * hours = 0, * tshours = 0;
	QAction * mod = 0, * del = 0;

	if( isUser && count == 1 ) {
		tshours = p->addMenu( "Record TimeSheet..."  );
		if( hasSchedulePerms )
			p->addSeparator();
	}

	if( hasSchedulePerms ) {
		hours = p->addMenu( "Change Scheduled Hours(s)" );
		if( count == 1 )
			mod = p->addAction( "Modify Schedule Entry..." );
		del = p->addAction( "Delete Schedule Entry" );
	}

	for(int i=1; i<=12;i++) {
		if( hasSchedulePerms && hours ) {
			QAction * tmp = hours->addAction( QString::number(i) );
			tmp->setCheckable( true );
			if( i == (int)cur.duration().hours() )
				tmp->setChecked( true );
		}
		if( isUser && tshours )
			tshours->addAction( QString::number(i) );
	}

	QAction * res = p->exec( pos );
	if( !res ) {
		delete p;
		return;
	}
	if( res == mod ) {
		ScheduleDialog * sd = new ScheduleDialog( parent );
		sd->setSchedule( cur );
		sd->exec();
		delete sd;
	} else if( res == del ) {
		Database::current()->beginTransaction( "Delete Schedule(s)" );
		schedules.remove();
		Database::current()->commitTransaction();
	} else if( res->parentWidget() == hours ) {
		bool val;
		int hrs = res->text().toInt( &val );
		if( val ) {
			Database::current()->beginTransaction( "Change Schedule Hours" );
			schedules.setDurations( Interval().addHours(hrs) );
			schedules.commit();
			Database::current()->commitTransaction();
		}
	} else if( res->parentWidget() == tshours ) {
		Database::current()->beginTransaction( "Record Time Sheet" );
		int hours = res->text().toInt();
		TimeSheet ts;
		ts.setElement( cur.element() );
		ts.setAssetType( cur.element().assetType() );
		ts.setScheduledHour( hours );
		ts.setUser( cur.user() );
		ts.setProject( cur.element().project() );
		ts.setDateTime( QDateTime( cur.date() ) );
		ts.setDateTimeSubmitted( QDateTime::currentDateTime() );
		ts.commit();
		Database::current()->commitTransaction();
	}
	delete p;
}

int UserScheduleEntry::sortKey() const
{
	return byDate(dateStart()).element().getValue("assettype").toInt();
}

bool UserScheduleEntry::cmp( ScheduleEntry * other ) const
{
	if( typeid(UserScheduleEntry) != typeid(*other) ) return this->ScheduleEntry::cmp(other);
	int sk = sortKey();
	int osk = other->sortKey();
	if( sk != osk ) return sk > osk;
	return byDate(dateStart()).key() > ((TimeSheetEntry*)other)->byDate(other->dateStart()).key();
}

int UserScheduleEntry::heightForWidth( int width, QPaintDevice * device, const ScheduleDisplayOptions & displayOptions, int colWidth ) const
{
	return hoursBoxHeightForWidth(width,device,displayOptions,colWidth);
}

void UserScheduleEntry::paint( const PaintOptions & po )
{
	QColor bg = color();
	if( mPaintId != po.paintId ) {
		po.painter->setBrush( bg );
		po.painter->setPen( bg.dark( 115 ) );
		po.painter->drawRect( po.spanRect );
	}
	drawHoursBox(po, QBrush(bg.light(112),Qt::FDiagPattern), bg.dark( 115 ));
}

int UserScheduleEntry::allowResize() const
{
	if( User::hasPerms( "Schedule", true ) )
		return ResizeLeft | ResizeRight;
	return 0;
}

bool UserScheduleEntry::allowDrag( ScheduleEntrySelection selection ) const
{
	return User::hasPerms( "Schedule", true );
}

bool UserScheduleEntry::canDrop( ScheduleEntrySelection selection, const QDate & startDragDate, const QDate & dropDate, ScheduleRow * sr )
{
	return sr->inherits( "ElementScheduleRow" );
}

void UserScheduleEntry::drop( ScheduleEntrySelection selection, const QDate & startDragDate, const QDate & dropDate, ScheduleRow * sr )
{
	int offset = startDragDate.daysTo(dropDate);
	QDate start = selection.startDate();
	QDate end = selection.endDate();
	ScheduleList toCommit;
	ElementScheduleRow * esr = 0;
	if( sr != row() && sr->inherits( "ElementScheduleRow" ) )
		esr = qobject_cast<ElementScheduleRow*>(sr);
	for(;start <= end; start = start.addDays(1)) {
		Schedule s = byDate(start);
		QDate d = s.date();
		d = d.addDays(offset);
		s.setDate(d);
		if( esr ) {
			Element e = esr->element();
			if( User(e).isRecord() ) {
				LOG_5( "Changing Scheduled User" );
				s.setUser(e);
			} else {
				LOG_5( "Changing Scheduled Asset" );
				s.setElement(e);
			}
		}
		toCommit += s;
	}
	toCommit.commit();
}

CalendarEntry::CalendarEntry( ScheduleRow * row, const Calendar & cal )
: ScheduleEntry( row )
, mLastWidth(0)
, mLastHeight(0)
, mCal( cal )
{}

QDate CalendarEntry::dateStart() const
{
	return mCal.dateTime().date();
}

QDate CalendarEntry::dateEnd() const
{
	return mCal.dateTime().date();
}

void CalendarEntry::setDateStart( const QDate & date )
{
	mCal.setDateTime( QDateTime( date, mCal.dateTime().time() ) );
}

void CalendarEntry::setDateEnd( const QDate & date )
{
	mCal.setDateTime( QDateTime( date, mCal.dateTime().time() ) );
}

void CalendarEntry::applyChanges( bool )
{
	mCal.commit();
}

QColor CalendarEntry::color() const
{
	return QColor(205, 205, 208);
}

QString CalendarEntry::toolTip( const QDate &, int ) const
{
	return "Posted By: " + mCal.user().displayName() + "\n" + mCal.name();
}

void CalendarEntry::popup( QWidget * parent, const QPoint & pos, const QDate &, ScheduleSelection sel  )
{
	QMenu * p = new QMenu( parent );
	
	QAction * edit = p->addAction( "Edit Event..." );
	QAction * del = p->addAction( "Delete Event" );
	if( mCal.user() != User::currentUser() && !User::hasPerms( "Calendar", true ) ) {
		edit->setEnabled( false );
		del->setEnabled( false );
	}

	QAction * res = p->exec( pos );
	if( !res ) {
		delete p;
		return;
	}
	if( res == del ) {
		Database::current()->beginTransaction( "Delete Schedule(s)" );
		mCal.remove();
		Database::current()->commitTransaction();
	} else if( res == edit ) {
		EventDialog * ev = new EventDialog( parent );
		Database::current()->beginTransaction( "Change Schedule Hours" );
		ev->setEvent(mCal);
		if( ev->exec() == QDialog::Accepted )
			Database::current()->commitTransaction();
		else
			Database::current()->rollbackTransaction();
		delete ev;
	}
	delete p;
}

bool CalendarEntry::merge( ScheduleEntry *  )
{
	return false;
}

int CalendarEntry::sortKey() const
{
	return mCal.category().key();
}

bool CalendarEntry::cmp( ScheduleEntry * other ) const
{
	if( typeid(CalendarEntry) != typeid(*other) ) return this->ScheduleEntry::cmp(other);
	int sk = sortKey();
	int osk = other->sortKey();
	if( sk != osk ) return sk > osk;
	return mCal.key() > ((CalendarEntry*)other)->mCal.key();
}

int CalendarEntry::heightForWidth( int width, QPaintDevice * device, const ScheduleDisplayOptions & displayOptions, int /*colWidth*/ ) const
{
//	if( mLastWidth != width ) {
		mLastWidth = width;
		QFont font( displayOptions.entryFont, device );
		return QFontMetrics(font,device).boundingRect( 0, 0, width - 12, 100000, Qt::TextWordWrap, mCal.name() ).height();
//	}
	return mLastHeight;
}

void CalendarEntry::paint( const PaintOptions & po )
{

	QColor sc = color();
	QPainter * p = po.painter;
	QDate ed( po.cellDate );
	
	//p->setRenderHint( QPainter::Antialiasing, true );
	QColor bg(sc);

	p->setBrush( sc );
	p->setPen( sc.dark( 115 ) );

	//p->setBrush( bg.light( 115 ) );
	//p->setPen( QPen(bg, 1, Qt::DashLine) );
	p->drawRoundRect( po.spanRect , 10,  10 );

	// we differentiate the events from the timesheets adding a narrow box at the right of the event cell
	QRect nr( po.spanRect );
	nr.setTop( po.spanRect.top() + 1 );
	nr.setBottom( po.spanRect.bottom() );
	nr.setLeft( po.cellRect.right() - 9 );
	nr.setRight( po.cellRect.right() - 5 );

	p->setPen( Qt::NoPen );
	p->setBrush( sc.dark( 108 )  );
	p->drawRect( nr );

	QRect hr( po.spanRect );
	hr.setLeft( po.cellRect.x() );
	hr.setRight( po.cellRect.right() );

	// Text
	p->setBrush( QBrush() );
	p->setPen( sc.value() > 125 ? Qt::black : Qt::white );
	p->setFont( po.displayOptions->entryFont );
	p->drawText( po.spanRect.adjusted(4,2,-6,0), Qt::TextWordWrap, mCal.name() );
}

TimeSpanEntry::TimeSpanEntry( ScheduleRow * row, const Element & element )
: ScheduleEntry( row )
, mElement( element )
{
	if( mElement.dateComplete() < mElement.startDate() ) mElement.setDateComplete( mElement.startDate() );
}

QDate TimeSpanEntry::dateStart() const
{
	return mElement.startDate();
}

QDate TimeSpanEntry::dateEnd() const
{
	return mElement.dateComplete();
}

void TimeSpanEntry::setDateStart( const QDate & sd )
{
	mElement.setStartDate( sd );
}

void TimeSpanEntry::setDateEnd( const QDate & ed )
{
	mElement.setDateComplete( ed );
}

void TimeSpanEntry::applyChanges( bool commit )
{
	mElement.commit();
}

QColor TimeSpanEntry::color() const
{
	return Qt::black;
}

QString TimeSpanEntry::toolTip( const QDate &, int displayMode ) const
{
	return mElement.displayName() + "\n" + dateStart().toString() + " - " + dateEnd().toString();
}

int TimeSpanEntry::sortKey() const
{
	return -mElement.key();
}

int TimeSpanEntry::heightForWidth( int width, QPaintDevice * device, const ScheduleDisplayOptions & displayOptions, int colWidth ) const
{
	return 7;
}

void TimeSpanEntry::paint( const PaintOptions & po )
{
	QColor spanColor( 50, 50, 50 );
	QPointF tri[3];
	tri[0] = QPointF(0,.5);
	tri[1] = QPointF(12,.5);
	tri[2] = QPointF(6,7);
	po.painter->setBrush( spanColor );
	po.painter->setPen( spanColor );
	if( mPaintId != po.paintId ) {
		int offset_left = po.startDate == dateStart() ? 10 : 5;
		int offset_right = po.endDate == dateEnd() ? 10 : 5;
		po.painter->fillRect( po.spanRect.adjusted(offset_left,1,-offset_right,-4), spanColor );
		mPaintId = po.paintId;
	}
	po.painter->setRenderHint( QPainter::Antialiasing, true );
	po.painter->translate( po.spanRect.x(), po.spanRect.y()+1 );
	if( po.cellDate == dateStart() )
		po.painter->drawPolygon( tri, 3 );
	else if( po.column == po.startColumn ) {
		po.painter->translate( 0, -2 );
		tri[0] = QPointF(0.0,3.5);
		tri[1] = QPointF(5.5,0.0);
		tri[2] = QPointF(5.5,7.0);
		po.painter->drawPolygon( tri, 3 );
		po.painter->translate( 0, 1 );
	}
	po.painter->translate( po.spanRect.width()-12, 0 );
	if( po.cellDate == dateEnd() )
		po.painter->drawPolygon( tri, 3 );
	else if( po.column == po.endColumn ) {
		po.painter->translate( 12, -2 );
		tri[0] = QPointF(0.0,3.5);
		tri[1] = QPointF(-5.5,0.0);
		tri[2] = QPointF(-5.5,7.0);
		po.painter->drawPolygon( tri, 3 );
	}
}

int TimeSpanEntry::allowResize() const
{
	if( User::hasPerms( "Schedule", true ) )
		return ResizeLeft | ResizeRight;
	return 0;
}

ElementScheduleRow::ElementScheduleRow( const Element & e, ElementScheduleController * con, ScheduleRow * parent )
: ScheduleRow( parent )
, mShowGlobalEvents( false )
, mController( con )
, mElement( e )
, mNeedsUpdate( true )
{
	Table * t = TimeSheet::table();
	connect( t, SIGNAL( added( RecordList ) ), SLOT( timeSheetsAdded( RecordList ) ) );
	connect( t, SIGNAL( updated( Record, Record ) ), SLOT( timeSheetUpdated( Record, Record ) ) );
	connect( t, SIGNAL( removed( RecordList ) ), SLOT( timeSheetsRemoved( RecordList ) ) );
	t = Schedule::table();
	connect( t, SIGNAL( added( RecordList ) ), SLOT( schedulesAdded( RecordList ) ) );
	connect( t, SIGNAL( updated( Record, Record ) ), SLOT( scheduleUpdated( Record, Record ) ) );
	connect( t, SIGNAL( removed( RecordList ) ), SLOT( schedulesRemoved( RecordList ) ) );
	t = Calendar::table();
	connect( t, SIGNAL( added( RecordList ) ), SLOT( calendarsAdded( RecordList ) ) );
	connect( t, SIGNAL( updated( Record, Record ) ), SLOT( calendarUpdated( Record, Record ) ) );
	connect( t, SIGNAL( removed( RecordList ) ), SLOT( calendarsRemoved( RecordList ) ) );
	mTimeSpan = new TimeSpanEntry( this, mElement );
}

ElementScheduleRow::~ElementScheduleRow()
{
	clear();
}

void ElementScheduleRow::clear()
{
	clearTimeSheets();
	clearSchedules();
	clearCalendars();
}

QDate ElementScheduleRow::startDate()
{
	return mElement.startDate();
}

QDate ElementScheduleRow::endDate()
{
	return mElement.dateComplete();
}

void ElementScheduleRow::setStartDate( const QDate & sd )
{
	int duration = 1;
	if( mElement.startDate().isValid() && mElement.dateComplete().isValid() )
		duration = qMax(0,mElement.startDate().daysTo( mElement.dateComplete() ) );
	mElement.setStartDate(sd);
	mElement.setDateComplete(QDate(sd).addDays(duration));
	mElement.commit();
}

void ElementScheduleRow::setEndDate( const QDate & ed )
{
	mElement.setDateComplete( ed );
	mElement.commit();
}

void ElementScheduleRow::clearTimeSheets()
{
	for( QMap<QDate, QList<TimeSheetEntry*> >::Iterator it = mTimeSheetEntries.begin(); it != mTimeSheetEntries.end(); ++it )
		foreach( TimeSheetEntry * entry, it.value() )
			entry->deref();
	mTimeSheetEntries.clear();
}

void ElementScheduleRow::clearSchedules()
{
	for( QMap<QDate, QList<UserScheduleEntry*> >::Iterator it = mScheduleEntries.begin(); it != mScheduleEntries.end(); ++it )
		foreach( UserScheduleEntry * se, it.value() )
			se->deref();
	mScheduleEntries.clear();
}

void ElementScheduleRow::clearCalendars()
{
	for( QMap<QDate, QList<CalendarEntry*> >::Iterator it = mCalendarEntries.begin(); it != mCalendarEntries.end(); ++it )
		foreach( CalendarEntry * ce, it.value() )
			ce->deref();
	mCalendarEntries.clear();
}

void ElementScheduleRow::setDateRange( const QDate & start, const QDate & end )
{
	if( start != mStart || end != mEnd || mNeedsUpdate ) {
		ScheduleRow::setDateRange( start, end );
		if( mController->showSchedules() )
			updateSchedules();
		if( mController->showTimeSheets() )
			updateTimeSheets();
		if( mController->showCalendars() )
			updateCalendars();
		mNeedsUpdate = false;
	}
}

ScheduleList ElementScheduleRow::getSchedules( const ElementList & elements, const QDate & start, const QDate & end )
{
	UserList users( elements );
	ElementList assets = ElementList(elements) - users;
	ScheduleList ret;
	
	if( !users.isEmpty() ) {
		VarList vl;
		vl += start;
		vl += end;
		ret = Schedule::select( "date >=? and date <=? and fkeyuser in (" + users.keyString() + ")", vl );
	}
	
	if( !assets.isEmpty() ) {
		foreach( Element e, assets )
			ret += Schedule::recordsByElement( e );
//		ret += Schedule::select( "fkeyelement in (" + assets.keyString() + ")" );
	}
		// Load the user and element records together
	ret.users();
	ret.elements().parents();
	return ret;
}

TimeSheetList ElementScheduleRow::getTimeSheets( const ElementList & elements, const QDate & start, const QDate & end )
{
	UserList users( elements );
	ProjectList projects( elements );
	ElementList assets = ElementList(elements) - users - projects;
	TimeSheetList tsl;
	
	VarList vl;
	vl += start;
	vl += end;
	
	if( !users.isEmpty() )
		tsl = TimeSheet::select( "dateTime >= ? and dateTime <= ? and fkeyemployee in (" + users.keyString() + ")", vl );

	if( !assets.isEmpty() ) {
		foreach( Element e, assets )
			tsl += TimeSheet::recordsByElement( e );
//		tsl += TimeSheet::select( "dateTime >= ? and dateTime <= ? and fkeyelement in (" + assets.keyString() + ")", vl );
	}

	if( !projects.isEmpty() )
		tsl += TimeSheet::select( "dateTime >= ? and dateTime <= ? and fkeyproject in(" + projects.keyString() + ") and (fkeyelement=fkeyproject or fkeyelement is null)", vl );
	
	return tsl;
}

struct ScheduleSpanner {
	int fkeyuser, fkeyelement;
	bool operator < ( const ScheduleSpanner & other ) const {
		return fkeyuser < other.fkeyuser || fkeyelement < other.fkeyelement;
	}
};

void ElementScheduleRow::updateSchedules()
{
	clearSchedules();
	QMap<ScheduleSpanner, ScheduleList> grouped;
	bool isUser = User(mElement).isRecord();
	ScheduleList sl = getSchedules( mElement, mStart, mEnd );
	foreach( Schedule s, sl ) {
		ScheduleSpanner ss;
		ss.fkeyuser = s.user().key();
		ss.fkeyelement = s.element().key();
		grouped[ss] += s;
	}

	for( QMap<ScheduleSpanner,ScheduleList>::Iterator it = grouped.begin(); it != grouped.end(); ++it ) 
	{
		ScheduleList sl = it.value();
		sl = sl.sorted( "date" );
		ScheduleList currentSpan;
		QDate lastDate;
		foreach( Schedule s, sl ) {
			if( !currentSpan.isEmpty() && lastDate.addDays(1) != s.date() ) {
				UserScheduleEntry * se = new UserScheduleEntry( this, currentSpan, !isUser );
				mScheduleEntries[se->dateStart()] += se;
				currentSpan.clear();
			}
			currentSpan += s;
			lastDate = s.date();
		}
		if( !currentSpan.isEmpty() ) {
			UserScheduleEntry * se = new UserScheduleEntry( this, currentSpan, !isUser );
			mScheduleEntries[se->dateStart()] += se;
		}
	}
}

struct TSSpanner {
	int fkeyelement, fkeyuser, fkeyassettype;
	bool operator<(const TSSpanner & other ) const {
		return other.fkeyelement < fkeyelement || other.fkeyuser < fkeyuser || other.fkeyassettype < fkeyassettype;
	}
};

void ElementScheduleRow::updateTimeSheets()
{
	clearTimeSheets();
	TimeSheetList tsl = getTimeSheets( mElement, mStart, mEnd );
	LOG_3( "Got " + QString::number( tsl.size() ) + " timesheets" );
	// Group together
	QMap<TSSpanner,TimeSheetList> grouped;
	bool isUser = User(mElement).isRecord();
	foreach( TimeSheet ts, tsl ) {
		TSSpanner tsp;
		tsp.fkeyelement = ts.element().key();
		tsp.fkeyuser = ts.user().key();
		tsp.fkeyassettype = ts.assetType().key();
		grouped[tsp] += ts;
	}

	// Put into continuous spans
	for( QMap<TSSpanner,TimeSheetList>::Iterator it = grouped.begin(); it != grouped.end(); ++it )
	{
		TimeSheetList tsl = it.value();
		tsl = tsl.sorted( "dateTime" );
		TimeSheetList currentSpan;
		QDate lastDate;
		foreach( TimeSheet ts, tsl ) {
			if( !currentSpan.isEmpty() && lastDate.addDays(1) != ts.dateTime().date() ) {
				TimeSheetEntry * tse = new TimeSheetEntry( this, currentSpan, !isUser );
				mTimeSheetEntries[tse->dateStart()] += tse;
				currentSpan.clear();
			}
			currentSpan += ts;
			lastDate = ts.dateTime().date();
		}
		if( !currentSpan.isEmpty() ) {
			TimeSheetEntry * tse = new TimeSheetEntry( this, currentSpan, !isUser );
			mTimeSheetEntries[tse->dateStart()] += tse;
		}
	}
}

void ElementScheduleRow::updateCalendars()
{
	clearCalendars();
	CalendarList cl;
//	if( User(mElement).isRecord() )
//		cl = Calendar::select( "date >= ? and date <= ? and fkeyusr=?", VarList() << start << end << mElement.key()  );
//	else
	if( mShowGlobalEvents && (mController->projectFilters().size() || mController->showGlobalEvents()) && mController->calendarCategories().size() ) {
		VarList vars;
		QString where("date >= ? and date <= ? and fkeycalendarcategory IN (" +mController->calendarCategories().keyString()+ ")");
		vars <<  mStart << mEnd;
		ProjectList projectFilter = mController->projectFilters();
		if( projectFilter.size() || mController->showGlobalEvents() )
			where += " AND (";
		if( projectFilter.size() ) {
			where += " fkeyproject IN (?)";
			vars << projectFilter.keyString();
		}
		if( mController->showGlobalEvents() ) {
			where += projectFilter.size() ? " OR " : " AND ";
			where += " fkeyProject IS NULL";
		}
		if( projectFilter.size() || mController->showGlobalEvents() )
			where += ")";
		if( mController->showMyEventsOnly() ) {
			where += " AND fkeyusr=?";
			vars << User::currentUser().key();
		}
		LOG_5( "Selecting events" );
		cl = Calendar::select( where, vars );
		foreach( Calendar c, cl ) {
			CalendarEntry * ce = new CalendarEntry( this, c );
			mCalendarEntries[ce->dateStart()] += ce;
		}
	}
}

void ElementScheduleRow::calendarsAdded( RecordList rl )
{}

void ElementScheduleRow::calendarUpdated( Record, Record )
{}

void ElementScheduleRow::calendarsRemoved( RecordList )
{}

void ElementScheduleRow::timeSheetsAdded( RecordList rl )
{
	if( !rl.filter( "fkeyEmployee", mElement.key() ).isEmpty() || !rl.filter( "fkeyelement", mElement.key() ).isEmpty() ) {
		mNeedsUpdate = true;
		emit change( this, SpanChange );
	}
}

void ElementScheduleRow::timeSheetUpdated( Record r, Record )
{
	timeSheetsRemoved( r );
}

void ElementScheduleRow::timeSheetsRemoved( RecordList rl )
{
	bool nu = false;
	for( QMap<QDate, QList<TimeSheetEntry*> >::Iterator it = mTimeSheetEntries.begin(); it != mTimeSheetEntries.end(); ++it )
		foreach( TimeSheetEntry * ts, it.value() )
			if( rl && ts->mTimeSheets ) {
				nu = true;
				break;
			}
	if( nu ) {
		mNeedsUpdate = true;
		emit change( this, SpanChange );
	}	
}

void ElementScheduleRow::schedulesAdded( RecordList rl )
{
	if( !rl.filter( "fkeyUser", mElement.key() ).isEmpty() || !rl.filter( "fkeyelement", mElement.key() ).isEmpty() ) {
		mNeedsUpdate = true;
		emit change( this, SpanChange );
	}
}

void ElementScheduleRow::scheduleUpdated( Record r, Record )
{
	schedulesRemoved( r );
	schedulesAdded( r );
}

void ElementScheduleRow::schedulesRemoved( RecordList rl )
{
	for( QMap<QDate, QList<UserScheduleEntry*> >::Iterator it = mScheduleEntries.begin(); it != mScheduleEntries.end(); ++it )
		foreach( UserScheduleEntry * se, it.value() )
			if( rl && se->mSchedules ) {
				mNeedsUpdate = true;
				emit change( this, SpanChange );
				return;
			}
}

void ElementScheduleRow::update()
{
	emit change( this, SpanChange );
}

QList<ScheduleEntry*> ElementScheduleRow::range()
{
	QList<ScheduleEntry*> ret;
	QDate mf = mStart;
	
//	update( mStartDate, mEndDate );

	if( mController->showTimeSheets() )
	// Get timesheets
	{
		if( mTimeSheetEntries.isEmpty() ) updateTimeSheets();
		QMap<QDate, QList<TimeSheetEntry*> >::Iterator it = mTimeSheetEntries.end();
		// Find the first date
		while( it == mTimeSheetEntries.end() && mf <= mEnd ){
			it = mTimeSheetEntries.find( mf );
			mf = mf.addDays(1);
		}
		// Iterator through the dates in the range
		while( it != mTimeSheetEntries.end() && it.key() <= mEnd ){
			foreach( TimeSheetEntry * ts, it.value() )
				ret.append( ts );
			++it;
		}
	}
	
	if( mController->showSchedules() )
	// Get schedules
	{
		if( mScheduleEntries.isEmpty() ) updateSchedules();
		mf = mStart;
		QMap<QDate, QList<UserScheduleEntry*> >::Iterator it = mScheduleEntries.end();
		// Find the first date
		while( it == mScheduleEntries.end() && mf <= mEnd ){
			it = mScheduleEntries.find( mf );
			mf = mf.addDays(1);
		}
		// Iterator through the dates in the range
		while( it != mScheduleEntries.end() && it.key() <= mEnd ){
			foreach( UserScheduleEntry * entry, it.value() )
				ret.append( entry );
			++it;
		}
	}

	if( mController->showCalendars() ) {
		if( mCalendarEntries.isEmpty() ) updateCalendars();
		mf = mStart;
		QMap<QDate, QList<CalendarEntry*> >::Iterator it = mCalendarEntries.end();
		while( it == mCalendarEntries.end() && mf <= mEnd ){
			it = mCalendarEntries.find( mf );
			mf = mf.addDays(1);
		}
		
		while( it != mCalendarEntries.end() && it.key() <= mEnd ){
			foreach( CalendarEntry * entry, it.value() )
				ret.append( entry );
			++it;
		}
	}

	if( mController->showTimeSpans() && mElement.startDate() <= mEnd && mElement.dateComplete() >= mStart )
		ret.append( mTimeSpan );
	
	QList<CalendarPlugin*> plugs = mController->plugins();
	foreach( CalendarPlugin * p, plugs )
	{
		ret += p->range( this, mStart, mEnd );
	}
	return ret;
}

QPixmap ElementScheduleRow::image()
{
	return ElementUi(mElement).image();
}

QString ElementScheduleRow::name()
{
	if( parent() )
		return mElement.displayName();
	return mElement.displayPath();
}

QList<ScheduleRow*> ElementScheduleRow::children()
{
	QList<ScheduleRow*> ret;
	if( mController->showRecursive() ) {
		if( mChildren.isEmpty() ) {
			// Users don't have children
			if( User( mElement ).isRecord() ) return ret;

			if( 0 && Project( mElement ).isRecord() ) {
				// All Children Recursive
				ElementList allKids = mElement.children( ElementType(), true );
				AssetTypeList types = allKids.assetTypes().unique();
				foreach( AssetType at, types ) {
				//	CategoryGroupingRow * cgr = new CategoryGroupingRow( mElement, *it, mController, this );
				//	mChildren += cgr;
				}
			} else {
				ElementList kids = mElement.children();
				mChildren = mController->getRows( kids, this );
			}
		}
		ret = mChildren;
	}
	return ret;
}

void ElementScheduleRow::populateMenu( QMenu * menu, const QPoint & p, const QDate & selStart, const QDate & selEnd, int menuId )
{
	static int myMenuId = 0;
	if( myMenuId != menuId || !menuId ) {
		myMenuId = menuId;
		bool isUser = User(mElement).isRecord();
		ElementList toAdd;
		ElementList inMenu;
		bool canSchedule = User::hasPerms( "Schedule", true );

		if( isUser ) {
			QAction * ts = new TimeSheetAction( Element(), selStart, menu, selEnd );
			ts->setText( "Record TimeSheet..." );
			menu->addAction( ts );
			if( canSchedule )
				menu->addAction( new ScheduleAction( mElement, Element(), selStart, selEnd, menu ) );
			ElementList els = TimeSheet::select( "fkeyemployee=? order by dateTime desc limit 10", VarList() += mElement.key() ).elements().unique();
			if( !els.isEmpty() ) {
				QMenu * recent = menu->addMenu( "Record TimeSheet [Recent Entries]" );

				foreach( Element e, els ) {
					if( !e.isRecord() ) continue;
					recent->addAction( new TimeSheetAction( e, selStart, recent, selEnd ) );
				}
			}
		} else {
			QMenu * addSchedule = 0;
			if( canSchedule )
				addSchedule = menu->addMenu( "Schedule User" );
			QAction * tsa = new TimeSheetAction( mElement, selStart, menu, selEnd );
			tsa->setText( "Record TimeSheet: " + tsa->text() );
			menu->addAction( tsa );

			if( canSchedule ) {
				ElementList el;
				if( mElement.isTask() ) {
					el += mElement;
					addSchedule->setTitle( "Schedule User[" + mElement.displayName() + "]" );
				} else
					el = mElement.children();
				
				foreach( Element e, el ) {
					QMenu * m = el.size() == 1 ? addSchedule : addSchedule->addMenu( e.displayName() );
					UserList users = mElement.users().sorted("nameLast").sorted("nameFirst");
					foreach( User u, users )
						m->addAction( new ScheduleAction( u, e, selStart, selEnd, m ) );
					m->addSeparator();
					QAction * o = new ScheduleAction( User(), e, selStart, selEnd, m );
					o->setText( "Other..." );
					m->addAction( o );
				}
			}
		}
	}
}

CategoryGroupingRow::CategoryGroupingRow( const Element & element, const QStringList & groups, ElementScheduleController * controller, ScheduleRow * parent )
: ElementScheduleRow( element, mController, parent )
, mGroupList( groups )
{}

CategoryGroupingRow::~CategoryGroupingRow()
{}

QList<ScheduleEntry*> CategoryGroupingRow::range()
{
	QList<ScheduleEntry*> ret;
	return ret;
}

void CategoryGroupingRow::createChildren( QMap<int,ElementList> & byType )
{
	for( QMap<int,ElementList>::Iterator it = byType.begin(); it != byType.end(); ++it ) {
		ElementList list = it.value();
		foreach( Element e, list ) {
			//if( !next.isEmpty() )
			//	mChildren += new CategoryGroupingRow( *elit, next, mController, this );
			//else
				mChildren += new ElementScheduleRow( e, mController, this );
		}
	}
}

QList<ScheduleRow*> CategoryGroupingRow::children()
{
	ElementList elements( mElement );
	if( mController->showRecursive() )
		elements += mElement.children( ElementType(), true );

	QString group = mGroupList[0];
	QStringList next = mGroupList.mid(1);
	
	if( group == "Category" ) {
		QMap<int,ElementList> byType;
		foreach( Element e, elements )
			byType[e.assetType().key()] += e;
		createChildren( byType );
	}
	
	if( group == "User" ) {
		QMap<int,ElementList> byType;
		foreach( Element e, elements )
			byType[e.assetType().key()] += e;
		createChildren( byType );
	}
	
	if( group == "Asset" ) {
		QMap<int,ElementList> byType;
		foreach( Element e, elements )
			byType[e.key()] += e;
		createChildren( byType );
	}
	
	
	ScheduleList schedules = getSchedules( elements, mStart, mEnd );
	TimeSheetList timesheets = getTimeSheets( elements, mStart, mEnd );
	
	
	if( mChildren.isEmpty() ) {
		ElementList list = Element::select( "fkeyproject=? and fkeyassettype=?", VarList() << mProject.key() << mAssetType.key() );
//		if( mChildrenMode == ShowUsers )
//			list = ElementUser::select( "fkeyelement in (" + list.keyString() + ")" ).user().unique();
		foreach( Element e, list ) {
			ElementScheduleRow * row = new ElementScheduleRow( e, mController, this );
			mChildren += row;
		}
	}
	return mChildren;
}

void CategoryGroupingRow::populateMenu( QMenu * menu, const QPoint &, const QDate & dateStart, const QDate &, int menuId )
{
}

QDate CategoryGroupingRow::startDate()
{
	QList<ScheduleRow*> kids = children();
	QDate ret;
	foreach( ScheduleRow*row, kids ) {
		QDate d = row->startDate();
		if( !ret.isValid() || d < ret )
			ret = d;
	}
	return ret;
}

QDate CategoryGroupingRow::endDate()
{
	QList<ScheduleRow*> kids = children();
	QDate ret;
	foreach( ScheduleRow*row, kids ) {
		QDate d = row->startDate();
		if( !ret.isValid() || d > ret )
			ret = d;
	}
	return ret;
}

void CategoryGroupingRow::setStartDate( const QDate & )
{
}

void CategoryGroupingRow::setEndDate( const QDate & )
{
}

GlobalEventsRow::GlobalEventsRow( ElementScheduleController * con )
: mController( con )
, mNeedsUpdate( true )
{
	Table * t = Calendar::table();
	connect( t, SIGNAL( added( RecordList ) ), SLOT( calendarsAdded( RecordList ) ) );
	connect( t, SIGNAL( updated( Record, Record ) ), SLOT( calendarUpdated( Record, Record ) ) );
	connect( t, SIGNAL( removed( RecordList ) ), SLOT( calendarsRemoved( RecordList ) ) );	
}

GlobalEventsRow::~GlobalEventsRow()
{
	clear();
}

void GlobalEventsRow::clear()
{
	clearCalendars();
}

void GlobalEventsRow::clearCalendars()
{
	for( QMap<QDate, QList<CalendarEntry*> >::Iterator it = mCalendarEntries.begin(); it != mCalendarEntries.end(); ++it )
		foreach( CalendarEntry * ce, it.value() )
			ce->deref();
	mCalendarEntries.clear();
}

void GlobalEventsRow::setDateRange( const QDate & start, const QDate & end )
{
	if( start != mStart || end != mEnd || mNeedsUpdate ) {
		ScheduleRow::setDateRange(start,end);
		if( mController->showCalendars() )
			updateCalendars();
		mNeedsUpdate = !mController->showCalendars();
	}
}

void GlobalEventsRow::updateCalendars()
{
	clearCalendars();
	mFilter = mController->calendarCategories();
	mProjectFilters = mController->projectFilters();
	mShowGlobal = mController->showGlobalEvents();
	mShowMineOnly = mController->showMyEventsOnly();
	CalendarList cl;
//	if( User(mElement).isRecord() )
//		cl = Calendar::select( "date >= ? and date <= ? and fkeyusr=?", VarList() << start << end << mElement.key()  );
//	else
	if( (mProjectFilters.size() || mShowGlobal) && mFilter.size() ) {
		VarList vars;
		QString where("date >= ? and date <= ? and fkeycalendarcategory IN (" + mFilter.keyString() + ") AND (");
		vars << mStart << mEnd;
		if( mProjectFilters.size() )
			where += " fkeyproject IN (" + mProjectFilters.keyString() + ")";
		if( mProjectFilters.size() && mController->showGlobalEvents() )
			where += " OR";
		if( mShowGlobal )
			where += " fkeyProject IS NULL";
		where += ")";
		if( mController->showMyEventsOnly() ) {
			where += " AND fkeyusr=?";
			vars << User::currentUser().key();
		}
		LOG_5( "Selecting events" );
		cl = Calendar::select( where, vars );
	}
	foreach( Calendar c, cl ) {
		CalendarEntry * ce = new CalendarEntry( this, c );
		if( c.category().name() == "Personal" && c.user() != User::currentUser() )
			continue;
		mCalendarEntries[ce->dateStart()] += ce;
	}
}

void GlobalEventsRow::calendarsAdded( RecordList rl )
{
	CalendarList cl(rl);
	foreach( Calendar c, cl ) {
		if( c.dateTime().date() >= mStart && c.dateTime().date() <= mEnd && mFilter.contains( c.category() ) ) {
			CalendarEntry * ce = new CalendarEntry( this, c );
			mCalendarEntries[ce->dateStart()] += ce;
		}
	}
	emit change( this, SpanChange );
}

void GlobalEventsRow::calendarUpdated( Record, Record )
{
	mNeedsUpdate = true;
	emit change( this, SpanChange );
}

void GlobalEventsRow::calendarsRemoved( RecordList )
{
	mNeedsUpdate = true;
	emit change( this, SpanChange );
}

QList<ScheduleEntry*> GlobalEventsRow::range()
{
	QList<ScheduleEntry*> ret;
	QDate mf = mStart;
	
	if( mController->showCalendars() ) {
		CalendarCategoryList ccl = mController->calendarCategories();
		if( mCalendarEntries.isEmpty() || !(mFilter == ccl) || mShowMineOnly != mController->showMyEventsOnly() || mShowGlobal != mController->showGlobalEvents() || !(mProjectFilters == mController->projectFilters()) ) updateCalendars();
		mf = mStart;
		QMap<QDate, QList<CalendarEntry*> >::Iterator it = mCalendarEntries.end();
		while( it == mCalendarEntries.end() && mf <= mEnd ){
			it = mCalendarEntries.find( mf );
			mf = mf.addDays(1);
		}
		
		while( it != mCalendarEntries.end() && it.key() <= mEnd ){
			foreach( CalendarEntry * entry, it.value() )
				ret.append( entry );
			++it;
		}
	}
	return ret;
}

QString GlobalEventsRow::name()
{
	return "Global Events";
}

void GlobalEventsRow::populateMenu( QMenu * menu, const QPoint & p, const QDate & selStart, const QDate & selEnd, int menuId )
{
	static int myMenuId = 0;
	if( myMenuId != menuId ) {
		myMenuId = menuId;
		menu->addAction( new EventAction( selStart, this ) );
	}
}

ElementScheduleController::ElementScheduleController()
: mShowTimeSheets( true )
, mShowSchedules( true )
, mShowCalendars( false )
, mRecursive( true )
, mShowTimeSpans( false )
, mShowGlobalEvents( true )
, mCalendarCategories( CalendarCategory::select() )
, mShowMyEventsOnly( false )
, mGlobalEventsRow( 0 )
{
	connect( Element::table(), SIGNAL( updated( const Record &, const Record & ) ), SLOT( elementUpdated( const Record &, const Record & ) ) );
}

ElementScheduleController::~ElementScheduleController()
{
	for( QMap<uint,ElementScheduleRow*>::Iterator it = mCachedRows.begin(); it != mCachedRows.end(); ++it )
		delete it.value();
	if( mGlobalEventsRow )
		delete mGlobalEventsRow;
}

void ElementScheduleController::elementsAdded(RecordList)
{
}

void ElementScheduleController::elementsRemoved(RecordList)
{
}

void ElementScheduleController::elementUpdated(const Record & up, const Record & old)
{
	QMap<uint,ElementScheduleRow*>::Iterator it = mCachedRows.find( up.key() );
	if( it != mCachedRows.end() ) {
		ElementScheduleRow * row = (ElementScheduleRow*)it.value();
		row->update();
	}
}

QList<ScheduleRow*> ElementScheduleController::dataSources()
{
	QList<ScheduleRow*> ret = mCurRows;
	if( mShowCalendars ) {
		if( !mGlobalEventsRow )
			mGlobalEventsRow = new GlobalEventsRow( this );
		if( !ret.contains( mGlobalEventsRow ) )
			ret += mGlobalEventsRow;
	}
	foreach( CalendarPlugin * plug, mPlugins ) {
		ret += plug->dataSources( this );
	}
	return ret;
}

ScheduleRow * ElementScheduleController::getRow( const Element & e, ScheduleRow * parent )
{
	if( !e.isRecord() ) return 0;
//	if( e.isTask() && !mElements.contains( e ) && !mRecursive )
//		return 0;
	QMap<uint,ElementScheduleRow*>::Iterator cache = mCachedRows.find( e.key() );
	if( cache == mCachedRows.end() ) {
		ElementScheduleRow * newRow = new ElementScheduleRow( e, this, parent );
		mCachedRows[e.key()] = newRow;
		return newRow;
	}
	return *cache;
}

QList<ScheduleRow*> ElementScheduleController::getRows( ElementList el, ScheduleRow * parent )
{
//	LOG_5( "ElementScheduleController::getRows" );
	QList<ScheduleRow*> ret;
	ElementList rows = el;
	foreach( Element e, rows ) {
		ScheduleRow * row = getRow( e, parent );
		if( row )
			ret.append( row );
	}
	return ret;
}

void ElementScheduleController::setElementList( ElementList el )
{
	mElements = el;
	mCurRows.clear();
	mCurRows = getRows( el );
	emit rowsChanged();
}

void ElementScheduleController::setShowTimeSheets( bool st )
{
	if( st != mShowTimeSheets ) {
		mShowTimeSheets = st;
		emit rowsChanged();
	}
}

void ElementScheduleController::setShowSchedules( bool ss )
{
	if( ss != mShowSchedules ) {
		mShowSchedules = ss;
		emit rowsChanged();
	}
}

void ElementScheduleController::setShowCalendars( bool sc )
{
	if( sc != mShowCalendars ) {
		mShowCalendars = sc;
		emit rowsChanged();
	}
}

void ElementScheduleController::setShowRecursive( bool sr )
{
	if( sr != mRecursive ) {
		mRecursive = sr;
		setElementList( mElements );
	}
}

void ElementScheduleController::setShowTimeSpans( bool sts )
{
	if( sts != mShowTimeSpans ) {
		mShowTimeSpans = sts;
		emit rowsChanged();
	}
}
void ElementScheduleController::setShowCalendarCategories( CalendarCategoryList ccl )
{
	mCalendarCategories = ccl;
	emit rowsChanged();
}

void ElementScheduleController::setProjectFilters( ProjectList projectFilters )
{
	mProjectFilters = projectFilters;
	emit rowsChanged();
}

void ElementScheduleController::setShowGlobalEvents( bool showGlobalEvents )
{
	mShowGlobalEvents = showGlobalEvents;
	emit rowsChanged();
}

void ElementScheduleController::setShowMyEventsOnly( bool showMyEventsOnly )
{
	mShowMyEventsOnly = showMyEventsOnly;
	emit rowsChanged();
}

bool ElementScheduleController::showTimeSheets() const
{
	return mShowTimeSheets;
}

bool ElementScheduleController::showSchedules() const
{
	return mShowSchedules;
}

bool ElementScheduleController::showCalendars() const
{
	return mShowCalendars;
}

bool ElementScheduleController::showRecursive() const
{
	return mRecursive;
}

bool ElementScheduleController::showTimeSpans() const
{
	return mShowTimeSpans;
}

bool ElementScheduleController::showGlobalEvents() const
{
	return mShowGlobalEvents;
}

void ElementScheduleController::registerPlugin( CalendarPlugin * plug )
{
	mPlugins += plug;
}

QList<CalendarPlugin*> ElementScheduleController::plugins()
{
	return mPlugins;
}

QList<CalendarPlugin*> ElementScheduleController::mPlugins;

