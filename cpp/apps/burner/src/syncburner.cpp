
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

#ifdef COMPILE_SYNC_BURNER

#include <qprocess.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>

#include "path.h"
#include "process.h"
#include "database.h"

#include "jobsync.h"
#include "jobtype.h"
#include "user.h"

#include "config.h"
#include "syncburner.h"
#include "slave.h"

#ifdef Q_OS_WIN
#include "windows.h"
#endif // Q_OS_WIN

SyncBurner::SyncBurner( const JobAssignment & jobAssignment, Slave * slave )
: JobBurner( jobAssignment, slave )
, mCmdString("")
, mTask( 1 )
, mCompleteRE("^total size is")
, mFileSizeRE("^(\\d+)\\s+(.*)")
{
}

SyncBurner::~SyncBurner()
{
}

QString SyncBurner::executable()
{
	return "/usr/bin/rsync";
}

QStringList SyncBurner::buildCmdArgs()
{
	QStringList args;
	args << "-av";
	args << "--out-format=%l %f";
	args << "--numeric-ids";

	JobSync js(mJob);
	if( !js.append().isEmpty() )
		args << js.append().split(" ");
	if( js.direction() == "push" ) {
		args << mJob.outputPath();
		args << mJob.name();
	} else {
		args << mJob.name();
		args << mJob.outputPath();
	}
	return args;
}

QStringList SyncBurner::processNames() const
{
    return QStringList() << "rsync";
}

void SyncBurner::startProcess()
{
	LOG_5( "SYNC::startBurn() starting" );

	QString outputDir = Path( mJob.outputPath() ).dirPath();
	if( !QFile::exists( outputDir ) ) {
		logMessage( "creating output dir: "+ outputDir );
		QDir(outputDir).mkpath( outputDir );
		QFile::setPermissions( outputDir, QFile::ReadUser|QFile::WriteUser|QFile::ExeUser|QFile::ReadGroup|QFile::WriteGroup|QFile::ExeGroup|QFile::ReadOther|QFile::WriteOther|QFile::ExeOther );
	}

	mTask = assignedTasks().section("-",0,0).toInt();
	logMessage( "RSYNC: Starting chunk: " + QString::number(mTask) );
	taskStart( mTask );

	JobBurner::startProcess();

	mCheckupTimer->start( 30 * 1000 );
}

void SyncBurner::slotProcessOutputLine( const QString & line, QProcess::ProcessChannel channel )
{
	bool fileSizeLine = mFileSizeRE.indexIn(line) >= 0;
	bool complete = line.contains( mCompleteRE );

	if( fileSizeLine ) {
		mBytesComplete += mFileSizeRE.cap(1).toInt();
		while( mBytesComplete > 100000000 ) {
			taskDone( mTask );
			logMessage( "RSYNC: Completed chunk: " + QString::number(mTask) );
			mTask++;
			mBytesComplete = qMax(0, mBytesComplete-100000000);
			logMessage( "RSYNC: Starting chunk: " + QString::number(mTask) );
			taskStart( mTask );
		}
	}
	else if( complete ) {
		LOG_3( "RSYNC: Completed sync" );
		VarList vl;
		vl += mJob.key();
		vl += Host::currentHost().key();
		Database::current()->exec("UPDATE jobtask SET status='done', ended=extract(epoch from now()) WHERE fkeyjob=? AND fkeyhost=? AND status IN ('assigned','busy')", vl );
		jobFinished();
	}

	JobBurner::slotProcessOutputLine( line, channel );
}

void SyncBurner::cleanup()
{
	if( mCmd && mCmd->state() != QProcess::NotRunning )
		mCmd->kill();

	JobBurner::cleanup();
}

#endif
