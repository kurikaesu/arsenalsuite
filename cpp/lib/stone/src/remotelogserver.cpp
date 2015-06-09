
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

#include <qbytearray.h>
#include <qtcpserver.h>
#include <qtcpsocket.h>
#include <qtimer.h>

#include "blurqt.h"
#include "multilog.h"

#include "remotelogserver.h"

namespace Stone {

RemoteLogServer::RemoteLogServer( quint16 port, QObject * parent )
: QObject( parent )
, mTcpServer( 0 )
{
	mTcpServer = new QTcpServer( this );
	connect( mTcpServer, SIGNAL( newConnection() ), SLOT( acceptNewConnection() ) );

	mTcpServer->listen( QHostAddress::Any, port );
}

	
void RemoteLogServer::acceptNewConnection()
{
	while( mTcpServer->hasPendingConnections() ) {
		LOG_5( "Creating New RemoteLogServerConnection" );
		new RemoteLogServerConnection( this, mTcpServer->nextPendingConnection() );
	}
}


RemoteLogServerConnection::RemoteLogServerConnection( RemoteLogServer * parent, QTcpSocket * socket )
: QObject( parent )
, mState( Waiting )
, mCommandIncoming( false )
, mTcpSocket( socket )
, mFile( 0 )
, mTailTimer( 0 )
{
	connect( mTcpSocket, SIGNAL( readyRead() ), SLOT( readIncoming() ) );
	connect( mTcpSocket, SIGNAL( bytesWritten(qint64) ), SLOT( readyWrite() ) );
	connect( mTcpSocket, SIGNAL( error( QAbstractSocket::SocketError ) ), SLOT( socketError( QAbstractSocket::SocketError ) ) );
	connect( mTcpSocket, SIGNAL( disconnected() ), SLOT( socketDisconnected() ) );
	
	// Check for any data recieved before we connected to the readyRead signal
	readIncoming();
}

void RemoteLogServerConnection::socketDisconnected()
{
	//LOG_5( QString::number((long)this) );
	cancel();
	mState = Closed;
	deleteLater();
}

void RemoteLogServerConnection::socketError( QAbstractSocket::SocketError )
{
	//LOG_5( QString::number((long)this) );
	cancel();
	mState = Closed;
	deleteLater();
}

void RemoteLogServerConnection::readIncoming()
{
	while( mTcpSocket->bytesAvailable() ) {
		
		// Read a command header if there's enough data available
		if( !mCommandIncoming ) {
			if( mTcpSocket->bytesAvailable() >= sizeof(quint32) * 2 ) {
				QDataStream ds( mTcpSocket );
				ds >> mIncomingCommandSize >> mIncomingCommand;
				mCommandIncoming = true;
				mIncomingData = QByteArray();
				LOG_5( "Read Command Header, Command Size: " + QString::number( mIncomingCommandSize ) + " Command: " + QString::number( mIncomingCommand ) );
			} else
				break;
		}

		// Read command data
		if( mCommandIncoming && mIncomingData.size() < mIncomingCommandSize && mTcpSocket->bytesAvailable() ) {
			// Read at most the rest of the current command
			QByteArray readBytes = mTcpSocket->read( mIncomingCommandSize - mIncomingData.size() );
			mIncomingData.append( readBytes );
			LOG_5( "Read Command Bytes: " + QString::number( readBytes.size() ) + " Remaining: " + QString::number( mIncomingCommandSize - mIncomingData.size() ) );
		}

		// Is this command complete?
		if( mCommandIncoming && mIncomingData.size() == mIncomingCommandSize ) {
			parseCommand();
			mCommandIncoming = false;
		}
	}
}

void RemoteLogServerConnection::parseCommand()
{
	LOG_5( "Parsing Command" );
	QDataStream ds( &mIncomingData, QIODevice::ReadOnly );
	switch( mIncomingCommand ) {
		case CMD_SERVE:
		{
			quint64 startPos;
			quint32 size;
			QString fileName;
			ds >> startPos >> size;
			int headerSize = sizeof(startPos) + sizeof(size);
			fileName = QString::fromUtf8( mIncomingData.constData() + headerSize, mIncomingData.size() - headerSize );
			serve( startPos, size, fileName );
			break;
		}
		case CMD_TAIL:
		{
			qint64 startPos;
			QString fileName;
			ds >> startPos;
			int headerSize = sizeof(startPos);
			fileName = QString::fromUtf8( mIncomingData.constData() + headerSize, mIncomingData.size() - headerSize );
			tail( startPos, fileName );
			break;
		}
		case CMD_CANCEL:
			cancel();
			break;
		case CMD_GET_SIZE:
		{
			QString fileName;
			fileName = QString::fromUtf8( mIncomingData.constData(), mIncomingData.size() );
			getSize( fileName );
			break;
		}
		default:
			LOG_5( "Recieved Unknown Command: " + QString::number( mIncomingCommand ) + " with command size: " + QString::number( mIncomingData.size() ) );
	}
}

void RemoteLogServerConnection::tailLogSend( const QString & logString )
{
	QByteArray logData = logString.toUtf8();
	sendServeResponse( mFilePos, logData );
	mFilePos += logData.size();
}

void RemoteLogServerConnection::readyWrite()
{
	while( mOutgoingData.size() ) {
		quint32 written = mTcpSocket->write( mOutgoingData );
		if( written <= 0 ) break;
		mOutgoingData = mOutgoingData.mid( written );
	}
}

void RemoteLogServerConnection::serve( quint64 startPos, quint32 size, const QString & fileName )
{
	if( mState != Waiting )
		return;
	QString targetFileName( fileName == "APPLICATION_LOG" ? log()->logFileName() : fileName );
	LOG_5( "Serving File: " + targetFileName + " Start: " + QString::number( startPos ) + " Size: " + QString::number( size ) );
	mServeEnd = startPos + size;
	startServing( targetFileName, startPos, ServingFile );
}

void RemoteLogServerConnection::tail( qint64 startPos, const QString & fileName )
{
	if( mState != Waiting )
		return;
	int state = TailingFile;
	LOG_5( "Tailing File: " + fileName + " Start: " + QString::number( startPos ) );
	QString fileToOpen( fileName );
	if( fileName == "APPLICATION_LOG" ) {
		state = TailingLog;
		fileToOpen = log()->logFileName();
	}
	startServing( fileToOpen, startPos, state );
	
}

void RemoteLogServerConnection::startServing( const QString & fileName, qint64 startPos, int state )
{
	if( QFileInfo( fileName ).exists() ) {
		mFile = new QFile( fileName );
		if( mFile->open( QIODevice::ReadOnly ) ) {
			LOG_5( "File Opened Successfully" );
			// We start at the end here
			if( startPos < 0 || mFile->size() < startPos || (mFile->size() == startPos && state == ServingFile) ) 
				sendErrorResponse( RLS_ERROR_INVALID_BYTE_RANGE );
			else {
				mState = state;
				if( mFile->size() == startPos ) {
					if( mState == TailingFile )
						mFile->seek( mFile->size() );
					startTailing();
					return;
				}
				if( mState == TailingFile || mState == TailingLog ) {
					mServeEnd = mFile->size();
				}
				mFile->seek( startPos );
				mFilePos = startPos;
				QByteArray bytes = mFile->read( mServeEnd - mFilePos );
				if( bytes.size() == mServeEnd - mFilePos ) {
					sendServeResponse( mFilePos, bytes );
					mFilePos += bytes.size();
				}
				if( mState == TailingFile || mState == TailingLog )
					startTailing();
				else
					mState = Waiting;
				return;
			}
			return;
		} else
			sendErrorResponse( RLS_ERROR_FILE_NOT_FOUND );
	} else {
		sendErrorResponse( RLS_ERROR_FILE_NOT_FOUND );
	}
	mState = Waiting;
}

void RemoteLogServerConnection::startTailing()
{
	LOG_5( "startTailing" );
	if( mState == TailingFile ) {
		mTailTimer = new QTimer( this );
		connect( mTailTimer, SIGNAL( timeout() ), SLOT( tailCheck() ) );
		mTailTimer->start( 300 );
	} else if( mState == TailingLog ) {
		if( mFile ) {
			delete mFile;
			mFile = 0;
		}
		connect( log(), SIGNAL( logged( const QString & ) ), SLOT( tailLogSend( const QString & ) ) );
	}
}

void RemoteLogServerConnection::tailCheck()
{
	while( 1 ) {
		QByteArray bytes = mFile->read( 1024 );
		if( bytes.isEmpty() ) break;
		sendServeResponse( mFilePos, bytes );
		mFilePos += bytes.size();
	}
}

void RemoteLogServerConnection::cancel()
{
	LOG_5( "Cancel" );
	if( mFile ) {
		delete mFile;
		mFile = 0;
	}
	if( mTailTimer ) {
		mTailTimer->deleteLater();
		mTailTimer = 0;
	}
	log()->disconnect( SIGNAL( logged( const QString & ) ), this, SLOT( tailLogSend( const QString & ) ) );
	sendCancelResponse();
	mState = Waiting;
}

void RemoteLogServerConnection::getSize( const QString & fileName )
{
	if( mState != Waiting )
		return;
	QFileInfo fi( fileName == "APPLICATION_LOG" ? log()->logFileName() : fileName );
	qint64 fileSize = -1;
	if( fi.exists() ) {
		fileSize = fi.size();
		LOG_5( "getSize: " + fileName + " returning " + QString::number( fileSize ) );
		QByteArray bytes;
		QDataStream ds( &bytes, QIODevice::WriteOnly );
		ds << fileSize;
		sendResponse( RESPONSE_GET_SIZE, bytes );
	} else {
		sendErrorResponse( RLS_ERROR_FILE_NOT_FOUND );
	}
}

void RemoteLogServerConnection::sendServeResponse( qint64 pos, QByteArray bytes )
{
	QByteArray responseBytes;
	{
		QDataStream ds( &responseBytes, QIODevice::WriteOnly );
		ds << pos << quint32(bytes.size());
	}
	responseBytes.append( bytes );
	sendResponse( RESPONSE_SERVE, responseBytes );
}

void RemoteLogServerConnection::sendCancelResponse()
{
	sendResponse( RESPONSE_CANCELLED, QByteArray() );
}

void RemoteLogServerConnection::sendErrorResponse( quint32 errorCode )
{
	QByteArray bytes;
	QDataStream ds( &bytes, QIODevice::WriteOnly );
	ds << errorCode;
	sendResponse( RESPONSE_ERROR, bytes );
}

void RemoteLogServerConnection::sendResponse( qint32 response, const QByteArray & data )
{
	{
		QDataStream ds( &mOutgoingData, QIODevice::WriteOnly );
		ds << qint32(data.size()) << qint32(response);
	}
	//LOG_5( "Sent Response " + QString::number( response ) + " size " + QString::number( data.size() ) );
	mOutgoingData.append( data );
	readyWrite();
}

static QString makeLoggable( const QString & input )
{
	QString output = input.left( 30 );
	if( output.size() < input.size() )
		output += "...";
	output = output.replace( "\n", "\\n" );
	return output;
}

class RemoteLogClientConnection::Command
{
public:
	Command( int id, RemoteLogClientConnection::CommandType type )
	: mCommandId( id )
	, mCommandType( type )
	{}

	virtual ~Command(){}

	virtual void start(RemoteLogClientConnection *) = 0;

	int parseErrorResponse( const QByteArray & errorData )
	{
		quint32 errorCode;
		QDataStream ds(errorData);
		ds >> errorCode;
		return errorCode;
	}

	void reportError( RemoteLogClientConnection * conn, const QByteArray & errorData )
	{
		LOG_5( "Got Error Response, error code was: " + QString::number( parseErrorResponse( errorData ) ) );
		conn->abort();
	}

	virtual bool responseRecieved( RemoteLogClientConnection * conn, quint32 response, const QByteArray & data )
	{
		if( response == RemoteLogServerConnection::RESPONSE_ERROR ) {
			reportError( conn, data );
			return true;
		}
		if( response == RemoteLogServerConnection::RESPONSE_CANCELLED ) {
			LOG_5( "Command Cancelled" );
			conn->_commandFinished( false );
			return true;
		}
		return false;
	}

	virtual void abort(RemoteLogClientConnection * conn)
	{
		conn->_commandFinished( true );
	}

	virtual void socketStateChanged( RemoteLogClientConnection *, QAbstractSocket::SocketState ){}

	virtual	void socketError( RemoteLogClientConnection *, QAbstractSocket::SocketError ){}

protected:
	friend class RemoteLogClientConnection;
	int mCommandId;
	RemoteLogClientConnection::CommandType mCommandType;
};

class ConnectToHostCommand : public RemoteLogClientConnection::Command
{
public:
	ConnectToHostCommand( int id, const QHostAddress & host, quint16 port )
	: Command( id, RemoteLogClientConnection::ConnectToHost )
	, mHost( host )
	, mPort( port )
	{}

	ConnectToHostCommand( int id, const QString & host, quint16 port )
	: Command( id, RemoteLogClientConnection::ConnectToHost )
	, mHostName( host )
	, mPort( port )
	{}
	
	void start(RemoteLogClientConnection * conn) {
		LOG_5( "Creating Tcp Socket and connecting to host" ); 
		conn->createTcpSocket();
		if( !mHost.isNull() ) {
			conn->mTcpSocket->connectToHost( mHost, mPort );
			conn->mRemoteHostString = mHost.toString() + ":" + QString::number( mPort );
			emit conn->remoteHostChanged();
		} else if( !mHostName.isEmpty() ) {
			conn->mTcpSocket->connectToHost( mHostName, mPort );
			conn->mRemoteHostString = mHostName + ":" + QString::number( mPort );
			emit conn->remoteHostChanged();
		} else {
			LOG_5( "No valid hostname or hostaddress to connect to." );
			conn->_commandFinished( true );
		}
	}

	void socketStateChanged( RemoteLogClientConnection * conn, QAbstractSocket::SocketState ss )
	{
		if( ss == QAbstractSocket::ConnectedState ) {
			conn->_commandFinished( false );
		}
	}
	
	void socketError( RemoteLogClientConnection * conn, QAbstractSocket::SocketError )
	{
		conn->_commandFinished( true );
	}
protected:
	QHostAddress mHost;
	QString mHostName;
	quint16 mPort;
};

class GetFileSizeCommand : public RemoteLogClientConnection::Command
{
public:
	GetFileSizeCommand( int id, const QString & fileName )
	: Command( id, RemoteLogClientConnection::GetFileSize )
	, mFileName( fileName )
	{}

	void start( RemoteLogClientConnection * conn ) {
		LOG_5( "Sending CMD_GET_SIZE: " + mFileName );
		conn->sendCommand( RemoteLogServerConnection::CMD_GET_SIZE, mFileName.toUtf8() );
	}
	
	bool responseRecieved( RemoteLogClientConnection * conn, quint32 response, const QByteArray & data )
	{
		if( RemoteLogClientConnection::Command::responseRecieved( conn, response, data ) )
			return true;
		
		if( response == RemoteLogServerConnection::RESPONSE_GET_SIZE ) {
			if( data.size() != sizeof( qint64 ) ) {
				LOG_5( "RESPONSE_GET_SIZE has wrong payload size" );
				conn->abort();
			}
			qint64 fileSize;
			{
				QDataStream ds( data );
				ds >> fileSize;
			}
			emit conn->sizeRecieved( fileSize );
			conn->_commandFinished( false );
			return true;
		}
		return false;
	}

protected:
	QString mFileName;
};

class ReadFileCommand : public RemoteLogClientConnection::Command
{
public:
	ReadFileCommand( int id, const QString & fileName, quint64 startPos, quint32 size )
	: Command( id, RemoteLogClientConnection::ReadFile )
	, mFileName( fileName )
	, mSize( size )
	, mStartPos( startPos )
	, mFilePos( 0 )
	{}

	void start( RemoteLogClientConnection * conn ) {
		LOG_5( "Sending CMD_SERVE: " + mFileName + " Start Pos: " + QString::number( mStartPos ) + " Size: " + QString::number( mSize ) );
		QByteArray cmdData;
		QDataStream( &cmdData, QIODevice::WriteOnly ) << quint64(mStartPos) << quint32(mSize);
		mFilePos = mStartPos;
		cmdData.append( mFileName.toUtf8() );
		conn->sendCommand( RemoteLogServerConnection::CMD_SERVE, cmdData );
	}

	bool responseRecieved( RemoteLogClientConnection * conn, quint32 response, const QByteArray & data )
	{
		if( RemoteLogClientConnection::Command::responseRecieved( conn, response, data ) )
			return true;
		if( response == RemoteLogServerConnection::RESPONSE_SERVE ) {
			int headerSize = sizeof( qint64 ) + sizeof( quint32 );
			int correctPayloadSize = headerSize + mSize;
			if( data.size() != correctPayloadSize ) {
				LOG_5( "RESPONSE_SERVE_PACKET has invalid payload size: " + QString::number( data.size() ) + ", should be: " + QString::number( correctPayloadSize ) );
				conn->abort();
			}
			qint64 pos;
			quint32 size;
			QString dataString;
			{
				QDataStream ds( data );
				ds >> pos >> size;
			}
			if( size != mSize ) {
				LOG_5( "RESPONSE_SERVE_PACKET has incorrect data size" );
			}
			if( pos != mFilePos ) {
				LOG_5( "RESPONSE_SERVE_PACKET has incorrect file pos" );
			}
			dataString = QString::fromUtf8( data.constData() + headerSize, data.size() - headerSize );
			emit conn->dataRecieved( dataString, pos, size );
			conn->_commandFinished( false );
			return true;
		}
		return false;
	}

	void abort( RemoteLogClientConnection * conn )
	{
		conn->sendCommand( RemoteLogServerConnection::CMD_CANCEL, QByteArray() );
	}

protected:
	QString mFileName;
	quint32 mSize;
	quint64 mStartPos, mFilePos;
};

class TailFileCommand : public RemoteLogClientConnection::Command
{
public:
	TailFileCommand( int id, const QString & fileName, quint64 startPos )
	: Command( id, RemoteLogClientConnection::TailFile )
	, mFileName( fileName )
	, mReadFilePos( false )
	, mStartPos( startPos )
	, mFilePos( 0 )
	{}

	void start( RemoteLogClientConnection * conn )
	{
		LOG_5( "Sending CMD_TAIL: " + mFileName + " Start Pos: " + QString::number( mStartPos ) );
		QByteArray cmdData;
		QDataStream( &cmdData, QIODevice::WriteOnly ) << quint64(mStartPos);
		cmdData.append( mFileName.toUtf8() );
		mFilePos = mStartPos;
		conn->sendCommand( RemoteLogServerConnection::CMD_TAIL, cmdData );
	}

	bool responseRecieved( RemoteLogClientConnection * conn, quint32 response, const QByteArray & data )
	{
		if( RemoteLogClientConnection::Command::responseRecieved( conn, response, data ) )
			return true;
		if( response == RemoteLogServerConnection::RESPONSE_SERVE ) {
			int headerSize = sizeof( qint64 ) + sizeof( quint32 );
			qint64 pos;
			quint32 size;
			QString dataString;
			{
				QDataStream ds( data );
				ds >> pos >> size;
			}
			if( pos != mFilePos ) {
				LOG_5( "RESPONSE_SERVE_PACKET has incorrect file pos" );
			}
			dataString = QString::fromUtf8( data.constData() + headerSize, data.size() - headerSize );
			emit conn->dataRecieved( dataString, pos, size );
			mFilePos += size;
			return true;
		}
		return false;
	}

	void abort( RemoteLogClientConnection * conn )
	{
		conn->sendCommand( RemoteLogServerConnection::CMD_CANCEL, QByteArray() );
	}

protected:
	QString mFileName;
	bool mReadFilePos;
	qint64 mStartPos;
	qint64 mFilePos;
};

RemoteLogClientConnection::RemoteLogClientConnection( QObject * parent )
: QObject( parent )
, mNextCommandId( 1 )
, mCurrentCommand( 0 )
, mTcpSocket( 0 )
, mResponseIncoming( false )
, mIncomingResponse( 0 )
, mIncomingResponseSize( 0 )
{
}

RemoteLogClientConnection::CommandType RemoteLogClientConnection::currentCommand()
{
	if( mCurrentCommand )
		return mCurrentCommand->mCommandType;
	return None;
}
	
int RemoteLogClientConnection::currentCommandId()
{
	if( mCurrentCommand )
		return mCurrentCommand->mCommandId;
	return 0;
}

QAbstractSocket::SocketState RemoteLogClientConnection::state()
{
	if( !mTcpSocket )
		return QAbstractSocket::UnconnectedState;
	return mTcpSocket->state();
}

int RemoteLogClientConnection::connectToHost( const QHostAddress & host, quint16 port )
{
	int id = newCommandId();
	queueCommand( new ConnectToHostCommand( id, host, port ) );
	startNextCommand();
	return id;
}

int RemoteLogClientConnection::connectToHost( const QString & hostName, quint16 port )
{
	int id = newCommandId();
	queueCommand( new ConnectToHostCommand( id, hostName, port ) );
	startNextCommand();
	return id;
}

void RemoteLogClientConnection::createTcpSocket()
{
	delete mTcpSocket;
	mTcpSocket = new QTcpSocket( this );
	connect( mTcpSocket, SIGNAL( connected() ), SLOT( socketConnected() ) );
	connect( mTcpSocket, SIGNAL( error( QAbstractSocket::SocketError ) ), SLOT( socketError( QAbstractSocket::SocketError ) ) );
	connect( mTcpSocket, SIGNAL( disconnected() ), SLOT( socketDisconnected() ) );
	connect( mTcpSocket, SIGNAL( readyRead() ), SLOT( readyRead() ) );
	connect( mTcpSocket, SIGNAL( stateChanged( QAbstractSocket::SocketState ) ), SLOT( socketStateChanged( QAbstractSocket::SocketState ) ) );
}

void RemoteLogClientConnection::socketStateChanged( QAbstractSocket::SocketState ss )
{
	if( mCurrentCommand )
		mCurrentCommand->socketStateChanged( this, ss );
	emit stateChanged( ss );
}

void RemoteLogClientConnection::socketConnected()
{
}

void RemoteLogClientConnection::socketDisconnected()
{
}

void RemoteLogClientConnection::socketError( QAbstractSocket::SocketError se )
{
	//LOG_5( QString::number((long)this) );
	if( mCurrentCommand )
		mCurrentCommand->socketError( this, se );
}

int RemoteLogClientConnection::getFileSize( const QString & fileName )
{
	int id = newCommandId();
	queueCommand( new GetFileSizeCommand( id, fileName ) );
	startNextCommand();
	return id;
}

int RemoteLogClientConnection::readFile( const QString & fileName, quint64 startPos, quint32 length )
{
	int id = newCommandId();
	queueCommand( new ReadFileCommand( id, fileName, startPos, length ) );
	startNextCommand();
	return id;
}

int RemoteLogClientConnection::tailFile( const QString & fileName, qint64 startPos )
{
	int id = newCommandId();
	queueCommand( new TailFileCommand( id, fileName, startPos ) );
	startNextCommand();
	return id;
}

void RemoteLogClientConnection::clearPendingCommands()
{
	LOG_5( "Clearing " + QString::number( mCommandQueue.size() ) + " commands" );
	foreach( Command * c, mCommandQueue )
		delete c;
	mCommandQueue.clear();
}

void RemoteLogClientConnection::abort()
{
	clearPendingCommands();
	if( mCurrentCommand ) {
		LOG_5( "Aborting current command" );
		mCurrentCommand->abort(this);
	}
}

void RemoteLogClientConnection::sendCommand( int command, const QByteArray & data )
{
	{
		QDataStream ds( &mDataToSend, QIODevice::WriteOnly );
		ds << qint32(data.size()) << qint32(command);
	}
	LOG_5( "Sent Command " + QString::number( command ) + " size " + QString::number( data.size() ) );
	mDataToSend.append( data );
	sendData();
}

void RemoteLogClientConnection::sendData()
{
	while( mTcpSocket->state() == QAbstractSocket::ConnectedState && mDataToSend.size() ) {
		int written = mTcpSocket->write( mDataToSend );
		LOG_5( "Sent " + QString::number( written ) + " bytes, " + QString::number( mDataToSend.size() - written ) + " remaining" );
		if( written <= 0 ) break;
		mDataToSend = mDataToSend.mid( written );
	}
}

void RemoteLogClientConnection::readyRead()
{
	while( mTcpSocket->bytesAvailable() ) {
		// Read a command header if there's enough data available
		if( !mResponseIncoming ) {
			if( mTcpSocket->bytesAvailable() >= sizeof(quint32) * 2 ) {
				QDataStream ds( mTcpSocket );
				ds >> mIncomingResponseSize >> mIncomingResponse;
				mResponseIncoming = true;
				mIncomingData = QByteArray();
				LOG_5( "Read Command Header, Command Size: " + QString::number( mIncomingResponseSize ) + " Command: " + QString::number( mIncomingResponse ) );
			} else
				break;
		}

		// Read command data
		if( mResponseIncoming && mIncomingData.size() < mIncomingResponseSize && mTcpSocket->bytesAvailable() ) {
			// Read at most the rest of the current command
			QByteArray readBytes = mTcpSocket->read( mIncomingResponseSize - mIncomingData.size() );
			mIncomingData.append( readBytes );
			LOG_5( "Read Command Bytes: " + QString::number( readBytes.size() ) + " Remaining: " + QString::number( mIncomingResponseSize - mIncomingData.size() ) );
		}

		// Is this command complete?
		if( mResponseIncoming && mIncomingData.size() == mIncomingResponseSize ) {
			if( mCurrentCommand ) {
				if( !mCurrentCommand->responseRecieved( this, mIncomingResponse, mIncomingData ) ) {
					LOG_5( "Recieved unhandled response: " + QString::number( mIncomingResponse ) + " of size: " + QString::number( mIncomingData.size() ) );
					abort();
				}
			}
			mResponseIncoming = false;
		}
	}
}

void RemoteLogClientConnection::startNextCommand()
{
	if( !mCurrentCommand && !mCommandQueue.isEmpty() ) {
		mCurrentCommand = mCommandQueue.dequeue();
		LOG_5( "Starting command Id: " + QString::number( mCurrentCommand->mCommandId ) + " Command Type: " + QString::number( mCurrentCommand->mCommandType ) );
		emit commandStarted( mCurrentCommand->mCommandId );
		mCurrentCommand->start(this);
	}
}

void RemoteLogClientConnection::_commandFinished( bool error )
{
	int id = mCurrentCommand->mCommandId;
	LOG_5( "commandFinished, Id: " + QString::number( id ) + " error: " + QString( error ? "true" : "false" ) );
	delete mCurrentCommand;
	mCurrentCommand = 0;
	emit commandFinished( id, error );
	if( error ) {
		clearPendingCommands();
		emit done( true );
		return;
	}
	if( mCommandQueue.isEmpty() )
		emit done( false );
	else
		startNextCommand();
}

int RemoteLogClientConnection::newCommandId()
{
	return mNextCommandId++;
}

void RemoteLogClientConnection::queueCommand( Command * command )
{
	mCommandQueue.enqueue( command );
}

} // namespace