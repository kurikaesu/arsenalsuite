
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

#include "indexschema.h"
#include "tableschema.h"

namespace Stone {

IndexSchema::IndexSchema( const QString & name, TableSchema * parent, bool isList, bool useCache )
: mName( name )
, mTable( parent )
, mField( 0 )
, mIsList( isList )
, mUseCache( useCache )
, mIsDatabaseIndex( false )
{
	if( mTable )
		mTable->addIndex( this );
}

IndexSchema::~IndexSchema()
{
	if( mTable )
		mTable->removeIndex( this, true );
	if( mField )
		mField->removeIndex( this );
}

void IndexSchema::addColumn( const QString & column )
{
	Field * f = table()->field( column );
	if( !f ) {
		LOG_1( "Index::addColumn: Couldn't find " + table()->tableName() + "." + column );
		return;
	}
	addField( f );
}

void IndexSchema::addField( Field * f )
{
	if( f ) {
		mIndexColumns += f;
	}
}

/*
// Quick and dirty CREATE INDEX parser.
bool parseCreateIndex( const QString & create, QString * indexName, QString * tableName, QString * columns, QString * where )
{
	QString cmd( create );

	// Chop 'CREATE INDEX '
	cmd = cmd.mid( 13 );

	int next = cmd.find( ' ' );
	if( next < 0 )
		return false;

	if( indexName )
		*indexName = cmd.left( next );

	// Chop name and ' ON '
	cmd = cmd.mid( next + 4 );

	next = cmd.find( ' ' );
	if( next < 0 )
		return false;

	if( tableName )
		*tableName = cmd.left( next );
	
	// Chop tablename and ' ('
	cmd  = cmd.mid( next + 2 );
	
	next = cmd.find( ')' );
	if( next < 0 )
		return false;

	if( columns )
		*columns = cmd.left( next );

	next = cmd.find( " WHERE " );
	if( next < 0 )
		return false;

	// Get from after the ' WHERE ' to the ';' ex. ' WHERE XXXXXXXXXX;'
	if( where )
		*where = cmd.mid( next + 7, cmd.length() - next - 8 );

	return true;
}

bool IndexSchema::verifyDatabaseIndex( bool create )
{
	QString cmd;
	cmd = "SELECT pg_get_indexdef(pg_class.oid) from pg_class where relname=?;";
	
	FreezerCore::lockDb();
	QSqlQuery q;
	q.prepare( cmd );
	q.addBindValue( name() );
	bool res = q.exec();
	FreezerCore::unlockDb();

	if( !res ) {
		qWarning( q.lastError().databaseText() );
		qWarning( q.lastError().driverText() );
		return false;
	}
	
	if( q.size() != 1 ) {
		qWarning( "Index::verifyDatabaseIndex: pg_get_indexdef didn't return 1 row for index '" + name() + "'" );
		return false;
	}

	q.next();
	
	QString name, table, columns, where;
	if( !parseCreateIndex( q.value(0).toString(), &name, &table, &columns, &where ) ) {
		qWarning( "Index::verifyDatabaseIndex: Unable to parse CREATE INDEX cmd provided by postgres's pg_get_indexdef function." );
		return false;
	}

	bool success = true;
	
	// Check table
	if( table != mTable->tableName() ) {
		qWarning( "Index::verifyDatabaseIndex: Tablename for index " + name + " is listed as " + table +
					" in the database, " + mTable->tableName() + " locally" );
		success = false;
	}

	// Check columns
	if( success ) {
		QStringList cols = QStringList::split( ',', columns );
		if( cols.size() != mIndexColumns.size() ) {
			qWarning( "Index::verifyDatabaseIndex: Database reports " + QString::number( cols.size() ) + 
						"columns, our list has " + QString::number( mIndexColumns.size() ) );
			success = false;
		}
		QMap<QString, bool> checked;
		for( FieldIter it = mIndexColumns.begin(); it != mIndexColumns.end(); ++it )
		{
			checked[(*it).name()] = false;
		}
	
		for( QStringList::Iterator it = cols.begin(); it != cols.end(); ++it )
		{
			if( !checked.contains( *it ) ) {
				qWarning( "Index::verifyDatabaseIndex: Database report column " + *it + ", not found in our column list." );
				success = false;
			}
			checked[*it] = true;
		}
	}

	if( success ) {
		if( mDatabaseWhere != where ) {
			qWarning( "Index::verifyDatabaseIndex: Database where '" + where + "' doesn't match ours: '" + mDatabaseWhere );
			success = false;
		}
	}

	return success;	
}
*/

} //namespace

