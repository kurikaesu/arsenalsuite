
/*
 *
 * Copyright 2012 Blur Studio Inc.
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

#include <qfile.h>

#include "recordxml.h"

#include "database.h"
#include "path.h"
#include "table.h"
#include "tableschema.h"

RecordXmlSaver::RecordXmlSaver(RecordList rl)
: mNextInternalId( 1 )
{
	mRoot = mDocument.createElement( "RecordList" );
	mDocument.appendChild( mRoot );
	addRecords(rl);
}

QDomElement RecordXmlSaver::addRecord( const Record & r )
{
	TableSchema * ts = r.table()->schema();
	QDomElement record = mDocument.createElement( "Record" );
	mRoot.appendChild( record );
	record.setAttribute( "table", ts->tableName() );
	record.setAttribute( "state", QString::number(r.imp()->mState) );
	record.setAttribute( "internalId", internalId(r) );
	foreach( Field * field, ts->fields() ) {
		QVariant value = r.getValue( field->pos() );
		QString valueString = field->dbPrepare( value ).toString();
		if( value.userType() == qMetaTypeId<Record>() ) {
			Record fkey = r.foreignKey( field->pos() );
			valueString = internalId(fkey);
		}
		QDomElement fel = mDocument.createElement( "Field" );
		record.appendChild( fel );
		fel.setAttribute( "name", field->name() );
		QDomText valueEl = mDocument.createTextNode( valueString );
		fel.appendChild( valueEl );
	}
	return record;
}

QString RecordXmlSaver::internalId( const Record & record )
{
	if( !mInternalIdMap.contains( record ) )
		mInternalIdMap[record] = ++mNextInternalId;
	return "@" + QString::number( mInternalIdMap[record] );
}

QList<QDomElement> RecordXmlSaver::addRecords( RecordList rl )
{
	QList<QDomElement> ret;
	foreach( Record r, rl )
		ret += addRecord(r);
	return ret;
}

QDomDocument RecordXmlSaver::document() const
{
	return mDocument;
}

bool RecordXmlSaver::saveToFile(const QString & fileName, QString * )
{
	return writeFullFile( fileName, mDocument.toString() );
}

bool RecordXmlSaver::toFile(RecordList rl, const QString & fileName, QString * errorMessage)
{
	return RecordXmlSaver(rl).saveToFile(fileName,errorMessage);
}

QDomDocument RecordXmlSaver::toDocument(RecordList rl, QString *)
{
	return RecordXmlSaver(rl).document();
}


RecordXmlLoader::RecordXmlLoader(const QString & fileName)
{
	loadFile(fileName);
}

RecordXmlLoader::RecordXmlLoader(QDomDocument document)
{
	loadDocument(document);
}

RecordXmlLoader::RecordXmlLoader()
{
}

RecordList RecordXmlLoader::records() const
{
	return mRecords;
}

QMap<Record,QDomElement> RecordXmlLoader::recordElementMap() const
{
	return mRecordElementMap;
}

bool RecordXmlLoader::loadFile( const QString & fileName, QString * )
{
	QDomDocument doc;
	QString _errorMessage;
	int errorLine, errorColumn;

	QFile file( fileName );
	if ( !file.open( QIODevice::ReadOnly ) ) {
		LOG_1( "Couldn't Open File (" + fileName + ")" );
		return false;
	}
	
	if ( !doc.setContent( &file, &_errorMessage, &errorLine, &errorColumn ) ) {
		LOG_1( "Couldn't parse xml: line " + QString::number(errorLine) + " column " + QString::number(errorColumn) + " message " + _errorMessage );
		file.close();
		return false;
	}
	
	file.close();
	return loadDocument(doc);
}

bool RecordXmlLoader::isInternalIdLoaded( int internalId )
{
	return mInternalIdMap.contains(internalId);
}

Record RecordXmlLoader::recordByInternalId( int internalId )
{
	return mInternalIdMap[internalId];
}

bool RecordXmlLoader::loadDocument( const QDomDocument & doc, QString *)
{
	QDomNode root = doc.firstChild();
	QDomNode n = root.firstChild();
	while( !n.isNull() ) {
		QDomElement recordEl = n.toElement(); // try to convert the node to an element.
		if( !recordEl.isNull() && recordEl.tagName() == "Record" ) {
			QString tableName = recordEl.attribute("table");
			Table * table = Database::current()->tableByName( tableName );
			if( table ) {
				Record r = table->load();
				{
					int internalId = recordEl.attribute("internalId").mid(1).toInt();
					mInternalIdMap[internalId] = r;
					LOG_5( "Adding record to internal id map with value " + QString::number( internalId ) );
				}
				if( recordEl.hasAttribute( "state" ) )
					r.imp()->mState = recordEl.attribute( "state" ).toInt();
				QDomElement fieldEl = recordEl.firstChild().toElement();
				while( !fieldEl.isNull() ) {
					QString fieldName = fieldEl.attribute("name");
					Field * f = table->schema()->field( fieldName );
					if( f ) {
						QString valString = fieldEl.firstChild().toText().data();
						// Check for internal id reference
						if( (f->type() & Field::ForeignKey) && valString.startsWith("@") ) {
							int internalId = valString.mid(1).toInt();
							if( isInternalIdLoaded(internalId) ) {
								r.setForeignKey( f->pos(), recordByInternalId( internalId ) );
								LOG_5( "Found foreign key reference from internal id " + QString::number(internalId) );
							} else
								mNeedsFkeyFixup.append( qMakePair( r, fieldEl ) );
						} else {
							QVariant value = Field::variantFromString( valString, f->type() );
							r.setValue( f->pos(), value );
						}
					}
					fieldEl = fieldEl.nextSibling().toElement();
				}
				mRecords += r;
				mRecordElementMap[r] = recordEl;
			}
		}
		n = n.nextSibling();
	}

	QPair<Record,QDomElement> fp;
	foreach( fp, mNeedsFkeyFixup ) {
		Record r = fp.first;
		QDomElement fieldEl = fp.second;
		Table * table = r.table();
		Field * f = table->schema()->field( fieldEl.attribute("name") );
		QString valString = fieldEl.firstChild().toText().data();
		int internalId = valString.mid(1).toInt();
		if( isInternalIdLoaded(internalId) )
			r.setForeignKey( f->pos(), recordByInternalId( internalId ) );
	}
	return true;
}

RecordList RecordXmlLoader::fromFile(const QString & fileName, QString *)
{
	return RecordXmlLoader(fileName).records();
}

RecordList RecordXmlLoader::fromDocument(const QDomDocument & document, QString *)
{
	return RecordXmlLoader(document).records();
}

