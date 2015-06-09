
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

#ifndef RECORD_XML_H
#define RECORD_XML_H

#include <qdom.h>
#include <qmap.h>

#include "blurqt.h"
#include "recordlist.h"

class STONE_EXPORT RecordXmlSaver
{
public:
	RecordXmlSaver(RecordList rl = RecordList());

	QList<QDomElement> addRecords( RecordList rl );
	QDomElement addRecord( const Record & r );

	QDomDocument document() const;
	bool saveToFile(const QString & fileName, QString * errorMessage = 0);

	static bool toFile(RecordList rl, const QString & fileName, QString * errorMessage = 0);
	static QDomDocument toDocument(RecordList rl, QString * errorMessage = 0);
protected:
	QString internalId( const Record & record );

	QMap<Record,int> mInternalIdMap;
	QDomDocument mDocument;
	QDomElement mRoot;
	int mNextInternalId;
};

class STONE_EXPORT RecordXmlLoader
{
public:
	RecordXmlLoader(const QString & fileName);
	RecordXmlLoader(QDomDocument document);
	RecordXmlLoader();

	RecordList records() const;
	QMap<Record,QDomElement> recordElementMap() const;

	bool loadFile( const QString & fileName, QString * errorMessage = 0);
	bool loadDocument( const QDomDocument & document, QString * errorMessage = 0);

	static RecordList fromFile(const QString & fileName, QString * errorMessage = 0);
	static RecordList fromDocument(const QDomDocument & document, QString * errorMessage = 0);

protected:
	bool isInternalIdLoaded( int internalId );
	Record recordByInternalId( int internalId );

	QMap<Record,QDomElement> mRecordElementMap;
	QMap<int,Record> mInternalIdMap;
	QList< QPair<Record,QDomElement> > mNeedsFkeyFixup;

	RecordList mRecords;
};

#endif // RECORD_XML_H

