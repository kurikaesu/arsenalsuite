
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

#ifndef MULTILOG_H
#define MULTILOG_H

#include <qstring.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qtextstream.h>
#include <qregexp.h>
#include <qmutex.h>

#include "blurqt.h"
//#include <qmessagebox.h>
//#include <qobject.h>
/**
 * creates/opens a logfile of logfile_current.txt in given dir. when file reaches
 * maxsize, renames it with timestamp of last entry, opens new file and rotates
 * old logs out so no more than maxfiles are ever saved
 * \ingroup Stone
 */
class STONE_EXPORT Multilog : public QObject
{
Q_OBJECT
public:

	Multilog( const QString & logfile, bool stdout_option = false, int severity = 1, int maxfiles = 10, unsigned int maxsize = 2097152);
	
	~Multilog();

	/** Logs a message to the logfile.  The message will only be logged
	 *  if the severity is <= loglevel, and if the file matches the file
	 *  filter, if the filter is set. */
	void log( const int severity, const QString & logmessage, const QString & file = QString(), const QString & function = QString() );

	/** If filesFilter is set to a valid regular expression, then
	 *  only log messages that come from a file that matches the
	 *  expression will be logged */
	QRegExp filesFilter() const;
	void setFilesFilter( const QRegExp & );

/*	QRegExp functionsFilter() const;
	void setFunctionsFilter( const QRegExp & ); */

	/**  Sets whether to echo the log data to stdout */
	void setStdOut( bool stdout_option );
	bool stdOut() const;

	/**  if loglevel of message is less than or equal to desired loglevel, message will be logged
	 */
	void setLogLevel(int severity );
	int logLevel() const;

	/**  \param maxfiles sets the number of log files to be kept in the directory
	 *	 all other logfiles that match the logfile name passed to the ctor will
	 *   be deleted
	 */
	void setMaxFiles( int maxfiles );
	int maxFiles() const;

	/** maxsize is maximum size of each individual logfile before auto-rotation occurs
	 */
	void setMaxSize( unsigned int maxsize );
	int maxSize() const;

	QString logFileName() { return mLogFileName; }
signals:
	void logged( const QString & );

protected:
	bool rotate( int buffer );
	void renameCurrentLog();
	void removeOldLogs();
	bool openNewLog();

	bool copy_logfile( QString inFileStr, QString outFileStr );

	QString mLogFileName, mLogMessage;
	bool mEchoStdOut;
	int mSeverity, mMaxFiles;
	unsigned int mMaxSize;
	QFileInfo mLogFileInfo;
	QDir mLogFileDir;
	int mLogFileSize;

	QFile * mLogFile;
	QTextStream * mLogStream;
	QRegExp mFileFilter, mFunctionFilter;
	QMutex mMutex;
};

#endif

