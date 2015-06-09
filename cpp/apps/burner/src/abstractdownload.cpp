
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

#include <qftp.h>
#include <qfile.h>
#include <qfileinfo.h>

#include "abstractdownload.h"
#include "blurqt.h"
#include "config.h"
#include "maindialog.h"

AbstractDownload::AbstractDownload( QObject * parent )
: QObject( parent )
, mState( New )
{}

QString AbstractDownload::errorMsg()
{
	return mErrorMsg;
}

void AbstractDownload::setup( const QString src, const QString & dest )
{
	mSrc = src;
	mDest = dest;
}

void AbstractDownload::start()
{
	mTimer.start();
	mState = Started;
	emit stateChanged( Started );
}

void AbstractDownload::cancel()
{
	if( mState != Cancelled ) {
		mState = Cancelled;
		emit stateChanged( Cancelled );
	}
}

void AbstractDownload::error( const QString & er )
{
	LOG_5( "AbstractDownload::error: " + er );
	mErrorMsg = er;
	mState = Error;
	emit stateChanged( Error );
}

// Time spent downloading, in milliseconds
int AbstractDownload::elapsed()
{
	return mTimer.elapsed();
}

void AbstractDownload::finish( qint64 size )
{
	mSize = size;
	mState = Finished;
	emit stateChanged( Finished );
}

qint64 AbstractDownload::size()
{
	return mSize;
}

FtpDownload::FtpDownload( QObject * parent )
: AbstractDownload( parent )
, mLastProgress( -0.01 )
{
	mFtpEnabled = Config::getBool( "assburnerFtpEnabled", true );
	mFtpHost = Config::getString( "assburnerFtpHost", "stryfe" );
	mFtpPort = Config::getInt( "assburnerFtpPort", 21 );
}

FtpDownload::~FtpDownload()
{
	delete mFtp;
}

void FtpDownload::start()
{
	AbstractDownload::start();

	mFtp = new QFtp( this );
	/*
	 * Connect the qftp object, then start the get
	 */
	connect( mFtp, SIGNAL( commandStarted( int ) ), SLOT( ftpCmdStarted( int ) ) );
	connect( mFtp, SIGNAL( commandFinished( int, bool ) ), SLOT( ftpCmdFinished( int, bool ) ) );
	connect( mFtp, SIGNAL( done( bool ) ), SLOT( ftpDone( bool ) ) );
	connect( mFtp, SIGNAL( dataTransferProgress( qint64, qint64 ) ), SLOT( ftpProgress( qint64, qint64 ) ) );
	connect( mFtp, SIGNAL( stateChanged( int ) ), SLOT( ftpState( int ) ) );

	mFtp->connectToHost( mFtpHost, mFtpPort );
	mFtp->login();

	/*
	 * Get ready to write the destination file
	 */
	QFile * destFile = new QFile( mDest, mFtp );
	if( !destFile->open( QIODevice::WriteOnly ) ) {
		delete destFile;
		error( "SpoolItem::startFtpCopy: Couldn't open dest for writing: " + mDest );
		return;
	}

	QFileInfo srcInfo( mSrc );

	// Remove the drive letter
	QString dir = srcInfo.path().replace(QRegExp("^.:"),"").replace("\\","/");
	LOG_5( "SpoolItem::executeFtpGet: Cding to " + dir );
	mFtpCdCmd = mFtp->cd( dir );

	LOG_5( "SpoolItem::executeFtpGet: Getting " + srcInfo.fileName() );
	mFtpGetCmd = mFtp->get( srcInfo.fileName(), destFile );
}

void FtpDownload::ftpDone( bool isError )
{
	if( isError ) {
		error( "SpoolItem::ftpDone: FTP transaction failed" );
	}
	if( mFtp )
		mFtp->close();
	mFtpGetStarted = false;
}


void FtpDownload::cancel()
{
	AbstractDownload::cancel();
	if( mFtp ) {
		mFtp->disconnect( this );
		mFtp->clearPendingCommands();
		mFtp->deleteLater();
		mFtp = 0;
	}
}

void FtpDownload::ftpProgress( qint64 done, qint64 total )
{
	if( mFtpGetStarted ) {
		double cprogress = done / double(qMax(qint64(1),total));
		if( cprogress > mLastProgress + 0.05 ) {
			LOG_5( "FtpDownload::ftpProgress: Get Progress: " + QString::number( cprogress * 100.0 ) + "%" );
			emit progress( done, total );
			mLastProgress = cprogress;
		}
		mSize = total;
	}
}

void FtpDownload::ftpCmdStarted( int cmd )
{
	if( cmd == mFtpCdCmd ) {
//		LOG_5( "FtpDownload::ftpCmdStarted: cd sent" );
	}

	if( cmd == mFtpGetCmd ) {
//		LOG_5( "FtpDownload::ftpCmdStarted: Get has started" );
		mFtpGetStarted = true;
	}
}

void FtpDownload::ftpCmdFinished( int cmd, bool isError )
{
	LOG_5( "FtpDownload::ftpCmdFinished: " + QString::number(cmd) + " error:" + QString(isError ? " true" : " false") );

	if( cmd == mFtpCdCmd ) {
		if( isError ) {
			error( "SpoolItem::ftpCmdFinished: CD command errored" );
			return;
		} else {
			LOG_5( "SpoolItem::ftpCmdFinished: CD completed successfully" );
			return;
		}
	}

	if( cmd == mFtpGetCmd ) {
		if( isError ) {
			error( "SpoolItem::ftpCmdFinished: Get command errored" );
			return;
		} else {
			qint64 size = 0;
			if( mFtp && mFtp->currentDevice() ) {
				size = mFtp->currentDevice()->size();
				delete mFtp->currentDevice();
			}
			AbstractDownload::finish( size );
			return;
		}
	}
}

static const char * ftp_states [] = 
{
	"Unconnected",
	"Host Lookup",
	"Connecting",
	"Connected",
	"Logged In",
	"Closing"
};

void FtpDownload::ftpState( int state )
{
	if( state < 0 || state > 5 ) return;
	LOG_5( "SpoolItem::ftpState: State is " + QString(ftp_states[state]) );
	if( state == QFtp::Unconnected ) {
		if( mState != Finished )
			error( "FtpDownload::ftpState: Connection was lost" );
	}
}


#ifdef Q_OS_WIN

SmbDownload::SmbDownload( QObject * parent )
: AbstractDownload( parent )
, mWinCopy( 0 )
{}

SmbDownload::~SmbDownload()
{
	delete mWinCopy;
}

void SmbDownload::start()
{
	AbstractDownload::start();
	QFileInfo srcInfo( mSrc );
	LOG_3( "SmbDownload::start: " + mSrc );

	if( !srcInfo.exists() ) {
		error( "SmbDownload::start: " + mSrc + " does not exist" );
		return;
	}

	QFileInfo destInfo( mDest );
	mWinCopy = new WinCopy( this );
	mWinCopy->setSource( srcInfo.path() );
	mWinCopy->setDest( destInfo.path() );
	connect( mWinCopy, SIGNAL( stateChange( int ) ), SLOT( smbCopyUpdate( int ) ), Qt::QueuedConnection );
	mWinCopy->start();
}

void SmbDownload::cancel()
{
	delete mWinCopy;
	mWinCopy = 0;
	AbstractDownload::cancel();
}

void SmbDownload::smbCopyUpdate(int state)
{
	if( state == WinCopy::Complete ) {
		finish( mWinCopy->fileSize() );
	} else if( state == WinCopy::Failed ) {
		error( mWinCopy->errorMessage() );
	} // Do nothing for state == WinCopy::Copying
}

#endif // Q_OS_WIN

MultiDownload::MultiDownload( QObject * parent )
: AbstractDownload( parent )
, mCurrentDownload( 0 )
, mCurrentMethod( 0 )
, mFailureCount( 0 )
{
	mMethods = Config::getString( "assburnerDownloadMethods", "torrent,ftp,smb" ).split(",");
}

MultiDownload::~MultiDownload()
{
	destroyCurrent();
}

void MultiDownload::start()
{
	AbstractDownload::start();

	if( mMethods.isEmpty() ) {
		error( "MultiDownload::start: No downloads methods listed in assburnerDownloadMethods" );
		return;
	}

	createDownload();
}

void MultiDownload::createDownload()
{
	destroyCurrent();

	if( mCurrentMethod >= mMethods.size() ) {
		mFailureCount++;
		if( mMethods.isEmpty() || mFailureCount > 1 ) {
			error( "MultiDownload::createDownload: Out of download options\n" + mAllErrors.join("\n") );
			return;
		}
		mCurrentMethod = 0;
	}
	QString type = mMethods[mCurrentMethod];

	AbstractDownload * ret = 0;
	if( type == "ftp" )
		ret = new FtpDownload( this );
#ifdef Q_OS_WIN
	if( type == "smb" )
		ret = new SmbDownload( this );
#endif // Q_OS_WIN

	if( !ret ) {
		LOG_5( "MultiDownload::createDownload: Couldn't create download of type: " + type );
		mAllErrors += "MultDownload::createDownload: Couldn't create download of type: " + type;
		mCurrentMethod++;
		createDownload();
		return;
	}

	LOG_5( "MultiDownload::createDownload: Starting download of type: " + type );

	connect( ret, SIGNAL( progress( qint64, qint64 ) ), SLOT( downloadProgress( qint64, qint64 ) ) );
	connect( ret, SIGNAL( stateChanged( int ) ), SLOT( downloadStateChanged( int ) ) );
	connect( ret, SIGNAL( torrentHandleChanged( const libtorrent::torrent_handle & ) ), SIGNAL( torrentHandleChanged( const libtorrent::torrent_handle & th ) ) );

	mCurrentDownload = ret;
	mTimer.start();
	ret->setup( mSrc, mDest );
	ret->start();
}

void MultiDownload::destroyCurrent()
{
	if( mCurrentDownload ) {
		mCurrentDownload->disconnect( this );
		mCurrentDownload->cancel();
		mCurrentDownload->deleteLater();
		mCurrentDownload = 0;
	}
}

void MultiDownload::cancel()
{
	destroyCurrent();
	AbstractDownload::cancel();
}

void MultiDownload::downloadProgress( qint64 done, qint64 total )
{
	mSize = total;
	emit progress( done, total );
}

void MultiDownload::downloadStateChanged( int state )
{
	if( state == Error || state == Cancelled ) {
		LOG_5( "MultiDownload::downloadStateChanged: Download of type: " + QString(mCurrentDownload->typeName()) + " failed or was cancelled: " + mCurrentDownload->errorMsg() );
		mAllErrors += mCurrentDownload->errorMsg();
		mCurrentMethod++;
		createDownload();
	} else if( state == Finished ) {
		finish( mCurrentDownload->size() );
	}
}
