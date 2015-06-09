

#ifndef DATE_UTIL_H
#define DATE_UTIL_H

#include <qdatetime.h>

#include "blurqt.h"

STONE_EXPORT bool isHoliday( const QDate & );
STONE_EXPORT bool isWorkday( const QDate & );

#endif // DATE_UTIL_H
