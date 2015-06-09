
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
 * $Id: recordlistmodel.h 6486 2008-05-02 01:34:01Z newellm $
 */

#ifndef RECORD_LIST_MODEL_H
#define RECORD_LIST_MODEL_H

#include "recordsupermodel.h"
#include "recordlist.h"
#include "stonegui.h"

/*
struct STONEGUI_EXPORT record_cache : public RecordCacheBase {
	RecordList children( const QModelIndex & );
	QVariant data ( const QModelIndex &, int role, const QString & column = QString() ) const;
	int cmp( const RecordCacheBase & other, const QModelIndex &, const QModelIndex &, bool );
};

typedef RecordModelImp<TreeNodeT<record_cache> > RecordFastModel;
*/

class STONEGUI_EXPORT RecordListModel : public RecordSuperModel
{
Q_OBJECT
public:
	RecordListModel( QObject * parent=0 );

	void setRecordList( RecordList );
	RecordList recordList();
	
	void setColumn( const QString & column );
	QString column();

	void setColumns( QStringList columns );
	QStringList columns();

	virtual RecordList children( const Record & ) { return RecordList(); }

	virtual QVariant recordData( const Record & record, int role, const QString & column ) const;
	virtual Qt::ItemFlags recordFlags( const Record & record, const QString & ) const;
	virtual int compare( const Record & r1, const QString & column1, const Record & r2, const QString & column2 ) const;

	virtual bool setRecordData( const Record & record, const QString & col, const QVariant &, int );

	QString columnName( int col ) const;
protected:
	QStringList mColumns;
};


#endif // RECORD_LIST_MODEL_H

