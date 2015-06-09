
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

#include <qsqldriver.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <qregexp.h>

#include "database.h"
#include "iniconfig.h"
#include "joinedselect.h"
#include "pgconnection.h"
#include "schema.h"
#include "table.h"
#include "tableschema.h"

PGConnection::PGConnection()
: QSqlDbConnection( "QPSQL7" )
, mVersionMajor( 0 )
, mVersionMinor( 0 )
, mUseMultiTableSelect( false )
{
	connect( mDb.driver(), SIGNAL(notification(const QString&)), SIGNAL(notification(const QString&)) );
}

void PGConnection::setOptionsFromIni( const IniConfig & ini )
{
	QSqlDbConnection::setOptionsFromIni( ini );
	mUseMultiTableSelect = ini.readBool( "UseMultiTableSelect", false );
}

Connection::Capabilities PGConnection::capabilities() const
{
	Connection::Capabilities ret = static_cast<Connection::Capabilities>(QSqlDbConnection::capabilities() | Cap_Inheritance | Cap_MultipleInsert | (mUseMultiTableSelect ? Cap_MultiTableSelect : 0)
		| Cap_TableOids | Cap_Notifications | Cap_ChangeNotifications );
	if( checkVersion( 8, 2 ) )
		ret = static_cast<Connection::Capabilities>(ret | Cap_Returning);
	return ret;
}

static void getVersion( PGConnection * c, int & vMaj, int & vMin )
{
	QSqlQuery q = c->exec( "SELECT version()" );
	if( q.next() ) {
		QString versionString = q.value(0).toString();
		LOG_3( "Got Postgres Version string: " + versionString );
		QRegExp vre( "PostgreSQL (\\d+)\\.(\\d+)" );
		if( versionString.contains( vre ) ) {
			vMaj = vre.cap(1).toInt();
			vMin = vre.cap(2).toInt();
			LOG_3( "Got Postgres Version: " + QString::number( vMaj ) + "." + QString::number( vMin ) );
		}
	}
}

bool PGConnection::checkVersion( int major, int minor ) const
{
	PGConnection * c = const_cast<PGConnection*>(this);
	// By calling this we can ensure that we don't select the version twice by having this
	// same function called from code connected to the connected signal.  The function will run
	// twice but the first call will end up doing nothing since mVersionMajor will already be set.
	c->checkConnection();
	if( !mVersionMajor ) {
		PGConnection * c = const_cast<PGConnection*>(this);
		getVersion( c, c->mVersionMajor, c->mVersionMinor );
	}
	return (mVersionMajor > major) || (mVersionMajor == major && mVersionMinor >= minor);
}

bool PGConnection::reconnect()
{
	if( QSqlDbConnection::reconnect() ) {
		//checkVersion(0,0);
		return true;
	}
	return false;
}

uint PGConnection::newPrimaryKey( TableSchema * schema )
{
	while( schema->parent() ) schema = schema->parent();
	QString sql("SELECT nextval('" + schema->tableName().toLower() + "_" + schema->primaryKey().toLower() + "_seq')" );
	QSqlQuery q = exec( sql );
	if( q.isActive() && q.next() )
		return q.value(0).toUInt();
	return 0;
}

bool pgtypeIsCompat( const QString pg_type, uint enumType )
{
	if( pg_type == "text" )
		return ( enumType==Field::String );
	if( pg_type == "varchar" )
		return ( enumType==Field::String );
	if( pg_type == "int2" )
		return ( enumType==Field::UInt || enumType==Field::Int );
	if( pg_type == "int4" )
		return ( enumType==Field::UInt || enumType==Field::Int );
	if( pg_type == "int8" )
		return ( enumType==Field::UInt || enumType==Field::Int || enumType==Field::ULongLong );
	if( pg_type == "numeric" )
		return ( enumType==Field::Double || enumType==Field::Float );
	if( pg_type == "float4" )
		return (enumType==Field::Float || enumType==Field::Double);
	if( pg_type == "float8" )
		return (enumType==Field::Double || enumType==Field::Float);
	if( pg_type == "timestamp" )
		return ( enumType==Field::DateTime );
	if( pg_type == "time" )
		return ( enumType==Field::Time );
	if( pg_type == "date" )
		return ( enumType==Field::Date );
	if( pg_type == "boolean" || pg_type == "bool" )
		return ( enumType==Field::Bool );
	if( pg_type == "bytea" )
		return ( enumType==Field::ByteArray || enumType==Field::Image );
	if( pg_type == "color" )
		return ( enumType==Field::Color );
	if( pg_type == "interval" )
		return ( enumType==Field::Interval );
	return false;
}

uint fieldTypeFromPgType( const QString & pg_type )
{
	if( pg_type == "text" || pg_type == "varchar" )
		return Field::String;
	if( pg_type == "int2" || pg_type == "int4" )
		return Field::Int;
	if( pg_type == "int8" )
		return Field::ULongLong;
	if( pg_type == "numeric" )
		return Field::Double;
	if( pg_type == "float4" )
		return Field::Float;
	if( pg_type == "float8" )
		return Field::Double;
	if( pg_type == "timestamp" )
		return Field::DateTime;
	if( pg_type == "time" )
		return Field::Time;
	if( pg_type == "date" )
		return Field::Date;
	if( pg_type == "boolean" || pg_type == "bool" )
		return Field::Bool;
	if( pg_type == "bytea" )
		return Field::ByteArray;
	if( pg_type == "color" )
		return Field::Color;
	if( pg_type == "interval" )
		return Field::Interval;
	return Field::Invalid;
}

QString warnAndRet( const QString & s ) { LOG_3( s ); return s + "\n"; }

bool PGConnection::tableExists( TableSchema * schema )
{
	return exec( "select * from pg_class WHERE pg_class.relname=? AND pg_class.relnamespace=2200", VarList() << schema->tableName().toLower() ).size() >= 1;
}

bool PGConnection::verifyTable( TableSchema * schema, bool createMissingColumns, QString * output )
{
	QString out;
	bool ret = false, updateDocs = false;

	if( !tableExists( schema ) ) {
		out += warnAndRet( "Table does not exist: " + schema->tableName() );
		if( output )
			*output = out;
		return false;
	}

	FieldList fl = schema->columns();
	QMap<QString, Field*> fieldMap;
	FieldList updateDesc;

	// pg_class stores all of the tables
	// relnamespace 2200 is the current public database
	// private columns have negative attnums
	QString info_query( "select att.attname, typ.typname, des.description from pg_class cla "
			"inner join pg_attribute att on att.attrelid=cla.oid "
			"inner join pg_type typ on att.atttypid=typ.oid "
			"left join pg_description des on cla.oid=des.classoid AND att.attnum=des.objsubid "
			"where cla.relkind='r' AND cla.relnamespace=2200 AND att.attnum>0 AND cla.relname='" );
			
	info_query += schema->tableName().toLower() + "';";
	
	QSqlQuery q = exec( info_query );
	if( !q.isActive() )
	{
		out += warnAndRet( "Unable to select table information for table: " + schema->tableName() );
		out += warnAndRet( "Error was: " + q.lastError().text() );
		goto OUT;
	}
	
	for( FieldIter it = fl.begin(); it != fl.end(); ++it )
		fieldMap[(*it)->name().toLower()] = *it;
	
	while( q.next() )
	{
		QString fieldName = q.value(0).toString();
		Field * fp = 0;
		QMap<QString, Field*>::Iterator fi = fieldMap.find( fieldName );
		
		if( fi == fieldMap.end() )
			continue;
		
		fp = *fi;
		
		if( !pgtypeIsCompat( q.value(1).toString(), fp->type() ) )
		{
			out += warnAndRet( schema->tableName() + "." + fp->name() + "[" + fp->typeString() + "] not compatible: " + q.value(1).toString() );
			goto OUT;
		}

		if( !fp->docs().isEmpty() && q.value(2).toString() != fp->docs() )
			updateDesc += fp;

		fieldMap.remove ( fieldName );
	}

	if( !fieldMap.isEmpty() )
	{
		out += warnAndRet( "Couldn't find the following columns for " + schema->tableName() + ": " );
		QStringList cols;
		for( QMap<QString, Field*>::Iterator it = fieldMap.begin(); it != fieldMap.end(); ++it )
			cols += it.key();
		out += warnAndRet( cols.join( "," ) );
		if( createMissingColumns ) {
			out += warnAndRet( "Creating missing columns" );
			for( QMap<QString, Field*>::Iterator it = fieldMap.begin(); it != fieldMap.end(); ++it ) {
				Field * f = it.value();
				QString cc = "ALTER TABLE " + schema->tableName() + " ADD COLUMN \"" + f->name().toLower() + "\" " + f->dbTypeString() + ";";
				QSqlQuery query = exec(cc);
				if( !query.isActive() ) {
					out += warnAndRet( "Unable to create column: " + f->name() );
					out += warnAndRet( "Error was: " + query.lastError().text() );
					goto OUT;
				}
			}
		} else
			goto OUT;
	}
	
	if( updateDocs && updateDesc.size() ) {
		out += warnAndRet( "Updating column descriptions" );
		foreach( Field * f, updateDesc ) {
			QString sql( "COMMENT ON " + schema->tableName().toLower() + "." + f->name().toLower() + " IS '" + f->docs() + "'");
			QSqlQuery query = exec( sql );
			if( !query.isActive() ) {
				out += warnAndRet( "Unable to set description: " + sql );
			}
		}
	}

	if( schema->isPreloadEnabled() && !verifyChangeTrigger(schema,createMissingColumns) )
		goto OUT;

	ret = true;
	
	OUT:
	if( output )
		*output = out;
	return ret;
}

bool PGConnection::createTable( TableSchema * schema, QString * output )
{
	QString out;
	bool ret = false;
	QString cre("CREATE TABLE ");
	cre += schema->tableName().toLower() + "  (";
	QStringList columns;
	FieldList fl = schema->ownedColumns();
	foreach( Field * f, fl ) {
		QString ct("\"" + f->name().toLower() + "\"");
		if( f->flag( Field::PrimaryKey ) ) {
			ct += " SERIAL PRIMARY KEY";
			columns.push_front( ct );
		} else {
			ct += " " + f->dbTypeString();
			columns += ct;
		}
	}
	cre += columns.join(",") + ")";
	if( schema->parent() )
		cre += " INHERITS (" + schema->parent()->tableName().toLower() + ")";
	cre += ";";
	out += warnAndRet( "Creating table: " + cre );
	
	QSqlQuery query = exec( cre );
	if( !query.isActive() ){
		out += warnAndRet( "Unable to create table: " + schema->tableName() );
		out += warnAndRet( "Error was: " + query.lastError().text() );
		goto OUT;
	}
	
	if( schema->isPreloadEnabled() && !verifyChangeTrigger(schema,true) ) {
		out += warnAndRet( "Unable to create change trigger" );
		goto OUT;
	}

	ret = true;
	OUT:
	if( output )
		*output = out;
	return ret;
}

TableSchema * PGConnection::importTableSchema()
{
	return 0;
}

Schema * PGConnection::importDatabaseSchema()
{
	return 0;
}

static QString tableQuoted( const QString & table )
{
	if( table.contains( "-" ) || table == "user" )
		return "\"" + table + "\"";
	return table;
}

QString PGConnection::getSqlFields( TableSchema * schema, const QString & _tableAlias, bool needTableOid, FieldList * usedFields, int * pkeyPos )
{
	bool hasAlias = !_tableAlias.isEmpty();
	QString alias( tableQuoted(hasAlias ? _tableAlias : schema->tableName()) );
	QHash<TableSchema*,QString>::iterator it = (hasAlias || usedFields || pkeyPos) ? mSqlFields.end() : mSqlFields.find( schema );
	QString ret;
	if( it == mSqlFields.end() ) {
		QStringList fields;
		int i = 0;
		foreach( Field * f, schema->columns() ) {
			if( pkeyPos && f->flag( Field::PrimaryKey ) )
				*pkeyPos = i;
			if( !f->flag( Field::NoDefaultSelect ) ) {
				fields += f->name().toLower();
				i++;
				if( usedFields ) *usedFields += f;
			}
		}
		QString sql = alias + ".\"" + fields.join( "\", " + alias + ".\"" ) + "\"";
		if( !hasAlias )
			mSqlFields.insert( schema, sql );
		ret = sql;
	} else
		ret = it.value();
	if( needTableOid )
		return alias + ".tableoid, " + ret;
	return ret;
}

static QString prepareWhereClause(const QString & where)
{
	if( !where.isEmpty() ) {
		QString w(where.trimmed());
		bool needsWhere = true;
		const char * keyWords[] = {"LIMIT","ORDER","WHERE","INNER","OUTER","LEFT","RIGHT","FULL","JOIN", 0};
		for( int i = 0; keyWords[i]; ++i ) {
			if( w.startsWith(keyWords[i], Qt::CaseInsensitive) ) {
				needsWhere = false;
				break;
			}
		}
		if( needsWhere )
			w = " WHERE " + w;
		else
			w = " " + w;
		return w;
	}
	return where;
}

RecordList PGConnection::selectFrom( Table * table, const QString & _from, const QList<QVariant> & args )
{
	TableSchema * schema = table->schema();
	RecordList ret;
	
	QString from(_from.trimmed());
	if( !from.startsWith( "FROM", Qt::CaseInsensitive ) )
		from = "FROM " + from;
	
	QString select( "SELECT " + getSqlFields(schema) + " " + from );

	QSqlQuery sq = exec( select, args, true /*retry*/, table );
	while( sq.next() )
		ret += Record( new RecordImp( table, sq ), false );
	return ret;
}

RecordList PGConnection::selectOnly( Table * table, const QString & where, const QList<QVariant> & args )
{
	return selectFrom( table, "FROM ONLY " + tableQuoted(table->schema()->tableName()) + prepareWhereClause(where), args );
}

QList<RecordList> PGConnection::joinedSelect( const JoinedSelect & joined, QString where, QList<QVariant> args )
{
	Schema * schema = joined.table()->schema()->schema();
	Database * db = joined.table()->database();
	QList<RecordList> ret;
	QList<JoinCondition> joinConditions = joined.joinConditions();
	QList<Table*> tables;
	QList<bool> usingTableOid;
	QList<FieldList> fieldsPerTable;
	QList<int> pkeyPositions;
	
	tables += joined.table();
	usingTableOid += !joined.joinOnly();
	QString query( "SELECT " );
	// Generate column list
	QStringList fields;

	{
		// Add initial table
		ret += RecordList();
		int pkeyPos = -1;
		FieldList fl;
		fields += getSqlFields(joined.table()->schema(), joined.alias(), !joined.joinOnly(), &fl, &pkeyPos);
		fieldsPerTable += fl;
		pkeyPositions += pkeyPos;
	}
	
	foreach( JoinCondition jc, joinConditions ) {
		if( jc.ignoreResults )
			continue;
		tables += jc.table;
		usingTableOid += !jc.joinOnly;
		ret += RecordList();
		FieldList fl;
		int pkeyPos = -1;
		fields += getSqlFields(jc.table->schema(), jc.alias, !jc.joinOnly, &fl, &pkeyPos);
		fieldsPerTable += fl;
		pkeyPositions += pkeyPos;
		fl.clear();
	}
	
	query += fields.join(", ");
	query += " FROM ";
	if( joined.joinOnly() )
		query += "ONLY ";
	query += tableQuoted(joined.table()->schema()->tableName());
	
	// Generate join list
	int idx = 0;
	foreach( JoinCondition jc, joinConditions ) {
		switch( jc.type ) {
			case InnerJoin:
				query += " INNER";
				break;
			case LeftJoin:
				query += " LEFT";
				break;
			case RightJoin:
				query += " RIGHT";
				break;
			case OuterJoin:
				query += " OUTER";
				break;
		}
		query += " JOIN ";
		if( jc.joinOnly )
			query += "ONLY ";
		query += tableQuoted(jc.table->schema()->tableName());
		if( !jc.alias.isEmpty() && jc.alias != jc.table->schema()->tableName() )
			query += " AS " + jc.alias;

		query += " ON " + jc.condition;
		++idx;
	}
	
	query += prepareWhereClause(where) + ";";
	
	QSqlQuery sq = exec( query, args, true /*retry*/, tables[0] );
	
	while( sq.next() ) {
		int tableIndex = 0;
		int colOffset = 0;
		foreach( Table * table, tables ) {
			bool utoi = usingTableOid[tableIndex];
			Table * destTable = table;
			if( utoi ) {
				colOffset++;
				TableSchema * ts = tableByOid( sq.value(colOffset-1).toInt(), schema);
				destTable = ts ? db->tableFromSchema(ts) : table;
			}
			
			if( destTable ) {
				int pkeyPos = pkeyPositions[tableIndex];
				if( sq.value(colOffset + pkeyPos).toInt() != 0 )
					ret[tableIndex] += Record( new RecordImp( destTable, sq, colOffset, &fieldsPerTable[tableIndex] ), false );
				else
					ret[tableIndex] += Record(table);
			}
			
			colOffset += fieldsPerTable[tableIndex].size();
			
			++tableIndex;
		}
	}
	return ret;
}

QMap<Table *, RecordList> PGConnection::selectMulti( QList<Table*> tables,
	const QString & innerWhere, const QList<QVariant> & innerArgs,
	const QString & outerWhere, const QList<QVariant> & outerArgs )
{
	QMap<Table*,RecordList> ret;
	QMap<Table*,FieldList> fieldsByTable;
	QMap<Table*,QVector<int> > positionsByTable;
	QVector<int> typesByPosition(1);
	QList<QVariant> allArgs;
	QMap<Table*,QStringList> colsByTable;
	QStringList selects;
	QString innerW(prepareWhereClause(innerWhere)), outerW(prepareWhereClause(outerWhere));

	bool colsNeedTableName = innerW.toLower().contains( "join" );

	// First position is the table position
	typesByPosition[0] = Field::Int;
	int tablePos = 0;

	foreach( Table * table, tables ) {
		TableSchema * schema = table->schema();
		FieldList fields = schema->columns();
		fieldsByTable[table] = fields;
		QVector<int> positions(fields.size());
		int pos = 1, i = 0;
		QStringList sql;
		QString tableName = schema->tableName();
		sql << QString::number(tablePos);
		foreach( Field * f, fields ) {
			if( f->flag( Field::NoDefaultSelect ) ) continue;
			while( pos < typesByPosition.size() && typesByPosition[pos] != f->type() ) {
				if( tablePos == 0 )
					sql << "NULL::" + Field::dbTypeString(Field::Type(typesByPosition[pos]));
				else
					sql << "NULL";
				pos++;
			}
			if( pos >= typesByPosition.size() ) {
				typesByPosition.resize(pos+1);
				typesByPosition[pos] = f->type();
			}
			if( colsNeedTableName )
				sql << tableName + ".\"" + f->name().toLower() + "\"";
			else
				sql << "\"" + f->name().toLower() + "\"";
			positions[i] = pos;
			i++;
			pos++;
		}
		tablePos++;
		colsByTable[table] = sql;
		allArgs << innerArgs;
		positionsByTable[table] = positions;
	}

	tablePos = 0;
	foreach( Table * t, tables ) {
		QStringList cols = colsByTable[t];
		while( cols.size() < typesByPosition.size() ) {
			if( tablePos == 0 )
				cols << "NULL::" + Field::dbTypeString(Field::Type(typesByPosition[cols.size()]));
			else
				cols << "NULL";
		}

		QString w(innerW);
		w.replace( tables[0]->schema()->tableName() + ".", t->schema()->tableName() + ".", Qt::CaseInsensitive);
		selects << "SELECT " + cols.join(", ") + " FROM ONLY " + t->schema()->tableName() + " " + w;
		
		tablePos++;
	}
	allArgs << outerArgs;
	QString select = "SELECT * FROM ( (" + selects.join(") UNION (") + ") ) AS IQ " + outerW;
	QSqlQuery sq = exec( select, allArgs, true, tables[0] );
	while( sq.next() ) {
		Table * t = tables[sq.value(0).toInt()];
		ret[t] += Record( new RecordImp( t, sq, positionsByTable[t].data() ), false );
	}
	return ret;
}

void PGConnection::selectFields( Table * table, RecordList records, FieldList fields )
{
	TableSchema * schema = table->schema();
	RecordList ret;
	
	QStringList fieldNames;
	foreach( Field * f, fields )
		if( !f->flag( Field::LocalVariable ) )
			fieldNames += f->name().toLower();

	QString tableName = tableQuoted(schema->tableName());
	QString sql = "SELECT " + tableName + ".\"" + fieldNames.join( "\", " + tableName + ".\"" ) + "\"" + " FROM ONLY " + tableName + " WHERE " + schema->primaryKey() + " IN (" + records.keyString() + ");";

	QSqlQuery sq = exec( sql, VarList(), true /*retry*/, table );
	RecordIter it = records.begin();
	while( sq.next() ) {
		if( it == records.end() ) break;
		int i = 0;
		Record r(*it);
		foreach( Field * f, fields )
			r.imp()->fillColumn( f->pos(), sq.value(i++) );
	}
}

void PGConnection::insert( Table * table, const RecordList & rl )
{
	if( rl.size() <= 0 )
		return;

	TableSchema * schema = table->schema();

	bool multiInsert = checkVersion( 8, 2 );

	FieldList fields = schema->columns();
	FieldList literals;
	
	// Generate the first half of the statement INSERT INTO table (col1, col2, col3) VALUES
	QString sql( "INSERT INTO " + tableQuoted(schema->tableName()) + " ( " );
	{
		QStringList cols;
		foreach( Field * f, fields )
			cols += f->name().toLower();
		sql += "\"" + cols.join( "\", \"" ) + "\" ) VALUES ";
	}

	// multiInsert means the database supports inserting multiple rows with on statement
	// INSERT INTO table (column1,column2) VALUES (data1,data2), (data11,data22);
	if( multiInsert ) {
		QStringList recVals;
		st_foreach( RecordIter, it, rl ) 
		{
			QStringList vals;
			RecordImp * rb = it.imp();
			foreach( Field * f, fields )
			{
				int pos = f->pos();
				// A deleted record wont have any columns marked as modified, so we'll commit them all
				if( (f->flag(Field::PrimaryKey) && rb->getColumn(f).toInt() != 0) || rb->isColumnModified( pos ) || rb->mState == RecordImp::COMMIT_ALL_FIELDS ) {
					if( rb->isColumnLiteral( pos ) ) {
						literals += f;
						vals += rb->getColumn( pos ).toString();
					} else {
						vals += "?";
					}
				} else
					vals += "DEFAULT";
			}
			recVals +="( " + vals.join( "," ) + " ) ";
		}
		sql += recVals.join( ", " );

		// if we generated a new primary key, return it for the key cache
		sql += "RETURNING \"" + schema->primaryKey().toLower() + "\"";
		foreach( Field * f, literals )
			sql += ", \"" + f->name().toLower() + "\"";
		sql += ";";
		
		VarList args;

		st_foreach( RecordIter, it, rl ) 
		{
			RecordImp * rb = it.imp();
			foreach( Field * f, fields ) {
				int pos = f->pos();
				if( !rb->isColumnLiteral( pos ) && ((f->flag( Field::PrimaryKey ) && rb->getColumn(f).toInt() != 0) || (rb->isColumnModified( pos ) || rb->mState == RecordImp::COMMIT_ALL_FIELDS)) ) {
					QVariant v = f->dbPrepare( rb->getColumn( f->pos() ) );
					args += v;
				}
			}
		}
		QSqlQuery q = exec( sql, args, true /*retry*/, table );
		if( q.isActive() ) {
			if( q.size() != (int)rl.size() ) {
				LOG_1( "INSERT succeeded but rows returned(primary keys) doesnt match number of records inserted" );
				return;
			}
			
			uint i = 0;
			while( q.next() ) {
				Record r(rl[i]);
				r.imp()->mValues->replace(schema->primaryKeyIndex(),q.value(0));
				int ri = 1;
				foreach( Field * f, literals )
					r.imp()->mValues->replace(f->pos(),q.value(ri++));
				i++;
			}
		}
	} else {
		st_foreach( RecordIter, it, rl ) 
		{
			RecordImp * rb = (*it).imp();
			QString sql( "INSERT INTO %1 (" ), values( ") VALUES (" );
			sql = sql.arg( tableQuoted(schema->tableName()) );
			FieldList fields = schema->columns();
		
			bool nc = false;
			foreach( Field * f, fields )
			{
				int pos = f->pos();
				// A deleted record wont have any columns marked as modified, so we'll commit them all
				if( (f->flag(Field::PrimaryKey) && rb->getColumn(f).toInt() != 0) || rb->isColumnModified( pos ) || rb->mState == RecordImp::COMMIT_ALL_FIELDS ) {
					if( nc ) { // Need a comma ?
						sql += ", ";
						values += ", ";
					}
					sql += "\"" + f->name().toLower() + "\"";
					if( rb->isColumnLiteral( pos ) ) {
						literals += f;
						values += rb->getColumn( pos ).toString();
					} else {
						values += f->placeholder();
					}
					nc = true;
				}
			}
		
			if( values.isEmpty() ) continue;
		
			sql = sql + values + ")";
		
			TableSchema * base = schema;
			if( schema->inherits().size() )
				base = schema->inherits()[0];
		
			if( checkVersion( 8, 2 ) ) {
				sql += " RETURNING \"" + schema->primaryKey() + "\"";
				foreach( Field * f, literals )
					sql += ", \"" + f->name().toLower() + "\"";
				sql += ";";
			} else
				sql += "; SELECT currval('" + base->tableName() + "_" + schema->primaryKey() + "_seq');";
			
			QSqlQuery q = fakePrepare(sql);
		
			foreach( Field * f, fields ) {
				int pos = f->pos();
				if( !rb->isColumnLiteral( pos ) && ((f->flag( Field::PrimaryKey ) && rb->getColumn(f).toInt() != 0) || (rb->isColumnModified( pos ) || rb->mState == RecordImp::COMMIT_ALL_FIELDS)) ) {
					QVariant v = f->dbPrepare( rb->getColumn( pos ) );
					q.bindValue( f->placeholder(), v );
				}
			}
		
			if( exec( q, true /*retry*/, table, /* Using fake prepare */ true ) ) {
				if( q.next() ) {
	//				qWarning( "Table::insert: Setting primary key to " + QString::number( q.value(0).toUInt() ) );
					rb->mValues->replace(schema->primaryKeyIndex(),q.value(0));
					int ri = 1;
					foreach( Field * f, literals )
						rb->mValues->replace(f->pos(),q.value(ri++));
				} else {
					LOG_1( "Table::insert: Failed to get primary key after insert" );
					return;
				}
			}
		}

	}
	
	return;
}

bool PGConnection::update( Table * table, RecordImp * imp, Record * returnValues )
{
	TableSchema * schema = table->schema();
	bool needComma = false, selectAfterUpdate = false;
	FieldList fields = schema->columns();
	
	QString up("UPDATE %1 SET ");
	up = up.arg( tableQuoted(schema->tableName()) );
	for( FieldIter it = fields.begin(); it != fields.end(); ++it ){
		Field * f = *it;
		// if the field is the primary key we don't want to change it
		// or if the field hasn't been modified locally, don't change it
		if( f->flag( Field::PrimaryKey ) || !imp->isColumnModified( f->pos() ) )
			continue;
		if( needComma ) up += ", ";

		// column name is wrapped in quotes
		up += "\"" + f->name().toLower() + "\"";

		// if we're updating to a literal value ( such as NOW() or something )
		// put that value in, otherwise put in a bind marker
		if( imp->isColumnLiteral( f->pos() ) ) {
			up += "=" + imp->getColumn( f->pos() ).toString();
			selectAfterUpdate = true;
		} else
			up += "=" + f->placeholder();
		needComma = true;
	}
	
	// There were no columns to update
	if( !needComma ) {
		//qWarning( "Table::update: Record had MODIFIED field set, but no modified columns" );
		return false;
	}
		
	up += QString(" WHERE ") + schema->primaryKey() + "=:primarykey"; // + QString::number( imp->key() );

	if( returnValues )
		up += " RETURNING " + getSqlFields( schema );
	up += ";";

	QSqlQuery q = fakePrepare( up );
	for( FieldIter it = fields.begin(); it != fields.end(); ++it ){
		Field * f = *it;
		if( f->flag( Field::PrimaryKey ) || !imp->isColumnModified( f->pos() ) || imp->isColumnLiteral( f->pos() ) )
			continue;
		QVariant var = f->dbPrepare( imp->getColumn(f->pos()) );
		q.bindValue( f->placeholder(), var );
	}
    q.bindValue( ":primarykey", imp->key() );

	if( exec( q, true /*retryLostConn*/, table, /*usingFakePrepare =*/ true ) ) {
		if( returnValues && q.next() )
			*returnValues = Record( new RecordImp( table, q ), false );
		return true;
	}
	return false;
}

struct ColUpdate {
	Field * field;
	// Records that need this field updated
	RecordList records;
};

bool PGConnection::update( Table * table, RecordList records, RecordList * returnValues )
{
	// For a single record the other version provides more readable sql
	if( records.size() == 1 ) {
		Record retVal;
		bool ret = update( table, records[0].imp(), returnValues ? &retVal : 0 );
		if( returnValues )
			returnValues->append(retVal);
		return ret;
	}
	
	QList<ColUpdate> colUpdates;
	TableSchema * schema = table->schema();
	FieldList fields = schema->columns();
	QString pkn = schema->primaryKey(), tn = schema->tableName();
	
	foreach( Field * f, fields ) {
		ColUpdate cu;
		cu.field = f;
		foreach( Record r, records ) {
			if( r.imp()->isColumnModified(f->pos()) ) {
				cu.records.append(r);
			}
		}
		if( cu.records.size() )
			colUpdates.append(cu);
	}
	
	bool selectAfterUpdate = false;
	FieldList fieldsToUpdate;
	QStringList cols, values, valueAs;
	valueAs += pkn;
	foreach( const ColUpdate & cu, colUpdates ) {
		fieldsToUpdate += cu.field;
		QString col = "\"" + cu.field->name().toLower() + "\"";
		if( cu.records.size() == records.size() ) {
			col += " = v." + col;
			if( cu.field->type() == Field::DateTime )
				col += "::timestamp";
		} else {
			col += " = CASE WHEN " + tn + "." + pkn + " IN (" + cu.records.keyString() + ") THEN v." + col + " ELSE " + tn + "." + col + " END";
		}
		cols += col;
		valueAs += "\"" + cu.field->name().toLower() + "\"";
	}
	
	typedef QPair<QString,QVariant> StrVarPair;
	QList<StrVarPair> toBind;
	int i = 0;
	foreach( Record r, records ) {
		QStringList rvals;
		rvals += QString::number(r.key());
		
		foreach( Field * f, fieldsToUpdate ) {
			if( !r.imp()->isColumnModified(f->pos()) ) {
				rvals += "NULL::" + f->dbTypeString();
				continue;
			}
			if( r.imp()->isColumnLiteral( f->pos() ) ) {
				selectAfterUpdate = true;
				rvals += r.imp()->getColumn(f).toString();
			} else {
				QString ph = f->placeholder(i);
				QVariant val = f->dbPrepare(r.imp()->getColumn(f));
				if( val.isNull() ) {
					rvals += "NULL::" + f->dbTypeString();
				} else {
					rvals += ph;
					toBind.append( qMakePair(ph,val) );
				}
			}
		}
		values.append( "(" + rvals.join(",") + ")" );
		++i;
	}
	
	
	QString up("UPDATE %1 SET ");
	up = up.arg( tableQuoted(schema->tableName()) );
	up += cols.join(", ") + " FROM (VALUES " + values.join(", ") + ") AS v(" + valueAs.join(", ") + ") WHERE v." + pkn + "=" + tn + "." + pkn;
	
	if( returnValues )
		up += " RETURNING " + getSqlFields( schema );
	up += ";";

	QSqlQuery q = fakePrepare( up );
	foreach( StrVarPair svp, toBind )
		q.bindValue( svp.first, svp.second );

	if( exec( q, true /*retryLostConn*/, table, /*usingFakePrepare =*/ true ) ) {
		if( returnValues ) {
			while( q.next() )
				returnValues->append( Record( new RecordImp( table, q ), false ) );
		}
		return true;
	}
	return false;
}

int PGConnection::remove( Table * table, const QString & keys, QList<int> * rowsDeleted )
{
	TableSchema * schema = table->schema();
	TableSchema * base = schema;
	// is this an inherited table? if so remove from the base class
	if( schema->inherits().size() )
		base = schema->inherits()[0];
	QString del("DELETE FROM ");
	del += tableQuoted(base->tableName());
	del += " WHERE ";
	del += base->primaryKey();
	del += " IN(" + keys + ") RETURNING " + base->primaryKey() + ";";
	QSqlQuery q = exec( del, QList<QVariant>(), true /*retryLostConn*/, table );
	if( !q.isActive() ) return -1;
	if( rowsDeleted ) {
		while( q.next() )
			rowsDeleted->append( q.value(0).toInt() );
	}
	return q.numRowsAffected();
}

bool PGConnection::createIndex( IndexSchema * schema )
{
	QString cmd;
	QStringList cols;
	foreach( Field * f, schema->columns() )
		cols += f->name();
	cmd += "CREATE INDEX " + schema->name();
	cmd += " ON TABLE " + tableQuoted(schema->table()->tableName()) + "(" + cols.join(",") + ")";
	if( !schema->databaseWhere().isEmpty() )
		cmd += " WHERE " + schema->databaseWhere();
	cmd += ";";

	return exec( cmd ).isActive();
}

void PGConnection::loadTableOids()
{
	QSqlQuery namespace_q = exec( "SELECT oid FROM pg_namespace WHERE nspname='public'" );
	if( namespace_q.next() ) {
		uint namespace_oid = namespace_q.value(0).toUInt();
		QSqlQuery q = exec( "SELECT oid, relname FROM pg_class WHERE relkind='r' AND relnamespace=" + QString::number(namespace_oid) );
		while( q.next() ) {
			uint oid = q.value(0).toInt();
			QString tableName = q.value(1).toString().toLower();
			mTablesByOid[oid] = tableName;
			mOidsByTable[tableName] = oid;
		}
	}
}

TableSchema * PGConnection::tableByOid( uint oid, Schema * schema )
{
	if( mTablesByOid.size() == 0 )
		loadTableOids();
	QMap<uint,QString>::iterator it = mTablesByOid.find( oid );
	if( it != mTablesByOid.end() )
		return schema->tableByName( it.value() );
	if( oid != 0 )
		LOG_1( "Unable to find table by oid: " + QString::number(oid) );
	return 0;
}

uint PGConnection::oidByTable( TableSchema * schema )
{
	if( mTablesByOid.size() == 0 )
		loadTableOids();
	QMap<QString,uint>::iterator it = mOidsByTable.find( schema->tableName().toLower() );
	if( it != mOidsByTable.end() )
		return it.value();
	LOG_1( "Unable to find oid for table: " + schema->tableName() );
	return 0;
}

bool PGConnection::verifyTriggerExists( TableSchema * table, const QString & triggerName )
{
	LOG_5( "Verifying trigger " + triggerName + " for table " + table->tableName() + " with oid " + QString::number(oidByTable(table)) );
	return exec( "SELECT 1 FROM pg_trigger WHERE tgrelid=? AND tgname=?", VarList() << QVariant(oidByTable(table)) << QVariant(triggerName) ).size() == 1;
}

static const char * listen_func_template = 
"CREATE OR REPLACE FUNCTION %1_preload_trigger() RETURNS TRIGGER AS $$\n"
"DECLARE\n"
"BEGIN\n"
"\tNOTIFY %2_preload_change;\n"
"\tRETURN NEW;\n"
"END;\n"
"$$ LANGUAGE 'plpgsql';";

static const char * listen_trig_template =
"CREATE TRIGGER %1_preload_trigger AFTER INSERT OR DELETE OR UPDATE ON %2 FOR EACH STATEMENT EXECUTE PROCEDURE %3_preload_trigger();";

static const char * preload_change_str = "_preload_trigger";

bool PGConnection::verifyChangeTrigger( TableSchema * table, bool create )
{
	QString tableName = table->tableName();
	QString triggerName = tableName.toLower() + QString::fromAscii(preload_change_str);
	bool exists = verifyTriggerExists(table,triggerName);
	if( exists || !create )
		return exists;
	
	QString func = QString(listen_func_template).arg(tableName).arg(tableName);
	QString trig = QString(listen_trig_template).arg(tableName).arg(tableName).arg(tableName);
	exec(func);
	exec(trig);
	return true;
}
