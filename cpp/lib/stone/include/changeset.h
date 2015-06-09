
/*
 *
 * Copyright 2012 Blur Studio Inc.
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

#ifndef CHANGESET_H
#define CHANGESET_H

#include <qundostack.h>

#include "blurqt.h"

class QUndoStack;

namespace Stone {

class RecordList;
class Record;
class ChangeSetUndoCommand;
class ChangeSetNotifier;
class ChangeSetWeakRef;
class Table;
struct VRCH;

/*
 * Transaction safe commit strategy:
 * 
 * Changesets are committed within a single transaction to guarantee atomicity of the changeset commit.
 * In order to keep the in memory state consistent during a failed commit, we need to keep enough information
 * to be able to restore the pre-commit state of the changeset records.
 * 
 * In order to keep the changesets isolated from the rest of the application until it is assured the changeset
 * commit will succeed, we must execute all operations in two stages.
 * The first stage may throw exceptions and must be reversable.
 * In order to keep track of each change's commit state we store a commit state void * with the change.
 * This commitState allows the change to either be completed if nothing goes wrong during stage 1, or
 * allows the change to be completely reversed if something does go wrong.
 * 
 * On the database side we simply start a transaction before running stage 1, if stage 1 completes then we
 * commit the transaction and run stage 2.  If any errors happens during stage 1 then we roll back the transaction
 * and rollback all the changes made by stage 1.
 * 
 * Stage 2 is designed to never fail.  No sql is executed so there is no chance of sql errors or database connection
 * issues.  During stage 2 post triggers are executed.  Any exceptions thrown by post triggers are caught and discarded
 * with a simple warning.
 */

class STONE_EXPORT ChangeSet
{
public:
	enum ChangeType {
		Change_Insert,
		Change_Update,
		Change_Remove,
		// Indicates the change is a nested changeset
		Change_Nested,
		// This will be returned from changeType if the record has no changes, or is otherwise invalid
		Change_Invalid,
	};
	static QString changeTypeString(ChangeType);
	
	/// Determines which contents of a record are returned for reads when this changeset is current
	///  - Read_Current will read the values that have changed with the current changeset
	///  - Read_Direct will read whatever values are contained by the RecordImp that the Record points to
	///     and you can use Record::version to get whichever RecordImp you want first.
	///  - Read_Pristine will read values from the unchanged version of the Record, if one exists.
	///
	///  Generally Read_Current, the default, is all you will ever need.  The others are used internally,
	///  specifically for delivering the update signals where it must be possible to read both the pristine
	///  and the current changed records at the same time.  Read_Direct is used for this, as the correct
	///  versions are passed.
	enum ReadMode {
		Read_Current,
		Read_Direct,
		Read_Pristine,
		// This is not an actual read mode, but a value that can be passed to a ChangeSetEnabler
		// which will set the read mode based on the state of the changeset.  If the top-level
		// changeset is committed it will be Read_Pristine, otherwise it will be Read_Current
		Read_Default
	};
	
	enum State {
		Invalid,
		Unchanged,
		ChangesPending,
		Committed,
		Undone
	};
	static QString stateString(State);
	
	static ChangeSet current();
	
	static ChangeSet create( const QString & title = QString(), QUndoStack * undoStack = 0 );
	ChangeSet createChild( const QString & title = QString(), QUndoStack * undoStack = 0 );
	
	/// Constructs an Invalid Changeset.  Use ChangeSet::create or createChild to construct
	/// a working changeset.
	ChangeSet();
	ChangeSet( const ChangeSet & other );
	~ChangeSet();
	
	ChangeSet & operator=( const ChangeSet & other );
	
	bool operator==( const ChangeSet & other ) const { return p == other.p; }
	bool operator!=( const ChangeSet & other ) const { return p != other.p; }
	
	bool isValid() const { return p!=0; }
	
	QString title() const;
	void setTitle( const QString & title );
	
	QUndoStack * undoStack() const;
	ChangeSetUndoCommand * undoCommand() const;
	
	ChangeSet parent() const;
	State state() const;
	
	ReadMode readMode() const;

	/// If detach is true then the contents of the changeset are moved to a new changeset 
	/// which will have Committed state and this changeset will be set to Unchanged and no longer
	/// contain any changes.
	/// The return value is the changeset containing the committed changes, which can be undone/redone
	/// via the undo/redo methods.  Those methods should only be called through the QUndoManager system
	/// unless it is not in use.
	ChangeSet commit( bool detach = false );
	
	/// This reverts all changes which are discarded and the state will be set to Unchanged
	/// This function only works when in ChangesPending state.
	void revert();
	
	/// Only usable when state is Committed
	void undo();
	/// Only usable when state is Undone
	void redo();
	
	struct Change;
	
	QList<ChangeSet::Change> changes();
	
	void visibleRecordsChanged( RecordList * added = 0, RecordList * updated = 0, RecordList * removed = 0, QList<Table*> filter = QList<Table*>() );
	
	QString debug(int tab=0);
	
	// Returns true if cs is a direct anscestor(self, parent, grandparent...) of this
	bool isAnscestor( const ChangeSet & cs ) const;
	
	int depth() const;

	ChangeSet topLevel() const;

	// Determines whether changeset other is visible to this 
	// based on parent/child relationship and commit status
	bool isVisible( const ChangeSet & other ) const;

	static ChangeSet commonParent( ChangeSet a, ChangeSet b );

protected:
	class Private;
	ChangeSet( ChangeSet::Private * );
	
	static ChangeType changeType(const Record &);
	
	void queue( ChangeType, RecordList );
	void _commit(bool);
	void _commitComplete();
	void _commitRollback();
	void _commitHelper(bool);
	void addNotifier( ChangeSetNotifier * );
	void removeNotifier( ChangeSetNotifier * );
	void setReadMode( ChangeSet::ReadMode );
	
	enum DeliverSignalsMode {
		QueueMode,
		ChildCommitMode,
		UndoMode,
		RedoMode,
	};
	void deliverSignals( ChangeSet::Change, DeliverSignalsMode mode, ChangeSet skipChildNotifiers = ChangeSet() );
	Private * p;
	
	void visibleRecordsChangedHelper( const ChangeSet & visibleTo, VRCH * );
	
	friend class RecordImp;
	friend class RecordList;
	friend class Record;
	friend class ChangeSetEnabler;
	friend class ChangeSetNotifier;
	friend class ChangeSetWeakRef;
};

class STONE_EXPORT ChangeSetUndoCommand : public QUndoCommand
{
public:
	virtual void redo();
	virtual void undo();
	
	ChangeSet changeSet() const;
protected:
	ChangeSetUndoCommand( ChangeSet cs );
	
	ChangeSet mChangeSet;
	friend class ChangeSet;
};

class STONE_EXPORT ChangeSetWeakRef
{
public:
	ChangeSetWeakRef();
	ChangeSetWeakRef(const ChangeSet & cs);
	ChangeSetWeakRef(const ChangeSetWeakRef & cs);
	~ChangeSetWeakRef();
	
	void operator=( const ChangeSet & cs );
	void operator=( const ChangeSetWeakRef & cswr );
	
	bool operator==( const ChangeSet & other ) const { return other.p == p; }
	bool operator==( const ChangeSetWeakRef & other ) const { return other.p == p; }
	
	bool operator!=( const ChangeSet & other ) const { return other.p != p; }
	bool operator!=( const ChangeSetWeakRef & other ) const { return other.p != p; }
	
	ChangeSet operator()() const { return ChangeSet(p); }
	operator ChangeSet() const { return ChangeSet(p); }
	
	QString pointerString() { return QString::number( (qulonglong)p, 16 ); }
private:
	ChangeSet::Private * p;
	ChangeSetWeakRef * next;
	friend class ChangeSet::Private;
};

}; // namespace Stone

#include "recordlist.h"

namespace Stone {
	
struct STONE_EXPORT ChangeSet::Change {
	ChangeSet::ChangeType type;
	ChangeSet changeSet() const;
	RecordList records() const;
protected:
	Change( ChangeType type, RecordList records );
	Change( ChangeSet );
	ChangeSetWeakRef changeset;
	RecordList mRecords;
	void * commitState;
	friend class ChangeSet;
};

class STONE_EXPORT ChangeSetEnabler
{
public:
	ChangeSetEnabler( const ChangeSet & changeSet, ChangeSet::ReadMode readMode = ChangeSet::Read_Default );
	~ChangeSetEnabler();
	
	ChangeSet changeSet() const;
	
	bool enabled() const;
	
	void enable();
	void disable();
	
	void setReadMode( ChangeSet::ReadMode );
	
protected:
	bool mEnabled;
	ChangeSet mChangeSet, mChangeSetRestore;
	ChangeSet::ReadMode mReadMode, mReadModeRestore;
};

#define CS_ENABLE(changeset) ChangeSetEnabler _cse(changeset);

/**
 *  This class represents a sort of proxy in front of either
 *  Database or Table signals that notify listeners of data
 *  changes(Inserts/Update/Delete).  This class adds notifications
 *  for changes as they are added to a changeset, and will
 *  replay the inverse if the changeset is reverted.  It also 
 *  filters global notifications ensuring that notifications already
 *  delivered from the changeset object aren't delivered a second
 *  time when the changeset is committed.
 */
class STONE_EXPORT ChangeSetNotifier : public QObject
{
Q_OBJECT
public:
	// Delivers notifications for only table
	ChangeSetNotifier( ChangeSet cs, Table * table, QObject * parent = 0 );
	// Delivers all notifications for the current database
	ChangeSetNotifier( ChangeSet cs, QObject * parent = 0 );
	~ChangeSetNotifier();
	
	ChangeSet changeSet() const { return mChangeSet; }
	
	Table * table() const { return mTable; }
	
signals:
	void added( RecordList );
	void removed( RecordList );
	void updated( Record, Record );
	
protected:
	Table * mTable;
	ChangeSet mChangeSet;
	friend class ChangeSet;
	friend class ChangeSetPrivate;
};


};

using Stone::ChangeSet;
using Stone::ChangeSetEnabler;
using Stone::ChangeSetNotifier;

#endif // CHANGESET_H