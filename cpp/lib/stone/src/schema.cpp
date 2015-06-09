
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

#include <qdatetime.h>
#include <qdom.h>
#include <qfile.h>

#include "path.h"
#include "schema.h"

namespace Stone {

Schema::~Schema()
{
	while( !mTablesByName.isEmpty() )
		delete *mTablesByName.begin();
}

TableSchemaList Schema::tables() const
{
	return mTablesByName.values();
}

TableSchema * Schema::tableByName( const QString & tableName ) const
{
	QMap<QString, TableSchema*>::const_iterator it = mTablesByName.find( tableName.toLower() );
	if( it != mTablesByName.end() )
		return it.value();
	return 0;
}

TableSchema * Schema::tableByClass( const QString & className ) const
{
	QMap<QString, TableSchema*>::const_iterator it = mTablesByClass.find( className );
	if( it != mTablesByClass.end() )
		return it.value();
	return 0;
}

void Schema::addTable( TableSchema * table )
{
	mTablesByName[table->tableName().toLower()] = table;
	mTablesByClass[table->className()] = table;
}

void Schema::removeTable( TableSchema * table, bool del )
{
	mTablesByName.remove( table->tableName().toLower() );
	mTablesByClass.remove( table->tableName() );
	if( del )
		delete table;
}

void Schema::tableRenamed( TableSchema * table, const QString & oldName )
{
	mTablesByName.remove( oldName.toLower() );
	addTable( table );
}

void Schema::tableClassRenamed( TableSchema * table, const QString & oldName )
{
	mTablesByClass.remove( oldName );
	addTable( table );
}

//
// Loads the database table definitions from
// the 'schema' xml file
//
void parseTable( Schema * schema, QDomElement table, QMap<QString, QList<QDomElement> > * toParse, bool ignoreDocs, QList<TableSchema*> * parsed, bool verbose=false )
{
	//bool newTable = false;
	QString tableName = table.attribute( "name" );

	TableSchema * ret = schema->tableByName( tableName );
	if( !ret ) {
		//newTable = true;
		ret = new TableSchema( schema );
		ret->setTableName( tableName );
	}

	if( parsed ) *parsed << ret;
	//LOG_5( "Database::parseTable: Created table " + ret->tableName() );

	ret->setClassName( table.attribute( "className" ) );
	ret->setPreloadEnabled( table.attribute( "preload" ) == "true" );
	ret->setProjectPreloadColumn( table.attribute( "projectPreload" ) );
	ret->setUseCodeGen( table.attribute( "useCodeGen" ) == "true" );

	if( !ignoreDocs )
		ret->setDocs( table.attribute( "docs" ) );

	// On by default
	ret->setExpireKeyCache( table.attribute( "expireKeyCache" ) != "false" );
	
	QString parent = table.attribute( "parent" );
	TableSchema * parentSchema = schema->tableByClass( parent );
	if( parentSchema && parentSchema != ret->parent() )
		ret->setParent( parentSchema );

//	if( newTable )
//		LOG_5( "Adding new table: " + tableName + " className: " + ret->className() + (parentSchema ? (" parent: " + parentSchema->tableName()) : QString()) );
	
	// Iterator through the table's nodes
	QDomNode n = table.firstChild();
	while( !n.isNull() ) {
	
		 // try to convert the node to an element.
		QDomElement el = n.toElement();
		if( !el.isNull() ) {
			
			// Parse a column
			if( el.tagName() == "column" ){
				Field::Type ct = Field::stringToType( el.attribute( "type" ) );
				if( ct == Field::Invalid ) {
					LOG_1( "Field has invalid type: " + el.attribute("type") );
					continue;
				}
				QString name = el.attribute( "name" );
				if( !name.isEmpty() ) {
					QDomElement fkey = el.firstChild().toElement();
					Field * f = ret->field(name,/*silent=*/true);
					bool modify = bool(f);
					if( !modify ) {
						//if( !newTable )
						//	LOG_5( "Adding new column: " + tableName + "." + name + "[" + el.attribute( "type" ) + "]" );
						if( el.attribute( "pkey" ) == "true" )
							f = new Field( ret, name, Field::UInt, Field::PrimaryKey );
						else if( !fkey.isNull() && fkey.tagName() == "fkey" ) {
							f = new Field( ret, name, fkey.attribute( "table" ), Field::Flags((fkey.attribute( "type" ) == "multi" ? Field::None : Field::Unique)  | Field::ForeignKey),
								fkey.attribute( "hasIndex" ) == "true", Field::indexDeleteModeFromString( fkey.attribute( "indexDeleteMode" ) ) );
							QString in = fkey.attribute( "indexName" );
							if( !in.isEmpty() && f->index() ) {
								if( !f->index() )
									f->setHasIndex( true, Field::indexDeleteModeFromString( fkey.attribute( "indexDeleteMode" ) ) );
								f->index()->setName( in );
								f->index()->setUseCache( fkey.attribute( "indexUseCache" ) != "false" );
							}
						} else {
							if( ct >= 0 )
								f = new Field( ret, name, ct );
							else
								LOG_1( "Couldn't find the correct type for field: " + name );
						}
					} else {
						if( fkey.attribute( "hasIndex" ) == "true" ) {
							QString in = fkey.attribute( "indexName" );
							if( !f->index() )
								f->setHasIndex( true, Field::indexDeleteModeFromString( fkey.attribute( "indexDeleteMode" ) ) );
							f->index()->setName( in );
							f->index()->setUseCache( fkey.attribute( "indexUseCache" ) != "false" );
						}
					}
					if( f ) {
						QString methodName = el.attribute("methodName");
						if( !methodName.isEmpty() ) {
							if( verbose && modify && !f->methodName().isEmpty() && f->methodName() != methodName )
								LOG_5( QString("Changing %1.%2 methodName from %3 to %4").arg(ret->tableName()).arg(f->name()).arg(f->methodName()).arg(methodName) );
							f->setMethodName( methodName );
						}
						QString pluralMethodName = el.attribute("pluralMethodName");
						if( !pluralMethodName.isEmpty() ) {
							if( verbose && modify && !f->pluralMethodName().isEmpty() && f->pluralMethodName() != pluralMethodName )
								LOG_5( QString("Changing %1.%2 pluralMethodName from %3 to %4").arg(ret->tableName()).arg(f->name()).arg(f->pluralMethodName()).arg(pluralMethodName) );
							f->setPluralMethodName( pluralMethodName );
						}
						QString displayName = el.attribute( "displayName" );
						if( !displayName.isEmpty() && f->displayName() != displayName ) {
							if( verbose && modify && !f->displayName().isEmpty()  )
								LOG_5( QString("Changing %1.%2 display name from %3 to %4").arg(ret->tableName()).arg(f->name()).arg(f->displayName()).arg(displayName) );
							f->setDisplayName( displayName );
						}
						if( f->flag(Field::ForeignKey) ) {
							bool ra = el.attribute("reverseAccess") == "true";
							if( verbose && modify && f->flag(Field::ReverseAccess) != ra )
								LOG_5( QString("Changing %1.%2 reverse access from %3 to %4").arg(ret->tableName()).arg(f->name()).arg(f->flag(Field::ReverseAccess)).arg(ra) );
							f->setFlag( Field::ReverseAccess, ra );
						}
						if( !ignoreDocs ) {
							QString docs = el.attribute("docs");
							if( verbose && modify && !docs.isEmpty() && f->docs() != docs )
								LOG_5( QString("Changing %1.%2 docs from %3 to %4").arg(ret->tableName()).arg(f->name()).arg(f->docs()).arg(docs) );
							f->setDocs( docs );
						}
						QString defValS = el.attribute("defaultValue");
						if( !defValS.isEmpty() ) {
							QVariant defVal = Field::variantFromString( defValS, f->type() );
							if( verbose && modify && defVal != f->defaultValue() )
								LOG_5( QString("Changing %1.%2 default value from %3 to %4").arg(ret->tableName()).arg(f->name()).arg(f->defaultValue().toString()).arg(defVal.toString()) );
							f->setDefaultValue( defVal );
						}
						f->setFlag( Field::TableDisplayName, el.attribute("tableDisplayName") == "true" );
						f->setFlag( Field::NoDefaultSelect, el.attribute("noDefaultSelect") == "true" );
					}
				}
			}
			else if( el.tagName() == "variable" ){
				Field::Type ct = Field::stringToType( el.attribute( "type" ) );
				QString name = el.attribute( "name" );
				if( !name.isEmpty() && !ret->field( name ) )
					new Field( ret, name, ct, Field::LocalVariable );
			}
			// Parse an index
			else if( el.tagName() == "index" ){
				QString name = el.attribute( "name" );
				bool multi = el.attribute( "type" ) == "multi";
				IndexSchema * idx = ret->index(name);
				FieldList newFields;
				bool existingIndex = bool(idx);
				if( !idx )
					idx = new IndexSchema( name, ret, multi );
				// Iterate through the index's nodes
				QDomNode index_c = el.firstChild();
				while( !index_c.isNull() ) {
					// try to convert the node to an element.
					QDomElement column = index_c.toElement();
					if( !column.isNull() && column.tagName() == "column" ) {
						const QString & colName( column.attribute( "name" ) );
						Field * f = ret->field( colName );
						if( f )
							newFields.append(f);
						else
							LOG_1( "ERROR: Column " + colName + " not found in table " + tableName + " for index " + name );
					}
					index_c = index_c.nextSibling();
				}
				if( existingIndex ) {
					if( newFields != idx->columns() )
						LOG_1( "ERROR: Index " + name + " in table " + tableName + " redifined with different list of fields" );
				} else {
					foreach( Field * f, newFields )
						idx->addField( f );
				}
				idx->setUseCache( el.attribute( "useCache" ) == "true" );
			}
		}
		
		n = n.nextSibling();
	}
		
	// We can now parse all of it's children
	QList<QDomElement> children = (*toParse)[ret->className()];
	for( QList<QDomElement>::Iterator it = children.begin(); it != children.end(); ++it )
		parseTable( schema, *it, toParse, ignoreDocs, parsed, verbose );
	(*toParse).remove( ret->tableName() );
}

bool Schema::mergeXmlSchema( const QString & schema, bool isFile, bool ignoreDocs, QList<TableSchema*> * tables, bool verbose )
{
	// Tables that we have deferred processing until their
	// parent tables have been processed
	QMap<QString,QList<QDomElement> > parseAfterParent;
	
	QDomDocument doc( "schema" );
	
	QString errorMessage;
	int errorLine, errorColumn;

	if( isFile ) {
		if( verbose )
			LOG_1( "Merging schema: " + schema );
		
		QFile file( schema );
		if ( !file.open( QIODevice::ReadOnly ) ) {
			LOG_1( "Database::mergeSchema: Couldn't Open File (" + schema + ")" );
			return false;
		}
		
		if ( !doc.setContent( &file, &errorMessage, &errorLine, &errorColumn ) ) {
			LOG_1( "Couldn't parse xml: line " + QString::number(errorLine) + " column " + QString::number(errorColumn) + " message " + errorMessage );
			file.close();
			return false;
		}
		
		file.close();
	} else {
		if ( !doc.setContent( schema, &errorMessage, &errorLine, &errorColumn ) ) {
			LOG_1( "Couldn't parse xml: line " + QString::number(errorLine) + " column " + QString::number(errorColumn) + " message " + errorMessage );
			return false;
		}
	}
	
	QDomElement db = doc.documentElement();
	setName( db.attribute("schemaName", "CLASSES") );
	
	QDomNode n = db.firstChild();
	while( !n.isNull() ) {
		QDomElement table = n.toElement(); // try to convert the node to an element.
		if( !table.isNull() && table.tagName() == "table" ) {
			//qWarning( "Found table " + table.attribute( "name" ) );
			QString parent = table.attribute( "parent" );
			
			// If we haven't parsed the parent yet, then add it to the map
			if( !parent.isNull() && !tableByName( parent ) )
				parseAfterParent[parent] += table;
			else
				parseTable( this, table, &parseAfterParent, ignoreDocs, tables, verbose );
		}
		n = n.nextSibling();
	}
	
	return true;
}

Schema * Schema::createFromXmlSchema( const QString & xmlSchema, bool isFile, bool ignoreDocs, bool verbose )
{
	Schema * ret = new Schema();
	if( !ret->mergeXmlSchema( xmlSchema, isFile, ignoreDocs, 0, verbose ) ) {
		delete ret;
		return 0;
	}
	return ret;
}

//
// Writes an XML schema file to dest
//
bool Schema::writeXmlSchema( const QString & outputFile, QString * xmlOut, TableSchemaList tables ) const
{
	// Create the root
	QDomDocument doc( "Schema" );
	
	QDomElement root = doc.createElement( "database" );
	root.setAttribute("schemaName", name());
	doc.appendChild( root );
	
	if( tables.isEmpty() )
		tables = mTablesByName.values();
	
	foreach( TableSchema * tbl, tables )
	{
		QDomElement table = doc.createElement( "table" );
		root.appendChild( table );
		
		table.setAttribute( "name", tbl->tableName() );
		table.setAttribute( "className", tbl->className() );
		
		if( tbl->useCodeGen() )
			table.setAttribute( "useCodeGen", "true" );
			
		if( tbl->isPreloadEnabled() )
			table.setAttribute( "preload", "true" );
		
		if( !tbl->projectPreloadColumn().isEmpty() )
			table.setAttribute( "projectPreload", tbl->projectPreloadColumn() );
			
		if( tbl->parent() )
			table.setAttribute( "parent", tbl->parent()->className() );
		
		if( !tbl->docs().isEmpty() )
			table.setAttribute( "docs", tbl->docs() );

		table.setAttribute( "expireKeyCache", tbl->expireKeyCache() ? "true" : "false" );

		IndexSchemaList fieldIndexes;
		FieldList columns = tbl->ownedColumns();
		for( FieldIter col_it = columns.begin(); col_it != columns.end(); ++col_it )
		{
			Field * f = *col_it;
			QDomElement column = doc.createElement( "column" );
			table.appendChild( column );
			
			column.setAttribute( "name", f->name() );
			if( f->displayName() != f->generatedDisplayName() )
				column.setAttribute( "displayName", f->displayName() );
			column.setAttribute( "type", f->typeString() );
			column.setAttribute( "methodName", f->methodName() );
			if( f->pluralMethodName() != f->generatedPluralMethodName() )
				column.setAttribute( "pluralMethodName", f->pluralMethodName() );
			if( !f->docs().isEmpty() )
				column.setAttribute( "docs", f->docs() );

			if( !f->defaultValue().isNull() ) {
				QString valString;
				if( f->type() == Field::Date )
					valString = f->defaultValue().toDate().toString( Qt::ISODate );
				else if( f->type() == Field::DateTime )
					valString = f->defaultValue().toDateTime().toString( Qt::ISODate );
				else
					valString = f->defaultValue().toString();
				column.setAttribute( "defaultValue", valString );
			}

			if( f->flag( Field::PrimaryKey ) )
				column.setAttribute( "pkey", "true" );
			if( f->flag( Field::ReverseAccess ) )
				column.setAttribute( "reverseAccess", "true" );
			if( f->flag( Field::TableDisplayName ) )
				column.setAttribute( "tableDisplayName", "true" );
			if( f->flag( Field::NoDefaultSelect ) )
				column.setAttribute( "noDefaultSelect", "true" );
			if( f->flag( Field::ForeignKey ) ) {
				QDomElement fkey = doc.createElement( "fkey" );
				column.appendChild( fkey );
				fkey.setAttribute( "table", f->foreignKey() );
				fkey.setAttribute( "type", f->flag( Field::Unique ) ? "single" : "multi" );
				fkey.setAttribute( "hasIndex", f->hasIndex() ? "true" : "false" );
				fkey.setAttribute( "indexDeleteMode", f->indexDeleteModeString() );
				if( f->hasIndex() ) {
					fkey.setAttribute( "indexName", f->index()->name() );
					fkey.setAttribute( "indexUseCache", f->index()->useCache() ? "true" : "false" );
					fieldIndexes += f->index();
				}
			}
		}
		
		FieldList vars = tbl->ownedLocalVariables();
		for( FieldIter col_it = vars.begin(); col_it != vars.end(); ++col_it )
		{
			Field * f = *col_it;
			QDomElement var = doc.createElement( "variable" );
			table.appendChild( var );
			
			var.setAttribute( "name", f->name() );
			var.setAttribute( "type", f->typeString() );
		}
		
		IndexSchemaList idxs = tbl->indexes();
		foreach( IndexSchema * idx, idxs )
		{
			FieldList fl = idx->columns();
			
			// Skip the empty keyindex
			if( fl.isEmpty() || (idx->field() && idx->field()->flag( Field::PrimaryKey )) )
				continue;
				
			// Skip the indexes that are owned by an fkey field
			if( fieldIndexes.contains( idx ) )
				continue;
				
			QDomElement index = doc.createElement( "index" );
			table.appendChild( index );
			
			index.setAttribute( "type", idx->holdsList() ? "multi" : "single" );
			index.setAttribute( "name", idx->name() );
			index.setAttribute( "useCache", idx->useCache() ? "true" : "false" );
			
			for( FieldIter ci_it = fl.begin(); ci_it != fl.end(); ++ci_it ) {
				QDomElement col_info = doc.createElement( "column" );
				index.appendChild( col_info );
				
				col_info.setAttribute( "name", (*ci_it)->name() );
			}
		}
	}
	
	QString xml = doc.toString();
	
	if( !outputFile.isEmpty() ) {
		if( !writeFullFile( outputFile, xml ) ) {
			LOG_1( "Unable to save xml schema to " + outputFile );
			return false;
		}
	}
	
	if( xmlOut )
		*xmlOut = xml;
		
	return true;
}

QString Schema::name() const
{
	return mName;
}

void Schema::setName(const QString & name)
{
	mName = name;	
}

QString Schema::diff( Schema * before, Schema * after )
{
	QString ret;
	foreach( TableSchema * before_table, before->tables() ) {
		TableSchema * after_table = after->tableByName( before_table->tableName() );
		if( !after_table ) {
			ret += "Removed Table " + before_table->className() + "\n";
			continue;
		}
		ret += before_table->diff( after_table );
	}
	foreach( TableSchema * after_table, after->tables() ) {
		TableSchema * before_table = before->tableByName( after_table->tableName() );
		if( !before_table ) {
			ret += "Added Table " + after_table->className() + "\n";
			continue;
		}
	}
	return ret;
}

} // namespace