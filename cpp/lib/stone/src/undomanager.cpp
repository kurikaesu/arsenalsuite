
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

#include <qmenu.h>

#include "blurqt.h"
#include "database.h"
#include "undomanager.h"
#include "recordimp.h"
#include "table.h"

namespace Stone {

const char * RecordUndoOperation::typeToString(int type)
{
	return type==INSERT ? "INSERT" : (type == DELETE ? "DELETE" : "UPDATE");
}

RecordUndoOperation::RecordUndoOperation( RecordList list, int type )
: mRecords( list )
, mType( type )
{
	LOG_5( "RecordUndoOperation::ctor: Creating record undo operation for " + QString(typeToString(mType) ) );
}

void RecordUndoOperation::undo()
{
	if( mType == INSERT || mType == DELETE ) {
		RecordList temp = mRecords.reversed();
		if( mType == INSERT ) {
			temp.remove();
		} else {
			// Clear the DELETED state
			st_foreach( RecordIter, it, temp )
				it.imp()->mState = RecordImp::NEWRECORD | RecordImp::COMMIT_ALL_FIELDS;
			temp.commit( false );
		}
	} else if( mType ==  UPDATE ) {
		RecordList list = mRecords;
		Record oldvals( mRecords[0].imp()->copy(), false );
		mRecords[1].commit();
		mRecords[1] = oldvals;
	}
}

void RecordUndoOperation::redo()
{
	switch( mType ) {
		case INSERT:
			mRecords.commit( false );
			break;
		case DELETE:
			mRecords.remove();
			break;
		case UPDATE:
		{
			RecordList list = mRecords;
			Record oldvals( mRecords[0].imp()->copy(), false );
			mRecords[1].commit();
			mRecords[1] = oldvals;
		}
	}
}

// This resets the application state
// without executing any sql,
// the sql already executed will be undone
// via rolling back the transaction
void RecordUndoOperation::rollback()
{
	// We can't rollback, if it's already been undone
	if( !mUndone ) {
		switch( mType ) {
			case INSERT:
				st_foreach( RecordIter, it, mRecords )
					it.imp()->mState = RecordImp::DELETED;
				mRecords[0].table()->recordsRemoved( mRecords );
				Database::current()->recordsRemoved( mRecords );
				break;
			case DELETE:
				st_foreach( RecordIter, it, mRecords )
					it.imp()->mState = RecordImp::COMMITTED;
				mRecords[0].table()->recordsAdded( mRecords );
				Database::current()->recordsAdded( mRecords );
				break;
			case UPDATE:
			{
				Record old( mRecords[1] );
				mRecords[0].mImp->set( mRecords[1].mImp->values()->data() );
				mRecords[0].mImp->mState = RecordImp::COMMITTED;
				mRecords[1].mImp->mState = RecordImp::COMMITTED | RecordImp::MODIFIED;
				mRecords[0].table()->recordUpdated( mRecords[0], mRecords[1] );
				Database::current()->recordUpdated( mRecords[0], mRecords[1] );
			}
		}
	}
}

UndoManager * UndoManager::mSelf = 0;

UndoManager * UndoManager::instance()
{
	if( !mSelf )
		mSelf = new UndoManager;
	return mSelf;
}

UndoManager::UndoManager( QObject * parent )
: QObject( parent )
, mInsideUndoRedo( false )
, mCurrentBlock( 0 )
{}

UndoManager::~UndoManager()
{
	foreach( UndoBlock * ub, mUndoStack )
		delete ub;
	foreach( UndoBlock * ub, mRedoStack )
		delete ub;
}

QStringList UndoManager::getLastFromStack( int count, UndoStack & stack )
{
	QStringList ret;
	for( int i = stack.size() - 1; i >= 0 && count > 0; i--, count-- )
	{
		UndoBlock * ub = stack[i];
		QString ttl = ub->title();
		ret += ttl.isEmpty() ? "Unknown Operation" : ttl;
	}
	return ret;
}

QStringList UndoManager::undoTitles( int count )
{
	if( count < 0 ) count = mUndoStack.size();
	return getLastFromStack( count, mUndoStack );
}

QStringList UndoManager::redoTitles( int count )
{
	if( count < 0 ) count = mRedoStack.size();
	return getLastFromStack( count, mRedoStack );
}

void UndoManager::startBlock( const QString & title )
{
	if( !mCurrentBlock ) {
		mCurrentBlock = new UndoBlock( title );
		mCurrentTitle = title;
		mTitleStack += title;
	}
}

void UndoManager::commitBlock()
{
	if( mCurrentBlock ) {
		if( mCurrentBlock->operations.isEmpty() )
			delete mCurrentBlock;
		else {
			mUndoStack.push_back( mCurrentBlock );
			foreach( UndoBlock * ub, mRedoStack )
				delete ub;
			mRedoStack.clear();
			emit undoRedoChange( !mUndoStack.isEmpty(), !mRedoStack.isEmpty() );
		}
		mCurrentBlock = 0;
		mTitleStack.clear();
	}
}

void UndoManager::rollbackBlock()
{
	if( mCurrentBlock ) {
		foreach( UndoOperation * uo, mCurrentBlock->operations )
			uo->rollback();
		delete mCurrentBlock;
		mCurrentBlock = 0;
		mTitleStack.clear();
	}
}

void UndoManager::pushTitle( const QString & title )
{
	if( !title.isEmpty() )
		mCurrentTitle = title;
	mTitleStack.push_back( title );
}

void UndoManager::popTitle()
{
	mTitleStack.pop_back();
	if( !mTitleStack.isEmpty() && !mTitleStack.back().isEmpty() )
		mCurrentTitle = mTitleStack.back();
}

void UndoManager::addOperation( UndoOperation * op )
{
	if( mInsideUndoRedo || !mCurrentBlock ) {
		delete op;
		return;
	}
	
	mCurrentBlock->operations.append( op );
	mCurrentBlock->setTitle( mCurrentTitle );
}

bool UndoManager::canUndo()
{
	return !mUndoStack.isEmpty();
}

bool UndoManager::canRedo()
{
	return !mRedoStack.isEmpty();
}

void UndoManager::undo( int count )
{
	if( mInsideUndoRedo || Database::current()->insideTransaction() || mUndoStack.isEmpty() )
		return;
		
	Database::current()->beginTransaction();
	mInsideUndoRedo = true;
	
	for( int i=0; i<count; i++ )
	{
		if( mUndoStack.isEmpty() )
			break;
			
		UndoBlock * ub = mUndoStack.back();
		mUndoStack.pop_back();
	
		// Must go through each operation in the block
		// in reverse order
		for( int i = ub->operations.size()-1; i >= 0; i-- )
			ub->operations[i]->undo();
	
		mRedoStack.push_back( ub );
	}
	
	mInsideUndoRedo = false;
	Database::current()->commitTransaction();
	emit undoRedoChange( !mUndoStack.isEmpty(), !mRedoStack.isEmpty() );
}

void UndoManager::redo( int count )
{
	if( Database::current()->insideTransaction() || mInsideUndoRedo || mRedoStack.isEmpty() )
		return;

	mInsideUndoRedo = true;
	Database::current()->beginTransaction();
	
	for( int i = 0; i< count; i++ )
	{
		if( mRedoStack.isEmpty() )
			break;
	
		UndoBlock * ub = mRedoStack.back();
		mRedoStack.pop_back();
	
		// For redo, go through in the original order
		foreach( UndoOperation * uo, ub->operations )
			uo->redo();
	
		mUndoStack.push_back( ub );
	}
	
	mInsideUndoRedo = false;
	Database::current()->commitTransaction();
	
	emit undoRedoChange( !mUndoStack.isEmpty(), !mRedoStack.isEmpty() );
}

} //namespace

