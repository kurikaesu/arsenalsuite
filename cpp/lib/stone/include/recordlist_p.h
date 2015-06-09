
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


#ifndef RECORD_LIST_PRIVATE_H
#define RECORD_LIST_PRIVATE_H

#include <qlist.h>
#include <qatomic.h>

#include "recordlist.h"

namespace Stone {
class RecordImp;

class STONE_EXPORT ImpList : public QList<RecordImp*>
{
public:
	ImpList() {}
	ImpList( const ImpList & il ) : QList<RecordImp*>( il ) { detach(); }
	ImpList( const QList<RecordImp*> list ) : QList<RecordImp*>( list ) { detach(); }
	~ImpList() {}
	ImpList & operator = ( const ImpList & other ) { QList<RecordImp*>::operator=( other ); detach(); return *this; }
	bool operator==( const ImpList & other ) { return QList<RecordImp*>::operator==( other ); }
	bool operator!=( const ImpList & other ) { return QList<RecordImp*>::operator!=( other ); }
};

class RecordList::Private
{
public:
	Private()
	{}

	ImpList mList;
	QAtomicInt mCount;
};

}

using Stone::ImpList;

#endif // RECORD_LIST_PRIVATE_H

