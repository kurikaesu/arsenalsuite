
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

#include <stdio.h>
#include <assert.h>

#include <qsqlquery.h>

#include "blurqt.h"
#include "freezercore.h"
#include "index.h"
#include "interval.h"
#include "recordimp.h"
#include "table.h"
#include "tableschema.h"

static int sRecordImpCount = 0;

static QVariant sNullVariant;

namespace Stone {

static char * copyBitArray( char * ba, int size )
{
	if( !ba ) return 0;
	int stateSize = (size+7) / 8;
	char * ret = new char[stateSize];
	memcpy( ret, ba, stateSize );
	return ret;
}

static void clearBitArray( char * ba, int size )
{
	if( !ba ) return;
	int stateSize = (size+7) / 8;
	for( int i=0; i<stateSize; i++ )
		ba[i] = 0;
}

static char * newBitArray( int size )
{
	int stateSize = (size+7) / 8;
	char * ret = new char[stateSize];
	clearBitArray( ret, size );
	return ret;
}

static char * setBit( char * ba, int bit, bool value, int size )
{
	if( !ba )
		ba = newBitArray( size );
	int block = bit / 8;
	int shift = bit % 8;
	char c = ba[block];
	if( value )
		c = c | (1 << shift);
	else
		c = c ^ (1 << shift);
	ba[block] = c;
	return ba;
}

static void clearBit( char * ba, int bit )
{
	if( !ba ) return;
	ba[bit / 8] ^= 1 << (bit % 8);
}

static bool getBit( const char * ba, int bit )
{
	if( !ba ) return false;
	int block = bit / 8;
	int shift = bit % 8;
	char c = ba[block];
	return c & (1 << shift);
}

// static
int RecordImp::totalCount()
{ return sRecordImpCount; }

RecordImp::RecordImp( Table * table, QVariant * toLoad )
: mState( 0 )
, mTable( table )
, mValues( 0 )
, mModifiedBits( 0 )
, mLiterals( 0 )
, mNotSelectedBits( 0 )
, mNext( 0 )
, mParent( 0 )
{
	sRecordImpCount++;
	if( table ) {
		table->mImpCount++;
		FieldList allFields = table->schema()->fields();
		int size = allFields.size();
		mValues = new VariantVector( size );
		if( toLoad ) {
			int i = 0;
			foreach( Field * f, allFields )
				(*mValues)[f->pos()] = f->coerce(toLoad[i++]);
			mState = COMMITTED;
		} else {
			foreach( Field * f, allFields )
				(*mValues)[f->pos()] = f->defaultValue();
		}
//		printf( "NEW RecordImp %p Table: %s Key: %i Table Count: %i\n", this, qPrintable(mTable->tableName()), key(), mTable->mImpCount );
	}
}

RecordImp::RecordImp( Table * table, QSqlQuery & q, int queryPosOffset, FieldList * incomingFields )
: mState( 0 )
, mTable( table )
, mValues( 0 )
, mModifiedBits( 0 )
, mLiterals( 0 )
, mNotSelectedBits( 0 )
, mNext( 0 )
, mParent( 0 )
{
	sRecordImpCount++;
	if( table ) {
		table->mImpCount++;
		FieldList allFields = table->schema()->fields();
		int size = allFields.size();
		mValues = new VariantVector( size );
		int pos = queryPosOffset;
		foreach( Field * f, allFields ) {
			bool isIncoming = !f->flag(Field::LocalVariable) && ( incomingFields ? incomingFields->contains(f) : !f->flag(Field::NoDefaultSelect) );
			if( isIncoming )
				(*mValues)[f->pos()] = f->coerce(q.value(pos++));
			else {
				(*mValues)[f->pos()] = f->defaultValue();
				if( !f->flag(Field::LocalVariable) ) mNotSelectedBits = setBit( mNotSelectedBits, f->pos(), true, allFields.size() );
			}
		}
		mState = COMMITTED;
//		printf( "NEW RecordImp %p Table: %s Key: %i Table Count: %i\n", this, qPrintable(mTable->tableName()), key(), mTable->mImpCount );
	}
}

RecordImp::RecordImp( Table * table, QSqlQuery & q, int * queryColPos, FieldList * incomingFields )
: mState( 0 )
, mTable( table )
, mValues( 0 )
, mModifiedBits( 0 )
, mLiterals( 0 )
, mNotSelectedBits( 0 )
, mNext( 0 )
, mParent( 0 )
{
	sRecordImpCount++;
	if( table ) {
		table->mImpCount++;
		FieldList allFields = table->schema()->fields();
		int size = allFields.size();
		mValues = new VariantVector( size );
		int pos = 0;
		foreach( Field * f, allFields ) {
			bool isIncoming = !f->flag(Field::LocalVariable) && ( incomingFields ? incomingFields->contains(f) : !f->flag(Field::NoDefaultSelect) );
			if( isIncoming )
				(*mValues)[f->pos()] = f->coerce(q.value(queryColPos[pos++]));
			else {
				(*mValues)[f->pos()] = f->defaultValue();
				if( !f->flag(Field::LocalVariable) ) mNotSelectedBits = setBit( mNotSelectedBits, f->pos(), true, allFields.size() );
			}
		}
		mState = COMMITTED;
		//printf( "NEW RecordImp %p Table: %s Key: %i Table Count: %i\n", this, qPrintable(mTable->tableName()), key(), mTable->mImpCount );
	}
}

RecordImp::RecordImp( Table * table, QSqlQuery & q, const QVector<int> & queryColPos )
: mState( 0 )
, mTable( table )
, mValues( 0 )
, mModifiedBits( 0 )
, mLiterals( 0 )
, mNotSelectedBits( 0 )
, mNext( 0 )
, mParent( 0 )
{
	sRecordImpCount++;
	if( table ) {
		table->mImpCount++;
		FieldList allFields = table->schema()->fields();
		int size = allFields.size();
		mValues = new VariantVector( size );
		int pos = 0;
		foreach( Field * f, allFields ) {
			int colPos = -1;
			if( !f->flag(Field::LocalVariable) )
				colPos = queryColPos[pos++];
			if( colPos >= 0 )
				(*mValues)[f->pos()] = f->coerce(q.value(colPos));
			else {
				(*mValues)[f->pos()] = f->defaultValue();
				if( !f->flag(Field::LocalVariable) ) mNotSelectedBits = setBit( mNotSelectedBits, f->pos(), true, allFields.size() );
			}
		}
		mState = COMMITTED;
		//printf( "NEW RecordImp %p Table: %s Key: %i Table Count: %i\n", this, qPrintable(mTable->tableName()), key(), mTable->mImpCount );
	}
}

RecordImp::~RecordImp()
{
	sRecordImpCount--;
	if( mTable ) {
		mTable->mImpCount--;
		//printf( "DELETE RecordImp %p Table: %s Key: %i Table Count: %i\n", this, qPrintable(mTable->tableName()), key(), mTable->mImpCount );
	}
	delete mValues;
	mValues = 0;
	delete [] mModifiedBits;
	mModifiedBits = 0;
	delete [] mLiterals;
	mLiterals = 0;
	delete [] mNotSelectedBits;
	mNotSelectedBits = 0;
	mTable = 0;
	if( mParent )
		mParent->deref();
	if( mNext ) {
		RecordImp *prev = 0, *next = mNext;
		while( next && next != this ) {
			if( next->mNext == this )
				prev = next;
			next = next->mNext;
		}
		if( prev )
			prev->mNext = mNext;
	}
}

class ChildIter
{
public:
	ChildIter(RecordImp * imp)
	: mParent(imp)
	, mCurrent(imp)
	{next();}
	inline bool isChild( RecordImp * imp ) { while( imp ) { if( imp == mParent ) return true; imp = imp->mParent; } return false; }
	bool isValid() { return mCurrent != 0; }
	RecordImp * next() { mCurrent = (mCurrent->mNext && mCurrent->mNext != mParent && isChild(mCurrent->mNext)) ? mCurrent->mNext : 0; return mCurrent; }
	RecordImp * cur() { return mCurrent; }
	RecordImp * mParent, * mCurrent;
};

void RecordImp::ref()
{
	mRefCount.ref();
	//qDebug() << "Ref: " << this << " " << mTable->tableName() << " " << mRefCount;
}

void RecordImp::deref()
{
	bool neZero = mRefCount.deref();
	//qDebug() << "Deref: " << this << " " << mTable->tableName() << " " << mRefCount;

	if( !neZero ) {
		if( mTable && mValues ) {
			uint myKey = key();
			Table * table = mTable;
			// qDebug() << "Destroying " << table->schema()->tableName() << " " << myKey;
			while( table ) {
				KeyIndex * ki = mTable->keyIndex();
				if( ki ) ki->expire( myKey, this );
				table = table->parent();
			}
		}
		delete this;
	}
}

uint RecordImp::key() const
{
	return (mTable && mValues) ? mValues->at( mTable->schema()->primaryKeyIndex() ).toInt() : 0;
}

void RecordImp::set( QVariant * v )
{
	if( !mTable ) return;
	int fc = mTable->schema()->fieldCount();
	for( int i=0; i<fc; i++ )
		(*mValues)[i] = v[i];
}

void RecordImp::get( QVariant * v )
{
	if( !mTable ) return;
	int fc = mTable->schema()->fieldCount();
	for( int i=0; i<fc; i++ )
		v[i] = mValues->at( i );
}

void RecordImp::updateChildren()
{
	if( !mTable ) return;
	int fc = mTable->schema()->fieldCount();
	for( ChildIter it(this); it.cur(); it.next() ) {
		RecordImp * cur = it.cur();
		assert(cur != this);
		for( int i=0; i<fc; i++ ) {
			if( isColumnSelected(i) ) {
				// All children represent changes, we don't replace their changes
				if( cur->isColumnModified(i) )
					continue;
				cur->mValues->replace(i,mValues->at(i));
				if( !cur->isColumnSelected(i) )
					clearBit( cur->mNotSelectedBits, i );
			}
		}
	}
}

const QVariant & RecordImp::getColumn( int col ) const
{
	if( !mTable || !mValues || col >= (int)mTable->schema()->fieldCount() || col < 0 )
		return sNullVariant;
	if( getBit( mNotSelectedBits, col ) ) {
		mTable->selectFields( RecordList() += Record(const_cast<RecordImp*>(this)), FieldList() += mTable->schema()->field(col) );
	}
	return mValues->at(col);
}

const QVariant & RecordImp::getColumn( Field * f ) const
{
	if( !mTable || !mTable->schema()->fields().contains(f) )
		return sNullVariant;
	return getColumn( f->pos() );
}

RecordImp * RecordImp::setColumn( int col, const QVariant & v )
{
	FieldList fields = mTable->schema()->fields();
	if( !mTable || col >= (int)fields.size() || col < 0 ) {
		LOG_5( "RecordImp::setColumn: Column " + QString::number( col ) + " is out of range" );
		return this;
	}
	Field * f = fields[col];
	QVariant vnew(f->coerce(v));
	
	bool notSel = getBit( mNotSelectedBits, col );
	
	QVariant & vr = (*mValues)[col];
	if( notSel || (vr.isNull() != vnew.isNull()) || (vr != vnew) ) {
		bool isVar = f->flag( Field::LocalVariable );

		if( (mState == EMPTY_SHARED) || (!isVar && (mState & COMMITTED) && !(mState & MODIFIED)) || (ChangeSet::current() != mChangeSet) ) {
			RecordImp * ret = copy();
			//qDebug() << "Setting column " << f->name() << " to value " << v << " on imp " << ret;
			ret->setColumn( col, v );
			ret->ref();
			deref();
			return ret;
		}
		vr = v;
		if( !isVar )
			setColumnModified( col, true );
	}
//	LOG_5( "RecordImp::setColumn: Values are equal: " + QString( QVariant::typeToName( v.type() ) ) + ": " + v.toString() );
//	LOG_5( "RecordImp::setColumn: Values are equal: " + QString( QVariant::typeToName( vr.type() ) ) + ": " + vr.toString() );
	return this;
}

RecordImp * RecordImp::setColumn( Field * f, const QVariant & v )
{
	if( !mTable || !mTable->schema()->fields().contains(f) )
		return this;
	return setColumn( f->pos(), v );
}

void RecordImp::fillColumn( int col, const QVariant & v )
{
	FieldList fields = mTable->schema()->fields();
	if( !mTable || col >= (int)fields.size() || col < 0 ) {
		LOG_5( "RecordImp::fillColumn: Column " + QString::number( col ) + " is out of range" );
		return;
	}
	Field * f = fields[col];
	QVariant vnew(f->coerce(v));
	(*mValues)[col] = vnew;
	clearBit( mNotSelectedBits, col );
}

void RecordImp::setColumnModified( uint col, bool modified )
{
	if( !modified && !mModifiedBits )
		return;
	mModifiedBits = setBit( mModifiedBits, col, modified, mTable->schema()->fieldCount() );
	if( mChangeSet().isValid() )
		mState |= MODIFIED_SINCE_QUEUED;
}

bool RecordImp::isColumnModified( uint col ) const
{
	return getBit( mModifiedBits, col );
}

void RecordImp::clearModifiedBits()
{
	clearBitArray( mModifiedBits, mTable->schema()->fieldCount() );
}

bool RecordImp::isColumnSelected( uint col )
{
	return !getBit( mNotSelectedBits, col );
}

FieldList RecordImp::notSelectedColumns()
{
	FieldList ret;
	if( mTable && mNotSelectedBits ) {
		FieldList fields = mTable->schema()->fields();
		for( int i = 0; i < fields.size(); ++i )
			if( getBit( mNotSelectedBits, i ) )
				ret += fields[i];
	}
	return ret;
}

RecordImp * RecordImp::setColumnLiteral( uint col, bool literal )
{
	if( !literal && !mLiterals )
		return this;
	if( (mState == EMPTY_SHARED) || ((mState & COMMITTED) && !(mState & MODIFIED)) || (ChangeSet::current() != mChangeSet) ) {
		RecordImp * ret = copy();
		ret->setColumnLiteral( col, literal );
		ret->ref();
		deref();
		return ret;
	}
	mLiterals = setBit( mLiterals, col, literal, mTable->schema()->fieldCount() );
	if( mChangeSet().isValid() )
		mState |= MODIFIED_SINCE_QUEUED;
	return this;
}

bool RecordImp::isColumnLiteral( uint col ) const
{
	return getBit( mLiterals, col );
}

void RecordImp::clearColumnLiterals()
{
	clearBitArray( mLiterals, mTable->schema()->fieldCount() );
}

const QVariant & RecordImp::getValue( const QString & column ) const
{
	return mTable ? getColumn( mTable->schema()->fieldPos( column ) ) : sNullVariant;
}

RecordImp * RecordImp::setValue( const QString & column, const QVariant & var )
{
	if( mTable )
		return setColumn( mTable->schema()->fieldPos( column ), var );
	LOG_5( "RecordImp::setValue: WTF? mTable is 0" );
	return this;
}

RecordImp * RecordImp::copy(bool attachToPristine)
{
	if( mTable ) {
		TableSchema * schema = mTable->schema();
		Table * table = schema->table();
		if( table ) {
			RecordImp * t = new RecordImp( table, mValues ? mValues->data() : 0 );
			// qDebug() << "Making copy of imp: @" << QString::number((quint64)this,16) << " at: @" << QString::number((quint64)t,16);
			t->mState = (mState & ~(EMPTY_SHARED)) | MODIFIED;
			if( t->mState & INSERT_PENDING ) {
				t->mState &= ~INSERT_PENDING;
				t->mState |= INSERT_PENDING_CHILD;
			}
			t->mState &= ~UPDATE_PENDING;

			//t->mModifiedBits = copyBitArray( mModifiedBits, schema->fieldCount() );
			t->mLiterals = copyBitArray( mLiterals, schema->fieldCount() );
			
			t->mNotSelectedBits = copyBitArray( mNotSelectedBits, schema->fieldCount() );
			t->mChangeSet = ChangeSet::current();
			if( !(mState & EMPTY_SHARED) && attachToPristine ) {
				t->mNext = mNext ? mNext : this;
				mNext = t;
				t->mParent = this;
				ref();
			}
			//t->mRefCount = 1; qDebug() << "Created copy of: " << Record(this).debug() << " copy: " << Record(t).debug(); t->mRefCount = 0;
			return t;
		}
	}
	return 0;
}

/*
class ThreadCommitter : public ThreadTask
{
public:
	ThreadCommitter(RecordImp * toCommit, bool npk)
	: ThreadTask()
	, mToCommit(toCommit)
	, mNewPrimaryKey(npk)
	{
		mToCommit->ref();
	}
	
	~ThreadCommitter()
	{ mToCommit->deref(); }

	void run()
	{
		mToCommit = mToCommit->commit(mNewPrimaryKey,true);
	}

	RecordImp * mToCommit;
	bool mNewPrimaryKey;
};
*/

RecordImp * RecordImp::commit()
{
	if( !mTable )
		return this;

	if( mState == EMPTY_SHARED )
		return this;

	if( mChangeSet().isValid() ) {
		if( (mState & MODIFIED_SINCE_QUEUED) && (mState & (INSERT_PENDING|COMMITTED|INSERT_PENDING_CHILD)) )
			mChangeSet().queue(ChangeSet::Change_Update,RecordList(Record(this)));
		else if( !(mState & COMMITTED) && !(mState & DELETED) && !(mState & DISCARDED) && (mState & MODIFIED) && !(mState & (INSERT_PENDING|INSERT_PENDING_CHILD)) )
			mChangeSet().queue(ChangeSet::Change_Insert,RecordList(Record(this)));
		return this;
	}
	/*
	if( !sync ) {
		FreezerCore::addTask( new ThreadCommitter(this,newPrimaryKey) );
		if( mState == (COMMITTED|MODIFIED) ) {
			RecordImp * ret = mTable->record(key(),false ).mImp;
			if( ret ) {
				ret->ref();
				deref();
				return ret;
			}
		}
		return this;
	}
	*/
	
	if( (mState & COMMITTED) && (mState & MODIFIED) ){
		mTable->update( this );
		RecordImp * ret = version(ChangeSet());
		if( ret ) {
			ret->ref();
			deref();
			return ret;
		}
	// Don't automatically recommit a deleted record.
	// The caller needs to either call copy, or manually
	// clear the DELETED flag.
	} else if( !(mState & COMMITTED) && !(mState & DELETED) ) {
		mTable->insert( Record(this,false) );
	}
	return this;
}

// With changeset we can have a new record with multiple versions
// When the first one is inserted we need to clear the INSERT_PENDING
// flag on all of them and set the committed flag.  There's no
// reference from parent to child-only the other way, so it requires
// multiple loops through the chain, which in practice should
// be fine since it should be rare that the chain is very long
void RecordImp::setCommitted(uint key)
{
	const uint pki = mTable->schema()->primaryKeyIndex();
	(*mValues)[pki] = QVariant(key);
	mChangeSet = ChangeSet();
	if( key )
		mState = COMMITTED;
	else
		mState = INSERT_PENDING;
	clearModifiedBits();
	clearColumnLiterals();
	
	RecordImp * prev = this;
	for( ChildIter it(this); it.cur(); it.next() ) {
		RecordImp * c = it.cur();
		(*(c->mValues))[pki] = QVariant(key);
		if( key ) {
			c->mState &= ~INSERT_PENDING_CHILD;
			c->mState |= COMMITTED;
		} else {
			c->mState &= ~COMMITTED;
			c->mState |= INSERT_PENDING_CHILD;
		}
	}
}

void RecordImp::remove()
{
	if( mTable ) {
		if( mChangeSet().isValid() )
			mChangeSet().queue( ChangeSet::Change_Remove, RecordList(Record(this)) );
		else
			mTable->remove( Record(this,false) );
	}
}

QString RecordImp::debugString()
{
	QString ret("Record Dump:  Table: ");
	if( table() )
		ret += table()->schema()->tableName();
	ret += " Key: " + QString::number( key() );
	ret += " State: " + QString::number( mState );
	char buffer[30];
	snprintf(buffer,30,"%p", this);
	ret += " Address: " + QString::fromAscii(buffer);
	return ret;
}

RecordImp * RecordImp::version(const ChangeSet & cs)
{
	if( mTable && mNext ) {
		RecordImp * pristine = 0;
		RecordImp * cur = this, * ret = 0;
		// Advance cur to the pristine copy
		while( cur ) {
			if( !cur->mChangeSet().isValid() && cur->mState == COMMITTED ) {
				pristine = cur;
				break;
			}
			cur = cur->mNext;
			if( cur == this )
				break;
		}
		// If there's no pristine copy, that means this is a new record, use the first
		// version as the pristine copy
		if( !pristine ) {
			while( cur ) {
				if( cur->mParent == 0 )
					pristine = cur;
				cur = cur->mNext;
				if( cur == this )
					break;
			}
		}
		
		assert( pristine );
		if( !cs.isValid() ) return pristine;
		
		while( cur ) {
			// Because of the order of the mNext chain we should always be returning the last
			// record that is visible to our changeset
			if( cs.isValid() && cs.isVisible(cur->mChangeSet) && !(cur->mState & DISCARDED) ) {
				ret = cur;
			}
			cur = cur->mNext;
			if( cur == pristine ) break;
		}
		if( !ret )
			ret = pristine;
		return ret;
	}
	return this;
}

} // namespace
