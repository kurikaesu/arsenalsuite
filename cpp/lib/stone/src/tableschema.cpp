
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

#include "database.h"
#include "schema.h"
#include "tableschema.h"
#include "trigger.h"

namespace Stone {

TableSchema::TableSchema( Schema * schema )
: mSchema( schema )
, mParent( 0 )
, mColumnCount( 0 )
, mPrimaryKeyIndex( -1 )
, mFirstColumnIndex( 0 )
, mPreload( false )
, mBaseOnly( false )
, mUseCodeGen( false )
, mExpireKeyCache( true )
{
	if( schema )
		schema->addTable( this );
}

TableSchema::~TableSchema()
{
	while( !mChildren.isEmpty() )
		delete *mChildren.begin();
	IndexSchemaList il = mIndexes;
	mIndexes.clear();
	for( IndexSchemaIter it = il.begin(); it != il.end(); ++it )
		delete *it;
	if( mSchema )
	    mSchema->removeTable( this, false /*Dont delete, already been deleted*/ );
	if( mParent )
		mParent->removeChild( this );
	while( !mFields.isEmpty() )
		delete *mFields.begin();
	foreach( Trigger * trigger, mTriggers )
		delete trigger;
}

Record TableSchema::load( QVariant * v )
{
	return Record( new RecordImp( table(), v ), false );
}

Record * TableSchema::createObject( const Record & r )
{
	return new Record( r );
}

Record * TableSchema::newObject()
{
	return new Record( new RecordImp( table() ), false );
}

TableSchema * TableSchema::parent() const
{
	return mParent;
}

Table * TableSchema::table() const
{
	Database * db = Database::current( schema() );
	return db->tableFromSchema( const_cast<TableSchema*>(this) );
}

void TableSchema::setParent( TableSchema * t )
{
	if( t == mParent ) return;
	if( mParent )
		mParent->removeChild( this );
	mParent = t;
	if( mParent )
		mParent->addChild( this );
	recalcFieldPositions( mParent ? mParent->fields().size() : 0 );
}

TableSchemaList TableSchema::inherits()
{
	TableSchemaList ret;
	TableSchema * t = mParent;
	while( t ) {
		ret.push_front( t );
		t = t->parent();
	}
	return ret;
}

TableSchemaList TableSchema::children()
{
	return mChildren;
}

TableSchemaList TableSchema::tableTree()
{
	TableSchemaList ret;
	ret += this;
	foreach( TableSchema * ts, mChildren )
		ret += ts->tableTree();
	return ret;
}

bool TableSchema::isDescendant( const TableSchema * t ) const
{
	if( t == this ) return true;
	for( ConstTableSchemaIter it = mChildren.begin(); it != mChildren.end(); ++it )
		if( *it == t || (*it)->isDescendant( t ) )
			return true;
	return false;
}

void TableSchema::addChild( TableSchema * TableSchema )
{
	if( !mChildren.contains( TableSchema ) )
		mChildren += TableSchema;
}

void TableSchema::removeChild( TableSchema * TableSchema )
{
	mChildren.removeAll( TableSchema );
}

QString TableSchema::tableName()
{
	return mTableName;
}

void TableSchema::setTableName(const QString & tn)
{
	QString on = mTableName;
	mTableName = tn;
	if( mSchema )
		mSchema->tableRenamed( this, on );
}

QString TableSchema::className()
{
	return mClassName;
}

void TableSchema::setClassName( const QString & cn )
{
	QString ocn = mClassName;
	mClassName = cn;
	if( mClassName.isEmpty() )
		mClassName = tableName();
	if( mSchema )
		mSchema->tableClassRenamed( this, ocn );
}

QString TableSchema::docs() const
{
	return mDocs;
}

void TableSchema::setDocs( const QString & docs )
{
	mDocs = docs;
}

bool TableSchema::addField( Field * field )
{
	/*
	 * Only 1 primary key per TableSchema
	 */
	if( field->flag( Field::PrimaryKey ) )
	{
		foreach( Field * f, mFields )
			if( f->flag( Field::PrimaryKey ) )
				return false;
	}
	
	int pos = fields().size();
	mFields += field;
	mAllFieldsCache += field;
	if( !field->flag(Field::LocalVariable) )
		mAllColumnsCache += field;
	field->setPos( pos );
	if( field->flag( Field::PrimaryKey ) )
		mPrimaryKeyIndex = pos;

	// All the children TableSchemas need to recalc, but this TableSchema
	// is fine
	recalcFieldPositions( pos + 1, true );

	return true;
}

FieldList filterFields( FieldList src, bool local )
{
	FieldList ret;
	foreach( Field * f, src )
		if( f->flag( Field::LocalVariable ) == local )
			ret += f;
	return ret;
}

void TableSchema::recalcFieldPositions( int start, bool skipSelf )
{
	if( !skipSelf ) {
		if( mParent )
			mPrimaryKeyIndex = mParent->mPrimaryKeyIndex;
		else
			mPrimaryKeyIndex = -1;
		mFirstColumnIndex = start;
		for( FieldIter it = mFields.begin(); it != mFields.end(); ++it ) {
			if( mPrimaryKeyIndex < 0 && (*it)->flag( Field::PrimaryKey ) )
				mPrimaryKeyIndex = start;
			(*it)->setPos( start++ );
		}
		mAllFieldsCache = mParent ? mParent->fields() + mFields : mFields;
		mAllColumnsCache = filterFields( mFields, false );
		if( mParent ) mAllColumnsCache = mParent->columns() + mAllColumnsCache;
	}

	for( TableSchemaIter it = mChildren.begin(); it != mChildren.end(); ++it )
		(*it)->recalcFieldPositions( start );
	
}

FieldList TableSchema::localVariables()
{
	return filterFields( fields(), true );
}

FieldList TableSchema::ownedLocalVariables()
{
	return filterFields( ownedFields(), true );
}

void TableSchema::removeField( Field * f )
{
	mFields.removeAll( f );
	recalcFieldPositions( parent() ? parent()->columns().size() : 0 );
}

uint TableSchema::columnCount()
{
	return columns().size();
}
/*
FieldList TableSchema::columns()
{
	return mAllColumnsCache;
}
*/
FieldList TableSchema::ownedColumns()
{
	return filterFields( ownedFields(), false );
}

QStringList TableSchema::fieldDisplayNames()
{
	QStringList ret;
	foreach( Field * field, mAllFieldsCache )
		ret << field->displayName();
	return ret;
}

QStringList TableSchema::fieldNames()
{
	QStringList ret;
	foreach( Field * field, mAllFieldsCache )
		ret << field->name();
	return ret;
}
/*
FieldList TableSchema::fields()
{
	return mAllFieldsCache;
}
*/
FieldList TableSchema::ownedFields()
{
	return mFields;
}

FieldList TableSchema::defaultSelectFields() const
{
	FieldList ret;
	foreach( Field * field, mAllColumnsCache )
	{
		if( !field->flag(Field::NoDefaultSelect) )
			ret << field;
	}
	return ret;
}

Field * TableSchema::field( const QString & fieldName, bool silent )
{
	QString fnl = fieldName.toLower();
	FieldList fl = fields();
	foreach( Field * f, fl )
		if( f->methodName().toLower() == fnl || f->name().toLower() == fnl )
			return f;
	if( !silent )
		LOG_5( "TableSchema " + tableName() + " Couldn't find field: " + fieldName );
	return 0;
}

Field * TableSchema::field( int pos )
{
	FieldList cl = fields();
	if( pos >=0 && pos < (int)cl.size() )
		return cl[pos];
	return 0;
}

int TableSchema::fieldPos( const QString & column )
{
	Field * f = field( column );
	if( f ) return f->pos();
	return -1;
}

QString TableSchema::primaryKey()
{
	FieldList fields = columns();
	foreach( Field * f, fields )
		if( f->flag( Field::PrimaryKey ) )
			return f->name();
	return QString::null;
}

void TableSchema::setPreloadEnabled( bool pl )
{
	mPreload = pl;
}

bool TableSchema::isPreloadEnabled() const
{
	return mPreload;
}

void TableSchema::setProjectPreloadColumn( const QString & pp )
{
	mProjectPreloadColumn = pp;
}

QString TableSchema::projectPreloadColumn() const
{
	return mProjectPreloadColumn;
}

bool TableSchema::useCodeGen() const
{
	return mParent ? mParent->useCodeGen() : mUseCodeGen;
}

void TableSchema::setUseCodeGen( bool useCodeGen )
{
	if( mParent ) return;
	mUseCodeGen = useCodeGen;
}

void TableSchema::setBaseOnly( bool baseOnly )
{
	mBaseOnly = baseOnly;
}

IndexSchemaList TableSchema::indexes()
{
	return mIndexes;
}

void TableSchema::addIndex(IndexSchema * index)
{
	if( !mIndexes.contains( index ) ) {
		mIndexes += index;
	}
}

void TableSchema::removeIndex( IndexSchema * index, bool dontDelete )
{
	int pos = mIndexes.indexOf( index );
	if( pos >= 0 ) {
		mIndexes.removeAt( pos );
		if( !dontDelete )
			delete index;
	}
}

IndexSchema * TableSchema::index( const QString & name ) const
{
	foreach( IndexSchema * is, mIndexes )
		if( is->name() == name )
			return is;
	return 0;
}

QString TableSchema::diff( TableSchema * after )
{
	QStringList changes;
	if( className() != after->className() )
		changes += "Class Name change from " + className() + " to " + after->className();
	if( (bool(parent()) != bool(after->parent())) || (parent() && parent()->tableName() != after->parent()->tableName()) )
		changes += "Parent class changed from " + (parent() ? parent()->tableName() : "") + " to " + (after->parent() ? after->parent()->tableName() : "");
	if( docs() != after->docs() )
		changes += "Docs changed from: \n\t" + docs().simplified().replace("\n","\n\t") + "\nto:\n\t" + after->docs().simplified().replace("\n","\n\t");
	foreach( Field * before_field, ownedFields() )
	{
		Field * after_field = after->field( before_field->name() );
		if( !after_field || after_field->table() != after ) {
			changes += "Removed Column " + className() + "." + before_field->name();
			continue;
		}
		changes += before_field->diff( after_field );
	}
	foreach( Field * after_field, after->ownedFields() )
	{
		Field * before_field = field( after_field->name() );
		if( !before_field || before_field->table() != this ) {
			changes += "Added Column " + className() + "." + after_field->name();
		}
	}
	changes = changes.filter( QRegExp( "." ) );
	return changes.isEmpty() ? QString() : (tableName() + " Changes:\n\t" + changes.join("\n\t"));
}

void TableSchema::addTrigger( Trigger * trigger )
{
	mTriggers.push_front(trigger);
	emit triggerAdded(trigger);
}

void TableSchema::removeTrigger( Trigger * trigger )
{
	mTriggers.removeAll(trigger);
}

QList<Trigger*> TableSchema::triggers() const
{
	return mTriggers;
}

void TableSchema::processCreateTriggers( Record & record )
{
	foreach( Trigger * trigger, mTriggers )
		if( trigger->mTriggerTypes & Trigger::CreateTrigger )
			trigger->create(record);

	if( mParent )
		mParent->processCreateTriggers(record);
}

RecordList TableSchema::processIncomingTriggers( RecordList incoming )
{
	
	foreach( Trigger * trigger, mTriggers )
	if( trigger->mTriggerTypes & Trigger::IncomingTrigger )
		incoming = trigger->incoming(incoming);
	return incoming;
}

RecordList TableSchema::processPreInsertTriggers( RecordList toInsert )
{
	foreach( Trigger * trigger, mTriggers )
		if( trigger->mTriggerTypes & Trigger::PreInsertTrigger )
			toInsert = trigger->preInsert(toInsert);
	if( mParent )
		toInsert = mParent->processPreInsertTriggers(toInsert);
	return toInsert;
}

Record TableSchema::processPreUpdateTriggers( const Record & updated, const Record & before )
{
	Record toUpdate(updated);
	foreach( Trigger * trigger, mTriggers )
		if( trigger->mTriggerTypes & Trigger::PreUpdateTrigger ) {
			toUpdate = trigger->preUpdate(toUpdate, before);
			// If the trigger returns the old record then we reject the update and stop processing
			if( toUpdate.imp() == before.imp() )
				return toUpdate;
		}
	if( mParent )
		toUpdate = mParent->processPreUpdateTriggers( toUpdate, before );
	return toUpdate;
}

RecordList TableSchema::processPreDeleteTriggers( RecordList toDelete )
{
	foreach( Trigger * trigger, mTriggers )
		if( trigger->mTriggerTypes & Trigger::PreDeleteTrigger )
			toDelete = trigger->preDelete( toDelete );
	if( mParent )
		toDelete = mParent->processPreDeleteTriggers( toDelete );
	return toDelete;
}

void TableSchema::processPostInsertTriggers( RecordList toInsert )
{
	foreach( Trigger * trigger, mTriggers )
		if( trigger->mTriggerTypes & Trigger::PostInsertTrigger ) {
			try {
				trigger->postInsert(toInsert);
			} catch (const std::exception & e) {
				qDebug() << "Caught exception from post insert trigger: " << e.what();
			} catch (...) {
				qDebug() << "Caught unknown(doesn't inherit from std::exception) exception from post insert trigger";
			}
		}
	if( mParent )
		mParent->processPostInsertTriggers(toInsert);
}

void TableSchema::processPostUpdateTriggers( const Record & updated, const Record & before )
{
	foreach( Trigger * trigger, mTriggers )
		if( trigger->mTriggerTypes & Trigger::PostUpdateTrigger ) {
			try {
				trigger->postUpdate( updated, before );
			} catch (const std::exception & e) {
				qDebug() << "Caught exception from post update trigger: " << e.what();
			} catch (...) {
				qDebug() << "Caught unknown(doesn't inherit from std::exception) exception from post update trigger";
			}
		}
	if( mParent )
		mParent->processPostUpdateTriggers( updated, before );
}

void TableSchema::processPostDeleteTriggers( RecordList deleted )
{
	foreach( Trigger * trigger, mTriggers )
		if( trigger->mTriggerTypes & Trigger::PostDeleteTrigger ) {
			try {
				trigger->postDelete(deleted);
			} catch (const std::exception & e) {
				qDebug() << "Caught exception from post delete trigger: " << e.what();
			} catch (...) {
				qDebug() << "Caught unknown(doesn't inherit from std::exception) exception from post delete trigger";
			}
		}
	if( mParent )
		mParent->processPostDeleteTriggers( deleted );
}

} // namespace
