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

#ifdef HEADER_FILES
#include "host.h"

class HostService;
#endif

#ifdef CLASS_FUNCTIONS
	
	/// Looks up the hostservice record for this host(Host::currentHost())
	/// and service. If the hostservice record is not found and autoCreate is true,
	/// then the record is created.  If found or created the pulse column is updated
	/// to the current database datetime. 
	bool pulse( bool autoCreate = true );

	/// Looks up service entry by name, if the entry does not exist
	/// it creates one.
	static Service ensureServiceExists( const QString & name );

	HostService byHost( const Host & host = Host::currentHost(), bool create = true );

#endif

