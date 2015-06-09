
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

#ifndef TRANSACTION_MGR_H
#define TRANSACTION_MGR_H

#include <qlist.h>
#include <qstring.h>
#include <qthread.h>
#include <qmutex.h>
#include <qhash.h>

#include <utility>

#include "blurqt.h"

class QFile;

namespace Stone {

class STONE_EXPORT TransactionMgr : public QThread
{
public:
	TransactionMgr();

	/// Filename will be appended with .index and .data for the actual files
	bool initialize( const QString & fileName );
	
	virtual void run();
	
	void releaseOldCommands();

	void writeTransaction( uint transaction, const QByteArray & data );
	QByteArray loadTransaction( uint transaction );

	void appendTransaction( QByteArray transaction );
	
	////////////////////////////////////////////
	//////// thread safe ///////////////////////
	////////////////////////////////////////////
	uint lastTransaction();
	
	uint getTransactionId();
	
	void setTransaction( uint id, const QByteArray & packet );
	
	// Returns true if the transaction is updated to id,
	// returns false if it is still waiting on a transaction
	// smaller than id to be set
	bool updateLastTransaction( uint id );

	QByteArray getTransaction( uint id, uint version );

	////////////////////////////////////////////
	/////// end thread safe ////////////////////
	////////////////////////////////////////////
	
protected:
	QMutex mTransactionsToWriteMutex;
	QList< std::pair<uint,QByteArray> > mTransactionsToWrite;
	
	QMutex mFilesMutex;
	QFile * mIndexFile, * mDataFile;

	QMutex mTransactionMutex;
	QHash<int,QByteArray> mTransactions;

	QMutex mNextTransactionMutex;
	uint mNextTransaction;
	
	QMutex mLastTransactionMutex;
	uint mLastTransaction;
	
	QFile * mTransactionFile;
};

} //namespace

#endif // TRANSACTION_MGR_H

