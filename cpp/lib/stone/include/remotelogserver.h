
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


#ifndef REMOTE_LOG_SERVER_H
#define REMOTE_LOG_SERVER_H

#include <qabstractsocket.h>
#include <qobject.h>
#include <qqueue.h>

#include "blurqt.h"

class QTcpServer;
class QTcpSocket;
class QTimer;
class QHostAddress;
class QFile;

namespace Stone {

class STONE_EXPORT RemoteLogServer : public QObject
{
Q_OBJECT
public:
	RemoteLogServer( quint16 port = 0, QObject * parent = 0 );

	QTcpServer * tcpServer() { return mTcpServer; }

protected slots:
	void acceptNewConnection();

protected:

	QTcpServer * mTcpServer;
};

class STONE_EXPORT RemoteLogServerConnection : public QObject
{
Q_OBJECT
public:
	RemoteLogServerConnection( RemoteLogServer * parent, QTcpSocket * socket );

	enum State {
		Waiting,
		ServingFile,
		TailingFile,
		TailingLog,
		Closed,
		Error
	};

	// Each command starts with CMD_SIZE:INT, CMD:INT, followed by CMD_SIZE bytes
	enum {
		CMD_SERVE = 1, // START_POS:quint64, SIZE:quint32, FILENAME_DATA:BYTES
		CMD_TAIL = 2, // START_POS:quint64, FILENAME_DATA:BYTES
		CMD_CANCEL = 3, // EMPTY
		CMD_GET_SIZE = 4 // FILENAME_DATA:BYTES
	};

	enum {
		RLS_ERROR_FILE_NOT_FOUND,
		RLS_ERROR_INVALID_BYTE_RANGE
	};

	enum {
		RESPONSE_ERROR = 1, // ERROR_CODE:quint32
		// Used for Tail and Serve
		RESPONSE_SERVE = 2, // FILE_POS:qint64, SIZE:quint32, BYTES
		RESPONSE_GET_SIZE = 3, // SIZE:qint64
		RESPONSE_CANCELLED = 4
	};

protected slots:
	void readIncoming();
	void readyWrite();
	void socketDisconnected();
	void socketError(QAbstractSocket::SocketError);

	void tailLogSend( const QString & data = QString() );
	void tailCheck();
	
protected:
	void parseCommand();

	void serve( quint64 startPos, quint32 size, const QString & fileName );
	void tail( qint64 startPos, const QString & fileName );
	void cancel();
	void getSize( const QString & fileName );
	
	void startServing( const QString & fileName, qint64 startPos, int state );
	void startTailing();

	void sendServeResponse( qint64 pos, QByteArray bytes );
	void sendCancelResponse();
	void sendErrorResponse( quint32 errorCode );
	void sendResponse( qint32 response, const QByteArray & data );

	int mState;
	bool mCommandIncoming;
	qint32 mIncomingCommand;
	qint32 mIncomingCommandSize;
	QByteArray mIncomingData, mOutgoingData;
	QTcpSocket * mTcpSocket;
	QFile * mFile;

	quint64 mFilePos, mServeEnd;
	QByteArray mTailLogData;
	QTimer * mTailTimer;
};


class STONE_EXPORT RemoteLogClientConnection : public QObject
{
Q_OBJECT
public:
	RemoteLogClientConnection( QObject * parent = 0 );

	enum CommandType {
		None,
		ConnectToHost,
		ReadFile,
		TailFile,
		GetFileSize,
		Close
	};
	
	/// Returns a printable representation of the host(name or address) and port of the
	/// remote end of the connection.
	QString remoteHostString() const { return mRemoteHostString; }

	/// Puts a ConnectToHost command onto the command queue and returns the command ID
	int connectToHost( const QHostAddress &, quint16 port );
	int connectToHost( const QString &, quint16 port );

	/// Puts a GetFileSize command onto the command queue and returns the command ID
	int getFileSize( const QString & fileName );

	/// Puts a read file command onto the command queue, and returns the command ID
	int readFile( const QString & fileName, quint64 startPos, quint32 length );

	/// Puts a tail file Command onto the command queue, and returns the command ID
	/// A pos >= 0 indicates the position from the start of the file
	/// A pos < 0 indicates the the position from the end of the file ( Size + pos + 1 ),
	/// ie. pos = -1 indicates Size - 1 + 1 == Size, starts at the end of the file
	int tailFile( const QString & fileName, qint64 startPos = -1 );

	QTcpSocket * socket() const { return mTcpSocket; }

	/// Returns the current State of the connection
	QAbstractSocket::SocketState state();
	
	/// Returns the type of the current Command being executed, or None if there is no current command
	CommandType currentCommand();
	
	/// Returns the ID of the current command being executed, or 0 if there is no current command
	int currentCommandId();

	class Command;
public slots:
	void clearPendingCommands();
	void abort();

	void socketConnected();
	void socketDisconnected();
	void socketError( QAbstractSocket::SocketError );

	void readyRead();
signals:
	void remoteHostChanged();
	void stateChanged( QAbstractSocket::SocketState );
	void commandStarted( int commandId );
	void commandFinished( int commandId, bool error );
	void done( bool error );

	void dataRecieved( const QString & text, quint64 pos, quint32 size );
	void sizeRecieved( qint64 size );

protected slots:
	void socketStateChanged( QAbstractSocket::SocketState );

protected:
	friend class Command;
	friend class ConnectToHostCommand;
	friend class GetFileSizeCommand;
	friend class ReadFileCommand;
	friend class TailFileCommand;

	int newCommandId();
	void queueCommand( Command * command );
	void startNextCommand();

	void _commandFinished( bool error );

	void sendCommand( int command, const QByteArray & data );
	void sendData();
	void createTcpSocket();

	int mNextCommandId;

	Command * mCurrentCommand;
	QQueue<Command*> mCommandQueue;

	QTcpSocket * mTcpSocket;
	QString mRemoteHostString;

	QByteArray mDataToSend;

	bool mResponseIncoming;
	qint32 mIncomingResponse;
	qint32 mIncomingResponseSize;
	QByteArray mIncomingData;

};

} //namespace

#endif // REMOTE_LOG_SERVER_H

