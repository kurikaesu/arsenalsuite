
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
 * $Id: recorddrag.h 5411 2007-12-18 01:03:08Z brobison $
 */

#ifndef ELEMENT_DRAG_OBJECT_H
#define ELEMENT_DRAG_OBJECT_H

#include <qdrag.h>
#include <qmimedata.h>

#include "record.h"
#include "stonegui.h"

class STONEGUI_EXPORT RecordDrag
{
public:
	static QByteArray encodeRecordList( RecordList rl );
	static QDrag * create( RecordList, QWidget * parent );
	static QMimeData * toMimeData( RecordList );
	static bool canDecode( const QMimeData * );
	static bool decode( const QMimeData *, RecordList * );
	static const char * mimeType;
};

#endif // ELEMENT_DRAG_OBJECT_H

