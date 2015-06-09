
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

#ifdef COMPILE_RIBGEN_BURNER

#include <qprocess.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qfile.h>
#include <qfileinfo.h>

#include "path.h"
#include "process.h"

#include "jobribgen.h"
#include "jobtype.h"

#include "config.h"
#include "ribgenburner.h"
#include "slave.h"

#ifdef Q_OS_WIN
#include "windows.h"
#endif // Q_OS_WIN

RibGenBurner::RibGenBurner( const JobAssignment & jobAssignment, Slave * slave )
: JobBurner( jobAssignment, slave )
, mFrame( 0 )
, mFrameEnd( 0 )
, mFrameCompleteRE("(.*?\\.\\d+\\.rib)")
{
	mErrorREs += QRegExp("Error:.*?(?!ignored)");
}

RibGenBurner::~RibGenBurner()
{
}

QString RibGenBurner::buildCmdRibGen()
{
	JobRibGen jm( mJob );
	QString cmd;
	cmd += " -scene " + jm.fileName();
	cmd += " -mode std";

	mFrame = assignedTasks().section("-",0,0).toInt();
	QString frameEnd = assignedTasks().section("-",1,1);
	if( frameEnd.isEmpty() )
		mFrameEnd = mFrame;
	else 
		mFrameEnd = frameEnd.toInt();

	cmd += " -cmd \"";
	for(int i = mFrame; i<=mFrameEnd; i++)
		cmd += "genRib "+QString::number(i);
	cmd += "\"";

	return cmd;
}

QStringList RibGenBurner::processNames() const
{
    return QStringList() << "mtor";
}

QString RibGenBurner::executable() const
{
	return "/opt/pixar/rat/bin/mtor";
}

void RibGenBurner::startProcess()
{
	LOG_5( "starting" );

	QString jt = mJob.jobType().name();
	QString cmd = executable();

	QStringList env = QProcess::systemEnvironment();

	cmd += buildCmdRibGen();

	mCmd = new QProcess( this );
	mCmd->setEnvironment(env);
	connectProcess( mCmd );
	
	LOG_3( "RG: Starting Cmd: " + cmd );
	mCmd->start( cmd );

	mCheckupTimer->start( 3000 );

	taskStart( mFrame );
}

void RibGenBurner::slotProcessOutputLine( const QString & line, QProcess::ProcessChannel channel )
{
	if( mFrameCompleteRE.indexIn(line) != -1 ) {
		// Certain versions of max will not report finished frames,
		// they will just report that the next is started, so we
		// assume the current frame has finished when we get
		// frame finished, frame started, or job complete
		emit taskDone( mFrame );
		LOG_3( "MTOR: Completed frame: " + QString::number(mFrame) );
		mFrame++;

		if( mFrame > mFrameEnd ) {
			LOG_3("RG::slotReadOutput emit'ing jobFinished()");
			jobFinished();
		} else {
			LOG_3( "MTOR: Starting frame: " + QString::number(mFrame) );
			taskStart( mFrame );
		}
		return;
	}

	JobBurner::slotProcessOutputLine( line, channel );
}

#endif
