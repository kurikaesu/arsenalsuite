
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

#ifdef COMPILE_RENDERMAN_BURNER

#include <qprocess.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qfile.h>
#include <qfileinfo.h>

#include "path.h"
#include "process.h"

#include "jobrenderman.h"
#include "jobtype.h"

#include "config.h"
#include "rendermanburner.h"
#include "slave.h"

#ifdef Q_OS_WIN
#include "windows.h"
#endif // Q_OS_WIN

RenderManBurner::RenderManBurner( const JobAssignment & jobAssignment, Slave * slave )
: JobBurner( jobAssignment, slave )
, mFrame( 0 )
{
	mErrorREs += QRegExp("Error:.*?(?!ignored)");
}

RenderManBurner::~RenderManBurner()
{
}

QString RenderManBurner::buildCmdRenderMan()
{
	JobRenderMan jm( mJob );

	mFrame = assignedTasks().section("-",0,0).toInt();
	QString rib = jm.fileName();
	int len = QString::number(mFrame).length();
	int pos = rib.length() - 4 - len;
	rib.replace(pos, len, QString::number(mFrame));

	return rib;
}

QStringList RenderManBurner::processNames() const
{
    return QStringList() << "render";
}

QString RenderManBurner::executable() const
{
	JobRenderMan j( mJob );
	return "/opt/pixar/RenderManProServer-"+ j.version() +"/bin/render";
}

void RenderManBurner::startProcess()
{
	LOG_5( "RM::startBurn() starting" );

	QString jt = mJob.jobType().name();
	QString cmd = executable();

	QStringList env = QProcess::systemEnvironment();

	cmd += buildCmdRenderMan();

	mCmd = new QProcess( this );
	mCmd->setEnvironment(env);
	
	connectProcess( mCmd );
	
	LOG_3( "RM: Starting Cmd: " + cmd );
	mCmd->start( cmd );

	mCheckupTimer->start( 3000 );

	taskStart( mFrame );
}

void RenderManBurner::slotProcessExited()
{
	slotReadStdOut();
	slotReadStdError();
	if( mState == StateStarted ) {
		emit taskDone( mFrame );
		jobFinished();
	}
	JobBurner::slotProcessExited();
}

#endif
