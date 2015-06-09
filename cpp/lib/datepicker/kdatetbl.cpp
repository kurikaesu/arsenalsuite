/*  -*- C++ -*-
    This file is part of the KDE libraries
    Copyright (C) 1997 Tim D. Gilman (tdgilman@best.org)
              (C) 1998-2001 Mirko Boehm (mirko@kde.org)
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

/////////////////// KDateTable widget class //////////////////////
//
// Copyright (C) 1997 Tim D. Gilman
//           (C) 1998-2001 Mirko Boehm
// Written using Qt (http://www.troll.no) for the
// KDE project (http://www.kde.org)
//
// This is a support class for the KDatePicker class.  It just
// draws the calender table without titles, but could theoretically
// be used as a standalone.
//
// When a date is selected by the user, it emits a signal:
//      dateSelected(QDate)

#include <qapplication.h>
#include <qmenu.h>
#include "kcalendarsystem.h"
#include "kdatepicker.h"
#include "kdatetbl.h"
#include <qdatetime.h>
#include <qstring.h>
#include <qpen.h>
#include <qpainter.h>
#include <qdialog.h>
#include <q3dict.h>
#include <assert.h>
#include <QKeyEvent>



void DateColorControl::getColor( const QDate &, QColor & , KDateTable::BackgroundMode &, QColor &)
{
}


class KDateTable::KDateTablePrivate
{
public:
   KDateTablePrivate()
   {
      popupMenuEnabled=false;
 //     useCustomColors=false;
	  mDateColorControl = new DateColorControl;
   }

   ~KDateTablePrivate()
   {
   	delete mDateColorControl;
   }

   bool popupMenuEnabled;
 //  bool useCustomColors;

   struct DatePaintingMode
   {
     QColor fgColor;
     QColor bgColor;
     BackgroundMode bgMode;
   };
  // Q3Dict <DatePaintingMode> customPaintingModes;
	DateColorControl * mDateColorControl;
};


KDateValidator::KDateValidator(QWidget* parent, const char* name)
    : QValidator(parent, name)
{
}

QValidator::State
KDateValidator::validate(QString& text, int&) const
{
  QDate temp;
  // ----- everything is tested in date():
  return date(text, temp);
}

QValidator::State
KDateValidator::date(const QString& text, QDate& d) const
{
  QDate tmp = QDate::fromString( text );
  if (!tmp.isNull())
    {
      d = tmp;
      return Acceptable;
    } else
      return Valid;
}

void
KDateValidator::fixup( QString& ) const
{

}

KDateTable::KDateTable(QWidget *parent, QDate date_)
  : Q3GridView(parent)
{
  d = new KDateTablePrivate;
  setFontSize(10);
  if(!date_.isValid())
    {
      //kdDebug() << "KDateTable ctor: WARNING: Given date is invalid, using current date." << endl;
      date_=QDate::currentDate();
    }
  setFocusPolicy( Qt::StrongFocus );
  setNumRows(7); // 6 weeks max + headline
  setNumCols(7); // 7 days a week
  setHScrollBarMode(AlwaysOff);
  setVScrollBarMode(AlwaysOff);
  viewport()->setEraseColor( colorGroup().base() );
  setDate(date_); // this initializes firstday, numdays, numDaysPrevMonth
}

KDateTable::~KDateTable()
{
  delete d;
}

void
KDateTable::paintCell(QPainter *painter, int row, int col)
{
	QRect rect;
	QString text;
	QPen pen;
	int w=cellWidth();
	int h=cellHeight();
	int pos;
	QBrush brushBlue = Qt::blue;//KGlobalSettings::activeTitleColor());
	QBrush brushLightblue = QColor(Qt::blue).light();//(KGlobalSettings::baseColor());
	QColor dark( 115, 115, 115 );
	// -----
	QFont f = font();
	int firstWeekDay = 1;
	
	if(row==0)
	{ // we are drawing the headline
		f.setBold(true);
		painter->setFont(f);
		QString daystr = calendar()->weekDayName( (col+firstWeekDay)%8, true);;

		QColor fillColor;
		if ( daystr==QString("Sun") || daystr==QString("Sat") ) {
			fillColor = Qt::white;
			painter->setPen( dark );
		} else {
			fillColor = dark;
			painter->setPen( Qt::white );
		}
		painter->fillRect( 0, 0, w, h, fillColor );
		painter->drawText(0, 0, w, h-1, Qt::AlignCenter, daystr, -1, &rect);
		painter->setPen(colorGroup().text());
		painter->drawLine(0,h-1,w-1,h-1);
		// ----- draw the weekday:
	} else {
		bool paintEllipse = false;
		QColor ellipseColor;
		
		painter->setFont(f);
		pos=7*(row-1)+col;
	
		QDate pCellDate;
		// First day of month
		calendar()->setYMD(pCellDate, calendar()->year(date), calendar()->month(date), 1);
	
		if ( firstWeekDay < 4 )
			pos += firstWeekDay;
		else
			pos += firstWeekDay - 7;
		pCellDate = calendar()->addDays(pCellDate, pos-firstday);
		text = calendar()->dayString(pCellDate, true);
		
		painter->setPen( Qt::white );
		painter->setBrush( Qt::white );
		
		if(pos<firstday || (firstday+numdays<=pos))
		{ // we are either
			// ° painting a day of the previous month or
			// ° painting a day of the following month
			pen = QPen(Qt::gray);
		} else { // paint a day of the current month
			BackgroundMode bgMode = NoBgMode;
			QColor fgColor = colorGroup().text();
			QColor bgColor = Qt::white;
			d->mDateColorControl->getColor( pCellDate, fgColor, bgMode, bgColor );
			
			painter->setBrush( ( bgMode == RectangleMode ) ? bgColor : Qt::white );
			pen = fgColor;
			if (bgMode == CircleMode) {
				paintEllipse = true;
				ellipseColor = bgColor;
			}
		}
		
		if( (firstday+calendar()->day(date)-1==pos) && hasFocus())
		{ // draw the currently selected date
			painter->setPen(colorGroup().highlight());//KGlobalSettings::highlightColor());
			painter->setBrush(colorGroup().highlight());//KGlobalSettings::highlightColor());
			pen= QPen(Qt::white);
		}
	
		if ( pCellDate == QDate::currentDate() )
		{
			painter->setPen(colorGroup().text());
		}
	
		painter->drawRect(0, 0, w, h);
		if( paintEllipse )
			painter->drawEllipse(0,0,w,h);
		painter->setPen(pen);
		painter->drawText(0, 0, w, h, Qt::AlignCenter, text, -1, &rect);
	}
	if(rect.width()>maxCell.width()) maxCell.setWidth(rect.width());
	if(rect.height()>maxCell.height()) maxCell.setHeight(rect.height());
}

void
KDateTable::keyPressEvent( QKeyEvent *e )
{
    QDate temp = date;

    switch( e->key() ) {
    case Qt::Key_Prior:
        temp = calendar()->addMonths( date, -1 );
        setDate(temp);
        return;
    case Qt::Key_Next:
        temp = calendar()->addMonths( date, 1 );
        setDate(temp);
        return;
    case Qt::Key_Up:
        if ( calendar()->day(date) > 7 ) {
            setDate(date.addDays(-7));
            return;
        }
        break;
    case Qt::Key_Down:
        if ( calendar()->day(date) <= calendar()->daysInMonth(date)-7 ) {
            setDate(date.addDays(7));
            return;
        }
        break;
    case Qt::Key_Left:
        if ( calendar()->day(date) > 1 ) {
            setDate(date.addDays(-1));
            return;
        }
        break;
    case Qt::Key_Right:
        if ( calendar()->day(date) < calendar()->daysInMonth(date) ) {
            setDate(date.addDays(1));
            return;
        }
        break;
    case Qt::Key_Minus:
        setDate(date.addDays(-1));
        return;
    case Qt::Key_Plus:
        setDate(date.addDays(1));
        return;
    case Qt::Key_N:
        setDate(QDate::currentDate());
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        emit tableClicked();
        return;
    default:
        break;
    }

    //KNotifyClient::beep();
}

void
KDateTable::viewportResizeEvent(QResizeEvent * e)
{
  Q3GridView::viewportResizeEvent(e);

  setCellWidth(viewport()->width()/7);
  setCellHeight(viewport()->height()/7);
}

void
KDateTable::setFontSize(int size)
{
  int count;
  QFontMetrics metrics(fontMetrics());
  QRect rect;
  // ----- store rectangles:
  fontsize=size;
  // ----- find largest day name:
  maxCell.setWidth(0);
  maxCell.setHeight(0);
  for(count=0; count<7; ++count)
    {
      rect=metrics.boundingRect(calendar()->weekDayName(count+1, true));
      maxCell.setWidth(QMAX(maxCell.width(), rect.width()));
      maxCell.setHeight(QMAX(maxCell.height(), rect.height()));
    }
  // ----- compare with a real wide number and add some space:
  rect=metrics.boundingRect(QString::fromLatin1("88"));
  maxCell.setWidth(QMAX(maxCell.width()+2, rect.width()));
  maxCell.setHeight(QMAX(maxCell.height()+4, rect.height()));
}

void
KDateTable::wheelEvent ( QWheelEvent * e )
{
    setDate(date.addMonths( -(int)(e->delta()/120)) );
    e->accept();
}

void
KDateTable::contentsMousePressEvent(QMouseEvent *e)
{
   if(e->type()!=QEvent::MouseButtonPress)
    { // the KDatePicker only reacts on mouse press events:
      return;
    }
  if(!isEnabled())
    {
     // KNotifyClient::beep();
      return;
    }

  //int dayoff = KGlobal::locale()->weekStartsMonday() ? 1 : 0;
  int dayoff = 1;
  // -----
  int row, col, pos, temp;
  QPoint mouseCoord;
  // -----
  mouseCoord = e->pos();
  row=rowAt(mouseCoord.y());
  col=columnAt(mouseCoord.x());
  if(row<1 || col<0)
    { // the user clicked on the frame of the table
      return;
    }

  // Rows and columns are zero indexed.  The (row - 1) below is to avoid counting
  // the row with the days of the week in the calculation.  We however want pos
  // to be "1 indexed", hence the "+ 1" at the end of the sum.

  pos = (7 * (row - 1)) + col + 1;

  // This gets pretty nasty below.  firstday is a locale independant index for
  // the first day of the week.  dayoff is the day of the week that the week
  // starts with for the selected locale.  Monday = 1 .. Sunday = 7
  // Strangely, in some cases dayoff is in fact set to 8, hence all of the
  // "dayoff % 7" sections below.

  if(pos + dayoff % 7 <= firstday)
    { // this day is in the previous month
      setDate(date.addDays(-1 * (calendar()->day(date) + firstday - pos - dayoff % 7)));
      return;
    }
  if(firstday + numdays < pos + dayoff % 7)
    { // this date is in the next month
      setDate(date.addDays(pos - firstday - calendar()->day(date) + dayoff % 7));
      return;
    }

  temp = firstday + calendar()->day(date) - dayoff % 7 - 1;

  QDate clickedDate;
  calendar()->setYMD(clickedDate, calendar()->year(date), calendar()->month(date),
		   pos - firstday + dayoff % 7);
  setDate(clickedDate);

  updateCell(temp/7+1, temp%7); // Update the previously selected cell
  updateCell(row, col); // Update the selected cell
  // assert(QDate(date.year(), date.month(), pos-firstday+dayoff).isValid());

  emit tableClicked();

  if (  e->button() == Qt::RightButton && d->popupMenuEnabled )
  {
	QMenu *menu = new QMenu();
	//menu->insertTitle( clickedDate.toString() );
	emit aboutToShowContextMenu( menu, clickedDate );
	menu->popup(e->globalPos());
  }
}

bool
KDateTable::setDate(const QDate& date_)
{
  bool changed=false;
  QDate temp;
  // -----
  if(!date_.isValid())
    {
      //kdDebug() << "KDateTable::setDate: refusing to set invalid date." << endl;
      return false;
    }
  if(date!=date_)
    {
      date=date_;
      changed=true;
    }
  calendar()->setYMD(temp, calendar()->year(date), calendar()->month(date), 1);
  //temp.setYMD(date.year(), date.month(), 1);
  //kdDebug() << "firstDayInWeek: " << temp.toString() << endl;
  firstday=temp.dayOfWeek();
  if(firstday==1) firstday=8;
  //numdays=date.daysInMonth();
  numdays=calendar()->daysInMonth(date);

  temp = calendar()->addMonths(temp, -1);
  numDaysPrevMonth=calendar()->daysInMonth(temp);
  if(changed)
    {
      repaintContents(false);
	  emit(dateChanged(date));
    }
  return true;
}

const QDate&
KDateTable::getDate() const
{
  return date;
}

// what are those repaintContents() good for? (pfeiffer)
void KDateTable::focusInEvent( QFocusEvent *e )
{
//    repaintContents(false);
    Q3GridView::focusInEvent( e );
}

void KDateTable::focusOutEvent( QFocusEvent *e )
{
//    repaintContents(false);
    Q3GridView::focusOutEvent( e );
}

QSize
KDateTable::sizeHint() const
{
  if(maxCell.height()>0 && maxCell.width()>0)
    {
      return QSize(maxCell.width()*numCols()+2*frameWidth(),
             (maxCell.height()+2)*numRows()+2*frameWidth());
    } else {
      //kdDebug() << "KDateTable::sizeHint: obscure failure - " << endl;
      return QSize(-1, -1);
    }
}

void KDateTable::setPopupMenuEnabled( bool enable )
{
   d->popupMenuEnabled=enable;
}

bool KDateTable::popupMenuEnabled() const
{
   return d->popupMenuEnabled;
}

void KDateTable::setColorControl( DateColorControl * dcc )
{
	if( dcc ){
		delete d->mDateColorControl;
		d->mDateColorControl = dcc;
		repaintContents(false);
	}
}
/*
void KDateTable::setCustomDatePainting(const QDate &date, const QColor &fgColor, BackgroundMode bgMode, const QColor &bgColor)
{
    if (!fgColor.isValid())
    {
	unsetCustomDatePainting( date );
	return;
    }

    KDateTablePrivate::DatePaintingMode *mode=new KDateTablePrivate::DatePaintingMode;
    mode->bgMode=bgMode;
    mode->fgColor=fgColor;
    mode->bgColor=bgColor;

    d->customPaintingModes.replace( date.toString(), mode );
    d->useCustomColors=true;
    update();
}

void KDateTable::unsetCustomDatePainting( const QDate &date )
{
    d->customPaintingModes.remove( date.toString() );
}
*/
KDateInternalWeekSelector::KDateInternalWeekSelector
(QWidget* parent)
  : QLineEdit(parent),
    val(new QIntValidator(this)),
    result(0)
{
  //setFrameStyle(QFrame::NoFrame);
  setValidator(val);
  connect(this, SIGNAL(returnPressed()), SLOT(weekEnteredSlot()));
}

void
KDateInternalWeekSelector::weekEnteredSlot()
{
  bool ok;
  int week;
  // ----- check if this is a valid week:
  week=text().toInt(&ok);
  if(!ok)
    {
     // KNotifyClient::beep();
      return;
    }
  result=week;
  emit(closeMe(1));
}

int
KDateInternalWeekSelector::getWeek()
{
  return result;
}

void
KDateInternalWeekSelector::setWeek(int week)
{
  QString temp;
  // -----
  temp.setNum(week);
  setText(temp);
}

void
KDateInternalWeekSelector::setMaxWeek(int max)
{
  val->setRange(1, max);
}

// ### CFM To avoid binary incompatibility.
//     In future releases, remove this and replace by  a QDate
//     private member, needed in KDateInternalMonthPicker::paintCell
class KDateInternalMonthPicker::KDateInternalMonthPrivate {
public:
        KDateInternalMonthPrivate (int y, int m, int d)
        : year(y), month(m), day(d)
        {};
        int year;
        int month;
        int day;
};

KDateInternalMonthPicker::~KDateInternalMonthPicker() {
   delete d;
}

KDateInternalMonthPicker::KDateInternalMonthPicker
(const QDate & date, QWidget* parent)
  : Q3GridView(parent),
    result(0) // invalid
{
  QRect rect;
  // -----
  activeCol = -1;
  activeRow = -1;
  setHScrollBarMode(AlwaysOff);
  setVScrollBarMode(AlwaysOff);
  setFrameStyle(QFrame::NoFrame);
  setNumCols(3);
  d = new KDateInternalMonthPrivate(date.year(), date.month(), date.day());
  // For monthsInYear != 12
  setNumRows( (calendar()->monthsInYear(date) + 2) / 3);
  // enable to find drawing failures:
  // setTableFlags(Tbl_clipCellPainting);
  viewport()->setEraseColor(colorGroup().base()); // for consistency with the datepicker
  // ----- find the preferred size
  //       (this is slow, possibly, but unfortunately it is needed here):
  QFontMetrics metrics(font());
  for(int i = 1; ; ++i)
    {
      QString str = calendar()->monthName(i,calendar()->year(date), false);
      if (str.isNull()) break;
      rect=metrics.boundingRect(str);
      if(max.width()<rect.width()) max.setWidth(rect.width());
      if(max.height()<rect.height()) max.setHeight(rect.height());
    }
}

QSize
KDateInternalMonthPicker::sizeHint() const
{
  return QSize((max.width()+6)*numCols()+2*frameWidth(),
         (max.height()+6)*numRows()+2*frameWidth());
}

int
KDateInternalMonthPicker::getResult() const
{
  return result;
}

void
KDateInternalMonthPicker::setupPainter(QPainter *p)
{
  p->setPen(colorGroup().text());
}

void
KDateInternalMonthPicker::viewportResizeEvent(QResizeEvent*)
{
  setCellWidth(width() / numCols());
  setCellHeight(height() / numRows());
}

void
KDateInternalMonthPicker::paintCell(QPainter* painter, int row, int col)
{
  int index;
  QString text;
  // ----- find the number of the cell:
  index=3*row+col+1;
  text=calendar()->monthName(index,calendar()->year(QDate(d->year, d->month, d->day)), false);
  painter->drawText(0, 0, cellWidth(), cellHeight(), Qt::AlignCenter, text);
  if ( activeCol == col && activeRow == row )
      painter->drawRect( 0, 0, cellWidth(), cellHeight() );
}

void
KDateInternalMonthPicker::contentsMousePressEvent(QMouseEvent *e)
{
  if(!isEnabled() || e->button() != Qt::LeftButton)
    {
      //KNotifyClient::beep();
      return;
    }
  // -----
  int row, col;
  QPoint mouseCoord;
  // -----
  mouseCoord = e->pos();
  row=rowAt(mouseCoord.y());
  col=columnAt(mouseCoord.x());

  if(row<0 || col<0)
    { // the user clicked on the frame of the table
      activeCol = -1;
      activeRow = -1;
    } else {
      activeCol = col;
      activeRow = row;
      updateCell( row, col /*, false */ );
  }
}

void
KDateInternalMonthPicker::contentsMouseMoveEvent(QMouseEvent *e)
{
  if (e->state() & Qt::LeftButton)
    {
      int row, col;
      QPoint mouseCoord;
      // -----
      mouseCoord = e->pos();
      row=rowAt(mouseCoord.y());
      col=columnAt(mouseCoord.x());
      int tmpRow = -1, tmpCol = -1;
      if(row<0 || col<0)
        { // the user clicked on the frame of the table
          if ( activeCol > -1 )
            {
              tmpRow = activeRow;
              tmpCol = activeCol;
            }
          activeCol = -1;
          activeRow = -1;
        } else {
          bool differentCell = (activeRow != row || activeCol != col);
          if ( activeCol > -1 && differentCell)
            {
              tmpRow = activeRow;
              tmpCol = activeCol;
            }
          if ( differentCell)
            {
              activeRow = row;
              activeCol = col;
              updateCell( row, col /*, false */ ); // mark the new active cell
            }
        }
      if ( tmpRow > -1 ) // repaint the former active cell
          updateCell( tmpRow, tmpCol /*, true */ );
    }
}

void
KDateInternalMonthPicker::contentsMouseReleaseEvent(QMouseEvent *e)
{
  if(!isEnabled())
    {
      return;
    }
  // -----
  int row, col, pos;
  QPoint mouseCoord;
  // -----
  mouseCoord = e->pos();
  row=rowAt(mouseCoord.y());
  col=columnAt(mouseCoord.x());
  if(row<0 || col<0)
    { // the user clicked on the frame of the table
      emit(closeMe(0));
    }

  pos=3*row+col+1;
  result=pos;
  emit(closeMe(1));
}



KDateInternalYearSelector::KDateInternalYearSelector
(QWidget* parent)
  : QLineEdit(parent),
    val(new QIntValidator(this)),
    result(0)
{
//  setFrameStyle(QFrame::NoFrame);
  // we have to respect the limits of QDate here, I fear:
  val->setRange(0, 8000);
  setValidator(val);
  connect(this, SIGNAL(returnPressed()), SLOT(yearEnteredSlot()));
}

void
KDateInternalYearSelector::yearEnteredSlot()
{
  bool ok;
  int year;
  QDate date;
  // ----- check if this is a valid year:
  year=text().toInt(&ok);
  if(!ok)
    {
     // KNotifyClient::beep();
      return;
    }
  //date.setYMD(year, 1, 1);
  calendar()->setYMD(date, year, 1, 1);
  if(!date.isValid())
    {
     // KNotifyClient::beep();
      return;
    }
  result=year;
  emit(closeMe(1));
}

int
KDateInternalYearSelector::getYear()
{
  return result;
}

void
KDateInternalYearSelector::setYear(int year)
{
  QString temp;
  // -----
  temp.setNum(year);
  setText(temp);
}

KPopupFrame::KPopupFrame(QWidget* parent)
  : QFrame(parent),
    result(0), // rejected
    main(0)
{
  setFrameStyle(QFrame::Box|QFrame::Raised);
  setMidLineWidth(2);
}

void
KPopupFrame::keyPressEvent(QKeyEvent* e)
{
  if(e->key()==Qt::Key_Escape)
    {
      result=0; // rejected
      qApp->exit_loop();
    }
}

void
KPopupFrame::close(int r)
{
  result=r;
  qApp->exit_loop();
}

void
KPopupFrame::setMainWidget(QWidget* m)
{
  main=m;
  if(main!=0)
    {
      resize(main->width()+2*frameWidth(), main->height()+2*frameWidth());
    }
}

void
KPopupFrame::resizeEvent(QResizeEvent*)
{
  if(main!=0)
    {
      main->setGeometry(frameWidth(), frameWidth(),
          width()-2*frameWidth(), height()-2*frameWidth());
    }
}

void
KPopupFrame::popup(const QPoint &pos)
{
  // Make sure the whole popup is visible.
  QRect d;// = KGlobalSettings::desktopGeometry(pos);

  int x = pos.x();
  int y = pos.y();
  int w = width();
  int h = height();
  if (x+w > d.x()+d.width())
    x = d.width() - w;
  if (y+h > d.y()+d.height())
    y = d.height() - h;
  if (x < d.x())
    x = 0;
  if (y < d.y())
    y = 0;

  // Pop the thingy up.
  move(x, y);
  show();
}

int
KPopupFrame::exec(QPoint pos)
{
  popup(pos);
  repaint();
  qApp->enter_loop();
  hide();
  return result;
}

int
KPopupFrame::exec(int x, int y)
{
  return exec(QPoint(x, y));
}

