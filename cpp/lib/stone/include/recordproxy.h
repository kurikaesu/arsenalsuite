
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

#ifndef RECORD_PROXY_H
#define RECORD_PROXY_H

#include <qobject.h>
#include "record.h"

namespace Stone {

/**
 * \ingroup Stone
 */
class STONE_EXPORT RecordProxy : public QObject
{
Q_OBJECT
public:
	RecordProxy( QObject * parent );

	RecordList & records();

	void applyChanges( bool commit = false );
	
	void setRecordList( const RecordList &, bool applyChanges = true, bool commitChanges = true );

	void clear();

signals:
	void recordListChange();
	void updateRecordList();

protected:
	RecordList mRecords;
};

} //namespace

using Stone::RecordProxy;

#endif // RECORD_PROXY_H

