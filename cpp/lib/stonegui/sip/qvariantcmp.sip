
%ModuleHeaderCode
#include "qvariantcmp.h"
%End

enum QVariantCompareFlags {
	NullsAbove = 0,
	NullsBelow = 1
};

int qVariantCmp( const QVariant & v1, const QVariant & v2, int flags = NullsAbove );
