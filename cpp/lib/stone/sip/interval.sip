
class Interval
{
%TypeHeaderCode
#include <interval.h>
%End
public:
	Interval();
	Interval( const QDateTime &, const QDateTime & );
	Interval( const QDate &, const QDate & );
	Interval( int seconds, int microseconds = 0 );
	Interval( int months, int days, int seconds, int microseconds = 0 );
	Interval( const QString & );

	Interval operator+( const Interval & other ) const;
	//Interval & operator+=( const Interval & other );
	Interval operator-( const Interval & other ) const;
	//Interval & operator-=( const Interval & other );
	Interval operator*( double m ) const;
	//Interval & operator*=( double m );
	Interval operator/( double d ) const;
	//Interval & operator/=( double d );
	double operator/( const Interval & ) const;

	Interval abs() const;

	bool operator == ( const Interval & other ) const;
	bool operator != ( const Interval & other ) const;
	bool operator > ( const Interval & other ) const;
	bool operator < ( const Interval & other ) const;
	bool operator >= ( const Interval & other ) const;
	bool operator <= ( const Interval & other ) const;

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

	int microseconds() const;
	
	int milliseconds() const;

	qint64 seconds() const;

	int minutes() const;

	int hours() const;

	int days() const;

	int months() const;

	int years() const;

	enum OrderFlag {
		ChopMaximum = 1,
		ChopMinimum = 2,
		TrimMinimum = 4,
		TrimMaximum = 8
	};

	qint64 asOrder( Order order ) const;

	QString toString() const;

	QString toString( Order maximumOrder, Order minimumOrder = Microseconds, int orderFlags = TrimMinimum | TrimMaximum ) const;
	
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
	Interval addMicroseconds( int );

	QDateTime adjust( const QDateTime & ) const;

	static Interval fromString( const QString &, bool * success = 0 );

	static QString reformat( const QString & intervalString, Order maximumOrder = Years, Order minimumOrder = Microseconds, int orderFlags = TrimMinimum | TrimMaximum, bool * success = 0 );

	static int compare( const Interval & i1, const Interval & i2 );

};
