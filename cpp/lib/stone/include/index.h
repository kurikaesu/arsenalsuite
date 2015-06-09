
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

#ifndef INDEX_H
#define INDEX_H

#include <qlist.h>
#include <qvariant.h>
#include <qmutex.h>
#include <qhash.h>

#include "field.h"
#include "indexschema.h"

typedef QHash<QVariant, void*> VarHash;
typedef QHash<QVariant, void*>::iterator VarHashIter;

namespace Stone {
class Record;
class RecordImp;
class RecordList;
class Table;
class ChangeSet;

/**
 * \ingroup Stone
 * @{
 */
class STONE_EXPORT Index
{
public:
	Index( Table * table, IndexSchema * schema, bool useCache = false );
	virtual ~Index();

	enum {
		NoInfo,
		RecordsFound
	};
	
	enum LookupMode {
		// Allow getting values from the cache if available
		UseCache = 0x1,
		// Allow selecting from the database if not all values are available in the cache(or not using the cache)
		UseSelect = 0x2,
		// Only select values missing from the cache.  Useful only with both UseCache and UseSelect defined.
		// Only applicable to recordsByIndexMulti
		PartialSelect = 0x4,
	};
	
	IndexSchema * schema() const { return mSchema; }
	Table * table() const { return mTable; }

	// Setting this to false will clear all cached values
	void setCacheEnabled( bool cacheEnabled );
	bool cacheEnabled() const;

	// Set whether this index should be caching incoming values
	void cacheIncoming( bool ci ) { mCacheIncoming = ci; }

	// This returns results for multiple index entries at once.
	// The size of args must be a multiple of the number of columns
	// used in this index.
	RecordList recordsByIndexMulti( const QList<QVariant> & args, int lookupMode = UseCache | UseSelect | PartialSelect );
	RecordList recordsByIndexMulti( const QList<QVariant> & args, bool select );

	// Returns the records in the index that match args, will select
	// from database if the records dont exist in the index and select=true
	// *deprecated*, use the methods below with lookupMode bit field
	RecordList recordsByIndex( const QList<QVariant> & args, bool select );
	Record recordByIndex( const QVariant & arg, bool select );

	RecordList recordsByIndex( const QList<QVariant> & args, int lookupMode = UseCache | UseSelect );
	Record recordByIndex( const QVariant & arg, int lookupMode = UseCache | UseSelect );

	virtual RecordList records( QList<QVariant> vars, int & status )=0;
	virtual void setEmptyEntry( QList<QVariant> vars )=0;

	// Records that have been created locally but not committed to the database
	// This function is (currently) called when a record needs a primary key, though
	// in the future it may be called the first time a record is assigned
	virtual void recordsCreated( const RecordList & );
	virtual void recordAdded( const Record & )=0;
	virtual void recordRemoved( const Record & )=0;
	virtual void recordUpdated( const Record &, const Record & )=0;
	virtual void recordsIncoming( const RecordList &, bool ci = false )=0;
	virtual void clear()=0;
	virtual void reserve(int size);

	void printStats();

protected:
	IndexSchema * mSchema;
	Table * mTable;
	bool mCacheIncoming, mUseCache;
private:
	Index(const Index &) {}

	bool checkMatch( const Record & record, const QList<QVariant> & args, int entryCount );
	RecordList applyChangeSetHelper(ChangeSet cs, RecordList, const QList<QVariant> & args, int entryCount);
	RecordList applyChangeSet(RecordList, const QList<QVariant> & args, int entryCount);
	
public:
	uint mTime;
	QMutex mMutex;
};

class STONE_EXPORT HashIndex : public Index
{
public:
	HashIndex( Table * parent, IndexSchema * );
	~HashIndex();

	virtual RecordList records( QList<QVariant> vars, int & status );
	virtual void setEmptyEntry( QList<QVariant> vars );

	virtual void recordAdded( const Record & );
	virtual void recordRemoved( const Record & );
	virtual void recordUpdated( const Record &, const Record & );
	virtual void recordsIncoming( const RecordList &, bool ci = false );
	virtual void clear();
	virtual void reserve(int size);
protected:

	void recordIncomingNode( VarHash *, int fieldIndex , RecordImp * );
	void recordRemovedNode( VarHash * node, int fieldIndex, RecordImp * record, RecordImp * vals );
	void clearNode( VarHash * node, int fieldIndex );

	VarHash* mRoot;
	int mHashSize;
};

class STONE_EXPORT KeyIndex : public Index
{
public:
	KeyIndex( Table * parent, IndexSchema * schema );
	virtual ~KeyIndex();

	Record record( uint key, bool * foundEntry );

	virtual RecordList records( QList<QVariant> vars, int & status );
	virtual void setEmptyEntry( QList<QVariant> vars );
	
	virtual void recordsCreated( const RecordList & );
	virtual void recordAdded( const Record & );
	virtual void recordRemoved( const Record & );
	virtual void recordUpdated( const Record &, const Record & );
	virtual void recordsIncoming( const RecordList &, bool ci = false );
	virtual void clear();
	void expire( uint recordKey, RecordImp * imp );

	RecordList values();
	
protected:

	QHash<uint, RecordImp*> mDict;
	uint mPrimaryKeyColumn;
};

} //namespace

using Stone::Index;
using Stone::KeyIndex;
using Stone::HashIndex;

typedef QList<Index*> IndexList;
typedef QList<Index*>::Iterator IndexIter;

#include "record.h"

/// @}

#endif // INDEX_H

