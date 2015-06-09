
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

#ifndef UNDO_MANAGER_H
#define UNDO_MANAGER_H

#include <qobject.h>
#include <qlist.h>
#include <qstringlist.h>

#include "record.h"
#include "recordlist.h"

namespace Stone {

/**
 * \ingroup Stone
 * @{
 */

/**
 * Interface class for generic Undo/Redo functionality
 */
class STONE_EXPORT UndoOperation
{
public:
	UndoOperation():mUndone(false){}
	virtual ~UndoOperation(){}
	/// Implement undo functionality in this function
	virtual void undo()=0;
	/// Implement redo functionality in this function
	virtual void redo()=0;
	/// Roll back any changes that have been made
	/// before this block is finished
	virtual void rollback()=0;
protected:
	bool mUndone;
};

#undef DELETE

/**
 * Implements the UndoOperation class in order to provide Undo/Redo
 * support for any database operations that are executed.
 *
 *  It rolls back the changes by deleting INSERTS, inserting DELETES,
 *  and updating previous updates with the old data.
 */
class STONE_EXPORT RecordUndoOperation : public UndoOperation
{
public:
	enum { INSERT,
            DELETE,
            UPDATE };
	
	RecordUndoOperation( RecordList, int type );
	virtual ~RecordUndoOperation(){}
	
	virtual void undo();
	virtual void redo();
	virtual void rollback();
	static const char * typeToString(int type);
protected:
	RecordList mRecords;
	int mType;
};

/**
 *  This class keeps two stacks of UndoOperation objects.  One
 *  stack for Undo, and another for redo.  It provides functions
 *  to view those stacks, and to undo or redo any number of operations.
 *  
 *  To provide custom operations, one must implement the \ref UndoOperation
 *  class.  \ref RecordUndoOperation class provides for undo/redoing any
 *  database transactions.
 */
class STONE_EXPORT UndoManager : public QObject
{
Q_OBJECT
public:
	UndoManager( QObject * parent = 0 );
	~UndoManager();

	/// Returns true if there are any operations on the undo stack
	bool canUndo();
	/// Returns true if there are any operations on the redo stack
	bool canRedo();

	/// Returns the single UndoManager instance for the application
	static UndoManager * instance();
	
//	bool insideUndoRedo();
	
	QStringList undoTitles( int count = -1 );
	QStringList redoTitles( int count = -1 );
	
public slots:
	/// Performs undo on the last \ref count blocks of the undo
	/// stack.  If count is greater than the number of blocks
	/// on the undo stack, then all the blockss will be undone.
	void undo( int count = 1 );

	/// Performs redo on the last \ref count blocks of the redo
	/// stack.  If count is greater than the number of blocks
	/// on the redo stack, then all the blocks will be redone.
	void redo( int count = 1 );

	/// Starts a new undo block
	void startBlock( const QString & title = QString::null );

	/// Commits the current undo block to the undo stack
	void commitBlock();

	/// Rolls back each operation in the current undo block.
	void rollbackBlock();

	/// Pushes a title onto the title stack for the current undo block
	void pushTitle( const QString & );

	/// Pops the top title from the title stack for the current undo block
	void popTitle();
	
	/// Adds an undo operation to the current undo block.
	void addOperation( UndoOperation * );
	
signals:
	/// This signal is emitted each time the result of
	/// canUndo() or canRedo() changes.
	void undoRedoChange( bool canUndo, bool canRedo );

protected:

	bool mInsideUndoRedo;

	class UndoBlock 
	{
	public:
		UndoBlock( const QString & title = QString::null )
		: mTitle( title )
		{}
		
		~UndoBlock() {
			foreach( UndoOperation * op, operations )
				delete op;
		}
			
		QString title() const { return mTitle; }
		void setTitle( const QString & title ) { mTitle = title; }

		QList<UndoOperation*> operations;
		
		protected:
		QString mTitle;
	};
	
	typedef QList<UndoBlock*> UndoStack;
	typedef QList<UndoBlock*>::Iterator UndoStackIter;

	QStringList getLastFromStack( int count, UndoStack & );

	UndoBlock * mCurrentBlock;
	UndoStack mUndoStack, mRedoStack;

	QStringList mTitleStack;
	QString mCurrentTitle;
	
	static UndoManager * mSelf;
};

/// @}

} //namespace

using Stone::UndoOperation;
using Stone::RecordUndoOperation;
using Stone::UndoManager;

#endif // UNDO_MANAGER_H

