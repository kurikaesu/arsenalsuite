
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

#include <memory>

#include <qapplication.h>
#include <qcursor.h>
#include <qdatetime.h>
#include <qsqldatabase.h>
#include <qsqlquery.h>
#include <qsqlerror.h>
#include <qtextstream.h>
#include <qstringlist.h>

#include "blurqt.h"
#include "connection.h"
#include "database.h"
#include "expression.h"
#include "freezercore.h"
#include "indexschema.h"
#include "recordimp.h"
#include "record.h"
#include "sqlerrorhandler.h"
#include "table.h"
#include "tableschema.h"
#include "trigger.h"
#include "updatemanager.h"

namespace Stone {

Table::Table( Database * database, TableSchema * schema )
: mDatabase( database )
, mSchema( schema )
, mKeyIndex( 0 )
, mParent( 0 )
, mPreloaded( false )
, mPreloadNotificationsSetup(false)
, mImpCount( 0 )
, mEmptyImp( 0 )
{
	connect( schema, SIGNAL( triggerAdded( Trigger * ) ), SLOT( slotTriggerAdded( Trigger * ) ) );
	
	for( int i = 0; i < 4; i++ )
		mSqlElapsed[i] = 0;
	for( int i = 0; i < 5; i++ )
		mIndexElapsed[i] = 0;

	// Child tables are created after their parents, so the normal propogation in slotTriggerAdded
	// won't be effective.  So we call slotTriggerAdded for parent tables here.
	while( schema ) {
		foreach( Trigger * trigger, schema->triggers() )
		slotTriggerAdded( trigger );
		schema = schema->parent();
	}
}

void Table::setup()
{
	foreach( IndexSchema * is, mSchema->indexes() ) {
		Index * i = 0;
		if( is->field() && is->field()->flag(Field::PrimaryKey) )
			i = mKeyIndex = new KeyIndex( this, is );
		else
			i = new HashIndex( this, is );
		addIndex( i );
	}
	if( mSchema->parent() )
		mParent = mDatabase->tableFromSchema( mSchema->parent() );
	foreach( TableSchema * ts, mSchema->children() )
		mChildren += mDatabase->tableFromSchema( ts );
	foreach( Field * f, mSchema->columns() ) {
		if( !mKeyIndex && f->flag( Field::PrimaryKey ) )
			addIndex( mKeyIndex = new KeyIndex( this, f->index() ) );
		if( f->indexDeleteMode() ) {
			Table * fkey = mDatabase->tableFromSchema( f->foreignKeyTable() );
			if( !fkey ) {
				LOG_1( "Unable to find table for delete action: " + f->foreignKey() );
				continue;
			}
			mDatabase->tableFromSchema( f->foreignKeyTable() )->addDeleteAction( f );
		}
	}
}

Table::~Table()
{
	clearIndexes();
	foreach( Table * t, mChildren )
		delete t;
	foreach( Index * i, indexes() )
		if( i != mKeyIndex )
			delete i;
	if( mEmptyImp ) {
		mEmptyImp->deref();
		mEmptyImp = 0;
	}
	if( mImpCount > 0 ) {
		LOG_1( schema()->tableName() + " has " + QString::number( mImpCount ) + " references remaining" );
	}
	delete mKeyIndex;
	mKeyIndex = 0;
}

Connection * Table::connection() const
{
	return mDatabase->connection();
}

Table * Table::parent() const
{
	return mParent;
}

QList<Table*> Table::inherits()
{
	TableList ret;
	Table * t = mParent;
	while( t ) {
		ret.push_front( t );
		t = t->parent();
	}
	return ret;
}

QList<Table*> Table::children()
{
	return mChildren;
}

QList<Table*> Table::tableTree()
{
	TableList ret;
	ret += this;
	foreach( Table * t, mChildren )
		ret += t->tableTree();
	return ret;
}

Record Table::load( QVariant * v )
{
	RecordImp * imp = emptyImp();
	if( v ) {
		imp = new RecordImp( this, v );
		imp->mChangeSet = ChangeSet::current();
	}
	return Record(imp,false);
}

Record * Table::newObject()
{
	return new Record( emptyImp(), false );
}

void Table::preload()
{
	if( schema()->isPreloadEnabled() && !mPreloaded ) {
		if( !mPreloadNotificationsSetup ) {
			mPreloadNotificationsSetup = true;
			mDatabase->setupPreloadListen(this);
		}
		select("",VarList(), true, false, false);
	}
}

bool Table::isPreloaded() const
{
	return mPreloaded;
}

void Table::invalidatePreload()
{
	mPreloaded = false;
	preload();
}

void Table::preloadProject( uint pkey )
{
	mProjectPreload[pkey] = true;
	TableList tl = tableTree() + inherits();
	foreach( Table * t, tl )
		t->setCacheIncoming( true );
	if( mSchema->fieldPos( "fkeyProject" ) >= 0 )
		select("WHERE fkeyproject=" + QString::number( pkey ),VarList(), true, false, false);
	else
		select("INNER JOIN Element ON " + mSchema->tableName() + "." + mSchema->projectPreloadColumn() +
			"=Element.keyElement WHERE Element.fkeyProject=" + QString::number(pkey), VarList(), true, false, false);
	foreach( Table * t, tl )
		t->setCacheIncoming( false );
}
	
bool Table::isProjectPreloaded( uint fkeyElement, bool preloadNow )
{
	if( mSchema->isPreloadEnabled() ) {
		preload();
		return true;
	}
	
	if( mSchema->projectPreloadColumn().isEmpty() ){
		if( mParent )
			return mParent->isProjectPreloaded( fkeyElement );
		return false;
	}
	
	Table * elTable = mDatabase->tableByName( "Element" );
	if( !elTable )
		return false;

	Record r( elTable->record( fkeyElement, false ) );
	if( r.isRecord() ){
		uint projectKey = r.getValue( "fkeyProject" ).toUInt();
		if( projectKey ) {
			QMap<uint,bool>::Iterator it = mProjectPreload.find( projectKey );
			if( it==mProjectPreload.end() ) {
				if( preloadNow )
					preloadProject( projectKey );
				else
					return false;
			}
			return true;
		}
	}
	return false;
}

IndexList Table::indexes()
{
	return mIndexes;
}

Index * Table::indexFromSchema( IndexSchema * schema ) const
{
	QHash<IndexSchema*,Index*>::const_iterator it = mSchemaToIndex.find( schema );
	if( it == mSchemaToIndex.end() ) {
		if( mParent ) return mParent->indexFromSchema( schema );
		return 0;
	}
	return it.value();
}

Index * Table::indexFromField( const QString & name ) const
{
	Field * field = schema()->field(name);
	if( field && field->index() )
		return indexFromSchema( field->index() );
	return 0;
}

Index * Table::index( const QString & name ) const
{
	IndexSchema * is = mSchema->index(name);
	return is ? indexFromSchema(is) : 0;
}

KeyIndex * Table::keyIndex() const
{
	return mKeyIndex;
}

void Table::clearIndexes()
{
	foreach( Index * i, mIndexes )
		if( i != mKeyIndex )
			i->clear();
}

void Table::addIndex( Index * index )
{
	if( !mIndexes.contains( index ) ) {
		mIndexes += index;
		mSchemaToIndex.insert( index->schema(), index );
	}
}

void Table::removeIndex( Index * index, bool dontDelete )
{
	mIndexes.removeAll( index );
	mSchemaToIndex.remove( index->schema() );
	if( !dontDelete )
		delete index;
}

Record Table::record( uint key, bool doSelect, bool useCache, bool baseOnly )
{
	return record( key, (doSelect ? Index::UseSelect : 0) | (useCache ? Index::UseCache : 0), baseOnly );
}

Record Table::record( uint key, int lookupMode, bool baseOnly, bool * _found )
{
	Record ret( this );

	if( key==0 )
		return ret;

	bool found = false;
	bool preloadCheck = false;
	while( lookupMode & Index::UseCache ) {
		KeyIndex * ki = keyIndex();
		if( ki ) {
			ret = ki->record( key, &found );
			// Ignore empty entries for child tables
			if( found )
				break;
		}
		if( !baseOnly ) {
			foreach( Table * t, mChildren ) {
				ret = t->record( key, Index::UseCache, false );
				// If found in a child table, we don't need to check against the current
				// changeset because that will have already been done
				if( ret.isRecord() ) {
					if( _found ) *_found = true;
					return ret;
				}
			}
		}

		if( preloadCheck ) break;
		preload();
		preloadCheck = true;
	}

	if( !found && (lookupMode & Index::UseSelect) ){
		VarList args;
		args += key;
		RecordList res = select( mSchema->primaryKey() + "=?", args, !baseOnly, true );
		if( res.size() ) {
			if( _found ) *_found = true;
			return res[0];
		}
		// Since this record was never found, we don't want to waste time selecting again
		if( mKeyIndex )
			mKeyIndex->setEmptyEntry( args );
	}
	
	ChangeSet cs = ChangeSet::current();
	if( cs.isValid() ) {
		RecordList changes;
		cs.visibleRecordsChanged( ret.isRecord() ? 0 : &changes, 0, ret.isRecord() ? &changes : 0, QList<Table*>()<<this );
		if( ret.isRecord() ) {
			if( changes.contains(ret) )
				ret = Record();
		} else {
			foreach( Record r, changes )
				if( r.key() == key )
					ret = r;
		}
	}
	if( _found ) *_found = found || ret.key() == key;
	return ret;
}

template<class T> static QList<T> unique( QList<T> list )
{
	return list.toSet().toList();
}

RecordList Table::records( QList<uint> keys, bool select, bool useCache )
{
	return records( keys, (select ? Index::UseSelect : 0) | (useCache ? Index::UseCache : 0) | Index::PartialSelect );
}

RecordList Table::records( QList<uint> keys, int lookupMode )
{
	RecordList ret;
	if( keys.isEmpty() )
		return ret;

	// Make the list unique
	keys = unique(keys);

	QStringList strs;
	QList<uint> selecting;
	foreach( uint key, keys ) {
		if( key == 0 ) continue;
		if( lookupMode & Index::UseCache ) {
			bool found = false;
			Record r = record( key, Index::UseCache, false, &found );
			if( r.isRecord() ) {
				ret += r;
				continue;
			} else if( found )
				continue;
		}
		// If we have to select all of them, then stop checking the index
		if( !(lookupMode & Index::PartialSelect) )
			break;
		
		if( !(lookupMode & Index::UseSelect) )
			continue;
		
		strs += QString::number( key );
		selecting += key;
	}
	
	if( ret.size() < keys.size() && (lookupMode & Index::UseSelect) ) {
		if( !(lookupMode & Index::PartialSelect) ) {
			foreach( uint key, keys )
				strs << QString::number(key);
			selecting = keys;
			ret.clear();
		}
		
		RecordList tmp = records( strs.join( "," ) );
		// Set empty entries
		if( selecting.size() != static_cast<int>(tmp.size()) ) {
			foreach( uint key, selecting ) {
				if( mKeyIndex && tmp.filter( mSchema->primaryKey(), key ).isEmpty() )
					mKeyIndex->setEmptyEntry( VarList() += key );
			}
		}
		ret += tmp;
	}
	return ret;
}

// keystring is a comma seperated list of keys eg "1,2,999999,10202020202020"
RecordList Table::records( const QString & keystring )
{
	if( keystring.isEmpty() ) return RecordList();
	return select( "WHERE " + mSchema->primaryKey() + " IN (" + keystring + ")" );
}

void Table::setCacheIncoming( bool ci )
{
	foreach( Index * i, mIndexes )
		i->cacheIncoming( ci );
}

void Table::recordsAdded( RecordList recs, bool notifyIndexes )
{
	if( notifyIndexes ) {
		QTime time;
		time.start();
		foreach( Index * i, mIndexes )
			foreach( Record r, recs )
				i->recordAdded( r );
		addIndexTime( time.elapsed(), IndexAdded );

		// Call added on the parent after the current, so that
		// another select(project preload) doesn't load another
		// version of one of the records and put it in our key
		// cache before we can do it, causing two versions of
		// the same record
		if( mParent )
			mParent->recordsAdded( recs );
	}

	if( notifyIndexes && (QThread::currentThread() != qApp->thread() || mDatabase->queueRecordSignals()) ) {
		QMetaObject::invokeMethod( this, "added", Qt::QueuedConnection, Q_ARG(RecordList, recs), Q_ARG(bool, false) );
		return;
	}
	
	emit added( recs );
}

void Table::recordsRemoved( RecordList recs, bool notifyIndexes )
{
	if( notifyIndexes ) {
		if( mParent )
			mParent->recordsRemoved( recs );
		
		QTime time;
		time.start();
		foreach( Index * i, mIndexes )
			foreach( Record r, recs )
				i->recordRemoved( r );
		addIndexTime( time.elapsed(), IndexRemoved );
	}

	if( notifyIndexes && (QThread::currentThread() != qApp->thread() || mDatabase->queueRecordSignals()) ) {
		QMetaObject::invokeMethod( this, "removed", Qt::QueuedConnection, Q_ARG(RecordList, recs), Q_ARG(bool, false) );
		return;
	}

	emit removed( recs );
}

void Table::recordUpdated( const Record & current, const Record & upd, bool notifyIndexes )
{
	if( notifyIndexes ) {
		if( mParent )
			mParent->recordUpdated( current, upd );
			
		QTime time;
		time.start();
		foreach( Index * i, mIndexes )
			i->recordUpdated( current, upd );
		addIndexTime( time.elapsed(), IndexUpdated );
	}

	if( notifyIndexes && (QThread::currentThread() != qApp->thread() || mDatabase->queueRecordSignals()) ) {
		QMetaObject::invokeMethod( this, "updated", Qt::QueuedConnection, Q_ARG(Record, current), Q_ARG(Record, upd) );
		return;
	}

	emit updated( current, upd );
}

void Table::slotTriggerAdded( Trigger * trigger )
{
	if( trigger->mTriggerTypes & Trigger::CreateTrigger ) {
		if( mEmptyImp ) {
			// Have to temporarily change the state so that writing to the record doesn't cause a copy
			mEmptyImp->mState = RecordImp::MODIFIED;
			trigger->create( Record(mEmptyImp) );
			mEmptyImp->mState = RecordImp::EMPTY_SHARED;
		}
	}
	foreach( Table * t, mChildren )
		t->slotTriggerAdded( trigger );
}

RecordList Table::processIncoming( const RecordList & records, bool cacheIncoming, bool checkForUpdates )
{
	RecordList processed;
	
	if( checkForUpdates ) {
		foreach( Record r, records )
			processed.append( checkForUpdate( r ) );
	} else
		processed = records;

	processed = mSchema->processIncomingTriggers( processed );

	if( mParent )
		mParent->processIncoming( processed, cacheIncoming, false );
	
	foreach( Index * i, mIndexes )
		i->recordsIncoming( processed, cacheIncoming );
	
	mDatabase->recordsIncoming( processed, cacheIncoming );
	return processed;
}

JoinedSelect Table::join( Table * table, QString condition, JoinType joinType, bool ignoreResults, const QString & alias )
{
	return JoinedSelect( this ).join( table, condition, joinType, ignoreResults, alias );
}

static RecordList applyChangeSet( QList<Table*> tables, const RecordList & ret, const Expression & exp )
{
	ChangeSet cs(ChangeSet::current());
	if( cs.isValid() ) {
		RecordList added, updated, removed;
		cs.visibleRecordsChanged(&added, &updated, &removed, tables);
		RecordList ret2;
		foreach( Record r, (ret-removed) + updated + added )
			if( exp.matches(r) )
				ret2.append(r);
		qDebug() << "Got " << ret.size() << " records from database, " << ret2.size() << " after updating to reflect changeset";
		return ret2.unique();
	}
	return ret;
}

RecordList Table::select( const Expression & exp, bool selectChildren, bool expectSingle, bool needResults )
{
	RecordList ret;
	TableList tables;

	// Fill mgl with tables to select from
	if( selectChildren )
		tables = tableTree();
	else
		tables += this;

	if( tables.size() > 1 && (connection()->capabilities() & Connection::Cap_MultiTableSelect) ) {
		ret = selectMulti( tables, exp, needResults, false );
	} else
	{
		foreach( Table * t, tables ) {
			ret += t->selectOnly( exp, needResults, false );
			if( ret.size() && expectSingle )
				break;
		}
	}

	return ret;
}

RecordList Table::select( const QString & where, const VarList & args, bool selectChildren, bool expectSingle, bool needResults )
{
	RecordList ret;
	TableList mgl;
	QString w = where;
	bool cacheCandidate = where.simplified().isEmpty();

	// Fill mgl with tables to select from
	if( selectChildren )
		mgl = tableTree();
	else
		mgl += this;

	if( mgl.size() > 1 && (connection()->capabilities() & Connection::Cap_MultiTableSelect) ) {
		ret = selectMulti( mgl, w, args, QString(), VarList(), needResults, cacheCandidate );
	} else {
		for( TableIter it = mgl.begin(); ; ){
			Table * man = *it;
	
			//LOG_5( "Selecting from table: " + man->tableName() );
			// Perform the select
			RecordList res = man->selectOnly( w, args, needResults, cacheCandidate && (selectChildren || man->children().isEmpty()) );
	
			if( res.size() )
			{
				// Add results to return list
				ret += res;
			}
			
			++it;
	
			// Return if that was the last table or if we have enough results
			if( it == mgl.end() || (ret.size() && expectSingle) )
				break;
	
			// Massage where clause for next table
			w.replace( man->schema()->tableName() + ".", (*it)->schema()->tableName() + ".", Qt::CaseInsensitive);
		}
	}
	
	if( where.isEmpty() && ChangeSet::current().isValid() ) {
		RecordList added, removed;
		ChangeSet::current().visibleRecordsChanged(&added, 0, &removed, QList<Table*>() << this);
		//qDebug() << ret.debug() << "\n" << removed.debug();
		ret -= removed;
		ret += added;
	}
	
	return ret;
}

/*
Table * Table::importSchema( const QString & tableName, Database * parent )
{
	QString out;
	QString info_query( "select att.attname, typ.typname from pg_class cla "
			"inner join pg_attribute att on att.attrelid=cla.oid "
			"inner join pg_type typ on att.atttypid=typ.oid "
			"where cla.relkind='r' AND cla.relnamespace=2200 AND att.attnum>0 AND cla.relname='" );
	
	info_query += tableName().lower() + "';";
	
	QSqlQuery q( info_query );
	if( !q.isActive() )
	{
		out += warnAndRet( "Unable to select table information for table: " + tableName() );
		out += warnAndRet( "Error was: " + q.lastError().text() );
		return 0;
	}

	Table * ret = new Table( parent );

	while( q.next() ) {
		QString fieldName = q.value(0).toString();
		QString type = q.value(1).toString();
		int ft = fieldTypeFromPgType( type );
		if( ft == Field::Invalid ) continue;
		new Field( ret, fieldName, false, ft );
	}
	
	return ret;
}
*/

RecordList Table::selectFrom( const QString & from, const VarList & args )
{
	return processIncoming( connection()->selectFrom( this, from, args ) );
}

RecordList Table::selectOnly( const QString & where, const VarList & args, bool needResults, bool cacheIncoming )
{
	RecordList ret;

	// No values in the table, only used as a base for inheritance
	if( mSchema->baseOnly() )
		return ret;

	// If this table is preloaded, and a parent table is filling some
	// caches, we need to return the records to fill the cache.
	// Returning all values from this table should work fine.
	// This could be slower in some situations if there is an index
	// on this select in the db and there are a lot of records in
	// this table, but usually one wouldn't use preload if there are
	// a lot of records.
	if( mPreloaded && mKeyIndex && (!needResults || where.isEmpty()) ) {
		//LOG_5( "Returning preloaded values" );
		return mKeyIndex->values();
	}
	
	if( where.toLower().contains( "for update" ) && !mDatabase->ensureInsideTransaction() )
		return ret;
	
	ret = connection()->selectOnly( this, where, args );
	ret = processIncoming( ret, cacheIncoming );

	if( where.isEmpty() && mSchema->isPreloadEnabled() )
		mPreloaded = true;

	return ret;
}

RecordList Table::selectOnly( const Expression & exp, bool needResults, bool cacheIncoming )
{
	bool reflectsChangeset = false;
	Connection * conn = connection();
	FieldList fields = schema()->defaultSelectFields();
	Expression transformed = Query( fields, Expression(this,/*only=*/true), exp ).prepareForExec(conn,&reflectsChangeset);
	RecordList ret = processIncoming( conn->executeExpression(this,fields,transformed), cacheIncoming );
	if( !reflectsChangeset )
		ret = applyChangeSet( QList<Table*>() << this, ret, exp );
	return ret;
}

RecordList Table::selectMulti( TableList tables, const Expression & exp, bool needResults, bool cacheIncoming )
{
	RecordList ret;
	// If this table is preloaded, and a parent table is filling some
	// caches, we need to return the records to fill the cache.
	// Returning all values from this table should work fine.
	// This could be slower in some situations if there is an index
	// on this select in the db and there are a lot of records in
	// this table, but usually one wouldn't use preload if there are
	// a lot of records.
	
	// I dont think we actually hit this case currently, and it doesn't look correct because the child
	// tables may not be preloaded, therefore we can't rely on their values being in our keyCache.
	/*
	if( mPreloaded && mKeyIndex && (!needResults || (innerWhere.isEmpty() && outerWhere.isEmpty())) ) {
		//LOG_5( "Returning preloaded values" );
		return mKeyIndex->values();
	}*/
	
	// Expressions don't have for update support yet
	//if( innerWhere.toLower().contains( "for update" ) && !mDatabase->ensureInsideTransaction() )
	//	return ret;
	bool reflectsChangeset = false;
	Connection * conn = connection();
	QList<RecordReturn> rrl;
	Expression transformed = exp.copy().transformToInheritedUnion(tables,rrl).prepareForExec(conn,&reflectsChangeset,/*makeCopy=*/false);
	
	QMap<Table*,RecordList> resultsByTable;
	if( rrl.size() == 1 )
		resultsByTable = connection()->executeExpression( this, rrl[0], transformed );
	else
		LOG_1( "Unable to execute expression, the number of RecordReturn records isnt 1" );
	
	for( QMap<Table*,RecordList>::iterator it = resultsByTable.begin(); it != resultsByTable.end(); ++it ) {
		Table * t = it.key();
		ret += t->processIncoming( it.value(), cacheIncoming );
	}

	/*if( innerWhere.isEmpty() && outerWhere.isEmpty() && mSchema->isPreloadEnabled() ) {
		foreach( Table * t, tables )
			t->mPreloaded = true;
	}*/

	if( !reflectsChangeset )
		ret = applyChangeSet( tables, ret, exp );
	
	return ret;
}

RecordList Table::selectMulti( TableList tables, const QString & innerWhere, const VarList & innerArgs, const QString & outerWhere, const VarList & outerArgs, bool needResults, bool cacheIncoming )
{
	RecordList ret;

	// If this table is preloaded, and a parent table is filling some
	// caches, we need to return the records to fill the cache.
	// Returning all values from this table should work fine.
	// This could be slower in some situations if there is an index
	// on this select in the db and there are a lot of records in
	// this table, but usually one wouldn't use preload if there are
	// a lot of records.
	
	// I dont think we actually hit this case currently, and it doesn't look correct because the child
	// tables may not be preloaded, therefore we can't rely on their values being in our keyCache.
	/*
	if( mPreloaded && mKeyIndex && (!needResults || (innerWhere.isEmpty() && outerWhere.isEmpty())) ) {
		//LOG_5( "Returning preloaded values" );
		return mKeyIndex->values();
	}*/
	
	if( innerWhere.toLower().contains( "for update" ) && !mDatabase->ensureInsideTransaction() )
		return ret;
	
	QMap<Table*,RecordList> resultsByTable = connection()->selectMulti( tables, innerWhere, innerArgs, outerWhere, outerArgs );

	for( QMap<Table*,RecordList>::iterator it = resultsByTable.begin(); it != resultsByTable.end(); ++it ) {
		Table * t = it.key();
		ret += t->processIncoming( it.value(), cacheIncoming );
	}

	// Don't select if where is empty and we already have the table contents
	if( innerWhere.isEmpty() && outerWhere.isEmpty() && mSchema->isPreloadEnabled() ) {
		foreach( Table * t, tables )
			t->mPreloaded = true;
	}

	return ret;
}

void Table::selectFields( RecordList records, FieldList fields )
{
	if( fields.isEmpty() || records.isEmpty() ) return;
	RecordList validRecords;
	foreach( Record r, records )
		if( r.table() == this )
			validRecords += r;
	FieldList validFields, myFields( schema()->columns() );
	foreach( Field * f, fields )
		if( myFields.contains( f ) )
			validFields += f;
	connection()->selectFields( this, validRecords, validFields );
}

struct InsertState
{
	RecordList toInsert, inserted;
};

void * Table::insertBegin( RecordList rl )
{
	if( !mDatabase->ensureInsideTransaction() )
		return 0;
	
	RecordList toInsert = mSchema->processPreInsertTriggers( rl );
	
	if( toInsert.isEmpty() )
		return 0;
	
	std::auto_ptr<InsertState> is(new InsertState);
	is->toInsert = toInsert;
	
	foreach( Record r, toInsert ) {
		RecordImp * copy = r.imp()->copy(false);
		// Borrow the pointer to the modified bits
		copy->mModifiedBits = r.imp()->mModifiedBits;
		is->inserted.append(copy);
	}
	
	try {
		connection()->insert( this, is->inserted );
	} catch(...) {
		// Clear the borrowed pointer to avoid double delete
		foreach( Record r, is->inserted )
			r.imp()->mModifiedBits = 0;
		throw;
	}
	
	for( int i = 0; i < is->inserted.size(); ++i ) {
		is->toInsert[i].imp()->setCommitted(is->inserted[i].imp()->key());
		// Clear the borrowed pointer to avoid double delete
		is->inserted[i].imp()->mModifiedBits = 0;
	}
	return is.release();
}

bool Table::insertComplete( void * insertState )
{
	InsertState * is = (InsertState*)insertState;
	
	for( int i=0; i < is->toInsert.size(); ++i ) {
		RecordImp * src = is->inserted[i].imp(), * dst = is->toInsert[i].imp();
		qSwap( src->mValues, dst->mValues );
		qSwap( src->mLiterals, dst->mLiterals );
		// Clear the link from the now pristine record to the changeset
		// It is now visible to all
		dst->mChangeSet = ChangeSet();
	}
	
	mSchema->processPostInsertTriggers( is->toInsert );

	//LOG_5( "UpdateManager::recordsAdded" );
	UpdateManager::instance()->recordsAdded( this, is->toInsert );
	//LOG_5( "recordsAdded" );
	recordsAdded( is->toInsert );
	//LOG_5( "mDatabase->recordsAdded" );
	mDatabase->recordsAdded( is->toInsert, true );
	
	delete is;

	return true;
}

void Table::insertRollback( void * insertState )
{
	InsertState * is = (InsertState*)insertState;
	// Clear the committed state
	foreach( Record r, is->toInsert ) {
		r.imp()->setCommitted(0);
	}
	delete is;
}

bool Table::insert( const RecordList & rl )
{
	void * insertState = insertBegin(rl);
	return insertState ? insertComplete(insertState) : false;
}

bool Table::insert( const Record & rb )
{
	return insert( RecordList(rb) );
}

// incoming is either a record just created from data coming from the database(via select or update returning),
// or it's the contents of a just executed update to the database, either way it reflects the current database state.
// Now that we have linked versions on records we could theoretically avoid the swap below by marking the current pristine
// record as discarded and marking the incoming as pristine, but that would need changes elsewhere in the record/recordimp/changeset
// interactions.
Record Table::checkForUpdate( Record incoming )
{
	Record pristine = mKeyIndex->record(incoming.key(),0).pristine();
	
	if( pristine.isValid() && pristine.imp() != incoming.imp() ) {
		// We bypass the Records here and go straight to the recordimps, both for speed and
		// because we don't want to be messing about with the changeset values, we already
		// have the versions we want
		RecordImp * pristine_imp = pristine.imp();
		RecordImp * incoming_imp = incoming.imp();
		
		bool needsUpdateSignal = false;
		FieldList fl = mSchema->fields();
		foreach( Field * f, fl ) {
			// Don't do a comparison if the before-update record didn't have this column selected
			if( f->flag( Field::NoDefaultSelect ) && !pristine_imp->isColumnSelected( f->pos() ) )
				continue;
			QVariant v1 = pristine_imp->getColumn( f->pos() );
			QVariant v2 = incoming_imp->getColumn( f->pos() );
			if( (v1.isNull() != v2.isNull()) || (v1 != v2) ) {
				needsUpdateSignal = true;
				break;
			}
		}
		
		// Optimization to keep old values in memory if they haven't changed at all
		if( !needsUpdateSignal )
			return pristine;
		
		qSwap( incoming_imp->mValues, pristine_imp->mValues );
		incoming_imp->mState |= RecordImp::HOLDS_OLD_VALUES;
		
		// Update all unchanged columns of any modified versions
		pristine_imp->updateChildren();
		
		if( needsUpdateSignal )
			callUpdateSignals(pristine,incoming);
		return pristine;
	}
	return incoming;
}

void Table::callUpdateSignals( const Record & pristine, const Record & old )
{
	mSchema->processPostUpdateTriggers( pristine, old );
	UpdateManager::instance()->recordUpdated( this, pristine, old );
	recordUpdated( pristine, old );
	mDatabase->recordUpdated( pristine, old, true );
}

void Table::update( RecordImp * imp )
{
	update( RecordList(Record(imp)) );
}

struct UpdateState
{
	UpdateState() : selectAfterUpdate(false), askForRet(false) {}
	RecordList ret, pristine, toUpdate;
	bool selectAfterUpdate, askForRet;
};

void * Table::updateBegin( RecordList records )
{
	std::auto_ptr<UpdateState> us(new UpdateState);
	
	// Must be a modified, committed record to be updated
	foreach( Record r, records ) {
		RecordImp * imp = r.imp();
		if( (imp->mState & (RecordImp::MODIFIED|RecordImp::COMMITTED)) != (RecordImp::MODIFIED|RecordImp::COMMITTED) ) {
			qWarning( "Table::update: Update on record that is not both committed and modified" );
			continue;
		}

		// There were no columns to update
		if( !imp->mModifiedBits )  {
			//qWarning( "Table::update: Record had MODIFIED field set, but no modified columns" );
			continue;
		}
		
		Record p = r.pristine();
		us->pristine.append(p);
		r = mSchema->processPreUpdateTriggers( r, p );
		us->selectAfterUpdate = us->selectAfterUpdate || bool(imp->mLiterals);
		us->toUpdate.append(r);
	}
	
	if( !mDatabase->ensureInsideTransaction() ) {
		qDebug() << "Database ensure inside transaction failed";
		return 0;
	}
	
	Connection * c = connection();

	us->askForRet = us->selectAfterUpdate && (c->capabilities() & Connection::Cap_Returning);
	bool success = c->update( this, us->toUpdate, us->askForRet ? &(us->ret) : 0 );
	if( !success ) {
		qDebug() << "Error sending update to the database";
		return 0;
	}
	return us.release();
}

void Table::updateComplete( void * updateState )
{
	UpdateState * us = (UpdateState*)updateState;
	
	for( int i=0; i < us->toUpdate.size(); ++i ) {
		RecordImp * pristine_imp = us->pristine[i].imp();
		RecordImp * imp = us->toUpdate[i].imp();
		
		// Swap values from old to new
		qSwap( imp->mValues, pristine_imp->mValues );
		
		// If we already got updated values then fill them in to the pristine
		if( us->askForRet ) {
			RecordImp * incoming_imp = us->ret[i].imp();
			foreach( Field * f, mSchema->fields() ) {
				int i = f->pos();
				if( incoming_imp->isColumnSelected(i) ) {
					pristine_imp->mValues->replace(i,incoming_imp->mValues->at(i));
				}
			}
		}

		imp->mState |= RecordImp::HOLDS_OLD_VALUES;
		imp->clearColumnLiterals();
	}
	
	if( us->selectAfterUpdate && !us->askForRet )
			// Refresh the record from the database, to get any columns changed via triggers, etc.
	
			try {
				// TODO: Need 2 stage select with the first stage executed in updateBegin, this can throw
				// Leaving the changeset in a inconsistent state
				selectOnly( mSchema->primaryKey() + " IN (" + us->toUpdate.keyString() + ")", VarList(), true );
			} catch(...) {
				qDebug() << "Exception thrown in updateComplete's select, this code needs fixed!";
			}
	else {
		for( int i=0; i < us->toUpdate.size(); ++i ) {
			Record p = us->pristine[i].imp();
			// Then we update the pristine
			p.imp()->updateChildren();

			callUpdateSignals( p, us->askForRet ? us->ret[i] : us->toUpdate[i] );
		}
	}
	delete us;
}

void Table::updateRollback( void * updateState )
{
	delete (UpdateState*)updateState;
}

void Table::update( RecordList records )
{
	void * updateState = updateBegin(records);
	if( updateState )
		updateComplete(updateState);
}

int Table::remove( const Record & rb )
{
	return remove( RecordList() += Record( rb ) );
}

struct RemoveState
{
	QList<int> removed;
	RecordList rl;
	int result;
};

void * Table::removeBegin( const RecordList & const_list )
{
	std::auto_ptr<RemoveState> rs(new RemoveState);
	
	RecordList rl( const_list );

	rl = mSchema->processPreDeleteTriggers(rl);

	// Pre-delete trigger may have removed all records
	if( rl.isEmpty() ) {
		qDebug() << "No records left to delete after processing pre delete triggers";
		return 0;
	}
	
	//rl = gatherToRemove( rl );
	
	for( RecordIter it = rl.begin(); it != rl.end(); )
	{
		if( !(*it).key() || (it.imp()->mState & RecordImp::DELETED) ){
			it = rl.remove( it );
			continue;
		}
		++it;
	}

	if( !mDatabase->ensureInsideTransaction() )
		return 0;

	QString keys = rl.keyString();

	if( keys.isEmpty() ) {
		qDebug() << "No records with primary keys to delete";
		return 0;
	}
	
	rs->rl = rl;
	rs->result = connection()->remove( this, keys, &(rs->removed) );
	return rs.release();
}

int Table::removeComplete( void * removeState )
{
	RemoveState * rs = (RemoveState*)removeState;
	if( rs->result >= 0 ) {
		QSet<int> removedSet;
		bool partialDelete = (rs->result != (int)rs->rl.size());
		if( partialDelete )
			removedSet = rs->removed.toSet();
		for( RecordIter it = rs->rl.begin(); it != rs->rl.end(); ) {
			// Remove the records that weren't removed in the db
			if( partialDelete && !removedSet.contains((*it).key()) ) {
				it = rs->rl.remove(it);
				continue;
			}
			it.imp()->mState = RecordImp::DELETED;
			++it;
		}
		mSchema->processPostDeleteTriggers( rs->rl );
		UpdateManager::instance()->recordsDeleted( this, rs->rl );
		recordsRemoved( rs->rl );
		mDatabase->recordsRemoved( rs->rl, true );
	}
	delete rs;
	return rs->result;
}

void Table::removeRollback( void * removeState )
{
	delete (RemoveState*)removeState;
}

int Table::remove( const RecordList & const_list )
{
	void * removeState = removeBegin(const_list);
	return removeState ? removeComplete(removeState) : 0;
}

RecordList Table::gatherToRemove( const RecordList & toRemove )
{
	RecordList our_remove;
	FieldList das = deleteActions();
	for( RecordIter it = toRemove.begin(); it != toRemove.end(); ++it )
	{
		if( (*it).key() == 0 ) continue;
		Record r = mKeyIndex ? mKeyIndex->record( (*it).key(), 0 ) : Record();
		our_remove += r.isRecord() ? r : *it;
		foreach( Field * f, das ) {
			int dm = f->indexDeleteMode();
			if( dm == Field::DoNothingOnDelete ) continue;
			Table * t = mDatabase->tableFromSchema( f->table() );
			Index * i = t->indexFromSchema( f->index() );
			RecordList fkeys = i->recordsByIndex( VarList() << QVariant((*it).key()) );
			if( dm == Field::UpdateOnDelete ) {
				for( RecordIter uit = fkeys.begin(); uit != fkeys.end(); ++uit )
					(*it).setValue( f->name(), QVariant( 0 ) );
				fkeys.commit();
			} else if( dm == Field::CascadeOnDelete ) {
				if( t == this ) {
					fkeys = mSchema->processPreDeleteTriggers(fkeys);
					our_remove += gatherToRemove( fkeys );
				}
				else
					fkeys.remove();
			}
		}
	}
	return our_remove;
}

FieldList Table::deleteActions() const
{
	FieldList ret = mDeleteActions;
	if( mParent ) ret += mParent->deleteActions();
	return ret;
}

void Table::addDeleteAction( Field * f )
{
	if( !mDeleteActions.contains( f ) ) {
		if( mDatabase->tableFromSchema( f->table() ) == this ) // A relation to the same table
			mDeleteActions.push_front( f );
		else
			mDeleteActions += f;
	}
}

void Table::removeDeleteAction( Field * f )
{
	mDeleteActions.removeAll( f );
}

void Table::addSqlTime( int ms, int action )
{
	if( action >= SqlInsert && action <= SqlDelete )
		mSqlElapsed[action] += ms;
}

int Table::elapsedSqlTime( int action ) const
{
	int ret = 0;
	for( int i = SqlInsert; i <= SqlDelete; ++i )
		if( action == i || action == SqlAll )
			ret += mSqlElapsed[i];
	return ret;
}

void Table::printStats()
{
	// Dont print stats for a table that has none
	if( elapsedIndexTime() + elapsedSqlTime() == 0 ) return;

	LOG_3( "'" + mSchema->tableName() + "'          Sql Time Elapsed" );
	LOG_3( "|   Select  |   Update  |  Insert  |  Delete  |  Total  |" );
	LOG_3( "-----------------------------------------------" );
	LOG_3( QString(	"|     %1    |     %2    |    %3    |    %4    |    %5   |\n")
						.arg( elapsedSqlTime( Table::SqlSelect ) )
						.arg( elapsedSqlTime( Table::SqlUpdate ) )
						.arg( elapsedSqlTime( Table::SqlInsert ) )
						.arg( elapsedSqlTime( Table::SqlDelete ) )
						.arg( elapsedSqlTime() )
	);
	LOG_3(	"                  Index Time Elapsed" );
	LOG_3(	"|   Added  |   Updated  |  Incoming  |  Deleted  |  Search  |  Total  |" );
	LOG_3( "-----------------------------------------------" );
	LOG_3( QString(	"|     %1     |     %2    |    %3    |    %4   |    %5    |   %6    |\n")
						.arg( elapsedIndexTime( Table::IndexAdded ) )
						.arg( elapsedIndexTime( Table::IndexUpdated ) )
						.arg( elapsedIndexTime( Table::IndexIncoming ) )
						.arg( elapsedIndexTime( Table::IndexRemoved ) )
						.arg( elapsedIndexTime( Table::IndexSearch ) )
						.arg( elapsedIndexTime() )
	);

	foreach( Index * i, mIndexes )
		i->printStats();
}

void Table::addIndexTime( int ms, int action )
{
	if( action >= IndexAdded && action <= IndexSearch )
		mIndexElapsed[action] += ms;
}

int Table::elapsedIndexTime( int action ) const
{
	int ret = 0;
	for( int i = IndexAdded; i <= IndexSearch; ++i )
		if( action == i || action == IndexAll )
			ret += mIndexElapsed[i];
	return ret;
}

bool Table::exists() const
{
	return connection()->tableExists( schema() );
}

bool Table::verifyTable( bool createMissingColumns, QString * output )
{
	return connection()->verifyTable( schema(), createMissingColumns, output );
}

bool Table::createTable( QString * output )
{
	return connection()->createTable( schema(), output );
}

RecordImp * Table::emptyImp()
{
	if( !mEmptyImp ) {
		mEmptyImp = new RecordImp( this, 0 );
		mEmptyImp->ref();
		// Have to temporarily change the state so that writing to the record doesn't cause a copy
		mEmptyImp->mState = RecordImp::MODIFIED;
		Record r(mEmptyImp);
		mSchema->processCreateTriggers(r);
		mEmptyImp->mState = RecordImp::EMPTY_SHARED;
	}
	return mEmptyImp;
}

} // namespace
