/*
 *
 * Copyright 2003, 2004 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Resin is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Resin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef COMMIT_CODE

#include <qsqldatabase.h>

#include "database.h"

#include "host.h"
#include "hoststatus.h"
#include "job.h"

void HostStatus::returnSlaveFrames( bool commitHostStatus )
{
	Database::current()->beginTransaction();
	Database::current()->exec( "SELECT return_slave_tasks_3(?)", VarList() << host().key() );
	Database::current()->commitTransaction();
}

#endif // CLASS_FUNCTIONS

