
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

#ifdef COMPILE_AFTER_EFFECTS_BURNER

#include <qprocess.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>

#include "slave.h"
#include "jobtype.h"
#include "aftereffectsburner.h"
#include "jobaftereffects.h"
#include "config.h"
#include "process.h"
#include "path.h"

#ifdef Q_OS_WIN
#include "windows.h"
#endif // Q_OS_WIN

AfterEffectsBurner::AfterEffectsBurner( const Job & job, Slave * slave )
: JobBurner( job, slave, OptionMergeStdError )
, mFrame( 0 )
, mCompleteRE("Total Time Elapsed")
, mAssignedRE("")

// PROGRESS:  (Skipping 0;00;19;04) (526): 0 Seconds
// cap(1) == "Skipping", cap(2) == "526"

// PROGRESS:  0;00;26;20 (752): 0 Seconds
// cap(1) == "", cap(2) == "752"

// PROGRESS: (Skipping 000+03) (4): 0 Seconds
// cap(1) == "Skipping", cap(2) == "4"
, mFrameCompleteRE("PROGRESS:\\s+\\(?(Skipping)?\\s*[\\d;+]*\\)?\\s*\\((\\d+)\\):")
, mErrorOutput("")
{
	mErrorREs += QRegExp("Error: ");
	mErrorREs += QRegExp("ERROR: ");
	mErrorREs += QRegExp("The directory originally specified by an output module for this render item no longer exists.");
	// INFO:This project contains 1 reference to a missing effect. Please install the following effect to restore this reference. (Unmult)
	mErrorREs += QRegExp("INFO:This project contains \\d+ reference");
}

AfterEffectsBurner::~AfterEffectsBurner()
{
}

QStringList AfterEffectsBurner::buildCmdArgs()
{
	JobAfterEffects j( mJob );
	QStringList args;
	mFrame = assignedTasks().section("-",0,0).toInt();
	mFrameEnd = assignedTasks().section("-",1,1).toInt();

	args << "-s" << QString::number(mFrame) << "-e" << QString::number(mFrameEnd > 0 ? mFrameEnd : mFrame);

	args << "-project" << mBurnFile;

	if( !j.comp().isEmpty() )
		args << "-comp" << j.comp();

	//if( mJob.jobType().name() == "AfterEffects8" )
	//	args << "-mp";

	return args;
}

QStringList AfterEffectsBurner::processNames() const
{
    return QStringList() << "aerender";
}

QString AfterEffectsBurner::executable() const
{
	QString jobType = mJob.jobType().name();
	
	// AfterEffects 6.5
	if( jobType == "AfterEffects" ) {
#ifdef Q_OS_WIN
		return Config::getString( "assburnerAfterEffectsExe6.5_win32", "C:/Program Files/Adobe/After Effects 6.5/aerender" );
#endif
#ifdef Q_OS_MAC
		return Config::getString( "assburnerAfterEffectsExe6.5_osx", "sh -c \"/Applications/Adobe\\ After\\ Effects\\ 6.5/aerender" );
#endif
	}

	// AfterEffects 7
	else if( jobType == "AfterEffects7" ) {
#ifdef Q_OS_WIN
		return Config::getString( "assburnerAfterEffectsExe7_win32", "C:/Program Files/Adobe/After Effects 7.0/aerender" );
#endif
#ifdef Q_OS_MAC
		return Config::getString( "assburnerAfterEffectsExe7_osx", "sh -c \"exec /Applications/Adobe\\ After\\ Effects\\ 7.0/aerender" );
#endif
	}
	
	// AfterEffects 7 (CS3)
	else if( jobType == "AfterEffects8" ) {
#ifdef Q_OS_WIN
		return Config::getString( "assburnerAfterEffectsExe8_win32", "C:/Program Files/Adobe/After Effects CS3/aerender" );
#endif
#ifdef Q_OS_MAC
		return Config::getString( "assburnerAfterEffectsExe8_osx", "sh -c \"exec /Applications/Adobe\\ After\\ Effects\\ CS3/aerender" );
#endif
	}
	
	return "";
}

void AfterEffectsBurner::startProcess()
{
	LOG_5( "AEB::startBurn() starting" );

	QString cmd = executable();
	QStringList args = buildCmdArgs();

	mCmd = new QProcess( this );
	connectProcess( mCmd );
	
	QString outputDir = Path( mJob.outputPath() ).dirPath();
	if( !QFile::exists( outputDir ) ) {
		logMessage( "creating output dir: "+ outputDir );
		QDir(outputDir).mkpath( outputDir );
		QFile::setPermissions( outputDir, QFile::ReadUser|QFile::WriteUser|QFile::ExeUser|QFile::ReadGroup|QFile::WriteGroup|QFile::ExeGroup|QFile::ReadOther|QFile::WriteOther|QFile::ExeOther );
	}

	logMessage( "AEB: Starting Cmd: " + cmd + " " + args.join(" ") );

#ifdef Q_OS_WIN
	mCmd->start( cmd, args );
	mJobAssignment.setCommand(cmd + " " + args.join(" "));
#else
	cmd = cmd + " " + args.join(" ") + "\"";
	mCmd->start( cmd );
	mJobAssignment.setCommand( cmd );
#endif

	mCheckupTimer->start( 15 * 1000 );
	taskStart( mFrame );
}

void AfterEffectsBurner::slotProcessOutputLine( const QString & line, QProcess::ProcessChannel channel )
{
	bool com = line.contains( mCompleteRE );
	bool framecom = line.contains( mFrameCompleteRE );
#ifdef Q_OS_MAC
	bool fatalMachError = line.contains(QRegExp("failed to name Mach port"));
	if( fatalMachError ) {
		logMessage(line);
		jobErrored("the Host has fatal Mach Errors until AB restarts.");
		mSlave->setStatus("restart");
	}
#endif

	if( framecom ) {
		if( taskStarted() ) {
			bool frameCheckFailure = false;
			if( !mJob.outputPath().endsWith( ".avi" ) && !mJob.outputPath().endsWith( ".mov" ) ) {
				QString framePath = makeFramePath( mJob.outputPath(), mFrame );
				QFileInfo fi( framePath );
				frameCheckFailure = !(fi.exists() && fi.isFile() && fi.size() > 0);
				if( !frameCheckFailure )
					emit fileGenerated(framePath);
				QString log = QString("Frame %1 %2").arg(framePath).arg(frameCheckFailure ? "missing" : "found");
				logMessage(log);
			}

			if( frameCheckFailure ) {
				jobErrored("AE: Got frame complete message, but frame doesn't exist or is zero bytes");
				return;
			}

			logMessage("AE: Completed frame: " + QString::number(mFrame));
			taskDone( mFrame );
			mFrame++;
			if( !com && mFrame <= mFrameEnd )
				taskStart( mFrame );
		}
	}

	if( com ) {
		if( mJob.outputPath().endsWith( ".avi" ) || mJob.outputPath().endsWith( ".mov" ) )
			emit fileGenerated(mJob.outputPath());
		LOG_3("AEB::slotReadOutput emit'ing jobFinished()");
		jobFinished();
		return;
	}

	JobBurner::slotProcessOutputLine( line, channel );
}

void AfterEffectsBurner::cleanup()
{
#ifndef Q_OS_WIN
	// sleep here to allow processes time to exit naturally
	sleep(3);
	system("killall -u root aerender");
	system("killall -u root aerendercore");
	system("killall -u root \"After Effects\"");
	system("killall -u root \"After Effects CS3\"");
	// sleep here to allow processes time to die
	sleep(2);
#endif
	JobBurner::cleanup();
}

#endif
