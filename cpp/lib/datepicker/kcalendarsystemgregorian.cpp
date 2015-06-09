/*
   Copyright (c) 2002 Carlos Moro <cfmoro@correo.uniovi.es>
   Copyright (c) 2002-2003 Hans Petter Bieker <bieker@kde.org>

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

// Derived gregorian kde calendar class
// Just a schema.

#include <qdatetime.h>
#include <qstring.h>

#include "kcalendarsystemgregorian.h"

KCalendarSystemGregorian s_calendar;

KCalendarSystem * calendar()
{
	return &s_calendar;
}

KCalendarSystemGregorian::KCalendarSystemGregorian()
{
  //kdDebug(5400) << "\nCreated gregorian calendar" << endl;
}

KCalendarSystemGregorian::~KCalendarSystemGregorian()
{
}

int KCalendarSystemGregorian::year(const QDate& date) const
{
  //kdDebug(5400) << "Gregorian year..." << endl;
  return date.year();
}

int KCalendarSystemGregorian::monthsInYear( const QDate & date ) const
{
  Q_UNUSED( date )

  //kdDebug(5400) << "Gregorian monthsInYear" << endl;

  return 12;
}

int KCalendarSystemGregorian::weeksInYear(int year) const
{
  QDate temp;
  temp.setYMD(year, 12, 31);

  // If the last day of the year is in the first week, we have to check the
  // week before
  if ( temp.weekNumber() == 1 )
    temp = temp.addDays(-7);

  return temp.weekNumber();
}

int KCalendarSystemGregorian::weekNumber(const QDate& date,
                                         int * yearNum) const
{
  return date.weekNumber(yearNum);
}

QString KCalendarSystemGregorian::monthName(const QDate& date,
                                            bool shortName) const
{
  return monthName(month(date), shortName);
}

QString KCalendarSystemGregorian::monthNamePossessive(const QDate& date, bool shortName) const
{
  return monthNamePossessive(month(date), shortName);
}

QString KCalendarSystemGregorian::monthName(int month, int year, bool shortName) const
{
  //kdDebug(5400) << "Gregorian getMonthName" << endl;
  Q_UNUSED(year);

  if ( shortName )
    switch ( month )
      {
      case 1:
        return "Jan";
      case 2:
        return "Feb";
      case 3:
        return "Mar";
      case 4:
        return "Apr";
      case 5:
        return "May";
      case 6:
        return "Jun";
      case 7:
        return "Jul";
      case 8:
        return "Aug";
      case 9:
        return "Sep";
      case 10:
        return "Oct";
      case 11:
        return "Nov";
      case 12:
        return "Dec";
      }
  else
    switch ( month )
      {
      case 1:
        return "January";
      case 2:
        return "February";
      case 3:
        return "March";
      case 4:
        return "April";
      case 5:
        return "May";
      case 6:
        return "June";
      case 7:
        return "July";
      case 8:
        return "August";
      case 9:
        return "September";
      case 10:
        return "October";
      case 11:
        return "November";
      case 12:
        return "December";
      }

  return QString::null;
}

QString KCalendarSystemGregorian::monthNamePossessive(int month, int year,
                                                      bool shortName) const
{
  //kdDebug(5400) << "Gregorian getMonthName" << endl;
  Q_UNUSED(year);

  if ( shortName )
    switch ( month )
      {
      case 1:
        return "of Jan";
      case 2:
        return "of Feb";
      case 3:
        return "of Mar";
      case 4:
        return "of Apr";
      case 5:
        return "of May";
      case 6:
        return "of Jun";
      case 7:
        return "of Jul";
      case 8:
        return "of Aug";
      case 9:
        return "of Sep";
      case 10:
        return "of Oct";
      case 11:
       return "of Nov";
      case 12:
        return "of Dec";
      }
  else
    switch ( month )
      {
      case 1:
        return "of January";
      case 2:
        return "of February";
      case 3:
        return "of March";
      case 4:
        return "of April";
      case 5:
        return "of May";
      case 6:
        return "of June";
      case 7:
        return "of July";
      case 8:
        return "of August";
      case 9:
        return "of September";
      case 10:
        return "of October";
      case 11:
        return "of November";
      case 12:
        return "of December";
      }

  return QString::null;
}

bool KCalendarSystemGregorian::setYMD(QDate & date, int y, int m, int d) const
{
  // We don't want Qt to add 1900 to them
  if ( y >= 0 && y <= 99 )
    return false;

  // QDate supports gregorian internally
  return date.setYMD(y, m, d);
}

QDate KCalendarSystemGregorian::addYears(const QDate & date, int nyears) const
{
  return date.addYears(nyears);
}

QDate KCalendarSystemGregorian::addMonths(const QDate & date, int nmonths) const
{
  return date.addMonths(nmonths);
}

QDate KCalendarSystemGregorian::addDays(const QDate & date, int ndays) const
{
  return date.addDays(ndays);
}

QString KCalendarSystemGregorian::weekDayName(int col, bool shortName) const
{
  // ### Should this really be different to each calendar system? Or are we
  //     only going to support weeks with 7 days?

  ////kdDebug(5400) << "Gregorian wDayName" << endl;
   if ( shortName )
    switch ( col )
      {
      case 1:  return "Mon";
      case 2:  return "Tue";
      case 3:  return "Wed";
      case 4:  return "Thu";
      case 5:  return "Fri";
      case 6:  return "Sat";
      case 7:  return "Sun";
      }
  else
    switch ( col )
      {
      case 1:  return "Monday";
      case 2:  return "Tuesday";
      case 3:  return "Wednesday";
      case 4:  return "Thursday";
      case 5:  return "Friday";
      case 6:  return "Saturday";
      case 7:  return "Sunday";
      }

  return QString::null;
}

QString KCalendarSystemGregorian::weekDayName(const QDate& date, bool shortName) const
{
  return weekDayName(dayOfWeek(date), shortName);
}


int KCalendarSystemGregorian::dayOfWeek(const QDate& date) const
{
  return date.dayOfWeek();
}

int KCalendarSystemGregorian::dayOfYear(const QDate & date) const
{
  return date.dayOfYear();
}

int KCalendarSystemGregorian::daysInMonth(const QDate& date) const
{
  //kdDebug(5400) << "Gregorian daysInMonth" << endl;
  return date.daysInMonth();
}

int KCalendarSystemGregorian::minValidYear() const
{
  return 1753; // QDate limit
}

int KCalendarSystemGregorian::maxValidYear() const
{
  return 8000; // QDate limit
}

int KCalendarSystemGregorian::day(const QDate& date) const
{
  return date.day();
}

int KCalendarSystemGregorian::month(const QDate& date) const
{
  return date.month();
}

int KCalendarSystemGregorian::daysInYear(const QDate& date) const
{
  return date.daysInYear();
}

int KCalendarSystemGregorian::weekDayOfPray() const
{
  return 7; // sunday
}

QString KCalendarSystemGregorian::calendarName() const
{
  return QString::fromLatin1("gregorian");
}

bool KCalendarSystemGregorian::isLunar() const
{
  return false;
}

bool KCalendarSystemGregorian::isLunisolar() const
{
  return false;
}

bool KCalendarSystemGregorian::isSolar() const
{
  return true;
}

int KCalendarSystemGregorian::yearStringToInteger(const QString & sNum, int & iLength) const
{
  int iYear;
  iYear = KCalendarSystem::yearStringToInteger(sNum, iLength);
  
  // Qt treats a year in the range 0-100 as 1900-1999.
  // It is nicer for the user if we treat 0-68 as 2000-2068
  if (iYear < 69)
    iYear += 2000;
  else if (iYear < 100)
    iYear += 1900;

  return iYear;
}
