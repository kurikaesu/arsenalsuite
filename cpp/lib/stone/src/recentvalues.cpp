
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

#include <qmap.h>
#include <qregexp.h>

#include "blurqt.h"
#include "recentvalues.h"
#include "iniconfig.h"

RecentValues::RecentValues( const QString & configGroup, int defaultLimit, const QString & configKey )
: mConfigGroup( configGroup )
, mConfigKey( configKey )
, mRecentLimit( defaultLimit )
{}

RecentValues::RecentValues( const RecentValues & other )
: mConfigGroup( other.mConfigGroup )
, mConfigKey( other.mConfigKey )
, mRecentLimit( other.mRecentLimit )
{}

QRegExp RecentValues::configValueRegEx() const
{ return QRegExp( mConfigKey + "Value(\\d+)" ); }

QRegExp RecentValues::configNameRegEx() const
{ return QRegExp( mConfigKey + "Name(\\d+)" ); }

QList<QVariant> RecentValues::recentValues() const
{
	IniConfig & cfg = userConfig();
	cfg.pushSection( mConfigGroup );
	QMap<int,QVariant> byIndex;
	QRegExp regEx = configValueRegEx();
	foreach( QString configKey, cfg.keys( regEx ) )
	{
		regEx.exactMatch(configKey);
		int recentIndex = regEx.cap(1).toInt();
		if( recentIndex >= 0 && recentIndex < mRecentLimit )
			byIndex[recentIndex] = cfg.readValue( configKey );
	}
	QList<QVariant> ret;
	for( int i=0; i < mRecentLimit; i++ ) {
		if( !byIndex.contains(i) )
			return ret;
		ret.append( byIndex[i] );
	}
	cfg.popSection();
	return ret;
}

QList<RecentValues::RecentNameValue> RecentValues::recentNameValues() const
{
	IniConfig & cfg = userConfig();
	cfg.pushSection( mConfigGroup );
	QMap<int,RecentNameValue> byIndex;
	QRegExp regEx = configValueRegEx();
	foreach( QString configKey, cfg.keys( regEx ) )
	{
		regEx.exactMatch(configKey);
		int recentIndex = regEx.cap(1).toInt();
		if( recentIndex >= 0 && recentIndex < mRecentLimit ) {
			byIndex[recentIndex].value = cfg.readValue( configKey );
			LOG_5( QString("Read Recent Value: %1 at pos %2").arg(byIndex[recentIndex].value.toString()).arg(recentIndex) );
		}
	}
	regEx = configNameRegEx();
	foreach( QString configKey, cfg.keys( regEx ) )
	{
		regEx.exactMatch(configKey);
		int recentIndex = regEx.cap(1).toInt();
		if( recentIndex >= 0 && recentIndex < mRecentLimit ) {
			byIndex[recentIndex].name = cfg.readString( configKey );
			LOG_5( QString("Read Recent Value Name: %1 at pos %2").arg(byIndex[recentIndex].name).arg(recentIndex) );
		}
	}
	QList<RecentNameValue> ret;
	for( int i=0; i < mRecentLimit; i++ ) {
		if( !byIndex.contains(i) )
			return ret;
		RecentNameValue rnv = byIndex[i];
		if( rnv.name.isEmpty() )
			rnv.name = rnv.value.toString();
		ret.append( rnv );
	}
	cfg.popSection();
	return ret;
}


void RecentValues::writeValue( int index, RecentNameValue rnv )
{
	IniConfig & cfg = userConfig();
	cfg.pushSection( mConfigGroup );
	cfg.writeValue( mConfigKey + "Value" + QString::number(index), rnv.value );
	QString nameKey = mConfigKey + "Name" + QString::number(index);
	if( rnv.name.isEmpty() || rnv.name == rnv.value.toString() )
		cfg.removeKey(nameKey);
	else
		cfg.writeString(nameKey, rnv.name);
	cfg.popSection();
	cfg.writeToFile();
}

RecentValues::RecentNameValue RecentValues::readValue( int index )
{
	IniConfig & cfg = userConfig();
	cfg.pushSection( mConfigGroup );
	RecentNameValue ret;
	ret.value = cfg.readValue( mConfigKey + "Value" + QString::number(index) );
	ret.name = cfg.readString( mConfigKey + "Name" + QString::number(index) );
	cfg.popSection();
	return ret;
}

void RecentValues::addRecentValue( const QVariant & value, const QString & name )
{
	RecentNameValue toAdd( name, value );
	if( readValue( 0 ) == toAdd ) return;
	QList<RecentNameValue> cur = recentNameValues();
	if( cur.size() == mRecentLimit )
		cur.removeAt( mRecentLimit - 1 );
	cur.prepend( toAdd );
	int idx = 0;
	foreach( RecentNameValue rnv, cur )
		writeValue( idx++, rnv );
}

