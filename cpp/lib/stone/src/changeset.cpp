
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

#include <assert.h>

#include <qbitarray.h>
#include <qhash.h>
#include <qpointer.h>
#include <qthreadstorage.h>
#include <qundostack.h>

#include "changeset.h"
#include "database.h"
#include "table.h"

namespace Stone {

static QThreadStorage<ChangeSet *> sCurrentChangeSet;

class ChangeSet::Private
{
public:
	
	Private()
	: state( Unchanged )
	, weakRefs( 0 )
	, undoCommand( 0 )
	, ownUndoCommand( false )
	, depth( 0 )
	{
		refCount.ref();
	}
	
	Private( const ChangeSet::Private & other )
	: title( other.title )
	, state( other.state )
	, readMode( other.readMode )
	, parent( other.parent )
	, children( other.children )
	, changes( other.changes )
	// We actually keep the notifiers with the original, not the detached
	// but we pass them here so they can be disabled during the commit, then
	// they will be cleared afterwards
	, notifiers( other.notifiers )
	, weakRefs( other.weakRefs )
	, undoStack( other.undoStack )
	, undoCommand( other.undoCommand )
	, ownUndoCommand( other.ownUndoCommand )
	, depth( other.depth )
	{
		refCount.ref();
	}
	
	~Private()
	{}
	
	void ref()
	{
		if( refCount > 0 ) {
			refCount.ref();
		}
	}

	void deref()
	{
		if( refCount == 0 ) return;
		bool neZero = refCount.deref();
		if( !neZero ) {
			// No matter if our changes are pending, or committed, we
			// call revert with marks each of our changes as discarded
			// If we are already committed then it's just the old values
			revert();
			// Invalidate the weak refs
			ChangeSetWeakRef * wr = weakRefs;
			while( wr ) {
				wr->p = 0;
				wr = wr->next;
			}
			if( ownUndoCommand )
				delete undoCommand;
			delete this;
		}
	}

	/* This function is used to split off a committed changeset into a separate
	 * object, and clear the state of the current one.
	 * Notifiers and weak refs are kept with the current one.
	 * Changes and the undo command are moved to the returned one */
	ChangeSet::Private * detach()
	{
		ChangeSet::Private * ret = new ChangeSet::Private(*this);
		
		ret->parent = parent;
		
		// The weak refs should all point to the detached copy
		ChangeSetWeakRef * wr = weakRefs;
		while( wr ) {
			wr->p = ret;
			wr = wr->next;
		}
		// We gave away the weak refs, clear our own pointer
		weakRefs = 0;
		changes.clear();
		children.clear();
		
		// The undo command needs to point to the copy
		if( ret->undoCommand )
			ret->undoCommand->mChangeSet = ChangeSet(ret);
		
		// And we no longer have one, it'll get created again on demand
		undoCommand = 0;
		ownUndoCommand = false;
		
		state = ChangeSet::Unchanged;
		return ret;
	}
	
	QList<ChangeSetNotifier*> gatherNotifiers(ChangeSet cs = ChangeSet());
	void revert();
	void addWeakRef(ChangeSetWeakRef *);
	void removeWeakRef(ChangeSetWeakRef *);
	QAtomicInt refCount;
	QString title;
	ChangeSet::State state;
	ChangeSet::ReadMode readMode;
	ChangeSetWeakRef parent;
	QList<ChangeSet> children;
	QList<ChangeSet::Change> changes;
	QList<ChangeSetNotifier*> notifiers;
	ChangeSetWeakRef * weakRefs;
	QPointer<QUndoStack> undoStack;
	ChangeSetUndoCommand * undoCommand;
	bool ownUndoCommand;
	int depth;
};

static RecordList tableFilter( QList<Table*> tables, RecordList rl )
{
	RecordList ret;
	foreach( Record r, rl )
		if( tables.contains(r.table()) )
			ret += r;
	return ret;
}

static RecordList tableFilter( Table * table, RecordList rl )
{
	if( !table ) return rl;
	RecordList ret;
	QList<Table*> tt = table->tableTree();
	foreach( Record r, rl )
		if( tt.contains(r.table()) )
			ret += r;
	return ret;
}

void ChangeSet::Private::revert()
{
	// Ensure we are enabled so that the records can be read correctly from the change signals
	ChangeSet self(this);
	foreach( ChangeSet::Change change, changes ) {
		self.deliverSignals( change, UndoMode );
		switch( change.type ) {
			case ChangeSet::Change_Insert:
			case ChangeSet::Change_Update:
			case ChangeSet::Change_Remove:
				foreach( Record r, change.mRecords )
					r.imp()->mState |= RecordImp::DISCARDED;
				break;
			case ChangeSet::Change_Nested:
				ChangeSet(change.changeset).revert();
				break;
			default:
				break;
		}
	}
	
	state = ChangeSet::Unchanged;
	changes.clear();
	if( ownUndoCommand )
		delete undoCommand;
	undoCommand = 0;
}

void ChangeSet::Private::addWeakRef(ChangeSetWeakRef * wr)
{
	wr->next = weakRefs;
	weakRefs = wr;
}

void ChangeSet::Private::removeWeakRef(ChangeSetWeakRef * wr)
{
	if( wr == weakRefs ) {
		weakRefs = wr->next;
		return;
	}
	ChangeSetWeakRef * prev = weakRefs;
	while( prev && prev->next != wr )
		prev = prev->next;
	// Undo no circumstances should a weak ref try to remove itself yet not have anything point to it
//	assert(prev);
	if( prev->next == wr )
		prev->next = wr->next;
}

QList<ChangeSetNotifier*> ChangeSet::Private::gatherNotifiers(ChangeSet skipChildNotifiers)
{
	QList<ChangeSetNotifier*> ret(notifiers);
	foreach( ChangeSet cs, children )
		if( cs != skipChildNotifiers )
			ret += cs.p->gatherNotifiers(skipChildNotifiers);
	return ret;
}

QString ChangeSet::changeTypeString(ChangeType type)
{
	switch( type ) {
		case Change_Insert:
			return "Insert";
		case Change_Update:
			return "Update";
		case Change_Remove:
			return "Remove";
		case Change_Nested:
			return "Nested";
		case Change_Invalid:
			return "Invalid";
	}
	return QString();
}

QString ChangeSet::stateString(State state)
{
	switch( state ) {
		case Invalid:
			return "Invalid";
		case Unchanged:
			return "Unchanged";
		case ChangesPending:
			return "ChangesPending";
		case Committed:
			return "Committed";
		case Undone:
			return "Undone";
	}
	return QString();
}

ChangeSet::ChangeType ChangeSet::changeType(const Record & r)
{
	RecordImp * ri = r.imp();
	if( ri->mState & RecordImp::MODIFIED_SINCE_QUEUED && ri->mState & (RecordImp::INSERT_PENDING|RecordImp::COMMITTED) )
		return Change_Update;
	if( ri->mState & RecordImp::MODIFIED_SINCE_QUEUED && !(ri->mState & (RecordImp::INSERT_PENDING|RecordImp::COMMITTED)) )
		return Change_Insert;
	return Change_Invalid;
}

ChangeSet ChangeSet::create( const QString & title, QUndoStack * undoStack )
{
	ChangeSet ret;
	ret.p = new Private();
	ret.p->title = title;
	ret.p->undoStack = undoStack;
	return ret;
}

ChangeSet ChangeSet::createChild( const QString & title, QUndoStack * undoStack )
{
	if( !p )
		return ChangeSet::create(title,undoStack);
	ChangeSet ret;
	ret.p = new Private();
	ret.p->title = title;
	ret.p->parent = *this;
	ret.p->undoStack = undoStack;
	p->children.append(ret);
	ret.p->depth = p->depth + 1;
	ChangeSet::Change change(ret);
	p->changes.append( change );
	return ret;
}

ChangeSet ChangeSet::current()
{
	ChangeSet * csp = sCurrentChangeSet.localData();
	if( csp == 0 )
		return ChangeSet();
	return *csp;
}

QUndoStack * ChangeSet::undoStack() const
{
	return p ? p->undoStack : 0;
}

ChangeSetUndoCommand * ChangeSet::undoCommand() const
{
	if( p && p->undoStack && !p->undoCommand ) {
		p->undoCommand = new ChangeSetUndoCommand( *this );
		p->ownUndoCommand = true;
	}
	return p ? p->undoCommand : 0;
}

ChangeSet::Change::Change( ChangeType _type, RecordList _records )
: type(_type), mRecords(_records) {}

ChangeSet::Change::Change( ChangeSet cs )
: type(ChangeSet::Change_Nested)
, changeset(cs)
{}

RecordList ChangeSet::Change::records() const
{
	return mRecords;
}
// 
ChangeSet ChangeSet::Change::changeSet() const
{
	return ChangeSet(changeset);
}

ChangeSet::ChangeSet()
: p(0)
{
}

ChangeSet::ChangeSet( const ChangeSet & other )
: p( other.p )
{
	if( p )
		p->ref();
}

ChangeSet::ChangeSet( ChangeSet::Private * _p )
: p(_p)
{
	if( p )
		p->ref();
}

ChangeSet::~ChangeSet()
{
	if( p )
		p->deref();
}

ChangeSet & ChangeSet::operator=( const ChangeSet & other )
{
	if( other.p != p ) {
		if( p )
			p->deref();
		p = other.p;
		if( p )
			p->ref();
	}
	return *this;
}

QString ChangeSet::title() const
{
	return p ? p->title : QString();
}

void ChangeSet::setTitle( const QString & title )
{
	if( p )
		p->title = title;
}

ChangeSet ChangeSet::parent() const
{
	return p ? ChangeSet(p->parent) : ChangeSet();
}

ChangeSet::State ChangeSet::state() const
{
	return p ? p->state : Invalid;
}

ChangeSet::ReadMode ChangeSet::readMode() const
{
	return p ? p->readMode : Read_Current;
}

void ChangeSet::setReadMode( ChangeSet::ReadMode readMode )
{
	if( p ) {
		if( readMode == Read_Default )
			readMode = (topLevel().state() == Committed) ? Read_Pristine : Read_Current;
		p->readMode = readMode;
	}
}

ChangeSet ChangeSet::commit( bool detach )
{
	if( p ) {
		// We don't have to do anything in detach mode or not if there's no changes
		if( state() == Unchanged )
			return *this;
		
		ChangeSet ret;
		if( detach ) {
			ret.p = p->detach();
		} else {
			ret = *this;
		}
		ret.p->state = Committed;

		// Can't undo/redo children of a committed changeset
		foreach( ChangeSet::Change change, ret.p->changes ) {
			if( change.type == Change_Nested ) {
				ChangeSet cs(change.changeSet());
				if( cs.undoStack() )
					cs.undoStack()->clear();
			}
		}
		
		if( ret.p->parent().isValid() ) {
			// If the changeset is empty we can simply ignore it
			if( !ret.p->changes.isEmpty() ) {
				ChangeSet parent(ret.p->parent);
				foreach( ChangeSet::Change change, ret.p->changes )
					parent.deliverSignals( change, ChildCommitMode, ret );
				if( detach ) {
					parent.p->changes.append(Change(*this));
					parent.p->children.append(ret);
				}
				if( parent.p->state == Unchanged )
					parent.p->state = ChangesPending;
			}
		} else {
			ret._commit(false);
		}
		
		// Notifiers are kept with the original, not the detached copy
		if( detach )
			ret.p->notifiers.clear();

		// If any class accidentilly uses an already committed changeset,
		// have it read the pristine(committed) values by default.
		ret.p->readMode = Read_Pristine;
		
		if( ret.p->undoStack ) {
			ret.p->undoStack->push( ret.undoCommand() );
			ret.p->ownUndoCommand = false;
		}
		
		return ret;
	}
	return ChangeSet();
}

void ChangeSet::_commitHelper( bool redoMode )
{
	for( QList<ChangeSet::Change>::iterator it = p->changes.begin(), end = p->changes.end(); it != end; ++it ) {
		ChangeSet::Change & change = *it;
		Table * t = change.mRecords.size() ? change.mRecords[0].table() : 0;
		switch( change.type ) {
			case Change_Insert:
				if( redoMode ) {
					foreach( Record r, change.mRecords ) {
						r.imp()->mState = RecordImp::COMMIT_ALL_FIELDS;
				//		LOG_5( r.debug() );
					}
				}
				assert(t);
				change.commitState = t->insertBegin(change.mRecords);
				//change.mRecords._commit(true);
				break;
			case Change_Update:
				assert(t);
				change.commitState = t->updateBegin(change.mRecords);
				qDebug() << change.commitState;
				break;
			case Change_Remove:
				assert(t);
				change.commitState = t->removeBegin(change.mRecords);
				break;
			case Change_Nested:
				if( redoMode )
					ChangeSet(change.changeset).redo();
				else {
			//		LOG_1( "Committing nested changeset" );
					ChangeSet child(change.changeset);
					if( child.state() == ChangeSet::Committed ) {
						LOG_TRACE;
						child._commitHelper(redoMode);
					} else if( child.p )
						// Once a changeset is committed, all uncommitted children(whether in ChangesPending, or Undone states)
						// are marked as invalid in order to not replay any signals related to them if this change is undone/redone
						child.p->state = Invalid;
				}
				break;
			default:
				break;
		}
	}
}

void ChangeSet::_commitComplete()
{
	for( QList<ChangeSet::Change>::iterator it = p->changes.begin(), end = p->changes.end(); it != end; ++it ) {
		ChangeSet::Change & change = *it;
		Table * t = change.mRecords.size() ? change.mRecords[0].table() : 0;
		switch( change.type ) {
			case Change_Insert:
				assert(t);
				t->insertComplete(change.commitState);
				change.commitState = 0;
				//change.mRecords._commit(true);
				break;
			case Change_Update:
				assert(t);
				t->updateComplete(change.commitState);
				change.commitState = 0;
				break;
			case Change_Remove:
				assert(t);
				t->removeComplete(change.commitState);
				change.commitState = 0;
				break;
			case Change_Nested:
			{
				ChangeSet child(change.changeset);
				if( child.state() == ChangeSet::Committed ) {
					LOG_TRACE;
					child._commitComplete();
				}
				break;
			}
			default:
				break;
		}
	}
}

void ChangeSet::_commitRollback()
{
	for( QList<ChangeSet::Change>::iterator it = p->changes.begin(), end = p->changes.end(); it != end; ++it ) {
		ChangeSet::Change & change = *it;
		Table * t = change.mRecords.size() ? change.mRecords[0].table() : 0;
		switch( change.type ) {
			case Change_Insert:
				assert(t);
				t->insertRollback(change.commitState);
				change.commitState = 0;
				//change.mRecords._commit(true);
			case Change_Update:
				assert(t);
				t->updateRollback(change.commitState);
				change.commitState = 0;
				break;
			case Change_Remove:
				assert(t);
				t->removeRollback(change.commitState);
				change.commitState = 0;
				break;
			case Change_Nested:
			{
				ChangeSet child(change.changeset);
				if( child.state() == ChangeSet::Committed ) {
					LOG_TRACE;
					child._commitRollback();
				}
				break;
			}
			default:
				break;
		}
	}
}

void ChangeSet::_commit( bool redoMode )
{
	if( p ) {
		Database * db = Database::current();
		Connection * c = db ? db->connection() : 0;
		bool useTrans = c && (c->capabilities() & Connection::Cap_Transactions);
		bool retry = false;
		
		do {
			if( useTrans )
				db->beginTransaction();
			try {
				_commitHelper(redoMode);
				db->commitTransaction();
			} catch(const LostConnectionException &) {
				_commitRollback();
				retry = true;
			} catch(...) {
				db->rollbackTransaction();
				_commitRollback();
				throw;
			}
		} while (retry);
		
		QList<ChangeSetNotifier*> notifiers = p->gatherNotifiers();
		QBitArray signalsRestore(notifiers.size());
		int i = 0;
		foreach( ChangeSetNotifier * csn, notifiers ) {
			signalsRestore[i++] = csn->blockSignals(true);
		}
		_commitComplete();
		i = 0;
		foreach( ChangeSetNotifier * csn, notifiers ) {
			if( !signalsRestore[i++] )
				csn->blockSignals(false);
		}
		p->state = Committed;
	}
}

void ChangeSet::revert()
{
	if( p )
		p->revert();
}

void ChangeSet::undo()
{
	if( p && p->state == Committed ) {
		p->state = Undone;
		if( parent().isValid() ) {
			parent().deliverSignals( ChangeSet::Change(*this), UndoMode, ChangeSet() );
		} else {
			foreach( ChangeSet::Change change, p->changes ) {
				Table * t = change.mRecords.size() ? change.mRecords[0].table() : 0;
				RecordList records = change.mRecords.reversed();
				switch( change.type ) {
					case Change_Insert:
						assert(t);
						t->remove(records);
					case Change_Update:
						assert(t);
						t->update(records);
						break;
					case Change_Remove:
						assert(t);
						foreach( Record r, records )
							r.imp()->mState = RecordImp::COMMIT_ALL_FIELDS;
						t->insert(records);
						break;
					case Change_Nested:
						ChangeSet(change.changeset).undo();
						break;
					default:
						break;
				}
			}
		}
	}
}

void ChangeSet::redo()
{
	if( p && p->state == Undone ) {
		p->state = Committed;
		if( parent().isValid() ) {
			parent().deliverSignals( ChangeSet::Change(*this), RedoMode );
		} else {
			_commit(true);
		}
	}
}

QList<ChangeSet::Change> ChangeSet::changes()
{
	return p ? p->changes : QList<ChangeSet::Change>();
}

struct ChangeHashes
{
	QHash<RecordImp*, int> added, updated, removed;
};

struct VRCH
{
	QMap<Table*,ChangeHashes> changeHashesByTable;
	QList<Table*> filter;
};

void ChangeSet::visibleRecordsChangedHelper( const ChangeSet & visibleTo, VRCH * vrch )
{
	foreach( Change change, changes() ) {
		switch( change.type ) {
			case Change_Nested:
			{
				ChangeSet child = change.changeSet();
				if( child.state() == Committed || visibleTo.isAnscestor(child) )
					child.visibleRecordsChangedHelper( visibleTo, vrch );
			}
				break;
			case Change_Insert:
				foreach( Record r, change.mRecords ) {
					if( !vrch->filter.contains(r.table()) ) continue;
					ChangeHashes & ch = vrch->changeHashesByTable[r.table()];
					RecordImp * first = r.imp()->first();
					QHash<RecordImp*,int>::iterator rem = ch.removed.find(first);
					// If a record is removed then inserted we report it as being updated
					if( rem != ch.removed.end() ) {
						ch.removed.erase(rem);
						ch.updated.insert(first,0);
					} else
						ch.added.insert(first,0);
				}
				break;
			case Change_Update:
				foreach( Record r, change.mRecords ) {
					if( !vrch->filter.contains(r.table()) ) continue;
					ChangeHashes & ch = vrch->changeHashesByTable[r.table()];
					RecordImp * first = r.imp()->first();
					ch.updated.insert(first,0);
				}
				break;
			case Change_Remove:
				foreach( Record r, change.mRecords ) {
					if( !vrch->filter.contains(r.table()) ) continue;
					ChangeHashes & ch = vrch->changeHashesByTable[r.table()];
					RecordImp * first = r.imp()->first();
					// If a record is removed, then we don't report any previous inserts or updates
					ch.added.remove(first);
					ch.updated.remove(first);
					ch.removed.insert(first,0);
				}
				break;
			default:
				break;
		}
	}
}

void ChangeSet::visibleRecordsChanged( RecordList * added, RecordList * updated, RecordList * removed, QList<Table*> filter )
{
	VRCH vrch;
	vrch.filter = filter;
	topLevel().visibleRecordsChangedHelper( *this, &vrch );
	for( QMap<Table*,ChangeHashes>::iterator it = vrch.changeHashesByTable.begin(); it != vrch.changeHashesByTable.end(); ++it )
	{
		ChangeHashes & ch = it.value();
		if( added ) {
			QHash<RecordImp*, int>::const_iterator i = ch.added.constBegin();
			for (; i != ch.added.constEnd(); ++i)
				*added += i.key();
		}
		if( updated ) {
			QHash<RecordImp*, int>::const_iterator i = ch.updated.constBegin();
			for (; i != ch.updated.constEnd(); ++i)
				*updated += i.key();
		}
		if( removed ) {
			QHash<RecordImp*, int>::const_iterator i = ch.removed.constBegin();
			for (; i != ch.removed.constEnd(); ++i)
				*removed += i.key();
		}
	}
}

QString ChangeSet::debug( int tab )
{
	QStringList ret;
	ret += QString::number((qulonglong)p,16) + " " + stateString(state());
	if( p ) {
		foreach( ChangeSet::Change change, p->changes ) {
			ret += changeTypeString(change.type) + ": " + change.mRecords.debug();
			if( change.type == Change_Nested )
				ret += ChangeSet(change.changeset).debug(tab+1);
		}
	}
	QString tabs;
	while(tab--)
		tabs += "    ";
	return tabs + ret.join("\n" + tabs);
}

bool ChangeSet::isAnscestor( const ChangeSet & cs ) const
{
	ChangeSet tmp(*this);
	while( tmp.isValid() ) {
		if( cs == tmp ) return true;
		tmp = tmp.parent();
	}
	return false;
}

int ChangeSet::depth() const
{
	return p ? p->depth : 0;
}

static int changeTypeToPendingState( ChangeSet::ChangeType type, bool undoMode )
{
	switch( type ) {
		case ChangeSet::Change_Insert:
			return undoMode ? RecordImp::DELETE_PENDING : RecordImp::INSERT_PENDING;
		case ChangeSet::Change_Update:
			return RecordImp::UPDATE_PENDING;
		case ChangeSet::Change_Remove:
			return undoMode ? RecordImp::INSERT_PENDING : RecordImp::DELETE_PENDING;
		default:
			break;
	}
	return 0;
}

void ChangeSet::deliverSignals( ChangeSet::Change change, DeliverSignalsMode mode, ChangeSet skipChildNotifiers )
{
	if( change.type == ChangeSet::Change_Nested ) {
		ChangeSet child = change.changeset;
		if( child.state() != Invalid ) {
			foreach( ChangeSet::Change childChange, child.p->changes ) {
				deliverSignals( childChange, mode, skipChildNotifiers );
			}
		}
		return;
	}
	bool queuing = (mode == QueueMode);
	RecordList toQueue, toFireInsert, toFireUpdate, toFireRemove;
	int pendingState = changeTypeToPendingState(change.type, mode == UndoMode);
	foreach( Record r, change.mRecords ) {
		bool queue = queuing;
		int impState = r.imp()->mState;
		if( pendingState == RecordImp::INSERT_PENDING ) {
			toFireInsert.append(r);
		}
		else if( pendingState == RecordImp::UPDATE_PENDING ) {
			if( queuing && (impState & (RecordImp::UPDATE_PENDING|RecordImp::INSERT_PENDING)) )
				queue = false;
			toFireUpdate.append(r);
		}
		else if( pendingState == RecordImp::DELETE_PENDING ) {
			if( queuing && (impState & RecordImp::DELETE_PENDING) )
				queue = false;
			toFireRemove.append(r);
		}
		r.imp()->mState &= ~RecordImp::MODIFIED_SINCE_QUEUED;
		if( queuing )
			r.imp()->mState |= pendingState;
		if( queue )
			toQueue.append(r);
	}

	if( queuing && toQueue.size() ) {
		p->changes.append( ChangeSet::Change(change.type,toQueue) );
	}
	
	foreach( ChangeSetNotifier * csn, p->gatherNotifiers(skipChildNotifiers) ) {
		RecordList insert = tableFilter(csn->table(), toFireInsert),
		update = tableFilter(csn->table(), toFireUpdate),
		remove = tableFilter(csn->table(), toFireRemove);
		//qDebug() << "Inserts: " << insert.size() << " Updates: " << update.size() << " Deletes: " << remove.size();
		if( insert.size() ) {
			emit csn->added(insert);
		}
		foreach( Record r, update ) {
			if( mode == UndoMode )
				emit csn->updated( r, r.parent() );
			else
				emit csn->updated( r.parent(), r );
		}
		if( remove.size() )
			emit csn->removed(remove);
	}
}

void ChangeSet::queue( ChangeSet::ChangeType type, RecordList rl )
{
	if( p && p->state & (Unchanged | ChangesPending) ) {
		p->state = ChangesPending;
		deliverSignals( ChangeSet::Change(type, rl), QueueMode );
	}
}

void ChangeSet::addNotifier( ChangeSetNotifier * no )
{
	if( p ) {
		p->notifiers.append(no);
	}
}

void ChangeSet::removeNotifier( ChangeSetNotifier * no )
{
	if( p ) {
		p->notifiers.removeAll(no);
	}
}

ChangeSet ChangeSet::topLevel() const
{
	if( !p ) return ChangeSet();
	ChangeSet cs = *this;
	while( cs.parent().isValid() )
		cs = cs.parent();
	return cs;
}

ChangeSet ChangeSet::commonParent( ChangeSet a, ChangeSet b )
{
	int a_depth = a.depth(), b_depth = b.depth();
	while( a_depth > b_depth ) {
		a = a.parent();
		a_depth--;
	}
	while( b_depth > a_depth) {
		b = b.parent();
		b_depth--;
	}
	while( a != b && a.isValid() && b.isValid() ) {
		a = a.parent();
		b = b.parent();
	}
	if( a == b ) return a;
	return ChangeSet();
}

bool ChangeSet::isVisible( const ChangeSet & _cs ) const
{
	if( !isValid() ) return false;
	// In the case that a record is not attached to a changeset, it is valid to all changesets
	if( !_cs.isValid() ) return true;
	if( _cs == *this ) return true;
	ChangeSet cp = commonParent(*this,_cs);
	if( !cp.isValid() ) {
		//qDebug() << "Changeset @" << qPrintable(QString::number((qint64)_cs.p,16)) << " is not visible to @" << qPrintable(QString::number((qint64)p,16)) << " as they do not share a common parent";
		return false;
	}
	ChangeSet cs(_cs);
	while (cs != cp) {
		if( cs.state() != Committed ) {
			//qDebug() << "Uncommitted changeset @" << qPrintable(QString::number((qint64)cs.p,16)) << " is not visible to @" << qPrintable(QString::number((qint64)p,16));
			return false;
		}
		cs = cs.parent();
	}
	//qDebug() << "Changeset @" << qPrintable(QString::number((qint64)_cs.p,16)) << " is visible to @" << qPrintable(QString::number((qint64)p,16));
	return true;
}

ChangeSetWeakRef::ChangeSetWeakRef()
: p( 0 )
, next( 0 )
{
}

ChangeSetWeakRef::ChangeSetWeakRef(const ChangeSet & cs)
: p( cs.p )
, next( 0 )
{
	if( p )
		p->addWeakRef(this);
}

ChangeSetWeakRef::ChangeSetWeakRef(const ChangeSetWeakRef & cswr)
: p(cswr.p)
, next(0)
{
	if( p )
		p->addWeakRef(this);
}

void ChangeSetWeakRef::operator=( const ChangeSet & cs )
{
	if( cs.p == p ) return;
	if( p )
		p->removeWeakRef(this);
	p = cs.p;
	if( p )
		p->addWeakRef(this);
}

void ChangeSetWeakRef::operator=( const ChangeSetWeakRef & cswr )
{
	if( cswr.p == p ) return;
	if( p )
		p->removeWeakRef(this);
	p = cswr.p;
	if( p )
		p->addWeakRef(this);
}

ChangeSetWeakRef::~ChangeSetWeakRef()
{
	if( p )
		p->removeWeakRef(this);
}


ChangeSetEnabler::ChangeSetEnabler( const ChangeSet & changeSet, ChangeSet::ReadMode readMode )
: mEnabled( false )
, mChangeSet( changeSet )
, mReadMode( readMode )
{
	enable();
}

ChangeSetEnabler::~ChangeSetEnabler()
{ disable(); }
	
ChangeSet ChangeSetEnabler::changeSet() const
{
	return mChangeSet;
}
	
bool ChangeSetEnabler::enabled() const
{
	return mEnabled;
}

void ChangeSetEnabler::enable()
{
	if( !mEnabled ) {
		mEnabled = true;
		ChangeSet * csr = sCurrentChangeSet.localData();
		if( csr )
			mChangeSetRestore = *csr;
		else
			mChangeSetRestore = ChangeSet();
		sCurrentChangeSet.setLocalData( mChangeSet.isValid() ? new ChangeSet(mChangeSet) : 0 );
		mReadModeRestore = mChangeSet.readMode();
		mChangeSet.setReadMode(mReadMode);
	}
}

void ChangeSetEnabler::disable()
{
	if( mEnabled ) {
		mChangeSet.setReadMode(mReadModeRestore);
		sCurrentChangeSet.setLocalData( mChangeSetRestore.isValid() ? new ChangeSet(mChangeSetRestore) : 0 );
		mEnabled = false;
	}
}

void ChangeSetEnabler::setReadMode(ChangeSet::ReadMode readMode)
{
	mReadMode = readMode;
	if( mEnabled )
		mChangeSet.setReadMode(readMode);
}

ChangeSetNotifier::ChangeSetNotifier( ChangeSet cs, Table * table, QObject * parent )
: QObject( parent )
, mTable( table )
, mChangeSet(cs)
{
	cs.addNotifier(this);
	connect( table, SIGNAL(added(RecordList)), SIGNAL(added(RecordList)) );
	connect( table, SIGNAL(removed(RecordList)), SIGNAL(removed(RecordList)) );
	connect( table, SIGNAL(updated(Record,Record)), SIGNAL(updated(Record,Record)) );
}

ChangeSetNotifier::ChangeSetNotifier( ChangeSet cs, QObject * parent )
: QObject( parent )
, mTable( 0 )
, mChangeSet(cs)
{
	cs.addNotifier(this);
	Database * db = Database::current();
	connect( db, SIGNAL(recordsAddedSignal(RecordList)), SIGNAL(added(RecordList)) );
	connect( db, SIGNAL(recordsRemovedSignal(RecordList)), SIGNAL(removed(RecordList)) );
	connect( db, SIGNAL(recordUpdatedSignal(Record,Record)), SIGNAL(updated(Record,Record)) );
}

ChangeSetNotifier::~ChangeSetNotifier()
{
	mChangeSet.removeNotifier(this);
}

ChangeSetUndoCommand::ChangeSetUndoCommand( ChangeSet cs )
: QUndoCommand()
, mChangeSet(cs)
{
	setText( cs.title() );
}

ChangeSet ChangeSetUndoCommand::changeSet() const
{
	return mChangeSet;
}

void ChangeSetUndoCommand::redo()
{
	QUndoCommand::redo();
	mChangeSet.redo();
}

void ChangeSetUndoCommand::undo()
{
	QUndoCommand::undo();
	mChangeSet.undo();
}

}
