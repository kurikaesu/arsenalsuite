
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

#ifndef PG_CONNECTION_H
#define PG_CONNECTION_H

#include "connection.h"

namespace Stone {
class IndexSchema;
class Schema;
}
using namespace Stone;


class STONE_EXPORT PGConnection : public QSqlDbConnection
{
Q_OBJECT
public:
	PGConnection();

	virtual void setOptionsFromIni( const IniConfig & );

	virtual Capabilities capabilities() const;

	virtual bool reconnect();

	//
	// Db table verification and creation
	//
	virtual bool tableExists( TableSchema * schema );
	/// Verify that the table and needed columns exist in the database
	virtual bool verifyTable( TableSchema * schema, bool createMissingColumns = false, QString * output=0 );
	/// Trys to create the table in the database
	virtual bool createTable( TableSchema * schema, QString * output = 0 );

	virtual TableSchema * importTableSchema();

	virtual Schema * importDatabaseSchema();

	virtual uint newPrimaryKey( TableSchema * );

	virtual RecordList selectFrom( Table * table, const QString & from, const QList<QVariant> & args = QList<QVariant>() );

	/// Same as above, but won't select from offspring
	virtual RecordList selectOnly( Table *, const QString & where = QString::null, const QList<QVariant> & vars = QList<QVariant>() );
	virtual QList<RecordList> joinedSelect( const JoinedSelect &, QString where, QList<QVariant> vars );

	virtual QMap<Table *, RecordList> selectMulti( QList<Table*>, const QString & innerWhere = QString::null, const QList<QVariant> & innerArgs = QList<QVariant>(), const QString & outerWhere = QString::null, const QList<QVariant> & outerArgs = QList<QVariant>() );

	virtual void selectFields( Table * table, RecordList, FieldList );

	virtual void insert( Table *, const RecordList & rl );

	/**
	 * Generates and executes a sql update
	 * to the database.
	 **/
	virtual bool update( Table *, RecordImp * imp, Record * returnValues = 0 );
	virtual bool update( Table * table, RecordList records, RecordList * returnValues = 0 );

	virtual int remove( Table *, const QString &, QList<int> * rowsDeleted = 0 );

	virtual bool createIndex( IndexSchema * schema );

	bool checkVersion( int major, int minor ) const;

	TableSchema * tableByOid( uint oid, Schema * );
	uint oidByTable( TableSchema * );

	bool verifyChangeTrigger( TableSchema * table, bool create = false );

signals:
	void notification( const QString & name );

protected:
	QString getSqlFields(TableSchema*, const QString & _tableAlias = QString(), bool needTableOid = false, FieldList * usedFields = 0, int * pkeyPos=0);

	void loadTableOids();
	bool verifyTriggerExists( TableSchema * table, const QString & triggerName );
	
	QHash<TableSchema*,QString> mSqlFields;
	int mVersionMajor, mVersionMinor;
	bool mUseMultiTableSelect;
	QMap<uint, QString> mTablesByOid;
	QMap<QString, uint> mOidsByTable;
};

#endif // PG_CONNECTION_H

