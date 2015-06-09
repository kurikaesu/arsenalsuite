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

#include "dynamichostgroup.h"
#include "hostgroup.h"
#include "hostgroupitem.h"

HostList HostGroup::hosts()
{
	DynamicHostGroup dhg(*this);
	if( dhg.isRecord() )
		return Host::select( dhg.hostWhereClause() );
	return hostGroupItems().hosts();
}

void HostGroup::addHost( const Host & host )
{
	HostGroupItem hgi = HostGroupItem();
	hgi.setHostGroup( key() );
	hgi.setHost( host );
	hgi.commit();
}

void HostGroup::removeHost( const Host & host )
{
	hostGroupItems().filter( "fkeyhost", host.key() ).remove();
}

int HostGroup::status()
{
	int ret = 0;
	HostList hosts = (*this).hosts();
	foreach( Host h, hosts )
		if (h.syslogStatus() > ret)
			ret = h.syslogStatus();
	return ret;
}

#endif // CLASS_FUNCTIONS

