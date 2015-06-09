
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

#ifndef LIB_BLUR_QT_H
#define LIB_BLUR_QT_H

#include <qstring.h>
#include <qdebug.h>
#include <qstringlist.h>

#ifdef STONE_MAKE_DLL
#define STONE_EXPORT Q_DECL_EXPORT
#else
#define STONE_EXPORT Q_DECL_IMPORT
#endif

class Multilog;
class IniConfig;

/**
  * \defgroup Stone Stone - Library for database access, and other common functionality
  * 
  * Stone is an ORM ( Object Relational Mapper ) library. It provides an abstraction
  * layer between application logic and a relational database for data. Using PyQt
  * you can get easy access to centralized business logic from C++ and Python.
  *
  */

/** \brief Initializes the library, and parses options.
 *
 *  The options and their descriptions follow, unkown options are ignored
 *
 * -show-sql
 *		Outputs all executed sql to stdout
 *	-create-database
 *		Creates all missing tables in the database, then exits
 *	-verify-database
 *		Verifies all tables in the database, then exits
 *	-output-schema FILE
 *		Outputs the database schema in xml format to FILE
 *	-db-host HOST
 *		Set the database host to HOST. Can be either a hostname or an ipv4 address
 *	-db-port PORT
 *		Set the database port to PORT
 *	-db-user USER
 *		Set the database username to USER
 *	-db-password PASS
 *		Set the database password to PASS
 *	-rum-host HOST
 *		Set the update server host to HOST. Can be either a hostname or an ipv4 address
 *	-rum-port PORT
 *		Set the update server port to PORT
 */
STONE_EXPORT void initStone( int argc = 0, char ** argv = 0 );
STONE_EXPORT void initStone( const QStringList & args );

/** \brief Returns a string representation of the options listed for the initStone function. */
STONE_EXPORT QString stoneOptionsHelp();

/** \brief Opens config file with 'configName' */
STONE_EXPORT bool initConfig( const QString & configName, const QString & logfile = QString() );

/** \brief Opens user's config file with 'userConfigName' */
STONE_EXPORT void initUserConfig( const QString & userConfigFile );

/** \brief Writes out config file */
STONE_EXPORT void shutdown();

/** \brief Returns a reference to the config object */
STONE_EXPORT IniConfig & config();
/** \brief Returns a reference to the user's config object */
STONE_EXPORT IniConfig & userConfig();

/** \brief Returns a pointer to the default Multilog instance for this process */
STONE_EXPORT Multilog * log();

/** \brief Returns the userName of the current user */
STONE_EXPORT QString getUserName();

/** \brief Returns the usernames logged in on the current machine */
STONE_EXPORT QStringList getLoggedInUsers();

/** \brief Logs a message to the default MultiLog instance */
STONE_EXPORT void Log( const QString & message, int severity = 1, const QString & file = QString() );

STONE_EXPORT bool sendEmail( QStringList recipients, const QString & subject, const QString & body, const QString & sender, QStringList attachments = QStringList() );

STONE_EXPORT QStringList getBackTrace();
STONE_EXPORT void printBackTrace();

#ifdef _MSC_VER
#define __FUNC__ __FUNCTION__ 
#define snprintf _snprintf
#define llabs _abs64
#endif

#ifdef __GNUC__
#define __get_loc__ (QString(__FILE__) + ":" + QString::number(__LINE__) + " " + __PRETTY_FUNCTION__)
#else
#define __get_loc__ (QString(__FILE__) + ":" + QString::number(__LINE__) + " " + __FUNC__)
#endif
#define LOG_1( m ) Log( m, 1, __get_loc__ )
#define LOG_3( m ) Log( m, 3, __get_loc__ )
#define LOG_5( m ) Log( m, 5, __get_loc__ )
#define LOG_6( m ) Log( m, 6, __get_loc__ )
#define LOG_TRACE Log( "", 3, __get_loc__ );

/// @}

struct Warner {
	Warner( const QString & func )
	:mFunc(func)
		{ qWarning() << "Entering " << func; }
	~Warner(){ qWarning() << "Leaving " << mFunc; }
	QString mFunc;
};

#endif // LIB_BLUR_QT_H

