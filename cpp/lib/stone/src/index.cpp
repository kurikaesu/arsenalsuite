
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

#include <qatomic.h>
#include <qbitarray.h>
#include <qdatetime.h>
#include <qstringlist.h>

#include "blurqt.h"
#include "changeset.h"
#include "database.h"
#include "freezercore.h"
#include "recordimp.h"
#include "index.h"
#include "table.h"
#include "tableschema.h"
#include "record.h"

uint qHash( const QVariant & var )
{
	uint ret = 0;
	switch( var.type() ) {
		case QVariant::Bool:
			ret = var.toBool() ? 1 : 0;
			break;
		case QVariant::Date:
			ret = qHash( var.toDate().toJulianDay() );
			break;
		case QVariant::DateTime:
			ret = var.toDateTime().toTime_t();
			break;
		case QVariant::Double:
			ret = qHash(int(var.toDouble() * 10000));
			break;
		case QVariant::Int:
			ret = qHash(var.toInt());
			break;
		case QVariant::UInt:
			ret = qHash(var.toUInt());
			break;
		case QVariant::LongLong:
			ret = qHash( var.toLongLong() );
			break;
		case QVariant::ULongLong:
			ret = qHash( var.toULongLong() );
			break;
		case QVariant::Time:
			ret = QTime(0,0).msecsTo(var.toTime()) + 86400000;
			break;
		case QVariant::String: // Drop through
		default:
			ret = qHash( var.toString() );
	}
//        LOG_5( "qHash<QVariant>: returned " + QString::number(ret) );
	return ret;
}

namespace Stone {

Index::Index( Table * table, IndexSchema * schema, bool useCache )
: mSchema( schema )
, mTable( table )
, mCacheIncoming( false )
, mUseCache( useCache )
, mTime( 0 )
, mMutex( QMutex::Recursive )
{
}

Index::~Index()
{
}

void Index::printStats()
{
	QStringList cols;
	foreach( Field * f, mSchema->columns() )
		cols += f->name();
	LOG_1( mTable->schema()->tableName() + " index " + cols.join(",") + " spent " + QString::number( mTime ) + "ms in Incoming" );
}

void Index::setCacheEnabled( bool cacheEnabled )
{
	if( mUseCache && !cacheEnabled ) clear();
	mUseCache = cacheEnabled;
}

bool Index::cacheEnabled() const
{
	return mUseCache;
}

bool Index::checkMatch( const Record & record, const QList<QVariant> & args, int entryCount )
{
	int entrySize = mSchema->columns().size();
	FieldList fl = mSchema->columns();
	for( int entry = 0; entry < entryCount; entry++ ) {
		bool matches = true;
		for( int i = 0; i < entrySize; i++ ) {
			if( record.getValue(fl[i]) != args[entry * entrySize + i] ) {
				matches = false;
				break;
			}
		}
		if( matches )
			return true;
	}
	return false;
}

RecordList Index::applyChangeSetHelper(ChangeSet cs, RecordList records, const QList<QVariant> & args, int entryCount)
{
	if( !cs.isValid() ) return records;
//	qDebug() << "Modifying index results for changeset " << cs.title() << cs.debug();
	foreach( ChangeSet::Change change, cs.changes() ) {
		if( change.type == ChangeSet::Change_Nested ) {
			ChangeSet child = change.changeSet();
			if( child.state() == ChangeSet::Committed || ChangeSet::current().isAnscestor(child) )
				records = applyChangeSetHelper(child, records, args, entryCount);
			continue;
		}
		foreach( Record r, change.records() ) {
			for( int i = 0; i < entryCount; ++i ) {
				switch( change.type ) {
					case ChangeSet::Change_Insert:
						if( checkMatch(r,args,entryCount) ) {
							records += r;
						}
						break;
					case ChangeSet::Change_Update:
					{
						bool inList = records.contains(r), matches = checkMatch(r,args,entryCount);
						if( matches && !inList )
							records.append(r);
						else if( !matches && inList )
							records.remove(r);
						break;
					}
					case ChangeSet::Change_Remove:
						records.remove(r);
						break;
				}
			}
		}
	}
	return records;
}

RecordList Index::applyChangeSet(RecordList records, const QList<QVariant> & args, int entryCount)
{
	ChangeSet cs = ChangeSet::current();
	while( cs.parent().isValid() )
		cs = cs.parent();
	return applyChangeSetHelper(cs,records,args,entryCount);
}

RecordList Index::recordsByIndexMulti( const QList<QVariant> & args, bool select )
{
	return recordsByIndexMulti(args, select ? UseCache | UseSelect | PartialSelect : UseCache );
}

RecordList Index::recordsByIndexMulti( const QList<QVariant> & args, int lookupMode )
{
	RecordList ret;
	int entrySize = mSchema->columns().size();

	if( args.isEmpty() || (args.size() % entrySize) )
		return ret;

	int entryCount = args.size() / entrySize;

	QBitArray needSelect(entryCount,true);
	int needSelectCount = entryCount;
	
	if( mUseCache && (lookupMode & UseCache) ) {
		QString ppc = mTable->schema()->projectPreloadColumn();
		// Doubtful that this is cheaper than filling the list normally?
		QList<QVariant> entryArgs = QVector<QVariant>(entrySize).toList();
		// See if this index has a field that matches the project_preload
		int useProjectPreload = ppc.isEmpty() ? 0 : -1;  // 1 if we checked and are using it, 0 if we checked and aren't
		int projectPreloadColumn = 0;
		int status = NoInfo;
		ChangeSet cs = ChangeSet::current();
		
		for( int entry=0; entry < entryCount; entry++ ) {
			for( int a=0; a < entrySize; a++ )
				entryArgs[a] = args[entry*entrySize + a];

			RecordList entryRet = records( entryArgs, status );

			if( status == RecordsFound ) {
				ret += entryRet;
			}
			
			if( status == NoInfo && mTable->isPreloaded() ) {
				// If the table is preloaded, and this entry isn't found, then set an empty entry.
				setEmptyEntry( entryArgs );
				status = RecordsFound;
			}
			
			if( status == NoInfo && useProjectPreload != 0 ) {
				// Check to see if we can use project preload
				if( useProjectPreload < 0 ){
					int i=0;
					foreach( Field * f, mSchema->columns() )
					{
						QString fn = f->name();
						// Now down to a single hack for the element table
						if( fn == ppc || ( fn.toLower() == "fkeyelement" && ppc.toLower() == "fkeyproject" ) ) {
							useProjectPreload = 1;
							projectPreloadColumn = i;
							break;
						}
						++i;
					}
					if( useProjectPreload < 0 )
						useProjectPreload = 0;
				}
				
				// If using project preload, ensure this project is loaded
				if( useProjectPreload == 1 && mTable->isProjectPreloaded( entryArgs[projectPreloadColumn].toUInt(), true ) ) {
					entryRet = records( entryArgs, status );
					if( status == NoInfo )
						setEmptyEntry( entryArgs );
					else
						ret += entryRet;
					status = RecordsFound;
				}
			}
			
			if( status == RecordsFound ) {
				needSelect[entry] = false;
				needSelectCount--;
			}
		}
	}

	if( !(lookupMode & UseSelect) || needSelectCount == 0 )
		return applyChangeSet(ret, args, entryCount);
	
	QList<QVariant> mod_args;
	QString sel;
	int selectCount = 0;
	
	if( entrySize == 1 && mSchema->columns()[0]->flag( Field::ForeignKey ) ) {
		bool hasNull = false;
		Field * f = mSchema->columns()[0];
		QStringList ss;
		for( int entry = 0; entry < entryCount; entry++ ) {
			if( (lookupMode & PartialSelect) && !needSelect[entry] ) continue;
			if( args[entry].isNull() )
				hasNull = true;
			else
				ss += args[entry].toString();
			selectCount++;
		}
		sel = f->name() + " IN (" + ss.join(",") + ")";
		if( hasNull )
			sel += " OR " + f->name() + " IS NULL";
	} else {
		// Setup sql
		QStringList entryFilters;
		for( int entry = 0; entry < entryCount; entry++ ) {
			if( (lookupMode & PartialSelect) && !needSelect[entry] ) continue;
			selectCount++;
			QStringList ss;
			int column=0;
			foreach( Field * f, mSchema->columns() ) {
				QString check = "(" + f->name();
				QVariant arg = args[entry * entrySize + column];
				if( (f->flag( Field::ForeignKey ) && arg.toInt() == 0) || arg.isNull() ) {
					check += " is NULL)";
				} else {
					check += "=?)";
					mod_args += arg;
				}
				ss += check;
				++column;
			}
			entryFilters += ss.join( " AND " );
		}
	
		sel = entryFilters.join( " OR " );
	}
	
	bool ciRestore = false;
	if( mUseCache ) {
		mMutex.lock();
		ciRestore = mCacheIncoming;
		mCacheIncoming = true;
		reserve(selectCount);
	}

	RecordList selected = mTable->select( sel, mod_args, true, false, true );
	if( lookupMode & PartialSelect )
		ret += selected;
	else
		ret = selected;

	if( mUseCache ) {
		QList<QVariant> entryArgs = QVector<QVariant>(entrySize).toList();
		for( int entry = 0; entry < entryCount; entry++ ) {
			if( (lookupMode & PartialSelect) && !needSelect[entry] ) continue;
			for( int column = 0; column < entrySize; ++column )
				entryArgs[column] = args[entry*entrySize + column];
			int status = NoInfo;
			if( ret.isEmpty() || (records( entryArgs, status ).isEmpty() && status == NoInfo) ) {
				setEmptyEntry( entryArgs );
			}
		}
		mCacheIncoming = ciRestore;
		mMutex.unlock();
	}

	return applyChangeSet(ret, args, entryCount);
}

void Index::reserve(int)
{
}

RecordList Index::recordsByIndex( const QList<QVariant> & args, bool select )
{
	return recordsByIndex( args, select ? UseCache & UseSelect : UseCache );
}

RecordList Index::recordsByIndex( const QList<QVariant> & args, int lookupMode )
{
	RecordList ret;
	int status = NoInfo;
	
	if( args.size() != mSchema->columns().size() )
		return ret;
	
	if( (lookupMode & UseCache) && mUseCache ) {
		ret = records( args, status );
		if( status == RecordsFound )
			return applyChangeSet(ret, args, 1);

		// See if this index has a field that matches the project_preload
		bool projectPreloaded = false;
		QString ppc = mTable->schema()->projectPreloadColumn();
		if( !ppc.isEmpty() ){
			int i=0;
			foreach( Field * f, mSchema->columns() )
			{
				QString fn = f->name();
				// Now down to a single hack for the element table
				if( fn == ppc || ( fn.toLower() == "fkeyelement" && ppc.toLower() == "fkeyproject" ) ) {
					projectPreloaded = mTable->isProjectPreloaded( args[i].toUInt(), true );
					break;
				}
			}
		}
		
		if( projectPreloaded ) {
			ret = records( args, status );
			if( status == NoInfo )
				setEmptyEntry( args );
			return applyChangeSet(ret, args, 1);
		}

		if( mTable->isPreloaded() ) {
			setEmptyEntry( args );
			return applyChangeSet(ret, args, 1);
		}
	}

	if( !(lookupMode & UseSelect) )
		return applyChangeSet(ret, args, 1);

	int i=0;
	QStringList ss;
	QList<QVariant> mod_args(args);
	foreach( Field * f, mSchema->columns() ) {
		QString check = "(\"" + f->name().toLower();
		if( (f->flag( Field::ForeignKey ) && args[i].toInt() == 0) || args[i].isNull() ) {
			check += "\" is NULL)";
			//check += "\" is ?)";
			// Make sure the variant is NULL
			//mod_args[i] = QVariant(QVariant::Int);
		} else
			check += "\"=?)";
		ss += check;
		++i;
	}

	QString sel( ss.join( " AND " ) );
	
	if( mUseCache ) {
		mMutex.lock();
		bool ci = mCacheIncoming;
		mCacheIncoming = true;
		ret = mTable->select( sel, mod_args, true, false, true );
		if( ret.isEmpty() )
			setEmptyEntry( mod_args );
		mCacheIncoming = ci;
		mMutex.unlock();
	} else
		ret = mTable->select( sel, mod_args, true, false, true );
	return applyChangeSet(ret, args, 1);
}


Record Index::recordByIndex( const QVariant & arg, bool select )
{
	return recordByIndex(arg,select ? UseCache | UseSelect : UseCache);
}

Record Index::recordByIndex( const QVariant & arg, int lookupMode )
{
	if( mSchema->columns().size() != 1 )
		return Record( mTable );
	RecordList recs = recordsByIndex( VarList() << arg, lookupMode );
	if( recs.size() )
		return recs[0];
	return Record( mTable );
}

void Index::recordsCreated( const RecordList & )
{}


HashIndex::HashIndex( Table * parent, IndexSchema * schema )
: Index( parent, schema, schema->useCache() )
, mRoot( 0 )
, mHashSize( 0 )
{
	mRoot = new QHash<QVariant,void*>();
//	printf( "Creating root node %p, for index %s\n", mRoot, (parent->tableName() + ":" + name).toLatin1().constData() );
}

HashIndex::~HashIndex()
{
	clear();
	if( mRoot ) {
//		printf( "Deleting root node %p\n", mRoot );
		delete (VarHash*)mRoot;
		mRoot = 0;
	}
}

void HashIndex::recordAdded( const Record & r )
{
	if( !mUseCache ) return;

	// Save the current cacheIncoming value
	mMutex.lock();
	bool ci = mCacheIncoming;

	// Override cacheIncoming if the record should be
	// cached because it is in a project that has been,
	// or is being, preloaded.
	if( !ci ){
		QString ppc = mTable->schema()->projectPreloadColumn();
		if( mTable->isPreloaded() )
			mCacheIncoming = true;
		else if( !ppc.isEmpty() ) {
			int fp = mTable->schema()->fieldPos( ppc );
			if( (fp >= 0) && mTable->isProjectPreloaded( r.getValue( fp ).toUInt() ) )
				mCacheIncoming = true;
		}
	}
	
	recordIncomingNode( (VarHash*)mRoot, 0, r.imp() );
	
	// Restore the old value
	mCacheIncoming = ci;
	mMutex.unlock();
}

void HashIndex::recordRemoved( const Record & rec )
{
	if( !mUseCache ) return;
	mMutex.lock();
	recordRemovedNode( mRoot, 0, rec.imp(), rec.imp() );
	mMutex.unlock();
}

void HashIndex::recordUpdated( const Record & cur, const Record & old )
{
	if( !mUseCache ) return;
	mMutex.lock();
	recordRemovedNode( mRoot, 0, cur.imp(), old.imp() );
	recordAdded( cur );
	mMutex.unlock();
//	recordIncomingNode( &mRoot, mIndexColumns.begin(), cur );
}

void HashIndex::recordsIncoming( const RecordList & records, bool ci )
{
	if( !mUseCache ) return;
	mMutex.lock();
 	bool oci = mCacheIncoming;
	if( ci ) mCacheIncoming = true;
	for( RecordIter rec = records.begin(); rec != records.end(); ++rec )
		recordIncomingNode( mRoot, 0, rec.imp() );
	mCacheIncoming = oci;
	mTime++;
	mMutex.unlock();
}

void HashIndex::setEmptyEntry( QList<QVariant> vars )
{
	if( !mUseCache ) return;
	if( vars.size() != mSchema->columns().size() )
		return;

	VarHash * node = (VarHash*)mRoot;
	void * next = 0;
	mMutex.lock();
	//int loop = 0;
	for( QList<QVariant>::Iterator varit = vars.begin(); varit != vars.end();  ){
		QVariant v = *varit;
		next = node->value( v );
		++varit;

		if( varit != vars.end() ) {
			if( !next ) {
				VarHash * tmp = new VarHash();
				next = tmp;
//				printf( "Inserting VarHash node %p under node %p with value %s\n", tmp, node, v.toString().toLatin1().constData() );
				node->insert( v, next );
			}
			node = (VarHash*)next;
		} else {
			if( mSchema->holdsList() ) {
				if( !next )
					node->insert( v, ((void*)new RecordList()) );
				else
					((RecordList*)next)->clear();
			} else {
				if( next && next != (void*)0x1 )
					((RecordImp*)next)->deref();
				node->insert( v, (void*)0x1 );
			}
		}
	}
	mMutex.unlock();
}

RecordList HashIndex::records( QList<QVariant> vars, int & status )
{
	status = NoInfo;
	if( vars.size() != mSchema->columns().size() || !mUseCache )
		return RecordList();

	mTable->preload();
	
	VarHash * node = (VarHash*)mRoot;
	void * next=0;
	mMutex.lock();
	QList<QVariant>::Iterator varit = vars.begin();
	FieldList cols = mSchema->columns();
	for( FieldIter it = cols.begin(); it != cols.end(); ++varit ){

		next = node->value( *varit );

		if( !next ) {
			mMutex.unlock();
			return RecordList();
		}
		
		++it;

		if( it != cols.end() )
			node = (VarHash*)next;
	}

	status = RecordsFound;
	RecordList ret;
	if( mSchema->holdsList() ){
		RecordList * list = (RecordList*)next;
		ret = *list;
	}else{
		if( next != (void*)0x1 )
			ret += (RecordImp*)next;
	}
	mMutex.unlock();
	return ret;
}

static QString emptyString("");

void HashIndex::recordRemovedNode( VarHash * node, int fieldIndex, RecordImp * pointer, RecordImp * oldVals )
{
	FieldList cols = mSchema->columns();
	Field * f = cols[fieldIndex];
	fieldIndex++;
	bool isStorage = (fieldIndex == cols.size());
	VarHashIter hashit = node->find( oldVals->getColumn( f->pos() ) );

	if( hashit == node->end() ) return;

	if( isStorage ){
		if( mSchema->holdsList() ){
			RecordList * list = (RecordList*)(*hashit);
			if( list->contains( pointer ) ) {
//				printf( "Removing %p from list %p under node %p\n", pointer, list, node );
				list->remove( pointer );
			}
		}else{
			if( ((RecordImp*)*hashit)==pointer ) {
//				printf( "Removing %p from node %p\n", pointer, node );
				node->erase( hashit );
				pointer->deref();
			}
		}
	} else
		recordRemovedNode( (VarHash*)*hashit, fieldIndex, pointer, oldVals );
}


void HashIndex::recordIncomingNode( VarHash * node, int fieldIndex, RecordImp * valRecord )
{
	FieldList cols = mSchema->columns();
	Field * f = cols[fieldIndex];
	fieldIndex++;
	bool isStorage = (fieldIndex == cols.size());
	QVariant v = valRecord->getColumn( f->pos() );
	void * item = node->value( v );

	if( isStorage ){
		if( mSchema->holdsList() ){
			RecordList * list = (RecordList*)item;
			if( !list && mCacheIncoming ){
				list = new RecordList();
//				printf( "Inserting list %p under node %p with value %s\n", list, node, v.toString().toLatin1().constData() );
				node->insert( v, list );
			}
			if( list && !list->contains( valRecord ) ){
				(*list) += valRecord;
//				printf( "Adding %p to list %p under node %p with value %s\n", valRecord, list, node, v.toString().toLatin1().constData() );
			}
		}else if( !item || item == (void*)0x1 ){
			valRecord->ref();
//			printf( "Adding %p under node %p with value %s\n", valRecord, node, v.toString().toLatin1().constData() );
			node->insert( v, valRecord );
		}
	}else{
		if( !item ){
			item = new VarHash();
//			printf( "Adding node %p under node %p with value %s\n", item, node, v.toString().toLatin1().constData() );
			node->insert( v, item );
		}
		recordIncomingNode( (VarHash*)item, fieldIndex, valRecord );
	}
}

void HashIndex::clear()
{
	clearNode( mRoot, 0 );
}

void HashIndex::reserve(int space)
{
	mRoot->reserve(mRoot->size() + space);
}

void HashIndex::clearNode(VarHash * node, int fieldIndex )
{
	fieldIndex++;
	bool isStorage = (fieldIndex == mSchema->columns().size());
	QHashIterator<QVariant,void*> it(*node);
	while(it.hasNext()) {
		it.next();
		void * vp = it.value();
		if( isStorage ) {
			if( mSchema->holdsList() ) {
//				printf( "Removing list %p under node %p\n", vp, node );
				delete ((RecordList*)vp);
			} else if( vp != (void*)0x1 ) {
//				printf( "Removing imp %p under node %p\n", vp, node );
				((RecordImp*)vp)->deref();
			}
		} else if( vp != (void*)0x1 ) {
//			printf( "Clearing Node %p\n", vp );
			clearNode( (VarHash*)vp, fieldIndex );
		}
	}
	node->clear();
	if( node != mRoot ) {
//		printf( "Deleting node %p\n", node );
		delete node;
	}
}

/* The KeyIndex class only holds a reference to the records if
 * preloading is enabled( all records from table are loaded, and stay loaded), or
 * if expireKeyCache is false, so any previously loaded records stay loaded.
 */
KeyIndex::KeyIndex( Table * table, IndexSchema * is )
: Index( table, is )
, mPrimaryKeyColumn( table->schema()->primaryKeyIndex() )
{}

KeyIndex::~KeyIndex()
{
	clear();
}

void KeyIndex::recordsCreated( const RecordList & records )
{
	// Return if nothing to cache
	if( !records.size() )
		return;

	//LOG_5( "Caching Incoming records in keyindex for table: " + mTable->tableName() + " index id: " + QString::number( (int)this ) );
	TableSchema * schema = mTable->schema();
	mMutex.lock();
	st_foreach( RecordIter, rec, records ){
		RecordImp * imp = rec.imp();
		uint key = imp->key();
		//LOG_5( "Caching key: " + QString::number( key ) );
		if( key && imp && imp->table() == mTable ){
			RecordImp * ri = mDict.value( key );
			if( !ri || ri == (RecordImp*)1 || ri->refCount() == 0 )
				mDict.insert( key, imp );
		}
	}
	mMutex.unlock();

}

void KeyIndex::recordAdded( const Record & r )
{
	if( r.table() != mTable ) return;
	uint key = r.key();
	TableSchema * schema = mTable->schema();
	bool needRef = schema->isPreloadEnabled() || !schema->expireKeyCache();
	mMutex.lock();
	RecordImp * fin = mDict.value( key );
	if( !fin || fin == (RecordImp*)1 ) {
		//qDebug() << "Key " << key << " added to keyindex";
		fin = r.imp();
		mDict.insert( key, fin );
	}
	// If fin is already in the dict, it was added by recordsCreated and does not have a reference
	// so we ref the imp here always
	if( needRef )
		fin->ref();
	mMutex.unlock();
}

void KeyIndex::recordRemoved( const Record & r )
{
	uint key = r.key();
	TableSchema * schema = mTable->schema();
	bool needRef = schema->isPreloadEnabled() || !schema->expireKeyCache();
	mMutex.lock();
	RecordImp * fin = mDict.value( key );
	if( fin && fin != (RecordImp*)1 ) {
		//qDebug() << "Key " << key << " removed from keyindex";
		mDict.insert( key, (RecordImp*)1 );
		mMutex.unlock();
		if( needRef )
			fin->deref();
	} else
		mMutex.unlock();
}

void KeyIndex::recordUpdated( const Record&, const Record& )
{
}

void KeyIndex::recordsIncoming( const RecordList & records, bool )
{
	// Return if nothing to cache
	if( !records.size() )
		return;

	//LOG_5( "Caching Incoming records in keyindex for table: " + mTable->tableName() + " index id: " + QString::number( (int)this ) );
	TableSchema * schema = mTable->schema();
	bool needRef = schema->isPreloadEnabled() || !schema->expireKeyCache();
	mMutex.lock();
	st_foreach( RecordIter, rec, records ){
		RecordImp * imp = rec.imp();
		uint key = imp->key();
		//LOG_5( "Caching key: " + QString::number( key ) );
		if( imp && imp->table() == mTable ){
			RecordImp * ri = mDict.value( key );
			if( !ri || ri == (RecordImp*)1 || ri->refCount() == 0 ) {
				mDict.insert( key, imp );
				if( needRef )
					imp->ref();
			}
		}
	}
	mMutex.unlock();
}

Record KeyIndex::record( uint key, bool * found )
{
	//qDebug() << "Checking " << mTable->tableName() << " KeyIndex for record: " << key << " index id: " << this;

	QMutexLocker locker( &mMutex );
	RecordImp * ri = mDict.value( key );

	Record ret;
	// Not found
	if( ri ) {
		if( ri != (RecordImp*)1 ) {
			while (1) {
				int refCount = ri->refCount();
				if( refCount == 0 ) return Record();

				// We got a reference
				if( ri->mRefCount.testAndSetRelaxed(refCount, refCount+1) )
					break;
			}
		}

		// Past here we either found a valid record, or found an "empty entry"(record does not exist in db)
		if( found )
			*found = true;

		// "Empty Entry"
		if( ri != (RecordImp*)1 ){
			ret = Record( ri, false );
			ri->mRefCount.deref();
		}
	}
	//qDebug() << mTable->tableName() << " KeyIndex::record returning " << ri << " with count " << (ri ? ri->refCount() : 0);
	return ret;
}

RecordList KeyIndex::values()
{
	RecordList ret;
	for( QHash<uint, RecordImp*>::const_iterator it = mDict.begin(); it != mDict.end(); ++it )
		if( it.value() != (RecordImp*)1 ) {
			Record r(it.value());
			// Will check if the record is deleted in the current changeset
			if( r.isRecord() )
				ret += it.value();
		}
	ChangeSet cs = ChangeSet::current();
	if( cs.isValid() ) {
		RecordList added, removed;
		cs.visibleRecordsChanged( &added, 0, 0, QList<Table*>()<<mTable );
		ret += added;
	}
	return ret;
}

RecordList KeyIndex::records( QList<QVariant> , int & status )
{
	status = NoInfo;
	return RecordList();
}

void KeyIndex::setEmptyEntry( QList<QVariant> vars )
{
	mMutex.lock();
	if( vars.size() == 1 ) {
		//qDebug() << mTable->tableName() << " KeyIndex::record setting empty entry for record " << vars[0].toUInt();
		mDict.insert( vars[0].toUInt(),(RecordImp*)1 );
	}
	mMutex.unlock();
}

void KeyIndex::clear()
{
	RecordList tmp;
	mMutex.lock();
	for( QHash<uint, RecordImp*>::const_iterator it = mDict.begin(); it != mDict.end(); ++it )
		if( it.value() != (RecordImp*)1 ) {
			// Put it into a tmp list to deref later, to avoid mutex contention in expire
			tmp += it.value();
		}
	mDict.clear();
	mMutex.unlock();
	TableSchema * schema = mTable->schema();
	bool needRef = schema->isPreloadEnabled() || !schema->expireKeyCache();
	st_foreach( RecordIter, it, tmp )
		if( needRef )
			it.imp()->deref();
	//qDebug() << mTable->tableName() << " KeyIndex cleared";
}

void KeyIndex::expire( uint recordKey, RecordImp * imp )
{
	mMutex.lock();
	if( mDict.value(recordKey) == imp ) {
		mDict.remove( recordKey );
		//qDebug() <<  "Expiring " << mTable->tableName() << " KeyIndex for record: " << QString::number(recordKey) << " index id: " << this;
	}
	mMutex.unlock();
}

} //namespace
