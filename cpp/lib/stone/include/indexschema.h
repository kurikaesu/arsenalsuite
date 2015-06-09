
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

#ifndef INDEX_SCHEMA_H
#define INDEX_SCHEMA_H

#include <qlist.h>
#include <qstring.h>

#include "field.h"

namespace Stone {
class TableSchema;
class Field;

class STONE_EXPORT IndexSchema
{
public:
	// Constructs an index that indexes records from
	// table 'parent'.  Each index will either hold
	// a single record or a list of records depending
	// on isList
	IndexSchema( const QString & name, TableSchema * parent, bool isList=false, bool useCache=false );
	~IndexSchema();

	QString name() const { return mName; }
	void setName( const QString & name ) { mName = name; }

	// Returns the table that this index corrosponds to
	TableSchema * table() const { return mTable; }
	bool holdsList() const { return mIsList; }
	void setHoldsList( bool holdsList ) { mIsList = holdsList; }

	Field * field() const { return mField; }
	void setField( Field * f ) { mField = f; }
	
	QList<Field*> columns() const { return mIndexColumns; }
	
	// Adds a column to use for the index
	// Columns must be added in order of use
	void addColumn( const QString & name );
	void addField( Field * );
	
	bool databaseIndex() const { return mIsDatabaseIndex; }
	void setDatabaseIndex( bool di ) { mIsDatabaseIndex = di; }

	QString databaseWhere() const { return mDatabaseWhere; }
	void setDatabaseWhere( const QString & dw ) { mDatabaseWhere = dw; }
	
//	bool createDatabaseIndex();
	void setUseCache( bool uc ) { mUseCache = uc; }
	bool useCache() const { return mUseCache; }

protected:
	QString mName;
	TableSchema * mTable;
	Field * mField;
	bool mIsList;
	bool mUseCache;
	QList<Field*> mIndexColumns;
	bool mIsDatabaseIndex;
	QString mDatabaseWhere;
};

typedef QList<IndexSchema*> IndexSchemaList;
typedef QList<IndexSchema*>::iterator IndexSchemaIter;

} //namespace

#endif // INDEX_SCHEMA_H

