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

#include "threadnotify.h"
	
bool ThreadNotify::requiresSignoff() const
{
	return options() & RequiresSignoff;
}

void ThreadNotify::setRequiresSignoff( bool rs )
{
	if( requiresSignoff() != rs )
		setOptions( options() ^ RequiresSignoff );
}

bool ThreadNotify::signedOff() const
{
	return options() & SignedOff;
}

void ThreadNotify::setSignedOff( bool so )
{
	if( signedOff() != so )
		setOptions( options() ^ SignedOff );
}

bool ThreadNotify::read() const
{
	return options() & HasBeenRead;
}

void ThreadNotify::setRead( bool r )
{
	if( read() != r )
		setOptions( options() ^ HasBeenRead );
}

#endif

