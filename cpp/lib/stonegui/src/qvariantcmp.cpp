
#include <qdatetime.h>

#include "interval.h"

#include "qvariantcmp.h"

int qVariantCmp( const QVariant & v1, const QVariant & v2, int flags )
{
	bool n1 = v1.isNull(), n2 = v2.isNull();
	if( n1 || n2 )
		return compareRetI( (flags & NullsBelow) ? n2 : n1, (flags & NullsBelow) ? n1 : n2 );
	int t1 = v1.userType();
	int t2 = v2.userType();
	if( t1 != t2 ) return (int)t1 - (int)t2;
	switch( t1 ) {
		case QVariant::Bool:
			return compareRetI( int(v1.toBool() ? 1 : 0), int(v2.toBool() ? 1 : 0) );
		case QVariant::Date:
			return compareRetI( v1.toDate(), v2.toDate() );
		case QVariant::DateTime:
			return compareRetI( v1.toDateTime(), v2.toDateTime() );
		case QVariant::Double:
			return compareRetI( v1.toDouble(), v2.toDouble() );
		case QVariant::Int:
			return compareRetI( v1.toInt(), v2.toInt() );
		case QVariant::UInt:
			return compareRetI( v1.toUInt(), v2.toUInt() );
		case QVariant::LongLong:
			return compareRetI( v1.toLongLong(), v2.toLongLong() );
		case QVariant::ULongLong:
			return compareRetI( v1.toULongLong(), v2.toULongLong() );
		case QVariant::String: {
			QString s1(v1.toString()), s2(v2.toString());
			bool e1 = s1.isEmpty(), e2 = s2.isEmpty();
			if( e1 || e2 )
				return compareRetI( (flags & NullsBelow) ? e2 : e1, (flags & NullsBelow) ? e1 : e2 );
			return QString::localeAwareCompare( s1.toUpper(), s2.toUpper() );
		}
		case QVariant::Time:
			return compareRetI( v1.toTime(), v2.toTime() );
	}
	if( t1 == qMetaTypeId<Interval>() )
		return compareRetI( qvariant_cast<Interval>(v1), qvariant_cast<Interval>(v2) );
	return 0;
}
