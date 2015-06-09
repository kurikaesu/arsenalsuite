
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

#ifndef SPOOLER_H
#define SPOOLER_H

#include <qobject.h>
#include <qpointer.h>

#include <string>

#include "abstractdownload.h"
#include "job.h"

class QProcess;
class Spooler;
class QTimer;
class QFtp;

/// \ingroup ABurner
/// @{

class SpoolRef;

/**
 * SpoolItem - Used to keep track of torrents during downloading
 * and once they become seeds.
 */
class SpoolItem : public QObject
{
Q_OBJECT
public:
	SpoolItem( Spooler * spooler, const Job & job );
	virtual ~SpoolItem();

	void ref();

	void startCopy();

	enum State {
		Idle,
		Copy,
		Unzip,
		Errored
	};

	State state() const { return mState; }

	bool isCopied() { return mCopied; }
	QString destFile() const { return mDestFile; }

	bool isZip();
	bool isUnzipped();
	void unzip();
	void cleanupZipFolder();
	QString zipFolder();

	AbstractDownload * download();

signals:
	// Gui copy progress
	void copyProgressChange( int );

	void errored( const QString & msg, int state );
	void stateChanged( int oldState, int newState );
	void completed();

public slots:
	void unzipExited();
	void unzipReadyRead();
	void unzipTimedOut();

	void downloadProgress( qint64 done, qint64 total );
	void downloadStateChanged( int state );

protected:
	// Called by deref when ref count reaches 0
	void cancelCopy();

	bool checkUnzip();
	bool checkMd5();

	void _finished();
	void _error( const QString & msg );
	void _changeState( State state );

	State mState;
	int mRefCount;
	
	void deref();
	
	Job mJob;
	QString mSrcFile, mDestFile;

	Spooler * mSpooler;

	AbstractDownload * mDownload;

	QProcess * mUnzipProcess;
	QTimer * mCheckupTimer;
	int mUnzipFailureCount;
	int mUnzipTimeout;
	bool mCopied, mUnzipped;

	friend class SpoolRef;
	friend class Spooler;
};

class SpoolRef : public QObject
{
Q_OBJECT
public:
	SpoolRef( SpoolItem * item );
	~SpoolRef();

	void start();

	SpoolItem * spoolItem() const { return mSpoolItem; }

signals:
	// Gui copy progress
	void copyProgressChange( int );

	void errored( const QString & msg, int state );
	void stateChanged( int oldState, int newState );
	void completed();

protected:
	SpoolItem * mSpoolItem;
	friend class SpoolItem;
};

typedef QList<SpoolItem*> SpoolList;
typedef QList<SpoolItem*>::Iterator SpoolIter;

class Spooler : public QObject
{
Q_OBJECT
public:
	Spooler( QObject * parent );
	~Spooler();

	SpoolItem * startCopy( const Job & job );
	void cancelCopy( SpoolItem * );

	bool ensureDriveSpace( qint64 );

	bool checkDriveSpace( qint64 );

	// Returns the SpoolItem * associated witth he arg, or 0 if not found
	SpoolItem * itemFromJob( const Job & );

	// This is called to try to delete job files that
	// are no longer needed that are kept in the
	// mFilesToDelete list. We periodically try to
	// delete them because sometimes they are locked
	// by existing processes that will exit sometime
	// in the future. Called from loop
	void deleteFiles();

	QString spoolDir() const;

public slots:
	void cleanup();

	// This function is called when disk space is
	// needed.  It deletes old unneeded files
	// in assburnerSpoolDir.
	// Returns the number of bytes freed
	int fileCleanup();

signals:
	void cancelled( SpoolItem * );

protected:
	void addDownloadStat( const QString &, const Job &, int, int );

	QString mSpoolDir;
	SpoolList mSpool;
	QStringList mFilesToDelete;
	
	bool mRecordDownloadStats;

	QTimer * mCheckTimer;

	friend class SpoolItem;
};

std::string add_suffix(float val);

/// @}

#endif // SPOOLER_H

