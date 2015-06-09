
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

#ifdef COMPILE_SHAKE_BURNER

#include <qprocess.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>

#include "slave.h"
#include "jobtype.h"
#include "shakeburner.h"
#include "jobshake.h"
#include "config.h"
#include "process.h"
#include "path.h"

#ifdef Q_OS_WIN
#include "windows.h"
#endif // Q_OS_WIN

ShakeBurner::ShakeBurner( const JobAssignment & jobAssignment, Slave * slave )
: JobBurner( jobAssignment, slave )
, mFrame( 0 )
, mAssignedRE("info: rendering frame (\\d+)")
, mFrameCompleteRE("info: frame (\\d+) rendered in")
, mHasRange( true )
{
	mErrorREs += QRegExp("Error: ");
	mErrorREs += QRegExp("error:.* Invalid header");
}

ShakeBurner::~ShakeBurner()
{
}

QString ShakeBurner::buildCmd()
{
	JobShake js(mJob);
	if( js.frameRange().isEmpty() )
		mHasRange = false;
	QString cmd;
	cmd += " -v";
	cmd += " -cpu 2";
	if( js.frameRange().isEmpty() ) {
		cmd += " -t " + assignedTasks().section("-",0,0)
		    +  "-" + assignedTasks().section("-",1,1);
	} else {
		cmd += " -t " + js.frameRange();
	}
	if( js.fileName().endsWith(".shk") ) {
		cmd += " -exec " + js.fileName();
	} else {
		cmd += " -fi " + js.fileName();
		cmd += " -fo " + js.outputPath();
	}
	return cmd;
}

QStringList ShakeBurner::processNames() const
{
    return QStringList() << "shake";
}

QString ShakeBurner::executable() const
{
	if( QFile::exists("/usr/bin/shake") )
		return "/usr/bin/shake";
	else if( QFile::exists("/usr/local/bin/shake") )
		return "/usr/local/bin/shake";
	else
		return "";
}

void ShakeBurner::startProcess()
{
	LOG_5( "SHK::startProcess() called" );

	QString jt = mJob.jobType().name();
	QString cmd = executable();
	cmd += buildCmd();

	mCmd = new QProcess( this );
	connectProcess( mCmd );
	
	QString outputDir = Path( mJob.outputPath() ).dirPath();
	if( !QFile::exists( outputDir ) ) {
		logMessage( "creating output dir: "+ outputDir );
		QDir(outputDir).mkpath( outputDir );
		QFile::setPermissions( outputDir, QFile::ReadUser|QFile::WriteUser|QFile::ExeUser|QFile::ReadGroup|QFile::WriteGroup|QFile::ExeGroup|QFile::ReadOther|QFile::WriteOther|QFile::ExeOther );
	}

	LOG_3( "SHK: Starting Cmd: " + cmd );
	mCmd->start( cmd );

	mCheckupTimer->start( 30 * 1000 );

	mJobAssignment.setCommand(cmd);
	if( mHasRange )
		taskStart(1);
}

void ShakeBurner::slotProcessOutputLine( const QString & line, QProcess::ProcessChannel channel )
{
	bool ass = mAssignedRE.indexIn(line) >= 0;
	bool framecom = line.contains( mFrameCompleteRE );

	// Certain versions of max will not report finished frames,
	// they will just report that the next is started, so we
	// assume the current frame has finished when we get
	// frame finished, frame started, or job complete
	if( ass || framecom ) {
		if( taskStarted() ) {
			if( !mHasRange ) {
				QString framePath = makeFramePath( mJob.outputPath(), mFrame );
				emit fileGenerated( framePath );
				emit taskDone( mFrame );
			}
			LOG_3( "SHK: Completed frame: " + QString::number(mFrame) );
		}

		if( ass ) {
			mFrame = mAssignedRE.cap( 1 ).toInt();
			if( !mHasRange )
				taskStart( mFrame );
			LOG_3( "SHK: Starting frame: " + QString::number(mFrame) );
		}
		return;
	}

	JobBurner::slotProcessOutputLine( line, channel );
}

void ShakeBurner::slotProcessExited()
{
	slotReadStdOut();
	slotReadStdError();
	if( mCmd->exitCode() == 0 ) {
		if( mHasRange ) {
			emit fileGenerated( mJob.outputPath() );
			JobTaskList jtl = JobTask::recordsByJobAndHost( mJob, Host::currentHost() );
			jtl.setStatuses( "done" );
			jtl.setColumnLiteral( "ended", "extract(epoch from now())" );
			jtl.commit();
		}
		jobFinished();
	} else {
		jobErrored( "Shake exited with result: " + QString::number( mCmd->exitCode() ) );
	}
}

#endif
