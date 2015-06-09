/*
 *
 * Copyright 2006 Blur Studio Inc.
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

#include "host.h"
#include "hostservice.h"
#include "service.h"

bool Service::pulse( bool autoCreate )
{
	return byHost(Host::currentHost(),autoCreate).pulse();
}

Service Service::ensureServiceExists( const QString & name )
{
	Service s = recordByName( name );
	if( !s.isRecord() ) {
		s.setService( name );
		s.commit();
	}
	return s;
}

HostService Service::byHost( const Host & host, bool create )
{
	if( !isRecord() || !host.isRecord() ) return HostService();
	HostService hs = HostService::recordByHostAndService(host,*this);
	if( !hs.isRecord() && create ) {
		hs.setHost( host );
		hs.setService( *this );
		hs.commit();
	}
	return hs;
}

#endif

