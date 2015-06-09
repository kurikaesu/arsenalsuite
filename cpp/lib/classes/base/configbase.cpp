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

#include "config.h"
/*
bool Config::getBool( const QString & key )
{
	return recordByName( key ).value().toBool();
}*/

int Config::getInt( const QString & key, int def )
{
	Config c = recordByName( key );
	if( !c.isRecord() ) return def;
	bool valid;
	int ret = c.value().toInt( &valid );
	if( !valid ) return def;
	return ret;
}

uint Config::getUInt( const QString & key, uint def )
{
	Config c = recordByName( key );
	if( !c.isRecord() ) return def;
	bool valid;
	uint ret = c.value().toUInt( &valid );
	if( !valid ) return def;
	return ret;
}

float Config::getFloat( const QString & key, float def )
{
	Config c = recordByName( key );
	if( !c.isRecord() ) return def;
	bool valid;
	float ret = c.value().toDouble( &valid );
	if( !valid ) return def;
	return ret;
}

QString Config::getString( const QString & key, const QString & def )
{
	Config c = recordByName( key );
	if( !c.isRecord() ) return def;
	return c.value();
}

bool Config::getBool( const QString & key, bool def )
{
	Config c = recordByName( key );
	if( !c.isRecord() ) return def;
	QString v = c.value().toLower();
	return (v=="true"||v=="t"||v=="1");
}

#endif
