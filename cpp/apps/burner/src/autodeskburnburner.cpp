
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

#ifdef COMPILE_AUTODESK_BURNER

#include <qprocess.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>

#include "path.h"
#include "process.h"
#include "database.h"

#include "jobautodeskburn.h"
#include "jobtype.h"
#include "user.h"

#include "config.h"
#include "autodeskburnburner.h"
#include "slave.h"

#ifdef Q_OS_WIN
#include "windows.h"
#endif // Q_OS_WIN

AutodeskBurnBurner::AutodeskBurnBurner( const Job & job, Slave * slave )
: JobBurner( job, slave )
, mCompleteRE("Application is down")
{
}

AutodeskBurnBurner::~AutodeskBurnBurner()
{
}

QString AutodeskBurnBurner::executable()
{
	return "/usr/discreet/backburner/backburnerServer";
}

QStringList AutodeskBurnBurner::buildCmdArgs()
{
	return QStringList() << "-m" << mJob.host().name();
}

QStringList AutodeskBurnBurner::processNames() const
{
    return QStringList() << "backburnerServer";
}

void AutodeskBurnBurner::startProcess()
{
	LOG_5( "ADB::startBurn() starting" );

	JobBurner::startProcess();

	mLogCmd = new QProcess( this );
	connect( mLogCmd, SIGNAL( readyReadStandardOutput() ), SLOT( slotReadLog() ) );
	mLogCmd->start("tail -F -n 1 /usr/discreet/backburner/log/backburnerServer.log");

	mCheckupTimer->start( 30 * 1000 );
}

void AutodeskBurnBurner::slotReadLog()
{
	while( mLogCmd->canReadLine() ) {
		QString line = QString::fromAscii( mLogCmd->readLine() );
		if( checkIgnoreLine( line ) ) continue;
		logMessage( "LOG: "+line, QProcess::StandardOutput );
		slotProcessOutputLine( line, QProcess::StandardOutput );
	}
}

void AutodeskBurnBurner::slotProcessOutputLine( const QString & line, QProcess::ProcessChannel channel )
{
	QRegExp frameComplete("Processing frame (\\d+) of output '\\w+'.");
	bool frameCompleteLine = line.contains( frameComplete );

	bool complete = line.contains( mCompleteRE );

	JobBurner::slotProcessOutputLine( line, channel );
}

void AutodeskBurnBurner::cleanup()
{
	if( mCmd && mCmd->state() != QProcess::NotRunning )
		mCmd->kill();

	JobBurner::cleanup();
}

#endif
