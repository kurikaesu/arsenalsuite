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

#include "pyembed.h"

#include <qprocess.h>
#include <qsqlquery.h>
#include <limits>

#include "blurqt.h"
#include "database.h"
#include "connection.h"

#include "host.h"
#include "hostload.h"
#include "hostmapping.h"
#include "jobtype.h"
#include "jobtypemapping.h"
#include "mapping.h"
#include "mappingtype.h"
#include "path.h"

bool Mapping::map( bool forceUnmount, QString * errMsg )
{
	bool isLoadBalanced = false;
	Host h = host();
	LOG_TRACE
	
	if( h.isRecord() ) {
		// Reload host to verify it is online/offline.  If we are not connected to the database
		// then we will skip this check.
		if( Database::current()->connection()->isConnected() )
			h.reload();
	}
	// If there is a group of hosts, pick the one with the lowest load
	else { // !h.isRecord()
		isLoadBalanced = true;
		HostList hl = hostMappings().hosts();
		LOG_5( "Got " + QString::number( hl.size() ) + " hosts for this mapping" );
		hl.reload();
		LOG_5( "Got " + QString::number( hl.size() ) + " hosts after reload" );
		hl = hl.filter( "online", 1 );
		LOG_5( "Got " + QString::number( hl.size() ) + " hosts after online filter" );
		double bestLoad = std::numeric_limits<double>::max();
		foreach( Host hit, hl ) {
			HostLoad hl = hit.hostLoad();
			double computedAvg = hl.loadAvg() + hl.loadAvgAdjust();
			LOG_5( "Checking host " + hit.name() + "  Load: " + QString::number( computedAvg ) + " Online: " + QString::number(hit.online()) );
			if( hit.online() && computedAvg < bestLoad ) {
				bestLoad = computedAvg;
				h = hit;
			}
		}
	}

	if( !h.isRecord() || !h.online() ) {
		if( errMsg ) *errMsg = "Unable to find online host for mapping.  keyMapping: " + QString::number( key() );
		return false;
	}

	bool ret = false;
#ifdef Q_OS_WIN

	if( mappingType().name() == "smb" ) {
		QString unc;
		unc += "\\\\";
		unc += h.name();
		unc += "\\";
		unc += share();
		QString err;
		ret = mapDrive( mount()[0].toLatin1(), unc, forceUnmount, &err );
		if( !ret && !forceUnmount && err.contains("The local device name has a remembered connection to another network resource.") )
			ret = mapDrive( mount()[0].toLatin1(), unc, true, &err );
		if( !ret && errMsg )
			*errMsg = "Unable to map share: " + unc + " to drive: " + mount() + " Error was:" + err;
		else
			LOG_3( "Smb share mapped: " + unc + " to drive: " + mount() );
	}
#else
	Q_UNUSED(forceUnmount);

	if( mappingType().name() == "nfs" ) {
		QString cmd;
		cmd += "mount -t nfs ";
		cmd += h.name();
		cmd += ":/";
		cmd += share() + " ";
		cmd += mount();
		QProcess p;
		p.start( cmd );
		ret = p.exitCode() == 0;
	}

#endif // Q_OS_WIN

	// Update loadavgadjust, for better load balancing
	if( ret && isLoadBalanced )
		Database::current()->exec("SELECT * FROM increment_loadavgadjust(" + QString::number(h.key()) + ")");

	return ret;
}

bool Mapping::isMirrorSyncPath( const QString & fullPath )
{
	// Keep this in python so we can modify it quickly without recompile
	return runPythonFunction( "blur.mappingutil", "isMirrorSyncPath", VarList() << QVariant(fullPath) ).toBool();
}

bool Mapping::isMappedPath( const JobType & jobType, const QString & path )
{
	QString drive = path[0].toLower();
	MappingList mappings = jobType.jobTypeMappings().mappings();
	foreach( Mapping mapping, mappings ) {
		if( mapping.mount() == drive ) {
			if( !mapping.host().isRecord() )
				return isMirrorSyncPath( path );
			return true;
		}
	}
	return false;
}

#endif // CLASS_FUNCTIONS

