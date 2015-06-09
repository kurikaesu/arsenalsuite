

#ifndef __CLASSES_H__
#define __CLASSES_H__

#include <qobject.h>

#ifdef BACH_MAKE_DLL
#define BACH_EXPORT Q_DECL_EXPORT
#else
#define BACH_EXPORT Q_DECL_IMPORT
#endif

/**
 * \defgroup Classes Classes - auto-generated classes to access database tables
 * \details Classes are defined using the app @ref classmaker
 * All fields defined have mutators, and indexes have static methods to retrieve a @ref RecordList of @ref stone objects.
 * Additional methods for a database class can be defined in the base/ directory.
 */
BACH_EXPORT void bach_loader();

namespace Stone {
class Database;
class Schema;
}
using namespace Stone;

BACH_EXPORT Schema * bachSchema();
BACH_EXPORT Database * bachDb();

#endif // CLASSES_H
