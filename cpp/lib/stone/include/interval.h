
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
 * $Id$
 */

#ifndef INTERVAL_H
#define INTERVAL_H

#include <qdatetime.h>
#include <qstring.h>
#include <qvariant.h>

#include "blurqt.h"

class STONE_EXPORT Interval
{
public:
	Interval( const QDateTime &, const QDateTime & );
	Interval( const QDate &, const QDate & );
	Interval( int seconds = 0, int microseconds = 0 );
	Interval( int months, int days, int seconds, int microseconds = 0 );
	Interval( const QString & string );
	
	enum Order {
		Microseconds = 1,
		Milliseconds,
		Seconds,
		Minutes,
		Hours,
		Days,
		Months,
		Years,
		Decades,
		Centuries,
		Millenia
	};

	Interval operator+( const Interval & other ) const;
	Interval & operator+=( const Interval & other );
	Interval operator-( const Interval & other ) const;
	Interval & operator-=( const Interval & other );
	Interval operator*( double m ) const;
	Interval & operator*=( double m );
	Interval operator/( double d ) const;
	Interval & operator/=( double d );

	Interval abs() const;
	
	double operator/( const Interval & ) const;
	
	bool operator == ( const Interval & other ) const;
	bool operator != ( const Interval & other ) const;
	bool operator > ( const Interval & other ) const;
	bool operator < ( const Interval & other ) const;
	bool operator >= ( const Interval & other ) const;
	bool operator <= ( const Interval & other ) const;

	/// Returns only the fractional seconds of the interval in microseconds
	/// So '1.5 seconds' would return 500000
	int microseconds() const;
	
	/// Returns only the fractional seconds of the interval in milliseconds,
	/// Fractional milliseconds are dropped if floor is true, otherwise rounded to the nearest millisecond
	int milliseconds() const;

	/// returns the entire interval expressed in seconds
	qint64 seconds() const;

	int minutes() const;

	int hours() const;

	/// Returns the number of days in the interval
	/// Fractional days are dropped if floor is true, otherwise the value
	/// returned is rounded to the nearest day.
	int days() const;

	int months() const;

	/// Returns the number of years
	/// Fractional years are dropped if floor is true, otherwise rounded to the nearest year
	int years() const;

	enum ToStringFlags {
		ChopMaximum = 1,
		ChopMinimum = 2,
		TrimMinimum = 4,
		TrimMaximum = 8,
		PadHours = 16
	};

	qint64 asOrder( Order order ) const;
	
	// Same as toString( Years, Microseconds, TrimMinimum | TrimMaximum ), which is compatible with postgres input/output.
	QString toString() const;

	QString toString( Order maximumOrder, Order minimumOrder = Seconds, int flags = TrimMinimum | TrimMaximum ) const;

	/// Same as toString, except will write "1 hour", "12 minutes", "25 seconds", in the cases where the
	/// interval is an even number of hours, minutes OR seconds
	QString toDisplayString() const;

	Interval addMillenia( double );
	Interval addCenturies( double );
	Interval addYears( double );
	Interval addMonths( double );
	Interval addWeeks( double );
	Interval addDays( double );
	Interval addHours( double );
	Interval addMinutes( double );
	Interval addSeconds( double );
	Interval addMilliseconds( double );
	Interval addMicroseconds( qint64 );

	QDateTime adjust( const QDateTime & ) const;

	static Interval fromString( const QString &, bool * valid = 0 );

	static QString reformat( const QString &, Order maximumOrder = Years, Order minimumOrder = Microseconds, int orderFlags = TrimMinimum | TrimMaximum, bool * success = 0 );

	static int compare( const Interval & i1, const Interval & i2 );

protected:
	qint64 mMicroseconds;
	int mDays, mMonths;
};

Q_DECLARE_METATYPE(Interval)

#endif // INTERVAL_H

