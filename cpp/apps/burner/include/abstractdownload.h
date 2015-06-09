
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Arsenal.
 *
 * Arsenal is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Arsenal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arsenal; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 */

#ifndef ABSTRACT_DOWNLOAD_H
#define ABSTRACT_DOWNLOAD_H

#include <qobject.h>
#include <qdatetime.h>
#include <qstringlist.h>

class QFtp;
class QTimer;
class TorrentManager;

/**
 *  \ingroup ABurner
 * @{
 */

/**
 * Base class for download plugins
 *
 *  Handles timer, error message, and
 *  state emittions.
 *
 * Defines interface and signals
 * that the plugins must implement
 *
 */
class AbstractDownload : public QObject
{
Q_OBJECT
public:
	AbstractDownload( QObject * parent = 0 );

	enum {
		New,
		Started,
		Finished,
		Cancelled,
		Error
	};

	virtual void setup( const QString src, const QString & dest );
	virtual void start();
	virtual void cancel();

	virtual const char * typeName() = 0;
	QString errorMsg();

	// Time spent downloading, in milliseconds
	int elapsed();
	qint64 size();

signals:
	void progress( qint64 done, qint64 total );
	void stateChanged( int state );

protected:
	void error( const QString & );
	void finish( qint64 size );
	
	QString mSrc, mDest, mErrorMsg;
	int mState;
	QTime mTimer;
	qint64 mSize;
};

class FtpDownload : public AbstractDownload
{
Q_OBJECT
public:
	FtpDownload( QObject * parent = 0 );
	~FtpDownload();

	virtual void start();
	virtual void cancel();

	const char * typeName() { return "ftp"; }

protected slots:
	void ftpProgress( qint64 done, qint64 total );
	void ftpCmdStarted( int cmd );
	void ftpCmdFinished( int cmd, bool error );
	void ftpState( int state );
	void ftpDone( bool isError );

protected:
	// Start the copy via qftp connection.
	void startFtpCopy();

	int mFtpGetCmd, mFtpCdCmd;
	bool mFtpGetStarted;

	QFtp * mFtp;
	bool mFtpFailed;
	bool mFtpEnabled;
	QString mFtpHost;
	int mFtpPort;
	double mLastProgress;
};


#ifdef Q_OS_WIN
#include "wincopy.h"
class SmbDownload : public AbstractDownload
{
Q_OBJECT
public:
	SmbDownload( QObject * parent = 0 );
	~SmbDownload();

	virtual void start();
	virtual void cancel();

	const char * typeName() { return "smb"; }

protected slots:
	void smbCopyUpdate(int);

protected:
	WinCopy * mWinCopy;
};
#endif // Q_OS_WIN

class MultiDownload : public AbstractDownload
{
Q_OBJECT
public:
	MultiDownload( QObject * parent = 0 );
	~MultiDownload();

	virtual void start();
	virtual void cancel();

	const char * typeName() { return mCurrentDownload ? mCurrentDownload->typeName() : "multi"; }

protected slots:
	void downloadProgress( qint64 done, qint64 total );
	void downloadStateChanged( int state );

protected:
	void createDownload();
	void destroyCurrent();

	QStringList mAllErrors;
	AbstractDownload * mCurrentDownload;
	int mCurrentMethod;
	int mFailureCount;
	QStringList mMethods;
};

/// @}

#endif // ABSTRACT_DOWNLOAD_H

