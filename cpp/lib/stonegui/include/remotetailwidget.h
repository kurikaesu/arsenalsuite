
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
 * $Id: remotetailwidget.h 10005 2010-06-17 00:12:01Z newellm $
 */

#ifndef REMOTE_TAIL_WIDGET_H
#define REMOTE_TAIL_WIDGET_H

#include <qtextedit.h>

#include "stonegui.h"

class QHostAddress;
namespace Stone {
class RemoteLogClientConnection;
}
using namespace Stone;

class STONEGUI_EXPORT RemoteTailWidget : public QTextEdit
{
Q_OBJECT
public:
	RemoteTailWidget( QWidget * parent = 0 );
	~RemoteTailWidget();

	RemoteLogClientConnection * logConnection() { return mLogConnection; }

	bool isConnected();

	int fetchedStartPosition();
	int fetchedEndPosition();
	int lastFileSize();

public slots:
	void connectToHost( const QHostAddress & hostAddress, int port );
	void connectToHost( const QString & hostName, int port );

	void setFileName( const QString & fileName );

	/// A pos >= 0 indicates the position from the start of the file
	/// A pos < 0 indicates the the position from the end of the file ( Size + pos + 1 ),
	/// ie. pos = -1 indicates Size - 1 + 1 == Size, starts at the end of the file
	void startTailing( qint64 pos = -1 );
	void stopTailing();

	void fetchPrevious( quint32 size = 1024 );
	void fetchNext( quint32 size = 1024 );

protected slots:
	void dataRecieved( const QString & text, quint64 pos, quint32 size );
	void sizeRecieved( qint64 size );

	void connectionIdle();
	void verticalScrollBarValueChanged();

protected:
	bool ensureConnectionReady();

	bool mSizeRecieved;
	quint64 mFetchedStart, mFetchedEnd; // mFetchedEnd is 1 byte after the last recieved byte, so if mFetchedStart == mFetchedEnd, there is not data
	quint64 mLastSize;
	QString mFileName;
	RemoteLogClientConnection * mLogConnection;

	int mFetchStart, mFetchSize;
	QString mTextToPrepend;
	int mTextToPrependSize;
};

#endif // REMOTE_TAIL_WIDGET_H

