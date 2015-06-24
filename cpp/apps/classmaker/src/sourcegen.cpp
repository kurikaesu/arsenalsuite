/*
 *
 * Copyright 2003, 2004 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Assburner is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Assburner is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <qfile.h>
#include <qtextstream.h>
#include <qdir.h>

#include "blurqt.h"
#include "schema.h"
#include "tableschema.h"
#include "indexschema.h"
#include "field.h"
#include "sourcegen.h"
#include "path.h"

QString ucf( const QString & orig )
{
	return orig.mid( 0, 1 ).toUpper() + orig.mid( 1 );
}

QString lcf( const QString & orig )
{
	return orig.mid( 0, 1 ).toLower() + orig.mid( 1 );
}

bool inheritsElement( TableSchema * table )
{
	while( table ) {
		if( table->tableName() == "Element" )
			return true;
		table = table->parent();
	}
	return false;
}

QString findFile( const QString & fileName )
{
	if( QFile::exists( fileName ) )
		return fileName;
	QString tmp = "/usr/share/classmaker/" + fileName;
	if( QFile::exists( tmp ) )
		return tmp;
	tmp = "/usr/lib/stone/" + fileName;
	if( QFile::exists( tmp ) )
		return tmp;
	return QString();
}

QString readFile( const QString & fileName )
{
	return readFullFile( findFile( fileName ) );
}

void write( QString & input,  QString output )
{
	if( readFile( output ) != input ) {
		if( !writeFullFile( output, input ) )
			LOG_1( "Couldn't open " + output + " for writing" );
	}
}

QString commentifyDocString( const QString & docString, int indentLevel = 0 )
{
	QString indentString;
	for( int i=0; i<indentLevel; i++ )
		indentString += "\t";

	QString outString(indentString +"/**\n");
	// Unix \n, windows \n\r, mac \r
	QStringList lines = docString.split( QRegExp( "[\n\r]\r?" ) );
	QString commentStart("/*"), commentEnd("*/");
	foreach( QString line, lines ) {
		line = line.replace( commentStart, "" );
		line = line.replace( commentEnd, "" );
		outString += indentString + "  * " + line + "\n";
	}
	outString += indentString + "  **/\n";
	return outString;
}

QString retify( const QString & type )
{
	return QString("RET(%1)").arg(type);
}

QString fieldArgName( const QString & fieldName )
{
	QString ret(fieldName);
	ret[0] = ret[0].toLower();
	return ret;
}

static void writeClass( TableSchema * table, const QString & path )
{
	Path autoPath( path + "/autocore/" );
	Path sipPath( path + "/sip/" );
	
	if( !autoPath.dirExists() && !autoPath.mkdir(false) ) {
		LOG_5( "Couldn't create autocore folder at " + autoPath.path() );
		return;
	}

	if( !sipPath.dirExists() && !sipPath.mkdir(false) ) {
		LOG_5( "Couldn't create sip folder at " + sipPath.path() );
		return;
	}

	bool hasElementBase = inheritsElement( table );
	// base
	QString name = table->className();
	QString tblname = table->tableName();
	QString tbldef( "__" + name.toUpper() + "_TABLE__" );
	QString tblclassdef( "__" + name.toUpper() + "_CLASS__" );
	QString coldef( "__" + name.toUpper() + "_%1_FIELD__" );
	QString colmethdef( "__" + name.toUpper() + "_%1_METHOD__" );
	QString colidx( "__" + name.toUpper() + "_%1_INDEX__" );
	
	QString base = table->parent() ? table->parent()->className() : "Record";
	FieldList fields = table->ownedFields();
	
	// classDefines
	QString classDefines;
	QString classHeaders;

	QString baseHeader, baseFunctions, baseSip;
	if( QFile::exists( path + "/base/" + name.toLower() + "base.h" ) )
		baseHeader = "#include \"base/tl__base.h\"";
	
	QString sipBase( path + "/base/" + name.toLower() + "base.sip" );
	if( QFile::exists( sipBase ) )
		baseSip = readFile( sipBase );
	
	classDefines += "extern const char * " + tbldef + ";\n";
	classDefines += "extern const char * " + tblclassdef + ";\n";
	
	foreach( Field * f, fields )
	{
		if( !f->flag( Field::ForeignKey ) )
			continue;
		QString fk = f->foreignKey();
		if( fk == "ElementType" ) {
			classDefines += "#include \"elementtypelist.h\"\n";
			classDefines += "#include \"elementtype.h\"\n";
		} else {
			classDefines += "class " + fk + "List;\n";
			classDefines += "class " + fk + ";\n";
		}
		classHeaders += "#include \"" + fk.toLower() + ".h\"\n";
		classDefines += "\n\n";
	}
	
	for( FieldIter it = fields.begin(); it != fields.end(); ++it )
	{
		classDefines += "extern const char * " + coldef.arg( (*it)->name().toUpper() ) + ";\n";
		classDefines += "extern const char * " + colmethdef.arg( (*it)->name().toUpper() ) + ";\n";
		classDefines += "#define " + colidx.arg( (*it)->name().toUpper() ) + " " + QString::number( (*it)->pos() - table->firstColumnIndex() ) + "\n";
	}
		
	QString elementHeaders;
	if( inheritsElement( table ) )
		elementHeaders = "#include \"user.h\"\n#include \"elementtype.h\"\n";
	
	QString indexHeaders;
	IndexSchemaList indexes = table->indexes();
	foreach( IndexSchema * index, indexes )
	{
		foreach( Field * f, index->columns() )
			if( f->flag( Field::ForeignKey ) )
				indexHeaders += "#include \"" + f->foreignKey().toLower() + ".h\"\n";
	}
	
	
	// methodDefs
	QStringList memberCtorList, schemaFieldDefList;
	QString methodDefs, sipMethodDefs, methods, listMethodDefs, sipListMethodDefs, listMethods, memberVars, memberCtors, setCode, getCode;
	QString getColumn, setColumn, addFields, tableData;
	QString tableMembers, schemaFieldDecls;

	foreach( Field * f, table->fields() ) {
		QString camelMeth = f->methodName();
		camelMeth[0] = camelMeth[0].toUpper();
		if( f->flag( Field::PrimaryKey ) )
			camelMeth = "Key";
		QString fullCamelMeth = "t__::c." + camelMeth;
		schemaFieldDefList += "0";
		schemaFieldDecls += "\t\tStaticFieldExpression " + camelMeth + ";\n";
		// Field comes from parent, we aren't creating the field below so we have
		// to get it from the parent
		if( f->table() != table )
			addFields += "\t" + fullCamelMeth + " = b__::c." + camelMeth + ";\n";
	}
	
	tableData += "const char * " + tbldef + " = \"" + tblname + "\";\n";
	tableData += "const char * " + tblclassdef + " = \"" + name + "\";\n";

	foreach( Field * f, fields )
	{
		QString fn = f->name();
		QString fndef = coldef.arg( fn.toUpper() );
		QString meth = f->methodName();
		QString pmeth = f->pluralMethodName();
		QString disp = f->displayName();
		QString methdef = colmethdef.arg( fn.toUpper() );
		QString camelMeth = meth;
		camelMeth[0] = camelMeth[0].toUpper();
		if( f->flag( Field::PrimaryKey ) )
			camelMeth = "Key";
		camelMeth = "t__::c." + camelMeth;
		QString idx = camelMeth;
		
		tableData += "const char * " + fndef + " = \"" + fn + "\";\n";
		tableData += "const char * " + methdef + " = ";
		if( fn == meth )
			tableData += fndef + ";\n";
		else
			tableData += "\"" + meth + "\";\n";
		
		if( f->flag( Field::PrimaryKey ) ){
			addFields += "\t" + camelMeth + " = new Field( this, " + fndef + ", Field::UInt, Field::Flags(Field::PrimaryKey | Field::Unique | Field::NotNull), \"key\" );\n";
			continue;
		}

		if( !f->docs().isEmpty() )
			methodDefs += commentifyDocString( f->docs(), 1 );
		addFields += "\t" + camelMeth + " = ";
		if( f->flag( Field::ForeignKey ) ) {
			QString fkt = f->foreignKey();
			methodDefs += "\t" + retify(fkt) + " " + lcf( meth ) + "(int lookupMode = Index::UseSelect|Index::PartialSelect|Index::UseCache) const;\n";
			methodDefs += "\tRET(t__) & set" + ucf( meth ) + "( const " + fkt + " & );\n";
			sipMethodDefs += "\t" + retify(fkt) + " " + lcf( meth ) + "(int lookupMode = Index::UseSelect|Index::PartialSelect|Index::UseCache) const  throw(SqlException,LostConnectionException,PythonException) /HoldGIL/;\n";
			sipMethodDefs += "\tRET(t__) & set" + ucf( meth ) + "( const " + fkt + " & );\n";
			methods += fkt + " t__::" + lcf( meth ) + "(int lookupMode) const\n{\n";
			methods += "\treturn foreignKey( " + idx + ", lookupMode );\n";
			methods += "}\n\n";
			methods += "t__ & t__::set" + ucf( meth ) + "( const " + fkt + " & val )\n{\n";
			methods += "\tRecord::setForeignKey( " + idx + ", val );\n\treturn *this;\n";
			methods += "}\n\n";
			listMethodDefs += "\t" + retify(fkt + "List") + " " + lcf( pmeth ) + "(int lookupMode = Index::UseSelect|Index::PartialSelect|Index::UseCache) const;\n";
			listMethodDefs += "\tRET(t__List) & set" + ucf( pmeth ) + "( const " + fkt + " & );\n";
			sipListMethodDefs += "\t" + retify(fkt + "List") + " " + lcf( pmeth ) + "(int lookupMode = Index::UseSelect|Index::PartialSelect|Index::UseCache) const  throw(SqlException,LostConnectionException,PythonException) /HoldGIL/;\n";
			sipListMethodDefs += "\tRET(t__List) & set" + ucf( pmeth ) + "( const " + fkt + " & );\n";
			listMethods += fkt + "List t__List::" + lcf( pmeth ) + "(int lookupMode) const\n{\n";
			listMethods += "\treturn RecordList::foreignKey( " + idx + ", lookupMode );\n}\n\n";
			listMethods += "t__List & t__List::set" + ucf( pmeth ) + "( const " + fkt + " & val )\n{\n";
			listMethods += "\tRecordList::setForeignKey( " + idx + ", val );\n";
			listMethods += "\treturn *this;\n";
			listMethods += "}\n\n";
			// Field Name, Foreign Key Table Name, Flags, hasIndex, indexDeleteMode
			addFields += QString( "new Field( this, %1, %2, %3, %4, %5 );\n" )
						.arg( fndef ) 
						.arg( "__" + fkt.toUpper() + "_CLASS__" )
						.arg( f->flagString() )
						.arg( f->hasIndex() ? "true": "false" )
						.arg( "Field::" + f->indexDeleteModeString() );
			if( f->hasIndex() ) {
				addFields += "\t\tif( " + camelMeth + "->index() ) m" + f->index()->name() + " = " + camelMeth + "->index();\n\t";
				addFields += "\t\t" + camelMeth + "->index()->setUseCache( " + QString( f->index()->useCache() ? "true" : "false" ) + " );";
			}
		} else {
			methodDefs += "\t" + f->typeString() + " " + lcf( meth ) + "() const;\n";
			methodDefs += "\tRET(t__) & set" + ucf( meth ) + "( const " + f->typeString() + " & );\n";
			sipMethodDefs += "\t" + f->typeString() + " " + lcf( meth ) + "() const throw(SqlException,LostConnectionException,PythonException) /HoldGIL/;\n";
			sipMethodDefs += "\tRET(t__) & set" + ucf( meth ) + "( const " + f->typeString() + " & );\n";
			methods += f->typeString() + " t__::" + lcf( meth ) + "() const\n{\n";
			methods += "\treturn " + f->typeFromVariantCode().arg( "Record::getValue( " + idx + " )") + ";\n";
			methods += "}\n\n";
			methods += "t__ & t__::set" + ucf( meth ) + "( const " + f->typeString() + " & val )\n{\n";
			methods += "\tRecord::setValue( " + idx + ", " + f->variantFromTypeCode().arg("val") + " );\n\treturn *this;\n";
			methods += "}\n\n";
			listMethodDefs += "\t" + f->listTypeString() + " " + lcf( pmeth ) + "() const;\n";
			listMethodDefs += "\tRET(t__List) & set" + ucf( pmeth ) + "( const " + f->typeString() + " & );\n";
			sipListMethodDefs += "\t" + f->listTypeString() + " " + lcf( pmeth ) + "() const throw(SqlException,LostConnectionException,PythonException) /HoldGIL/;\n";
			sipListMethodDefs += "\tRET(t__List) & set" + ucf( pmeth ) + "( const " + f->typeString() + " & );\n";
			listMethods += f->listTypeString() + " t__List::" + lcf( pmeth ) + "() const\n{\n";
			listMethods += "\t" + f->listTypeString() + " ret;\n";
			listMethods += "\tif( d )\n";
			listMethods += "\t\tfor( QList<RecordImp*>::Iterator it = d->mList.begin(); it != d->mList.end(); ++it )\n";
			listMethods += "\t\t\tret += t__(*it)." + lcf( meth ) + "();\n\treturn ret;\n}\n\n";
			listMethods += "t__List & t__List::set" + ucf( pmeth ) + "( const " + f->typeString() + " & val )\n{\n";
			listMethods += "\tRecordList::setValue( " + idx + ", " + f->variantFromTypeCode().arg("val") + " );\n";
			listMethods += "\treturn *this;\n";
			listMethods += "}\n\n";
			// TODO: the "key" string should be shared across the entire module
			addFields += QString( "new Field( this, %1, %2, %3, %4 );\n" )
						.arg( fndef )
						.arg( "Field::" + f->variantTypeString() )
						.arg( f->flagString() )
						.arg( f->flag( Field::PrimaryKey ) ? QString("\"key\"") : methdef );
		}
		if( f->generatedDisplayName() != disp )
			addFields += "\t\t" + camelMeth + "->setDisplayName( \"" + disp + "\" );\n";
	}
	
	if( !memberCtorList.isEmpty() )
		memberCtors = ": " + memberCtorList.join( "\n ," );

	// indexDefs
	QString indexDefs, sipIndexDefs, indexMethods, addIdxCols, indexCtors, indexFunctions;
	foreach( IndexSchema * index, indexes )
	{
		QString indexDef, sipIndexDef;
		FieldList fl = index->columns();
		if( fl.size() == 0 )
			continue;

		bool primaryKeyIndex = ( fl.size() == 1 && fl[0]->flag(Field::PrimaryKey) );

		if( !primaryKeyIndex ) {
			if( index->holdsList() )
				indexDef += "\tstatic RET(t__List) recordsBy" + index->name() + "( ";
			else
				indexDef += "\tstatic RET(t__) recordBy" + index->name() + "( ";

			tableMembers += QString(index->holdsList() ? "\tRET(t__List) recordsBy" : "\tRET(t__) recordBy");
			tableMembers += index->name() + "( ";
		}

		indexCtors += ", m" + index->name() + "( ";
		if( index->field() )
			indexCtors += " 0 )\n";
		else
			indexCtors += " new IndexSchema( \"" + index->name() + "\", this, " +
		 	QString(index->holdsList() ? "true" : "false") + "," + QString(index->useCache() ? "true" : "false") + " ) )\n";
		
		QStringList args, argsWOTypes, tml, nullChecks;
		if( !primaryKeyIndex ) {
			for( FieldIter it = fl.begin(); it != fl.end(); ++it ){
				Field * f = *it;
				QString fn = fieldArgName(f->name());
				if( f->flag( Field::ForeignKey ) ) {
					args += "const " + f->foreignKey() + " &" + fn;
					argsWOTypes += fn + ".key()";
					nullChecks += fn + " == 0";
				} else {
					args += "const " + f->typeString() + " &" + fn;
					argsWOTypes += fn;
				}
				if( !index->field() )
					addIdxCols += "\tm" + index->name() + "->addColumn( " + "__" + f->table()->className().toUpper() + "_" + fn.toUpper() + "_FIELD__ );\n";
				tml += "const " + f->typeString() + " & " + fn;
			}
 			tableMembers += tml.join(", ") + ", int lookupMode = Index::UseSelect|Index::PartialSelect|Index::UseCache );\n";
			indexDef += args.join(", ") + ", int lookupMode = Index::UseSelect|Index::PartialSelect|Index::UseCache )";
			sipIndexDef = indexDef + " throw(SqlException,LostConnectionException,PythonException)";
			indexDefs += indexDef + ";\n";
			sipIndexDefs += sipIndexDef + ";\n";
		}

		tableMembers += "\tIndexSchema * m" + index->name() + ";\n";
		
		if( !primaryKeyIndex ) {
			if( index->holdsList() ){
				indexMethods += "t__List t__::recordsBy" + index->name() + "( " + args.join(", ") + ", int lookupMode )\n{\n";
				indexMethods += "\treturn t__Schema::instance()->recordsBy" + index->name() + "( " + argsWOTypes.join(", ") + ", lookupMode );\n";
				
				indexFunctions += "t__List t__Schema::recordsBy" + index->name() + "( " + tml.join(", ") + ", int lookupMode  )\n{\n";
			} else {
				indexMethods += "t__ t__::recordBy" + index->name() + "( " + args.join(", ") + ", int lookupMode )\n{\n";
				indexMethods += "\treturn t__Schema::instance()->recordBy" + index->name() + "( " + argsWOTypes.join(", ") + ", lookupMode );\n";
				
				indexFunctions += "t__ t__Schema::recordBy" + index->name() + "( " + tml.join(", ") + ", int lookupMode  )\n{\n";
			}
			indexMethods += "}\n";
		
			if( nullChecks.size() ) {
				indexFunctions += "\tif( (" + nullChecks.join( ") || (" ) + ") )\n";
				if( index->holdsList() )
					indexFunctions += "\t\treturn t__List();\n";
				else
					indexFunctions += "\t\treturn t__();\n";
			}

			indexFunctions += "\tt__List ret;\n";
		
			bool checkPP = false;
			indexFunctions += "\tQList<QVariant> args;\n";
		
			foreach( Field * f, fl ) {
				if( f->name() == table->projectPreloadColumn() )
					checkPP = true;
				indexFunctions += "\targs += QVariant( " + fieldArgName(f->name()) + " );\n";
			}
		
			indexFunctions += "\tret = table()->indexFromSchema( m" + index->name() + " )->recordsByIndex( args, lookupMode );\n";
			if( index->holdsList() )
				indexFunctions += "\treturn ret;\n";
			else
				indexFunctions += "\treturn ret[0];\n";
		
			indexFunctions += "}\n\n";
		}
	}
	
	Schema * schema = table->schema();
	TableSchemaList tl = schema->tables();
	foreach( TableSchema * t, tl ) {
		if( t == table ) continue;
		FieldList cl = t->ownedColumns();
		foreach( Field * f, cl ) {
			if( f->foreignKeyTable() != table || !f->flag( Field::ReverseAccess ) || !f->index() ) continue;
			QString ret = t->className();
			QString method = lcf( t->className() );
			if( !f->flag( Field::Unique ) ) {
				ret += "List";
				method = pluralizeName(method);
			}
			classDefines += "class " + ret + ";\n";
			classDefines += "class " + ret + "List;\n";
			classHeaders += "#include \"" + t->className().toLower() + ".h\"\n";
			methodDefs += "\t" + retify(ret) + " " + method + "( int lookupMode=Index::UseCache|Index::PartialSelect|Index::UseSelect ) const;\n";
			sipMethodDefs += "\t" + retify(ret) + " " + method + "( int lookupMode=Index::UseCache|Index::PartialSelect|Index::UseSelect ) const throw(SqlException,LostConnectionException,PythonException);\n";
			methods += ret + " t__::" + method + "( int lookupMode ) const\n{\n";
			methods += "\treturn " + t->className() + QString(f->flag( Field::Unique ) ? "::recordBy" : "::recordsBy") + f->index()->name() + "( ";
			methods += "*this, lookupMode );\n}\n";

			QString listMethod = method;

			if( f->flag( Field::Unique ) ) {
				ret += "List";
				listMethod = pluralizeName(listMethod);
			}

			listMethodDefs += "\t" + retify(ret) + " " + listMethod + "( int lookupMode=Index::UseCache|Index::PartialSelect|Index::UseSelect );\n";
			sipListMethodDefs += "\t" + retify(ret) + " " + listMethod + "( int lookupMode=Index::UseCache|Index::PartialSelect|Index::UseSelect ) throw(SqlException,LostConnectionException,PythonException);\n";
			
			listMethods += ret + " t__List::" + listMethod + "( int lookupMode )\n{\n";
			listMethods += "\tIndex * idx = " + t->className() + "::table()->indexFromField( \"" + f->name() + "\" );\n";
			listMethods += "\treturn idx ? idx->recordsByIndexMulti( getValue( " + table->field(table->primaryKeyIndex())->table()->className() + "::c.Key ), lookupMode ) : " + t->className() + "List();\n}\n\n";
		}
	}

	QString preload;
	if( !table->projectPreloadColumn().isEmpty() )
		preload = "\tsetProjectPreloadColumn( \"" + table->projectPreloadColumn() + "\" );\n";
	if( table->isPreloadEnabled() )
		preload += "\tsetPreloadEnabled( true );\n";
	if( !table->expireKeyCache() )
		preload += "\tsetExpireKeyCache( false );\n";

	QString elementHacks, elementMethods;
	
	if( hasElementBase ) {
		elementHacks += "\tstatic ElementType type();\n";
		elementMethods += "ElementType t__::type()\n{\n\tstatic int mType=0;\n";
		elementMethods += "\tif( !mType )\n\t\tmType = ElementType::recordByName( \"t__\" ).key();\n";
		elementMethods += "\treturn ElementType(mType);\n}\n";
	}
	if( table->tableName() == "Element" )
	{
		listMethodDefs += "\tElementList children( const ElementType & et = ElementType(), bool recursive = false );\n";
		listMethodDefs += "\tElementList children( const ElementTypeList & et, bool recursive = false );\n";
		listMethods += "ElementList ElementList::children( const ElementType & et, bool recursive )\n{\n";
		listMethods += "\tElementList ret;\n\tfor( ElementIter it = begin(); it != end(); ++it )\n";
		listMethods += "\t\tret += (*it).children( et, recursive );\n\treturn ret;\n}\n";
		listMethods += "ElementList ElementList::children( const ElementTypeList & etl, bool recursive )\n{\n";
		listMethods += "\tElementList ret;\n\tfor( ElementIter it = begin(); it != end(); ++it )\n";
		listMethods += "\t\tret += (*it).children( etl, recursive );\n\treturn ret;\n}\n";
	}

	QString schemaFieldDefs = schemaFieldDefList.join(",\n\t");
	
	QString temp = readFile( "templates/autocore.h" );
	temp.replace( "<%SCHEMAFIELDDECLS%>", schemaFieldDecls );
	temp.replace( "<%METHODDEFS%>", methodDefs );
	temp.replace( "<%INDEXDEFS%>", indexDefs );
	temp.replace( "<%ELEMENTHACKS%>", elementHacks );
	temp.replace( "<%ELEMENTHEADERS%>", elementHeaders );
	temp.replace( "<%CLASSDEFS%>", classDefines );
	temp.replace( "<%CLASSDOCS%>", commentifyDocString( table->docs() + "\n\\ingroup Classes" ) );
	temp.replace( "<%CLASSHEADERS%>", classHeaders );
	temp.replace( "<%BASEHEADER%>", baseHeader );
	temp.replace( "t__", name );
	temp.replace( "tu__", name.toUpper() );
	temp.replace( "tl__", name.toLower() );
	temp.replace( "tlcf__", lcf( name ) );
	temp.replace( "b__", base );
	temp.replace( "bu__", base.toUpper() );
	temp.replace( "bl__", base.toLower() );
	temp.replace( "snu__", schema->name().toUpper() );
	temp.replace( "snl__", schema->name().toLower() );
	temp.replace( QRegExp( "RET\\(([^\\)]+)\\)" ), "\\1" );

	write( temp, path + "/autocore/" + name.toLower() + ".h" );

	temp = readFile( "templates/autocore.sip" );
	temp.replace( "<%SCHEMAFIELDDECLS%>", schemaFieldDecls );
	temp.replace( "<%METHODDEFS%>", sipMethodDefs );
	temp.replace( "<%INDEXDEFS%>", sipIndexDefs );
	temp.replace( "<%ELEMENTHACKS%>", elementHacks );
	temp.replace( "<%ELEMENTHEADERS%>", elementHeaders );
	temp.replace( "<%CLASSDEFS%>", classDefines );
	temp.replace( "<%CLASSHEADERS%>", classHeaders );
	temp.replace( "<%BASEHEADER%>", baseSip );
	temp.replace( QRegExp( "RET\\(([^\\)]+List)\\)" ), "Mapped\\1" );
	temp.replace( "t__", name );
	temp.replace( "tu__", name.toUpper() );
	temp.replace( "tl__", name.toLower() );
	temp.replace( "tlcf__", lcf( name ) );
	temp.replace( "b__", base );
	temp.replace( "bu__", base.toUpper() );
	temp.replace( "bl__", base.toLower() );
	temp.replace( "snu__", schema->name().toUpper() );
	temp.replace( "snl__", schema->name().toLower() );
	temp.replace( QRegExp( "RET\\(([^\\)]+)\\)" ), "Mapped\\1" );
	write( temp, path + "/sip/" + name.toLower() + ".sip" );
	
	temp = readFile( "templates/autocore.cpp" );
	temp.replace( "<%SCHEMAFIELDDEFS%>", schemaFieldDefs );
	temp.replace( "<%CLASSHEADERS%>", classHeaders );
	temp.replace( "<%ELEMENTHEADERS%>", elementHeaders );
	temp.replace( "<%CLASSDEFS%>", classDefines );
	temp.replace( "<%TABLEDATA%>", tableData );
	temp.replace( "<%METHODS%>", methods );
	temp.replace( "<%ELEMENTMETHODS%>", elementMethods );
	temp.replace( "<%INDEXMETHODS%>", indexMethods );
	temp.replace( "<%INDEXHEADERS%>", indexHeaders );
	temp.replace( "t__", name );
	temp.replace( "tu__", name.toUpper() );
	temp.replace( "tl__", name.toLower() );
	temp.replace( "tlcf__", lcf( name ) );
	temp.replace( "b__", base );
	temp.replace( "bu__", base.toUpper() );
	temp.replace( "bl__", base.toLower() );
	temp.replace( "snu__", schema->name().toUpper() );
	temp.replace( "snl__", schema->name().toLower() );
	write( temp, path + "/autocore/" + name.toLower() + ".cpp" );
	/*
	temp = readFile( "templates/autoimp.h" );
	temp.replace( "<%METHODDEFS%>", methodDefs );
	temp.replace( "<%INDEXDEFS%>", indexDefs );
	temp.replace( "<%ELEMENTHACKS%>", elementHacks );
	temp.replace( "<%ELEMENTHEADERS%>", elementHeaders );
	temp.replace( "<%CLASSDEFS%>", classDefines );
	temp.replace( "<%CLASSHEADERS%>", classHeaders );
	temp.replace( "<%MEMBERVARS%>", memberVars );
	temp.replace( "<%BASEHEADER%>", baseHeader );
	temp.replace( "<%BASEFUNCTIONS%>", baseFunctions );
	temp.replace( "t__", name );
	temp.replace( "tu__", name.upper() );
	temp.replace( "tl__", name.lower() );
	temp.replace( "tlcf__", lcf( name ) );
	temp.replace( "b__", base );
	temp.replace( "bu__", base.upper() );
	temp.replace( "bl__", base.lower() );
	temp.replace( "snu__", schema->name().toUpper() );
	temp.replace( "snl__", schema->name().toLower() );
	write( temp, path + "/autoimp/" + name.lower() + "imp.h" );
	
	temp = readFile( "templates/autoimp.cpp" );
	temp.replace( "<%ELEMENTHEADERS%>", elementHeaders );
	temp.replace( "<%CLASSDEFS%>", classDefines );
	temp.replace( "<%CLASSHEADERS%>", classHeaders );
	temp.replace( "<%ELEMENTTYPEHACK%>", elementTypeHacks );
	temp.replace( "<%METHODS%>", methods );
	temp.replace( "<%GETCODE%>", getCode );
	temp.replace( "<%SETCODE%>", setCode );
	temp.replace( "<%GETCOLUMNCODE%>", getColumn );
	temp.replace( "<%SETCOLUMNCODE%>", setColumn );
	temp.replace( "<%MEMBERCTORS%>", memberCtors );
	temp.replace( "<%BASEFUNCTIONS%>", baseFunctions );
	temp.replace( "t__", name );
	temp.replace( "tu__", name.upper() );
	temp.replace( "tl__", name.lower() );
	temp.replace( "tlcf__", lcf( name ) );
	temp.replace( "b__", base );
	temp.replace( "bu__", base.upper() );
	temp.replace( "bl__", base.lower() );
	temp.replace( "snu__", schema->name().toUpper() );
	temp.replace( "snl__", schema->name().toLower() );
	write( temp, path + "/autoimp/" + name.lower() + "imp.cpp" );
	*/
	temp = readFile( "templates/autotable.h" );
	temp.replace( "<%SCHEMAFIELDDECLS%>", schemaFieldDecls );
	temp.replace( "<%METHODDEFS%>", methodDefs );
	temp.replace( "<%INDEXDEFS%>", indexDefs );
	temp.replace( "<%ELEMENTHACKS%>", elementHacks );
	temp.replace( "<%ELEMENTHEADERS%>", elementHeaders );
	temp.replace( "<%CLASSDEFS%>", classDefines );
	temp.replace( "<%CLASSHEADERS%>", classHeaders );
	temp.replace( "<%TABLEMEMBERS%>", tableMembers );
	temp.replace( "<%BASEHEADER%>", baseHeader );
	temp.replace( "t__", name );
	temp.replace( "tu__", name.toUpper() );
	temp.replace( "tl__", name.toLower() );
	temp.replace( "tlcf__", lcf( name ) );
	temp.replace( "b__", base );
	temp.replace( "bu__", base.toUpper() );
	temp.replace( "bl__", base.toLower() );
	temp.replace( "snu__", schema->name().toUpper() );
	temp.replace( "snl__", schema->name().toLower() );
	temp.replace( QRegExp( "RET\\(([^\\)]+)\\)" ), "\\1" );
	write( temp, path + "/autocore/" + name.toLower() + "table.h" );
	
	temp = readFile( "templates/autotable.cpp" );
	temp.replace( "<%ELEMENTHEADERS%>", elementHeaders );
	temp.replace( "<%CLASSDEFS%>", classDefines );
	temp.replace( "<%CLASSHEADERS%>", classHeaders );
	temp.replace( "<%SCHEMAFIELDDEFS%>", schemaFieldDefs );
	temp.replace( "<%METHODS%>", methods );
	temp.replace( "<%SETPARENT%>", table->parent() ? "\tsetParent( b__Schema::instance() );\n" : "" );
	temp.replace( "<%ADDFIELDS%>", addFields );
	temp.replace( "<%SETWHERE%>", "" );
	temp.replace( "<%ADDINDEXCOLUMNS%>", addIdxCols );
	temp.replace( "<%INDEXCTORS%>", indexCtors );
	temp.replace( "<%PRELOAD%>", preload );
	temp.replace( "<%INDEXFUNCTIONS%>", indexFunctions );
	temp.replace( "<%BASEHEADER%>", baseHeader );
	temp.replace( "<%BASEFUNCTIONS%>", baseFunctions );
	temp.replace( "t__", name );
	temp.replace( "tu__", name.toUpper() );
	temp.replace( "tl__", name.toLower() );
	temp.replace( "tlcf__", lcf( name ) );
	temp.replace( "b__", base );
	temp.replace( "bu__", base.toUpper() );
	temp.replace( "bl__", base.toLower() );
	temp.replace( "snu__", schema->name().toUpper() );
	temp.replace( "snl__", schema->name().toLower() );
	temp.replace( "table__", tblname );
	temp.replace( "recordtable", "table" );
	write( temp, path + "/autocore/" + name.toLower() + "table.cpp" );
	
	temp = readFile( "templates/autolist.h" );
	temp.replace( "<%INDEXDEFS%>", indexDefs );
	temp.replace( "<%ELEMENTHACKS%>", elementHacks );
	temp.replace( "<%ELEMENTHEADERS%>", elementHeaders );
	temp.replace( "<%CLASSDEFS%>", classDefines );
	temp.replace( "<%CLASSHEADERS%>", classHeaders );
	temp.replace( "<%LISTDEFS%>", listMethodDefs );
	temp.replace( "t__", name );
	temp.replace( "tu__", name.toUpper() );
	temp.replace( "tl__", name.toLower() );
	temp.replace( "tlcf__", lcf( name ) );
	temp.replace( "b__", base );
	temp.replace( "bu__", base.toUpper() );
	temp.replace( "bl__", base.toLower() );
	temp.replace( "snu__", schema->name().toUpper() );
	temp.replace( "snl__", schema->name().toLower() );
	temp.replace( QRegExp( "RET\\(([^\\)]+)\\)" ), "\\1" );
	write( temp, path + "/autocore/" + name.toLower() + "list.h" );

	temp = readFile( "templates/autolist.sip" );
	temp.replace( "<%INDEXDEFS%>", indexDefs );
	temp.replace( "<%ELEMENTHACKS%>", elementHacks );
	temp.replace( "<%ELEMENTHEADERS%>", elementHeaders );
	temp.replace( "<%CLASSDEFS%>", classDefines );
	temp.replace( "<%CLASSHEADERS%>", classHeaders );
	temp.replace( "<%LISTDEFS%>", sipListMethodDefs );
	temp.replace( QRegExp( "RET\\(([^\\)]+List)\\)" ), "Mapped\\1" );
	temp.replace( "t__", name );
	temp.replace( "tu__", name.toUpper() );
	temp.replace( "tl__", name.toLower() );
	temp.replace( "tlcf__", lcf( name ) );
	temp.replace( "b__", base );
	temp.replace( "bu__", base.toUpper() );
	temp.replace( "bl__", base.toLower() );
	temp.replace( "snu__", schema->name().toUpper() );
	temp.replace( "snl__", schema->name().toLower() );
	temp.replace( QRegExp( "RET\\(([^\\)]+)\\)" ), "Mapped\\1" );
	write( temp, path + "/sip/" + name.toLower() + "list.sip" );

	temp = readFile( "templates/autolist.cpp" );
	temp.replace( "<%ELEMENTHEADERS%>", elementHeaders );
	temp.replace( "<%CLASSDEFS%>", classDefines );
	temp.replace( "<%CLASSHEADERS%>", classHeaders );
	temp.replace( "<%METHODS%>", methods );
	temp.replace( "<%LISTMETHODS%>", listMethods );
	temp.replace( "t__", name );
	temp.replace( "tu__", name.toUpper() );
	temp.replace( "tl__", name.toLower() );
	temp.replace( "tlcf__", lcf( name ) );
	temp.replace( "b__", base );
	temp.replace( "bu__", base.toUpper() );
	temp.replace( "bl__", base.toLower() );
	temp.replace( "snu__", schema->name().toUpper() );
	temp.replace( "snl__", schema->name().toLower() );
	write( temp, path + "/autocore/" + name.toLower() + "list.cpp" );
	
}

void writeSource( Schema * schema, const QString & path )
{
	LOG_1( "writeSource: Writing sources to " + path );
	TableSchemaList tables = schema->tables();
	QStringList headers, sources, classes;
	TableSchemaList nonCodeGen;
	QString autoSip;

	foreach( TableSchema * t, tables ) {
		if( !t->useCodeGen() ) {
			nonCodeGen += t;
			continue;
		}
		writeClass( t, path );
		QString lct = t->className().toLower();
		classes += t->className();
		headers += "\tautocore/" + lct + ".h";
		headers += "\tautocore/" + lct + "list.h";
//		headers += "\tautoimp/" + lct + "imp.h";
		headers += "\tautocore/" + lct + "table.h";
		sources += "\tautocore/" + lct + ".cpp";
		sources += "\tautocore/" + lct + "list.cpp";
//		sources += "\tautoimp/" + lct + "imp.cpp";
		sources += "\tautocore/" + lct + "table.cpp";
		if( QFile::exists( path + "/base/" + lct + "base.cpp" ) )
			sources += "\tbase/" + lct + "base.cpp";
		
		autoSip += "%Include " + lct + ".sip\n";
		autoSip += "%Include " + lct + "list.sip\n";
	}

	schema->writeXmlSchema( path + "/ng_schema.xml", 0, nonCodeGen );
	
	QString autoPri = "SOURCES += \\\nloader.cpp\\\n" + sources.join( " \\\n") + "\n\n";
	autoPri += "HEADERS += \\\n" + headers.join( " \\\n") + "\n\n";
	QDir par = QDir::current();
	par.cdUp();
	
	autoPri += "INCLUDEPATH+=autocore include\n\n";
	
	write( autoPri, path + "/auto.pri" );
	
	QString loader;
	loader += "\n";
	loader += "#include \"database.h\"\n";
	loader += "#include \"connection.h\"\n";
	loader += "#include \"" + schema->name().toLower() + ".h\"\n";
	for( QStringList::Iterator it = classes.begin(); it != classes.end(); ++it )
		loader += "#include \"" + (*it).toLower() + "table.h\"\n";
    loader += "\n\n" + schema->name().toUpper() + "_EXPORT void "+ schema->name().toLower() + "_loader() {\n\t";
	loader += "static bool classesLoaded = false;\n\t";
	loader += "if( classesLoaded )\n\t\treturn;\n\t";
	loader += "classesLoaded = true;\n\t";
	loader += classes.join( "Schema::instance();\n\t" ) + "Schema::instance();\n\t";
	loader += "Database::setCurrent( new Database( "+ schema->name().toLower() +"Schema(), Connection::createFromIni( config(), \"Database\" ) ) );\n";
	loader += "}\n";
	write( loader, path + "/loader.cpp" );
	write( autoSip, path + "/sip/auto.sip" );
}

