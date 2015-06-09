
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

#include <stdlib.h>

#include <qfile.h>
#include <qdatastream.h>
#include <qlist.h>
#include <qdir.h>

#include "blurqt.h"
#include "transactionmgr.h"

namespace Stone {

TransactionMgr::TransactionMgr()
: mIndexFile( 0 )
, mDataFile( 0 )
, mNextTransaction( 1 )
, mLastTransaction( 0 )
{
}

bool TransactionMgr::initialize( const QString & spoolFileName )
{
	QFileInfo fi( spoolFileName );
	QDir spoolDir = fi.dir();

	if( !spoolDir.exists() ) {
		if( !spoolDir.mkpath(".") ) {
			LOG_1( "Could not create (non-existent) spool dir at: " + spoolDir.path() );
			return false;
		}
	}

	mIndexFile = new QFile( spoolFileName + ".index" );
	mDataFile = new QFile( spoolFileName + ".data" );

	if ( !mIndexFile->open( QIODevice::ReadWrite ) ) {
		LOG_1("Could not open transaction index file at:" + mIndexFile->fileName() + " error code: " + QString::number(mIndexFile->error()));
		return false;
	}

	if ( !mDataFile->open( QIODevice::ReadWrite ) ) {
		LOG_1("Could not open transaction data file at:"+ mDataFile->fileName() + " error code: "+ QString::number(mIndexFile->error()));
		return false;
	}

	mLastTransaction = mIndexFile->size() / 8;
	mNextTransaction = mLastTransaction + 1;

	LOG_3( QString( "Transaction Manager Started: last transaction: %1  next transaction %2")
		.arg( mLastTransaction ).arg( mNextTransaction ) );

	return true;
}

void TransactionMgr::run()
{
	while( 1 ) {
		while( mTransactionsToWrite.size() ){
			// Get the transaction to write
			mTransactionsToWriteMutex.lock();
			std::pair<uint,QByteArray> toWrite = mTransactionsToWrite.back();
			mTransactionsToWrite.pop_back();
			mTransactionsToWriteMutex.unlock();
			
			writeTransaction( toWrite.first, toWrite.second );
		}
		msleep( 50 );
	}
}

QByteArray TransactionMgr::loadTransaction( uint transaction )
{
	mFilesMutex.lock();
	QDataStream ds( mIndexFile );
	mIndexFile->seek( (transaction-1) * 8 );
	quint32 pos, size;
	ds >> pos >> size;
	
	if( pos + size > mDataFile->size() ) {
		qWarning( "Fatal error: corrupt index file" );
		qWarning( qPrintable(QString("transaction %1 out of range: pos %2 size %3").arg( transaction ).arg( pos ).arg( size ) ) );
		return 0;
	}
	
	QByteArray array( size, 0 );
	mDataFile->seek( pos );
	mDataFile->read( array.data(), size );
	mFilesMutex.unlock();
	return array;
}

void TransactionMgr::writeTransaction( uint transaction, const QByteArray & data )
{
	mFilesMutex.lock();
	
	quint32 indexPos = (transaction-1) * 8;
	uint indexSize = mIndexFile->size();
	uint dataPos = mDataFile->size();
	
	if( indexPos > indexSize ) {
		char * tmp = (char*)malloc( indexPos - indexSize );
		mIndexFile->seek( indexSize );
		mIndexFile->write( tmp, indexPos - indexSize );
		delete tmp;
	}
	
	// Write the index data
	{
		QDataStream ds( mIndexFile );
		mIndexFile->seek( indexPos );
		ds << dataPos << data.size();
	}
	
	// Write the data
	mDataFile->seek( dataPos );
	mDataFile->write( data.data(), data.size() );
	
	mFilesMutex.unlock();
}

QByteArray TransactionMgr::getTransaction( uint id, uint /* version */ )
{
	QByteArray ba = mTransactions[id];
	if( !ba.size() ){
		qWarning( qPrintable(QString("Transaction %1 not in cache, loading...").arg( id )) );
		ba = loadTransaction( id );
		if( !ba.size() ){
			qWarning( qPrintable(QString("Transaction %1 is not in cache or data files").arg(id)) );
			return 0;
		}
		qWarning( "Transaction loaded successfully" );
		mTransactions.insert( id, ba );
	}
	return ba;
}

void TransactionMgr::releaseOldCommands()
{
}

uint TransactionMgr::getTransactionId()
{
	mNextTransactionMutex.lock();
	uint transactionId = mNextTransaction++;
	mNextTransactionMutex.unlock();
	return transactionId;
}

void TransactionMgr::setTransaction( uint id, const QByteArray & packet )
{
	mTransactionMutex.lock();
	mTransactions.insert( id, packet );
	mTransactionMutex.unlock();
	
	mTransactionsToWriteMutex.lock();
	mTransactionsToWrite += std::pair<uint,QByteArray>(id, packet);
	mTransactionsToWriteMutex.unlock();
}

bool TransactionMgr::updateLastTransaction( uint id )
{
	if( id > mLastTransaction + 1 )
		return false;
	mLastTransactionMutex.lock();
	mLastTransaction = id;
	mLastTransactionMutex.unlock();
	return true;
}
	
uint TransactionMgr::lastTransaction()
{
	return mLastTransaction;
}

} // namespace