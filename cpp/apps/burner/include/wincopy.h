
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

#ifndef WIN_COPY_H
#define WIN_COPY_H

#include <qobject.h>
#ifdef Q_OS_WIN

#undef arglist
#define _WIN32_WINNT 0x0500
#include "windows.h"

#include <qstring.h>

/// \ingroup ABurner
/// @{

class WinCopy : public QObject
{
Q_OBJECT
public:
	WinCopy( QObject * parent );
	~WinCopy();

	void start();
	void cancel();

	enum State {
		New,
		Copying,
		Failed,
		Complete
	};

	int state();

	void setSource( const QString & source );
	QString source() const;
	
	void setDest( const QString & dest );
	QString dest() const;

	qint64 fileSize() const;
	qint64 bytesTransfered() const;
	float progress() const;

	QString errorMessage() const;

signals:
	void stateChange( int );
	
protected:
	static DWORD CALLBACK copyFileExProgress( LARGE_INTEGER tfs, LARGE_INTEGER tbt, LARGE_INTEGER, LARGE_INTEGER, DWORD, DWORD cbr,
	HANDLE, HANDLE, LPVOID );

	QString mSource, mDest, mErrorMsg;
	qint64 mFileSize, mTransfered;
	int mState;
	bool mCancelBool;
	void run();
	friend class WinCopyThread;
};

/// @}
	
#endif // Q_OS_WIN

#endif // WIN_COPY_H

