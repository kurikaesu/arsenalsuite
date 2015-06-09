
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
 * $Id: recorddrag.cpp 5411 2007-12-18 01:03:08Z brobison $
 */

#include <qstringlist.h>

#include "blurqt.h"
#include "database.h"
#include "table.h"
#include "recorddrag.h"

const char * RecordDrag::mimeType = "application/x-resin-record-list";

QByteArray RecordDrag::encodeRecordList( RecordList rl )
{
	QStringList sl;
	foreach( Record r, rl )
		sl += r.table()->schema()->tableName() + "." + QString::number(r.key());
	return sl.join(",").toLatin1();
}

QDrag * RecordDrag::create( RecordList rl, QWidget * parent )
{
	QDrag * drag = new QDrag( parent );
	drag->mimeData()->setData( mimeType, encodeRecordList( rl ) );
	return drag;
}

QMimeData * RecordDrag::toMimeData( RecordList rl )
{
	QMimeData * md = new QMimeData();
	md->setData( mimeType, encodeRecordList( rl ) );
	return md;
}

bool RecordDrag::canDecode( const QMimeData * ms )
{
	return ms->formats().contains( mimeType );
}

bool RecordDrag::decode( const QMimeData * ms, RecordList * rl )
{
	if( rl && canDecode( ms ) ) {
		Database * db = Database::current();
		QString data = QString::fromAscii( ms->data(mimeType) );
		QStringList sl = data.split(',');
		foreach( QString si, sl )
		{
			int dotI = si.indexOf( '.' );
			if( dotI < 1 ) continue;
			QString tbl( si.left(dotI) );
			int key( si.mid(dotI+1).toInt() );
			LOG_5( "RecordDrag::decode: Table: " + tbl + " key: " + QString::number(key) );
			Table * t = db->tableByName( tbl );
			if( t )
				(*rl) += t->record( key );
		}
		return true;
	}
	return false;
}

