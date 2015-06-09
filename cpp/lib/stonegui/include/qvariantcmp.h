
#ifndef QVARIANT_CMP_H
#define QVARIANT_CMP_H

#include <qvariant.h>

#include "stonegui.h"

template<class T> int compareRetI( const T & a, const T & b )
{	return a > b ? 1 : (b > a ? -1 : 0); }

enum QVariantCompareFlags {
	NullsAbove = 0,
	NullsBelow = 1
};

STONEGUI_EXPORT int qVariantCmp( const QVariant & v1, const QVariant & v2, int flags = NullsAbove );

#endif // QVARIANT_CMP_H
