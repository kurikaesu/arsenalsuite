

#ifndef __CLASSES_H__
#define __CLASSES_H__

#include <qobject.h>

#ifdef CLASSES_MAKE_DLL
#define CLASSES_EXPORT Q_DECL_EXPORT
#else
#define CLASSES_EXPORT Q_DECL_IMPORT
#endif

namespace Stone {
class Database;
class Schema;
}
using namespace Stone;

/**
 * \defgroup Classes Classes - auto-generated classes to access database tables
 * \details Classes are defined using the app @ref classmaker
 * All fields defined have mutators, and indexes have static methods to retrieve a @ref RecordList of @ref stone objects.
 * Additional methods for a database class can be defined in the base/ directory.
 */
CLASSES_EXPORT void classes_loader();

CLASSES_EXPORT Schema * classesSchema();

CLASSES_EXPORT Database * classesDb();

#endif // CLASSES_H
