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

#include <qlayout.h>
#include <qframe.h>
#include <qpainter.h>
#include <qdialog.h>
#include <qstyle.h>
#include <qtoolbutton.h>
#include <qcombobox.h>
#include <qtooltip.h>
#include <qfont.h>
#include <qvalidator.h>
#include <qmenu.h>
#include <qlineedit.h>
#include <qtoolbar.h>
#include <qapplication.h>
#include <QKeyEvent>

/*
#include <kglobal.h>
#include <kapplication.h>
#include <klocale.h>
#include <kiconloader.h>
#include <ktoolbar.h>
#include <klineedit.h>
#include <kdebug.h>
#include <knotifyclient.h>
*/

#include "kcalendarsystem.h"
#include "kdatepicker.h"
#include "kdatetbl.h"

class KDatePicker::KDatePickerPrivate
{
public:
    KDatePickerPrivate() : closeButton(0L), selectWeek(0L), todayButton(0), navigationLayout(0) {}

    void fillWeeksCombo(const QDate &date);

    QWidget * tb;
    QToolButton *closeButton;
    QComboBox *selectWeek;
    QToolButton *todayButton;
    QBoxLayout *navigationLayout;
};

void KDatePicker::fillWeeksCombo(const QDate &date)
{
  // every year can have a different number of weeks
  int i, weeks = calendar()->weeksInYear(calendar()->year(date));

  if ( d->selectWeek->count() == weeks ) return;  // we already have the correct number

  d->selectWeek->clear();

  for (i = 1; i <= weeks; i++)
    d->selectWeek->insertItem(QString("Week %1").arg(i));
}

KDatePicker::KDatePicker(QWidget *parent, QDate dt)
  : QFrame(parent)
{
  init( dt );
}

void KDatePicker::init( const QDate &dt )
{
  d = new KDatePickerPrivate();

  d->tb = new QWidget(this);
  QHBoxLayout * tblayout = new QHBoxLayout( d->tb );
  tblayout->setAutoAdd( true );
  tblayout->setMargin( 0 );
  tblayout->setSpacing( 1 );

  yearBackward = new QToolButton(d->tb);
  monthBackward = new QToolButton(d->tb);
  selectMonth = new QToolButton(d->tb);
  selectYear = new QToolButton(d->tb);
  monthForward = new QToolButton(d->tb);
  yearForward = new QToolButton(d->tb);
  line = new QLineEdit(this);
  val = new KDateValidator(this);
  table = new KDateTable(this);
  fontsize = 10;//KGlobalSettings::generalFont().pointSize();
  if (fontsize == -1)
     fontsize = 10;//QFontInfo(KGlobalSettings::generalFont()).pointSize();

//  fontsize++; // Make a little bigger
 
  d->selectWeek = new QComboBox(false, this);  // read only week selection
  d->todayButton = new QToolButton(this);
  d->todayButton->setIcon(QIcon(":/images/today.png"));

  QToolTip::add(yearForward, QString("Next year"));
  QToolTip::add(yearBackward, QString("Previous year"));
  QToolTip::add(monthForward, QString("Next month"));
  QToolTip::add(monthBackward, QString("Previous month"));
  QToolTip::add(d->selectWeek, QString("Select a week"));
  QToolTip::add(selectMonth, QString("Select a month"));
  QToolTip::add(selectYear, QString("Select a year"));
  QToolTip::add(d->todayButton, QString("Select the current day"));

  // -----
  setFontSize(fontsize);
  line->setValidator(val);
  line->installEventFilter( this );
  yearForward->setIcon(QIcon(":/images/2rightarrow.png"));
  yearBackward->setIcon(QIcon( ":/images/2leftarrow.png"));
  monthForward->setIcon(QIcon( ":/images/1rightarrow.png"));
  monthBackward->setIcon(QIcon( ":/images/1leftarrow.png"));
  setDate(dt); // set button texts
  connect(table, SIGNAL(dateChanged(QDate)), SLOT(dateChangedSlot(QDate)));
  connect(table, SIGNAL(tableClicked()), SLOT(tableClickedSlot()));
  connect(monthForward, SIGNAL(clicked()), SLOT(monthForwardClicked()));
  connect(monthBackward, SIGNAL(clicked()), SLOT(monthBackwardClicked()));
  connect(yearForward, SIGNAL(clicked()), SLOT(yearForwardClicked()));
  connect(yearBackward, SIGNAL(clicked()), SLOT(yearBackwardClicked()));
  connect(d->selectWeek, SIGNAL(activated(int)), SLOT(weekSelected(int)));
  connect(d->todayButton, SIGNAL(clicked()), SLOT(todayButtonClicked()));
  connect(selectMonth, SIGNAL(clicked()), SLOT(selectMonthClicked()));
  connect(selectYear, SIGNAL(clicked()), SLOT(selectYearClicked()));
  connect(line, SIGNAL(returnPressed()), SLOT(lineEnterPressed()));
  table->setFocus();

  QBoxLayout * topLayout = new QVBoxLayout(this);

  topLayout->addWidget(d->tb);
  topLayout->addWidget(table);
  topLayout->setMargin(0);
  topLayout->setSpacing(1);

  QBoxLayout * bottomLayout = new QHBoxLayout(topLayout);
  bottomLayout->addWidget(d->todayButton);
  bottomLayout->addWidget(line);
  bottomLayout->addWidget(d->selectWeek);
  bottomLayout->setMargin(0);
  bottomLayout->setSpacing(1);
}

KDatePicker::~KDatePicker()
{
  delete d;
}

bool
KDatePicker::eventFilter(QObject *o, QEvent *e )
{
   if ( e->type() == QEvent::KeyPress ) {
      QKeyEvent *k = (QKeyEvent *)e;

      if ( (k->key() == Qt::Key_Prior) ||
           (k->key() == Qt::Key_Next)  ||
           (k->key() == Qt::Key_Up)    ||
           (k->key() == Qt::Key_Down) )
       {
          QApplication::sendEvent( table, e );
          table->setFocus();
          return true; // eat event
       }
	   else if( (k->key() == Qt::Key_Escape ) ||
	   			(k->key() == Qt::Key_Enter ) )
	   {
	   	QApplication::sendEvent( parent(), e );
	   }
   }
   return QFrame::eventFilter( o, e );
}

void
KDatePicker::resizeEvent(QResizeEvent* e)
{
  QWidget::resizeEvent(e);
}

void
KDatePicker::dateChangedSlot(QDate date)
{
   // kdDebug(298) << "KDatePicker::dateChangedSlot: date changed (" << date.year() << "/" << date.month() << "/" << date.day() << ")." << endl;

    line->setText(date.toString());
    selectMonth->setText(calendar()->monthName(date, false));
    fillWeeksCombo(date);
    d->selectWeek->setCurrentItem(calendar()->weekNumber(date) - 1);
    selectYear->setText(calendar()->yearString(date, false));

    emit(dateChanged(date));
}

void
KDatePicker::tableClickedSlot()
{
 // kdDebug(298) << "KDatePicker::tableClickedSlot: table clicked." << endl;
  emit(dateSelected(table->getDate()));
  emit(tableClicked());
}

const QDate&
KDatePicker::getDate() const
{
  return table->getDate();
}

const QDate &
KDatePicker::date() const
{
    return table->getDate();
}

bool
KDatePicker::setDate(const QDate& date)
{
    if(date.isValid())
    {
        table->setDate(date);
        fillWeeksCombo(date);
        d->selectWeek->setCurrentItem(calendar()->weekNumber(date) - 1);
        selectMonth->setText(calendar()->monthName(date, false));
        selectYear->setText(calendar()->yearString(date, true));
        line->setText(date.toString());
        return true;
    }
    else
    {
       // kdDebug(298) << "KDatePicker::setDate: refusing to set invalid date." << endl;
        return false;
    }
}

void
KDatePicker::monthForwardClicked()
{
    QDate temp;
    temp = calendar()->addMonths( table->getDate(), 1 );

    setDate( temp );
}

void
KDatePicker::monthBackwardClicked()
{
    QDate temp;
    temp = calendar()->addMonths( table->getDate(), -1 );

    setDate( temp );
}

void
KDatePicker::yearForwardClicked()
{
    QDate temp;
    temp = calendar()->addYears( table->getDate(), 1 );

    setDate( temp );
}

void
KDatePicker::yearBackwardClicked()
{
    QDate temp;
    temp = calendar()->addYears( table->getDate(), -1 );

    setDate( temp );
}

void KDatePicker::selectWeekClicked() {}  // ### in 3.2 obsolete; kept for binary compatibility

void
KDatePicker::weekSelected(int week)
{
  week++; // week number starts with 1

  QDate date = table->getDate();
  int year = calendar()->year(date);

  calendar()->setYMD(date, year, 1, 1);
  date = calendar()->addDays(date, -7);
  while (calendar()->weekNumber(date) != 1)
    date = calendar()->addDays(date, 1);

  // date is now first day in week 1 some day in week 1
  date = calendar()->addDays(date, (week - calendar()->weekNumber(date)) * 7);

  setDate(date);
}

void
KDatePicker::selectMonthClicked()
{
  // every year can have different month names (in some calendar systems)
  QDate date = table->getDate();
  int i, month, months = calendar()->monthsInYear(date);

  QMenu popup(selectMonth);

  for (i = 1; i <= months; i++)
    popup.insertItem(calendar()->monthName(i, calendar()->year(date)), i);

  QAction * cur = popup.actions().at(calendar()->month(date) - 1);
  popup.setActiveAction( cur );

	QAction * ret = popup.exec(selectMonth->mapToGlobal(QPoint(0, 0)), cur);
	if( !ret )
		return;

	month = popup.actions().indexOf( ret );

  int day = calendar()->day(date);
  // ----- construct a valid date in this month:
  //date.setYMD(date.year(), month, 1);
  //date.setYMD(date.year(), month, QMIN(day, date.daysInMonth()));
  calendar()->setYMD(date, calendar()->year(date), month,
                   QMIN(day, calendar()->daysInMonth(date)));
  // ----- set this month
  setDate(date);
}

void
KDatePicker::selectYearClicked()
{
  int year;
  KPopupFrame* popup = new KPopupFrame(this);
  KDateInternalYearSelector* picker = new KDateInternalYearSelector(popup);
  // -----
  picker->resize(picker->sizeHint());
  popup->setMainWidget(picker);
  connect(picker, SIGNAL(closeMe(int)), popup, SLOT(close(int)));
  picker->setFocus();
  if(popup->exec(selectYear->mapToGlobal(QPoint(0, selectMonth->height()))))
    {
      QDate date;
      int day;
      // -----
      year=picker->getYear();
      date=table->getDate();
      day=calendar()->day(date);
      // ----- construct a valid date in this month:
      //date.setYMD(year, date.month(), 1);
      //date.setYMD(year, date.month(), QMIN(day, date.daysInMonth()));
      calendar()->setYMD(date, year, calendar()->month(date),
                       QMIN(day, calendar()->daysInMonth(date)));
      // ----- set this month
      setDate(date);
    } else {
      //KNotifyClient::beep();
    }
  delete popup;
}

void
KDatePicker::setEnabled(bool enable)
{
  QWidget *widgets[]= {
    yearForward, yearBackward, monthForward, monthBackward,
    selectMonth, selectYear,
    line, table, d->selectWeek };
  const int Size=sizeof(widgets)/sizeof(widgets[0]);
  int count;
  // -----
  for(count=0; count<Size; ++count)
    {
      widgets[count]->setEnabled(enable);
    }
}

void
KDatePicker::lineEnterPressed()
{
  QDate temp;
  // -----
  if(val->date(line->text(), temp)==QValidator::Acceptable)
    {
  //      kdDebug(298) << "KDatePicker::lineEnterPressed: valid date entered." << endl;
        emit(dateEntered(temp));
        setDate(temp);
    } else {
      //KNotifyClient::beep();
    //  kdDebug(298) << "KDatePicker::lineEnterPressed: invalid date entered." << endl;
    }
}

void
KDatePicker::todayButtonClicked()
{
  setDate(QDate::currentDate());
}

QSize
KDatePicker::sizeHint() const
{
  return QWidget::sizeHint();
}

void
KDatePicker::setFontSize(int s)
{
  QWidget *buttons[]= {
    // yearBackward,
    // monthBackward,
    selectMonth,
    selectYear,
    // monthForward,
    // yearForward
  };
  const int NoOfButtons=sizeof(buttons)/sizeof(buttons[0]);
  int count;
  QFont font;
  QRect r;
  // -----
  fontsize=s;
  for(count=0; count<NoOfButtons; ++count)
    {
      font=buttons[count]->font();
      font.setPointSize(s);
      buttons[count]->setFont(font);
    }
  QFontMetrics metrics(selectMonth->fontMetrics());

  for (int i = 1; ; ++i)
    {
      QString str = calendar()->monthName(i,
         calendar()->year(table->getDate()), false);
      if (str.isNull()) break;
      r=metrics.boundingRect(str);
      maxMonthRect.setWidth(QMAX(r.width(), maxMonthRect.width()));
      maxMonthRect.setHeight(QMAX(r.height(),  maxMonthRect.height()));
    }
  QStyleOptionToolButton so;
	so.init(selectMonth);
  QSize metricBound = style()->sizeFromContents(QStyle::CT_ToolButton,
                                               &so, maxMonthRect);
  selectMonth->setMinimumSize(metricBound);

  table->setFontSize(s);
}

void
KDatePicker::setCloseButton( bool enable )
{
    if ( enable == (d->closeButton != 0L) )
        return;

    if ( enable ) {
        d->closeButton = new QToolButton( d->tb );
        //d->navigationLayout->addWidget(d->closeButton);
        QToolTip::add(d->closeButton, QString("Close"));
        d->closeButton->setPixmap( QPixmap("remove.png") );
        connect( d->closeButton, SIGNAL( clicked() ),
                 topLevelWidget(), SLOT( close() ) );
    }
    else {
        delete d->closeButton;
        d->closeButton = 0L;
    }

    updateGeometry();
}

bool KDatePicker::hasCloseButton() const
{
    return (d->closeButton != 0L);
}

QSize KDatePicker::minimumSizeHint() const
{
	return QSize( 280, 260 );
}


