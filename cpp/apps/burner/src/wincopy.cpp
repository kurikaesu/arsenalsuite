
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

#include "wincopy.h"

#ifdef Q_OS_WIN

#include <qthread.h>

#include "blurqt.h"

DWORD CALLBACK WinCopy::copyFileExProgress( LARGE_INTEGER totalFileSize, LARGE_INTEGER totalBytesTrans, LARGE_INTEGER, LARGE_INTEGER, DWORD, DWORD cbReason,
	HANDLE, HANDLE, LPVOID userData )
{
	WinCopy * wc = (WinCopy*)userData;
	wc->mFileSize = *((qint64*)&totalFileSize);
	wc->mTransfered = *((qint64*)&totalBytesTrans);
	if( wc->mState == New ) {
		wc->mState = Copying;
		emit wc->stateChange( Copying );
	}
	LOG_5( "WinCopy::copyFileExProgress" );
	return PROGRESS_CONTINUE;
}

class WinCopyThread : public QThread
{
public:
	WinCopyThread( WinCopy * wc ) : QThread( wc ), mWinCopy( wc ) {}
	void run() {
		LOG_5( "WinCopyThread::run" );
		//mWinCopy->moveToThread( this );
		mWinCopy->run();
		LOG_5( "WinCopyThread::run, returning" );
	}
	WinCopy * mWinCopy;
};

WinCopy::WinCopy( QObject * )
: QObject( 0 )
, mFileSize( 0 )
, mTransfered( 0 )
, mState( New )
, mCancelBool( false )
{
}

WinCopy::~WinCopy()
{
}

void WinCopy::start()
{
	LOG_5( "WinCopy::start" );
	WinCopyThread * wct = new WinCopyThread( this );
	moveToThread( wct );
	wct->start();
	LOG_5( "WinCopy::start, returning" );
}

void WinCopy::run()
{
	LOG_5( "WinCopy::run" );
	QString src( mSource );
	src = src.replace( "/", "\\" );
	QString dest( mDest );
	dest = dest.replace( "/", "\\" );
	DeleteFile( (WCHAR*)dest.utf16() );
	BOOL res = CopyFileEx( (WCHAR*)src.utf16(), (WCHAR*)dest.utf16(), (LPPROGRESS_ROUTINE)copyFileExProgress, (LPVOID)this, 0, COPY_FILE_RESTARTABLE);
	mState = (res!=0) ? Complete : Failed;
	if( mState == Failed ) {
		DWORD err = GetLastError();
		mErrorMsg = "Got windows error: " + QString::number( err );
	}
	emit stateChange( mState );
	LOG_5( "WinCopy::run, returning" );
}

void WinCopy::cancel()
{
	mCancelBool = true;
}

void WinCopy::setSource( const QString & source )
{
	mSource = source;
}

QString WinCopy::source() const
{
	return mSource;
}

void WinCopy::setDest( const QString & dest )
{
	mDest = dest;
}

QString WinCopy::dest() const
{
	return mDest;
}

qint64 WinCopy::fileSize() const
{
	return mFileSize;
}

qint64 WinCopy::bytesTransfered() const
{
	return mTransfered;
}

float WinCopy::progress() const
{
	return mFileSize == 0 ? 0.0 : mTransfered / mFileSize;
}

QString WinCopy::errorMessage() const
{
	return mErrorMsg;
}

#endif // Q_OS_WIN


