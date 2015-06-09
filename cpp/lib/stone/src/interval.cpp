
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

#include <stdlib.h>

#include <qregexp.h>

#include "blurqt.h"
#include "interval.h"

#define MICROSECONDS_PER_SECOND 1000000
#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR 3600
#define MINUTES_PER_DAY 1440
#define SECONDS_PER_DAY (MINUTES_PER_DAY * SECONDS_PER_MINUTE)
#define MICROSECONDS_PER_MINUTE (SECONDS_PER_MINUTE * qint64(MICROSECONDS_PER_SECOND))
#define MICROSECONDS_PER_DAY (qint64(SECONDS_PER_DAY) * qint64(MICROSECONDS_PER_SECOND))
#define HOURS_PER_DAY 24
#define MONTHS_PER_YEAR 12
#define MONTHS_PER_DECADE 120
#define MONTHS_PER_CENTURY 1200
#define MONTHS_PER_MILLENIUM 12000

Interval::Interval( const QDateTime & first, const QDateTime & second )
: mMicroseconds( 0 ), mDays( 0 ), mMonths( 0 )
{
	*this = addSeconds(first.secsTo(second));
}

Interval::Interval( const QDate & first, const QDate & second )
: mMicroseconds( 0 ), mDays( 0 ), mMonths( 0 )
{
	*this = addDays(first.daysTo(second));
}

Interval::Interval( int seconds, int microseconds )
: mMicroseconds( 0 ), mDays( 0 ), mMonths( 0 )
{
	*this = addMicroseconds(microseconds);
	*this = addSeconds(seconds);
}

Interval::Interval( int months, int days, int seconds, int microseconds )
: mMicroseconds( qint64(seconds) * MICROSECONDS_PER_SECOND + microseconds ), mDays( days ), mMonths( months )
{
}

Interval::Interval( const QString & string )
{
	*this = Interval::fromString(string);
}

Interval Interval::operator+( const Interval & other ) const
{
	Interval ret(*this);
	ret = ret.addMicroseconds( other.mMicroseconds );
	ret = ret.addDays( other.mDays );
	ret = ret.addMonths( other.mMonths );
	return ret;
}

Interval & Interval::operator+=( const Interval & other )
{
	*this = operator+( other );
	return *this;
}

Interval Interval::operator-( const Interval & other ) const
{
	Interval ret(*this);
	ret = ret.addMicroseconds( -other.mMicroseconds );
	ret = ret.addDays( -other.mDays );
	ret = ret.addMonths( -other.mMonths );
	return ret;
}

Interval & Interval::operator-=( const Interval & other )
{
	*this = operator-(other);
	return *this;
}

Interval Interval::operator*( double m ) const
{
	Interval ret;
	ret = ret.addMicroseconds( qint64(mMicroseconds * m) );
	ret = ret.addDays( mDays * m );
	ret = ret.addMonths( mMonths * m );
	return ret;
}

Interval & Interval::operator*=( double m )
{
	*this = operator*(m);
	return *this;
}

Interval Interval::operator/( double d ) const
{
	Interval ret;
	ret = ret.addMicroseconds( qint64(mMicroseconds / d) );
	ret = ret.addDays( mDays / d );
	ret = ret.addMonths( mMonths / d );
	return ret;
}

Interval & Interval::operator/=( double d )
{
	*this = operator/(d);
	return *this;
}

Interval Interval::abs() const
{
	if( *this > Interval() )
		return *this;
	return this->operator*(-1.0);
}

double Interval::operator/( const Interval & other ) const
{
	return asOrder(Microseconds) / double(other.asOrder(Microseconds));
}

int Interval::compare( const Interval & i1, const Interval & i2 )
{
	for( int order = (int)Millenia; order >= (int)Microseconds; order-- ) {
		qint64 v1 = i1.asOrder(Interval::Order(order)), v2 = i2.asOrder(Interval::Order(order));
		if( v1 > v2 ) return 1;
		if( v2 > v1 ) return -1;
	}
	return 0;
}

bool Interval::operator == ( const Interval & other ) const
{
	return compare( *this, other ) == 0;
}

bool Interval::operator != ( const Interval & other ) const
{
	return compare( *this, other ) != 0;
}

bool Interval::operator > ( const Interval & other ) const
{
	return compare( *this, other ) > 0;
}

bool Interval::operator < ( const Interval & other ) const
{
	return compare( *this, other ) < 0;
}

bool Interval::operator >= ( const Interval & other ) const
{
	return compare( *this, other ) >= 0;
}

bool Interval::operator <= ( const Interval & other ) const
{
	return compare( *this, other ) <= 0;
}

int Interval::microseconds() const
{ return mMicroseconds; }

int Interval::milliseconds() const
{
	return mMicroseconds / 1000;
}

qint64 Interval::seconds() const
{
	return mMicroseconds / MICROSECONDS_PER_SECOND;
}

int Interval::minutes() const
{
	return mMicroseconds / MICROSECONDS_PER_MINUTE;
}

int Interval::hours() const
{
	return seconds() / SECONDS_PER_HOUR;
}

int Interval::days() const
{
	return mDays;
}

int Interval::months() const
{
	return mMonths;
}

int Interval::years() const
{
	return mMonths / MONTHS_PER_YEAR;
}

QString aip( const QString & s, qint64 value, const QString & type, bool skipZero )
{
	if( skipZero && value == 0 ) return s;
	QString ret = s;
	if( s.size() > 0 )
		ret += " ";
	ret = ret + QString::number( value ) + type;
	if( llabs(value) > 1 ) ret += "s";
	return ret;
}

QString padString( qint64 num, int padSize = 2 )
{
	QString ret = QString::number( num );
	while( ret.size() < padSize )
		ret = "0" + ret;
	return ret;
}

QString Interval::toString() const
{ return toString( Years, Microseconds ); }

qint64 Interval::asOrder( Order order ) const
{
	qint64 months = mMonths, days = mDays, micros = mMicroseconds;

	if ( order < Months ) { days += (months / 12) * 365 + (months % 12) * 30; months = 0; }
	if ( order < Days ) { micros += days * MICROSECONDS_PER_DAY; days = 0; }
	if ( order >= Days ) { days += micros / MICROSECONDS_PER_DAY; }
	if ( order >= Months ) { months += days / 30; }

	switch( order ) {
		case Millenia:
			return months / MONTHS_PER_MILLENIUM;
		case Centuries:
			return months / MONTHS_PER_CENTURY;
		case Decades:
			return months / MONTHS_PER_DECADE;
		case Years:
			return months / 12;
		case Months:
			return months;
		case Days:
			return days;
		case Hours:
			return micros / (MICROSECONDS_PER_MINUTE * 60);
		case Minutes:
			return micros / MICROSECONDS_PER_MINUTE;
		case Seconds:
			return micros / MICROSECONDS_PER_SECOND;
		case Milliseconds:
			return micros / 1000;
		case Microseconds:
			return micros;
	}
	return 0;
}


QString Interval::toString( Order maximumOrder, Order minimumOrder, int flags ) const
{
	QString ret;
	qint64 months = mMonths, days = mDays, micros = mMicroseconds;
	qint64 millis = 0, seconds = 0, hours = 0, minutes = 0, millenia = 0, centuries = 0, decades = 0;
	bool isMinus = false;
	bool trimMax = flags & TrimMaximum;
#define TRIM_MIN ((flags & TrimMinimum) && months == 0 && days == 0 && micros == 0)
	if( maximumOrder < Seconds ) maximumOrder = Seconds;

	if( !(flags & ChopMaximum) ) {
		if ( maximumOrder < Months ) { days += (months / 12) * 365 + (months % 12) * 30; months = 0; }
		if ( maximumOrder < Days ) { micros += days * MICROSECONDS_PER_DAY; days = 0; }
	}
	switch( maximumOrder ) {
		case Millenia:
			millenia = months / MONTHS_PER_MILLENIUM;
			months %= MONTHS_PER_MILLENIUM;
			ret = aip( ret, millenia, " millenia", trimMax );
			if( minimumOrder == Millenia || TRIM_MIN ) break;
		case Centuries:
			centuries = months / MONTHS_PER_CENTURY;
			months %= MONTHS_PER_CENTURY;
			ret = aip( ret, centuries, " centuries", trimMax );
			if( minimumOrder == Centuries || TRIM_MIN ) break;
		case Decades:
			decades = months / MONTHS_PER_DECADE;
			months %= MONTHS_PER_DECADE;
			ret = aip( ret, decades, " decade", trimMax );
			if( minimumOrder == Decades || TRIM_MIN ) break;
		case Years:
			ret = aip( ret, months / 12, " year", trimMax );
			months %= MONTHS_PER_YEAR;
			if( minimumOrder == Years || TRIM_MIN ) break;
		case Months:
			if( flags & ChopMaximum ) months %= MONTHS_PER_YEAR;
			ret = aip( ret, months, " mon", trimMax );
			months = 0;
			if( minimumOrder == Months || TRIM_MIN ) break;
		case Days:
			ret = aip( ret, days, " day", trimMax );
			days = 0;
			if( minimumOrder == Days || TRIM_MIN ) break;
		case Hours:
			isMinus = (micros < 0);
			if( isMinus )
				micros = llabs(micros);
			hours = micros / (MICROSECONDS_PER_MINUTE * 60);
			micros %= (MICROSECONDS_PER_MINUTE * 60);
			if( minimumOrder == Hours || TRIM_MIN )
				ret = aip( ret + (isMinus ? "-" : ""), hours, " hour", trimMax );
			else {
				if( !ret.isEmpty() ) ret += " ";
				if( isMinus ) ret += "-";
				ret += padString( hours, (flags & PadHours) ? 2 : 0 ) + ":";
			}
			if( minimumOrder == Hours || TRIM_MIN ) break;
		case Minutes:
			if( micros < 0 && hours == 0 )
				ret += "-";
			minutes = llabs(micros / MICROSECONDS_PER_MINUTE);
			micros %= MICROSECONDS_PER_MINUTE;
			ret += padString( minutes, 2 );
			// If we display minutes always display seconds even if zero
			//if( minimumOrder == Minutes || TRIM_MIN ) break;
			ret += ":";
		case Seconds:
			if( micros < 0 && hours == 0 && minutes == 0 )
				ret += "-";
			seconds = micros / MICROSECONDS_PER_SECOND;
			micros %= MICROSECONDS_PER_SECOND;
			ret += padString( seconds, 2 );
			if( minimumOrder == Seconds || TRIM_MIN ) break;
			ret += ".";
		case Milliseconds:
			millis = micros / 1000;
			micros %= 1000;
			ret += padString(millis, 3);
			if( minimumOrder == Milliseconds || TRIM_MIN ) break;
		case Microseconds:
			ret += padString(micros, 3);
			break;
	}
	if( ret.isEmpty() )
		ret = "00:00:00";
	return ret;
}

QString Interval::toDisplayString() const
{
	if( mDays == 0 && mMonths == 0 ) {
		qint64 _hours = hours(), _minutes = minutes(), _seconds = seconds();
		if( _hours > 0 && (_minutes % 60 == 0) && (_seconds % 3600 == 0) )
			return aip( "", _hours, " hour", false );
		if( _hours == 0 && (_minutes > 0) && (_seconds % 60 == 0) )
			return aip( "", _minutes, " minute", false );
		if( _hours == 0 && _minutes == 0 )
			return aip( "", _seconds, " second", false );
	}
	return toString();
}

Interval Interval::fromString( const QString & iso, bool * valid )
{
	bool parseDayString = true;
	bool parseTimeString = true;
	Interval ret;
	QString is(iso);
	QRegExp mel( "^([-\\d\\.]+)\\s+(millennia|millennium)", Qt::CaseInsensitive );
	if( is.contains( mel ) ) {
		ret = ret.addMillenia( mel.cap(1).toDouble() );
		is = is.remove(mel).simplified();
	}
	QRegExp cent( "^([-\\d\\.]+) (centuries|century)", Qt::CaseInsensitive );
	if( is.contains(cent) ) {
		ret = ret.addCenturies( cent.cap(1).toDouble() );
		is = is.remove(cent).simplified();
	}
	QRegExp dec( "^([-\\d\\.]+) (decades?)", Qt::CaseInsensitive );
	if( is.contains(dec) ) {
		ret = ret.addYears( dec.cap(1).toDouble() * 10.0 );
		is = is.remove(dec).simplified();
	}
	QRegExp yr( "^([-\\d\\.]+) (years?)", Qt::CaseInsensitive );
	if( is.contains(yr) ) {
		ret = ret.addYears( yr.cap(1).toDouble() );
		is = is.remove(yr).simplified();
	}
	QRegExp mnth( "^([-\\d\\.]+) (months?)", Qt::CaseInsensitive );
	if( is.contains(mnth) ) {
		ret = ret.addMonths( mnth.cap(1).toDouble() );
		is = is.remove(mnth).simplified();
	}
	QRegExp week( "^([-\\d\\.]+) (weeks?)", Qt::CaseInsensitive );
	if( is.contains(week) ) {
		ret = ret.addWeeks( week.cap(1).toDouble() );
		is = is.remove(week).simplified();
	}
	QRegExp day( "^([-\\d\\.]+) (days?)", Qt::CaseInsensitive );
	if( is.contains(day) ) {
		ret = ret.addDays( day.cap(1).toDouble() );
		is = is.remove(day).simplified();
		parseDayString = false;
	}
	QRegExp hours( "^([-\\d\\.]+) (hours?)", Qt::CaseInsensitive );
	if( is.contains(hours) ) {
		ret = ret.addHours( hours.cap(1).toDouble() );
		is = is.remove(hours).simplified();
		parseTimeString = false;
	}
	QRegExp mins( "^([-\\d\\.]+) (minutes?)", Qt::CaseInsensitive );
	if( is.contains(mins) ) {
		ret = ret.addMinutes( mins.cap(1).toInt() );
		is = is.remove(mins).simplified();
		parseTimeString = false;
	}
	QRegExp seconds( "^([-\\d\\.]+) (seconds?)", Qt::CaseInsensitive );
	if( is.contains(seconds) ) {
		ret = ret.addSeconds( seconds.cap(1).toDouble() );
		is = is.remove(seconds).simplified();
		parseTimeString = false;
	}
	QRegExp ms( "^([-\\d\\.]+) (milliseconds?)", Qt::CaseInsensitive );
	if( is.contains(ms) ) {
		ret = ret.addMilliseconds( ms.cap(1).toDouble() );
		is = is.remove(ms).simplified();
		parseTimeString = false;
	}
	QRegExp micro( "^([-\\d\\.]+) (microseconds?)", Qt::CaseInsensitive );
	if( is.contains(micro) ) {
		ret = ret.addMicroseconds( (int)micro.cap(1).toDouble() );
		is = is.remove(micro).simplified();
		parseTimeString = false;
	}
	if( parseTimeString ) {
		QStringList timeParts = is.split(':');
		QList<bool> hasDec;
		bool valid = true;
		foreach( QString part, timeParts ) {
			part.toDouble(&valid);
			if( !valid ) break;
			hasDec.append(part.contains( '.' ));
				
		}
		while( valid ) {
			if( timeParts.size() == 1 ) {
				ret = ret.addSeconds( timeParts[0].toDouble() );
			}
			else if( timeParts.size() == 2 ) {
				if( hasDec[0] || hasDec[1] ) {
					valid = false;
					break;
				}
				ret = ret.addHours( timeParts[0].toInt() );
				ret = ret.addMinutes( timeParts[1].toInt() );
			}
			else if( timeParts.size() == 3 ) {
				if( (hasDec[0] && hasDec[2]) || hasDec[1] ) {
					valid = false;
					break;
				}
				int pos = 0;
				if( hasDec[0] ) {
					if( !parseDayString ) {
						valid = false;
						break;
					}
					ret = ret.addDays( timeParts[pos++].toDouble() );
				}
				ret = ret.addHours( timeParts[pos++].toInt() );
				ret = ret.addMinutes( timeParts[pos++].toInt() );
				if( pos < 3 )
					ret = ret.addSeconds( timeParts[2].toDouble() );
			}
			else if( timeParts.size() == 4 ) {
				if( hasDec[1] || hasDec[2] || !parseDayString ) {
					valid = false;	
					break;
				}
				ret = ret.addDays( timeParts[0].toDouble() );
				ret = ret.addHours( timeParts[1].toInt() );
				ret = ret.addMinutes( timeParts[2].toInt() );
				ret = ret.addSeconds( timeParts[3].toDouble() );
			}
			break;
		}
		if( valid ) is = QString();
	}
	if( !is.isEmpty() ) {
		LOG_5( "Invalid format " + is );
		if( valid ) *valid = false;
		return Interval();
	}
	if( valid ) *valid = true;
	return ret;
}

Interval Interval::addMillenia( double m )
{
	return addYears( m * 1000 );
}

Interval Interval::addCenturies( double c )
{
	return addYears( c * 100 );
}

Interval Interval::addYears( double y )
{
	return addMonths( y * 12.0 );
}

Interval Interval::addMonths( double m )
{
	Interval ret = *this;
	int mi = int(m);
	ret.mMonths += mi;
	if( m - double(mi) != 0 )
		ret = ret.addDays( (m - double(mi)) * 30.0 );
	return ret;
}

Interval Interval::addWeeks( double w )
{
	return addDays( w * 7 );
}

Interval Interval::addDays( double d )
{
	Interval ret = *this;
	int di = int(d);
	ret.mDays += di;
	if( d - double(di) != 0 )
		ret = ret.addSeconds( (d - double(di)) * double(SECONDS_PER_DAY) );
	return ret;
}

Interval Interval::addHours( double h )
{
	return addSeconds( h * SECONDS_PER_HOUR );
}

Interval Interval::addMinutes( double m )
{
	return addSeconds( m * SECONDS_PER_MINUTE );
}

Interval Interval::addSeconds( double s )
{
	return addMicroseconds( qint64(double(MICROSECONDS_PER_SECOND) * s) );
}

Interval Interval::addMilliseconds( double m )
{
	return (*this).addMicroseconds( qint64(m * 1000.0) );
}

Interval Interval::addMicroseconds( qint64 m )
{
	Interval ret = *this;
	ret.mMicroseconds += m;
	if( llabs(ret.mMicroseconds) >= MICROSECONDS_PER_DAY ) {
		ret = ret.addDays( ret.mMicroseconds / MICROSECONDS_PER_DAY );
		ret.mMicroseconds %= MICROSECONDS_PER_DAY;
	}
	return ret;
}

QDateTime Interval::adjust( const QDateTime & dt ) const
{
	return dt.addMonths( mMonths ).addDays( mDays ).addMSecs( mMicroseconds / 1000 );
}

QString Interval::reformat( const QString & is, Order maximumOrder, Order minimumOrder, int orderFlags, bool * success )
{
	bool valid;
	Interval i = fromString(is, &valid);
	if( success ) *success = valid;
	if( !valid ) return is;
	return i.toString( maximumOrder, minimumOrder, orderFlags );
}

