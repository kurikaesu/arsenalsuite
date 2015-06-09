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

#include <qdatetime.h>
#include <qtextstream.h>
#include <qapplication.h>
#include <qiodevice.h>

#include "multilog.h"
#include "iniconfig.h"
#include "process.h"

Multilog::Multilog( const QString & logfile, bool stdout_option, int severity, int maxfiles, unsigned int maxsize)
: mLogFileName( logfile )
, mEchoStdOut( stdout_option )
, mSeverity( severity )
, mMaxFiles( maxfiles )
, mMaxSize( maxsize)
, mLogFile( 0 )
, mLogStream( 0 )
, mLogFileSize( 0 )
{
	mLogFileInfo.setCaching( false );
	mLogFileInfo.setFile( mLogFileName );
	mLogFileInfo.refresh();
	mLogFileSize = mLogFileInfo.size();
	mLogFileDir = mLogFileInfo.dir(); //abs path of dir

	rotate( qMin(2000,(int)mMaxSize) );
}

Multilog::~Multilog()
{
	if( mLogFile ) {
		mLogFile->flush();
		mLogFile->close();
	}
	delete mLogFile;
	delete mLogStream;
}

void Multilog::renameCurrentLog()
{
	//close logfile, rename with date-timestamp in filename
	
	QString oldFileStr = mLogFileName;
	QString newFileStr = mLogFileInfo.path() + "/" + mLogFileInfo.completeBaseName() 
			+ QDateTime::currentDateTime().toString( ".yyyy-MM-ddThh.mm.ss.log" );

	delete mLogStream;
	mLogStream = 0;
	
	delete mLogFile;
	mLogFile = 0;

	qWarning("Multilog::rotate: trying to rename %s to %s", qPrintable(oldFileStr), qPrintable(newFileStr));

	if ( !QDir::current().rename( oldFileStr, newFileStr ) )
		qWarning("Multilog::rotate renaming failed! Giving up." );
	else
		qWarning("Multilog::rotate rename successful.");
}
	
void Multilog::removeOldLogs()
{
	QString logFileDirFilter = mLogFileInfo.completeBaseName() + ".*.log";
	//scan mLogFileDir, sort by datestr in filenames, nuke any old logfiles over maxfiles limit
	QFileInfoList fil = mLogFileDir.entryInfoList( QStringList() << logFileDirFilter, QDir::Files, QDir::Name | QDir::Reversed );
	
	int currentFile = 0;
	foreach( QFileInfo fi, fil ) {
		QString tempLogFileStr = fi.filePath();
		currentFile++;
		if (currentFile > (mMaxFiles - 1)) { // 1st logfile is always current logfile.
			QFile tempFile(tempLogFileStr);
			if( tempFile.remove() )
				qWarning( "Multilog::rotate delete logfile (# %s, over MaxFiles limit): %s", qPrintable( QString::number(currentFile)), qPrintable(tempLogFileStr) );
			else
				qWarning( "Multilog::rotate delete logfile (# %s) failed! -- %s",  qPrintable(QString::number(currentFile)),  qPrintable(tempFile.errorString()) );
		}
	}
}

bool Multilog::rotate( int buffer )
{
	mLogFileSize += buffer;
	if ( mLogFileSize > mMaxSize) {
		renameCurrentLog();
		removeOldLogs();
	}

	//open logfile if not already open
	return openNewLog();
}

bool Multilog::openNewLog()
{
	if ( !mLogFile ) {
		mLogFile = new QFile(mLogFileName);
		mLogFileInfo.setFile( mLogFileName );
		mLogFileInfo.refresh();
		mLogFileSize = 0;
	}

	if( !mLogFile->isOpen() ) {
		if ( !mLogFile->open(QIODevice::Append) ) {
			delete mLogFile;
#ifdef Q_OS_WIN
			QString tmp("c:/temp/" + QString::number( processID() ) + ".log" );
#else
			QString tmp("/tmp/" + QString::number( processID() ) + ".log" );
#endif
			mLogFile = new QFile(tmp);
			if( mLogFile->open(QIODevice::WriteOnly) )
				qWarning( "Opened temp log file at %s", qPrintable(tmp) );
			else {
				delete mLogFile;
				mLogFile = 0;
			}
		}
		if ( mLogFile )
			mLogStream = new QTextStream( mLogFile );
		else
			return false;
	}

	return true;
}

void Multilog::log( const int severity, const QString & logmessage, const QString & file, const QString & /* function */ )
{
	QMutexLocker lock( &mMutex );
	//check severity, log if required
	if (severity <= mSeverity) {
		QString line = QDateTime::currentDateTime().toString() + ":" + QString::number(qApp->applicationPid()) + " " + file + ":" + logmessage + "\r\n";
		
		if( !mFileFilter.isEmpty() )
			if( file.contains( mFileFilter ) )
				return;

		//if( !mFunctionFilter.isEmpty() )
		//	if( mFunctionFilter.search( file ) < 0 )
		//		return;
		rotate( line.length() ); //before output, check size of file, rotate if needed

		if( mLogStream ) {
			*mLogStream << line;
			mLogStream->flush();
		} //else
		//	qWarning( "Log stream not open!!!" );

		if( mEchoStdOut ) {
			QByteArray l1 = line.toLatin1();
			fwrite( l1.constData(), sizeof(char), l1.size(), stdout );
		}

		emit logged( line );
	}
}

void Multilog::setStdOut( bool stdout_option )
{
	if (stdOut() != stdout_option) 
		mEchoStdOut = stdout_option;
}

bool Multilog::stdOut() const
{
	return mEchoStdOut;
}

void Multilog::setLogLevel( int severity )
{
	if (mSeverity != severity && severity <= 10)
		mSeverity = severity; //set global int value
}

int Multilog::logLevel() const
{
	return mSeverity;
}

void Multilog::setMaxFiles( int maxfiles )
{
	if (mMaxFiles != maxfiles && maxfiles <= 100)
		mMaxFiles = maxfiles;
}

int Multilog::maxFiles() const
{
	return mMaxFiles;
}

void Multilog::setMaxSize( unsigned int maxsize ) // minimum size of 
{
	if (mMaxSize != maxsize && maxsize >= 2>>16)
		mMaxSize = maxsize;
}

int Multilog::maxSize() const
{
	return mMaxSize;
}

QRegExp Multilog::filesFilter() const
{
	return mFileFilter;
}

void Multilog::setFilesFilter( const QRegExp & ff )
{
	mFileFilter = ff;
}
/*
QRegExp Multilog::functionsFilter() const
{
	return mFunctionFilter;
}

void Multilog::setFunctionsFilter( const QRegExp & ff )
{
	mFunctionFilter = ff;
}
*/

