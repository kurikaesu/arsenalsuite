
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

#ifndef _TRIGGER_H_
#define _TRIGGER_H_

#include "record.h"
#include "recordlist.h"

namespace Stone {

class Table;
class TableSchema;

class STONE_EXPORT Trigger
{
public:
	enum TriggerType
	{
		CreateTrigger			= 0x01,
		IncomingTrigger			= 0x02,
		PreInsertTrigger		= 0x04,
		PreUpdateTrigger		= 0x08,
		PreDeleteTrigger		= 0x10,
		PostInsertTrigger		= 0x20,
		PostUpdateTrigger		= 0x40,
		PostDeleteTrigger		= 0x80,
	};

	Trigger( int triggerTypes ) : mTriggerTypes( triggerTypes ) {}
	virtual ~Trigger() {}
	
	int triggerTypes() const { return mTriggerTypes; }
	
	virtual Record create( Record r ) { return r; }
	virtual RecordList incoming( RecordList rl ) { return rl; }
	virtual RecordList preInsert( RecordList rl ) { return rl; }
	virtual Record preUpdate( const Record & updated, const Record & /*before*/ ) { return updated; }
	virtual RecordList preDelete( RecordList rl ) { return rl; }
	virtual void postInsert( RecordList ) {}
	virtual void postUpdate( const Record & /*updated*/, const Record & /*before*/ ) {}
	virtual void postDelete( RecordList ) {}

private:
	int mTriggerTypes;
	friend class Table;
	friend class TableSchema;
};

}; // namespace Stone

using Stone::Trigger;

#endif // _TRIGGER_H_

