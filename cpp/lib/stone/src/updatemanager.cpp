
/*
 *
 * Copyright 2003, 2004 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Assburner is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Assburner is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
 

#include <qdatastream.h>
#include <QTcpSocket>
#include <qstringlist.h>
#include <qtimer.h>
#include <qfile.h>
#include <qthread.h>

#include "blurqt.h"
#include "database.h"
#include "iniconfig.h"
#include "path.h"
#include "record.h"
#include "recordimp.h"
#include "table.h"
#include "tableschema.h"
#include "updatemanager.h"

namespace Stone {

UpdateManager::UpdateManager()
: QObject( 0 )
, mEnabled( true )
, mSocket( 0 )
, mAckTimer( 0 )
, mStatus( StatusDisconnected )
, mFlushAfterConnect( false )
{
	IniConfig & cfg = config();
	cfg.pushSection("Update Server");
	mHost = cfg.readString( "Host", "war.x" );
	mPort = cfg.readInt( "Port", 25565 );
	mRetryRate = cfg.readInt( "RetryRate", 1000 );
	mMaxRetrys = cfg.readInt( "MaxAttempts", -1 );
	mExecOfflineCommands = cfg.readBool( "ExecOfflineCommands", false );
	mLastTransactionFile = cfg.readString( "LastTransactionFile", "update_manager_transaction.last" );
	
	mLastTransactionProcessed = 0;
	
	mEnabled = cfg.readBool( "Enabled", false );
	cfg.popSection();
	
	if( !mEnabled )
		return;
		
	if( mExecOfflineCommands ){
		bool error = false;
		QString lts = readFullFile( mLastTransactionFile, &error );
		if( error )
			LOG_3( "Unable to read last transaction file at " + mLastTransactionFile );
		else
			mLastTransactionProcessed = lts.toUInt();
	}
	
	mAckTimer = new QTimer( this );
	connect( mAckTimer, SIGNAL( timeout() ), SLOT( ack() ) );

	connectToHost();
}

//
// Connection shit
// 
void UpdateManager::connectToHost()
{
	if( mSocket )
		delete mSocket;
	if( mMaxRetrys == 0 )
		return;
	LOG_5( "Connecting to Update Manager at: " + mHost + "." + QString::number( mPort ) );
	mSocket = new PacketSocket( this );
	connect( mSocket, SIGNAL( connected() ), SLOT( connected() ) );
	connect( mSocket, SIGNAL( packetAvailable() ), SLOT( processCommands() ) );
	connect( mSocket, SIGNAL( error( QAbstractSocket::SocketError ) ), SLOT( error( QAbstractSocket::SocketError ) ) );
	connect( mSocket, SIGNAL( disconnected() ), SLOT( connectionClosed() ) );
	mSocket->connectToHost( mHost, mPort );
	if( mMaxRetrys > 0 )
		--mMaxRetrys;
}

void UpdateManager::connected()
{
	LOG_5("Connected to Resin Update Manager");
	
	handshake();
	// Just listen
	if( mLastTransactionProcessed > 0 )
		replay( mLastTransactionProcessed + 1 );
	
	listen();
	
	if( mStatus != StatusConnected ) {
		mStatus = StatusConnected;
		emit statusChanged( true );
	}

	if( mFlushAfterConnect )
		flushPending();
}

void UpdateManager::connectionClosed()
{
	LOG_5("Connection to Resin Update Manager has been lost");
	QTimer::singleShot( mRetryRate, this, SLOT( connectToHost() ) );
	if( mStatus != StatusDisconnected ) {
		mStatus = StatusDisconnected;
		emit statusChanged( false );
	}
	mAckTimer->stop();
}

void UpdateManager::error( QAbstractSocket::SocketError code )
{
	mAckTimer->stop();
	
	QString errorMsg;
	switch( code ) {
		case QAbstractSocket::ConnectionRefusedError:
			errorMsg = "The connection was refused by the peer (or timed out)."; break;
		case QAbstractSocket::RemoteHostClosedError:
			errorMsg = "The remote host closed the connection."; break;
		case QAbstractSocket::HostNotFoundError:
			errorMsg = "The host address was not found."; break;
		case QAbstractSocket::SocketAccessError:
			errorMsg = "The socket operation failed because the application lacked the required privileges."; break;
		case QAbstractSocket::SocketResourceError:
			errorMsg = "The local system ran out of resources (e.g., too many sockets)."; break;
		case QAbstractSocket::SocketTimeoutError:
			errorMsg = "The socket operation timed out."; break;
		case QAbstractSocket::DatagramTooLargeError:
			errorMsg = "The datagram was larger than the operating system's limit (which can be as low as 8192 bytes)."; break;
		case QAbstractSocket::NetworkError:
			errorMsg = "An error occurred with the network (e.g., the network cable was accidentally plugged out)."; break;
		case QAbstractSocket::AddressInUseError:
			errorMsg = "The address specified to QUdpSocket::bind() is already in use and was set to be exclusive."; break;
		case QAbstractSocket::SocketAddressNotAvailableError:
			errorMsg = "The address specified to QUdpSocket::bind() does not belong to the host."; break;
		case QAbstractSocket::UnsupportedSocketOperationError:
			errorMsg = "The requested socket operation is not supported by the local operating system (lack of IPv6 support?)."; break;
		case QAbstractSocket::UnknownSocketError:
			errorMsg = "An unidentified error occurred."; break;
	}

	LOG_3("Connection failed with error: " + errorMsg);

	if( mStatus != StatusDisconnected ) {
		mStatus = StatusDisconnected;
		emit statusChanged( false );
	}
	
	QTimer::singleShot( mRetryRate, this, SLOT( connectToHost() ) );
}

//
// Command buffer shit
//
void UpdateManager::queuePacket( const Packet & pkt )
{
	mPendingPackets.append(pkt);
}

void UpdateManager::flushPending()
{
	if( mStatus == StatusConnected ) {
		foreach( Packet pkt, mPendingPackets )
			mSocket->sendPacket(pkt);
		mPendingPackets.clear();
		mFlushAfterConnect = false;
	} else {
		mFlushAfterConnect = true;
	}
}

void UpdateManager::clearPending()
{
	mPendingPackets.clear();
}

void UpdateManager::processCommands()
{
	while( mSocket->availablePacketCount() ) {
		parsePacket( mSocket->nextPacket() );
	}
}

//
// Functions that generate and buffer command packets
//

void UpdateManager::handshake()
{
	LOG_5( "Sending handshake" );
	mSocket->sendPacket( Packet( 123456789, QDS() << quint32(1) ) ); // Magic, Version
}

void UpdateManager::ack()
{
	LOG_5( "Sending ACK command" );
	mSocket->sendPacket( Packet( ACK ) );
}

void UpdateManager::listen()
{
	LOG_5( "Sending LISTEN command" );
	mSocket->sendPacket( Packet( LISTEN, QDS() << quint32(0) ) );
}

void UpdateManager::replay( uint transactionStart, uint transactionEnd )
{
	LOG_5( "Sending REPLAY command" );
	mSocket->sendPacket( Packet( REPLAY, QDS() << quint32(transactionStart) << quint32(transactionEnd) ) );
}

void UpdateManager::recordsAdded( Table * table, const RecordList & records )
{
	if( !mEnabled )
		return;
		
	quint32 count = records.size();
	FieldList fields = table->schema()->columns();
	
	// We don't count the primary key
	quint32 fieldCount = fields.size() - 1;
	
	// This is just a rough minimum to avoid the bytearray resizing
	// more than it will anyway
	uint sizeMin = count * ( fieldCount * 4 + 4 ) + fieldCount * 8 + 8;
	
	QDS ds(sizeMin);
	// table name
	ds << table->schema()->tableName();
	// record count
	ds << count;
	// field count
	ds << fieldCount;
		
	// Each field's name
	foreach( Field * f, fields )
		if( f->flag( Field::PrimaryKey ) )
			continue;
		else
			ds << f->name();
		
	foreach( Record r, records )
	{
		ds << quint32(r.key());
		foreach( Field * f, fields )
			if( f->flag( Field::PrimaryKey ) )
				continue;
			else
				ds << r.getValue( f->pos() );
	}

	queuePacket( Packet( ADDED, ds ) );
}

void UpdateManager::recordUpdated( Table * table, Record cur, Record old, bool sendAllFields )
{
	if( !mEnabled )
		return;
		
	QMap<QString,QVariant> fieldToValue;
	
	FieldList fields = table->schema()->columns();
	
	for( FieldIter it = fields.begin(); it != fields.end(); ++it )
	{
		Field * f = *it;
		if( f->flag( Field::PrimaryKey ) )
			continue;
			
		QVariant curv( cur.getValue( f->pos() ) );
		QVariant oldv( old.getValue( f->pos() ) );
		if( sendAllFields || curv != oldv ) {
			LOG_5( "recordUpdated: " + f->name() +
				" was updated from " + oldv.toString() + " to " + curv.toString() );
			fieldToValue[f->name()] = curv;
		}
	}
	
	QDS ds;
	ds << table->schema()->tableName();
	ds << quint32(cur.key());
	ds << quint32(fieldToValue.size());
	for( QMap<QString,QVariant>::Iterator it = fieldToValue.begin(); it != fieldToValue.end(); ++it )
		ds << it.key() << it.value();

	queuePacket( Packet( UPDATED, ds ) );
}

void UpdateManager::recordsDeleted( Table * table, const RecordList & records )
{
	if( !mEnabled )
		return;
		
	QDS ds;
	ds << table->schema()->tableName();
	ds << quint32( records.size() );
	foreach( Record r, records )
		ds << quint32(r.key());

	queuePacket( Packet( REMOVED, ds ) );
}

void UpdateManager::recordsDeleted( const QString & table, QList<uint> keys )
{
	if( !mEnabled )
		return;
		
	QDS ds;
	ds << table;
	ds << quint32( keys.size() );
	foreach( uint key, keys )
		ds << quint32( key );

	queuePacket( Packet( REMOVED, ds ) );
}

//
// Packet parsing
//
void UpdateManager::parsePacket( const Packet & pkt )
{
	QDS ds(pkt.data);

	quint32 transaction;
	
	ds >> transaction;
	
	//LOG_5( QString("Recieved Transaction %1 of size %2").arg( transaction ).arg( size ) );
	switch( pkt.id )
	{
		case ADDED:
			//qWarning( "Packet was of type ADDED" );
			parseAdded( ds, transaction );
			break;
		case UPDATED:
			//qWarning( "Packet was of type UPDATED" );
			parseUpdated( ds, transaction );
			break;
		case REMOVED:
			//qWarning( "Packet was of type REMOVED" );
			parseDeleted( ds, transaction );
			break;
		default:
			LOG_3( "Packet was of unknown type: " + QString::number( pkt.id ) );
	}
	
	mLastTransactionProcessed = transaction;
	
	if( mExecOfflineCommands ) {
		if( !writeFullFile( mLastTransactionFile, QString::number( mLastTransactionProcessed ) ) )
			LOG_3( "Error writing last transaction to file " + mLastTransactionFile );
	}
}

void UpdateManager::parseAdded( QDS & s, uint transaction )
{
	quint32 recordCount, fieldCount;
	QString tableName;
	RecordList ret;
	Table * table;
	uint pkeyIndex;
	
	s >> tableName;
	table = Database::current()->tableByName( tableName );
	if( !table ) {
		LOG_5( "Couldn't find table: " + tableName + QString(" for transaction %1").arg( transaction ) );
		return;
	}
	pkeyIndex = table->schema()->primaryKeyIndex();
	
	s >> recordCount >> fieldCount;
	//qWarning( QString("ADDED: recordCount = %1, fieldCount = %2").arg(recordCount).arg(fieldCount) );
	int * fieldPos = new int[fieldCount];
	for( uint i=0; i<fieldCount; ++i )
	{
		QString field;
		s >> field;
		int fp = table->schema()->fieldPos( field );
	//	qWarning( QString("ADDED: field %1 has index %2").arg( field ).arg( i ) );
		if( fp < 0 ) {
			LOG_5( QString("Couldn't find field pos for field: %1 while parsing transaction %2")
				.arg(field).arg(transaction));
		} else
		fieldPos[i] = fp;
	}
	
	int ncols = table->schema()->fields().size();
	
	QVariant * vars = new QVariant[ncols];
	for( int i=0; i<ncols; i++ )
		vars[i] = QVariant();
	
	for( uint i=0; i<recordCount; ++i )
	{
		quint32 key;
		s >> key;
		//qWarning( "ADDED: pkey is " + QString::number( key ) );
		vars[pkeyIndex] = QVariant( key );
		for( uint n=0; n<fieldCount; n++ ) {
			if( fieldPos[n] >= 0 && fieldPos[n] < ncols ) {
				//qWarning( QString( "Field %1, Column Pos %2 out of %3" ).arg( n ).arg( fieldPos[n] ).arg( ncols ) );
				s >> vars[fieldPos[n]];
			} else {
				QVariant v;
				s >> v;
			}
			//qWarning( QString("ADDED: field %1 has value %2").arg( n ).arg( vars[fieldPos[n]].toString() ) );
		}
		ret += table->load( vars );
	}
	if( ret.size() ) {
		table->recordsAdded( ret );
		Database::current()->recordsAdded( ret, false );
	}
	delete [] vars;
	delete [] fieldPos;
}

void UpdateManager::parseUpdated( QDS & s, uint transaction )
{
	quint32 fieldCount, pkey;
	QString tableName;
	Table * table;
	
	// Get the table name
	s >> tableName;
	
	// Get the table
	table = Database::current()->tableByName( tableName );
	if( !table ) {
		LOG_5( "Couldn't find table: " + tableName + QString(" for transaction %1").arg( transaction ) );
		return;
	}
	
	// Get the key, and number of updated fields
	s >> pkey >> fieldCount;
	
	// Find the record
	Record r = table->record( pkey, false );
	if( !r.isRecord() ) {
		LOG_3( "parseUpdated: Could not find record with pkey " + QString::number( pkey ) );
		return;
	}
	
	//LOG_3( table->tableName() + " record " + QString::number( r.key() ) + " UPDATED" );
	
	// Make a copy, for old
	Record old = r.copy();

	// Set it as modified	
	r.imp()->mState |= RecordImp::MODIFIED;

	// Update the fields
	for( uint i=0; i<fieldCount; ++i )
	{
		QString field;
		QVariant var;
		s >> field >> var;
		//LOG_3( "parseUpdated: field:"+field+" updated to: " + var.toString() );
		r.imp()->setColumn( table->schema()->fieldPos( field ), var );
	}

	r.imp()->mState = RecordImp::COMMITTED;
	
	// Signal the update
	table->recordUpdated( r, old );
	Database::current()->recordUpdated( r, old, false );
}

void UpdateManager::parseDeleted( QDS & s, uint transaction )
{
	QString tableName;
	quint32 recordCount;
	RecordList ret;
	
	s >> tableName;
	
	Table * table = Database::current()->tableByName( tableName );
	if( !table ) {
		LOG_5( "Couldn't find table: " + tableName + QString(" for transaction %1").arg( transaction ) );
		return;
	}

	s >> recordCount;
	
	//qWarning( QString("Table %1 got %2 deletions").arg( tableName ).arg( recordCount ) );
	
	for(uint i=0; i<recordCount; i++ )
	{
		quint32 pkey;
		s >> pkey;
		Record r( table->record( pkey, false ) );
		if( r.isRecord() ){
			r.imp()->mState |= RecordImp::DELETED;
			ret += r;
		} else {
		//	qWarning( "Couldn't find record with key: " + QString::number( pkey ) );
		}
	}
	if( ret.size() ) {
		table->recordsRemoved( ret );
		Database::current()->recordsRemoved( ret, false );
	}
}

UpdateManager * UpdateManager::mSelf=0;

UpdateManager * UpdateManager::instance()
{
	if( !mSelf )
		mSelf = new UpdateManager;
	return mSelf;
}

} // namespace