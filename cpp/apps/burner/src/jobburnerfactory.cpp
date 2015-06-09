
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Arsenal.
 *
 * Arsenal is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Arsenal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arsenal; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 */

#include "blurqt.h"

#include "job.h"
#include "jobassignment.h"
#include "jobtype.h"

#include "jobburnerfactory.h"
#include "jobburnerplugin.h"


bool JobBurnerFactory::mPluginsLoaded = false;
QMap<QString,JobBurnerPlugin*>  JobBurnerFactory::mBurnerPlugins;

bool JobBurnerFactory::supportsJobType( const Job & job )
{
	if( !job.jobType().isRecord() )
		return false;

	QString jobType = job.jobType().name();

	if( mBurnerPlugins.contains( jobType ) )
		return true;

	return false;
}

void JobBurnerFactory::registerPlugin( JobBurnerPlugin * bp, bool overrideExisting )
{
	QStringList types = bp->jobTypes();
	foreach( QString t, types )
		if( overrideExisting || !mBurnerPlugins.contains(t) ) {
			mBurnerPlugins[t] = bp;
			LOG_3( "Registering burner for jobtype: " + t );
		}
}

JobBurner * JobBurnerFactory::createBurner( const JobAssignment & jobAssignment, Slave * slave, QString * /* errMsg */ )
{
	Job job = jobAssignment.job();
	if( !job.isRecord() || !job.jobType().isRecord() )
		return 0;

	QString jobType = job.jobType().name();
	if( mBurnerPlugins.contains( jobType ) ) {
		JobBurner * jb = mBurnerPlugins[jobType]->createBurner(jobAssignment,slave);
		if( jb ) return jb;
	}
	return 0;
}

