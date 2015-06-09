
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

#ifndef SCHEMA_H
#define SCHEMA_H

#include <qstring.h>
#include <qmap.h>

#include "tableschema.h"

namespace Stone {

class IndexSchema;

class STONE_EXPORT Schema
{
public:
	virtual ~Schema();

	/** Case insensitive
	 */
	TableSchema * tableByName( const QString & tableName ) const;
	/** Case insensitive
	 */
	TableSchema * tableByClass( const QString & className ) const;
	
	/** Returns all the tables associated with this database
	 */
	TableSchemaList tables() const;
	
	/** Adds a table to this database.  The database takes
	 * ownership of the object.
	 */
	void addTable( TableSchema * table );

	/** Removes a table from this database.  The caller is
	 * responsible for deleting table.
	 */
	void removeTable( TableSchema * table, bool del = true );

	void tableRenamed( TableSchema * table, const QString & oldName );
	void tableClassRenamed( TableSchema * table, const QString & oldName );

	/**
	*  Merges this schema with the current schema
	*/
	bool mergeXmlSchema( const QString & schemaFile, bool isFile = true, bool ignoreDocs = true, QList<TableSchema*> * tables = 0, bool verbose = false );
	
	/**
	*  Loads the database table definitions from
	*  the 'schema' xml file, or string if isFile is false
	*/
	static Schema * createFromXmlSchema( const QString & schema, bool isFile = true, bool ignoreDocs = true, bool verbose = false );
	
	
	/** Writes an XML schema file to dest
	*/
	bool writeXmlSchema( const QString & dest, QString * xml = 0, TableSchemaList tables = TableSchemaList() ) const;
	QString name() const;
	void setName(const QString &);

	
	static QString diff( Schema * before, Schema * after );
	
protected:
	QMap<QString,TableSchema*> mTablesByName, mTablesByClass;
	QString mName;

};

} //namespace

#endif // SCHEMA_H

