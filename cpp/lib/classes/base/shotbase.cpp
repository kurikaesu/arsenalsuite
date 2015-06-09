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

#include <qdir.h>
#include <stdlib.h>

#include "blurqt.h"
#include "shot.h"
#include "shotgroup.h"
#include "project.h"
#include "rangefiletracker.h"
#include "path.h"

QString Shot::sortString() const
{
	return QString("%1.%2").arg( (int)shotNumber(), 6 ).arg( int(shotNumber()*100.0)%100 );
}

QString Shot::displayNumber() const
{
	double sn = shotNumber();
	QString snt;
	snt.sprintf("%04i.%02i", (int)sn, int(sn*100.0 + .9999999)%100 );
	return snt;
}

ShotGroup Shot::sequence()
{
	return parent();
}

#endif

