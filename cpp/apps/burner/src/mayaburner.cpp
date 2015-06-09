
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

#ifdef COMPILE_MAYA_BURNER

#include <qprocess.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qfile.h>
#include <qfileinfo.h>

#include "path.h"
#include "process.h"

#include "jobmaya.h"
#include "jobtype.h"
#include "jobenvironment.h"
#include "user.h"

#include "config.h"
#include "mayaburner.h"
#include "slave.h"
#include "project.h"

#ifdef Q_OS_WIN
#include "windows.h"
#endif // Q_OS_WIN

MayaBurner::MayaBurner( const JobAssignment & jobAssignment, Slave * slave )
: JobBurner( jobAssignment, slave )
, mCmdString("")
, mFrame( 0 )
, mErrorOutput("")
, mCompleteRE("^(M2AB: completed job)|(Scene .* completed.)")
, mAssignedRE("^(M2AB: starting frame)|(Starting Rendering)")
, mFrameCompleteRE("^(M2AB: completed frame)|(Finished Rendering)")
{
	//Error: (Mayatomr.Geometry) : Basket_GRP2|group30|frame|pCube2|pCubeShape2: no material assigned, ignored
	mIgnoreREs += QRegExp("^Error.*ignored");

	if( mJob.jobType().name().contains("MentalRay") ) {
		//Prog: (Mayatomr.Scene) : DAG node: football_stadium_01:light_box8|football_stadium_01:case1|football_stadium_01:light_case12
		mIgnoreREs += QRegExp("Prog: .Mayatomr.Scene. : DAG node:");
		//shader "mi_metallic_paint" called mi_sample_light with unknown light tag
		mIgnoreREs += QRegExp("shader .* called mi_sample_light with unknown light tag");
	}

	mErrorREs += QRegExp("^Error:.*");
	mErrorREs += QRegExp("Memory exception");
	mErrorREs += QRegExp("could not get a license");
	mErrorREs += QRegExp("Rendering aborted.");
	mErrorREs += QRegExp("maya encountered a fatal error");
	//Warning: Texture file /mnt/users/manny/MARTIAL_ARTS/Character/sourceimages_lo/actual_iff_1024/armC.iff doesn't exist
	mErrorREs += QRegExp("Warning: Texture file .* doesn't exist");
	//Warning: This scene does not have any renderable cameras. The renderable status can be set in th
	//e camera attribute editor.
	mErrorREs += QRegExp("Warning: This scene does not have any renderable cameras");
}

MayaBurner::~MayaBurner()
{
}

QString MayaBurner::buildCmdMaya7()
{
	JobMaya jm( mJob );
	QString cmd;
	mFrame = assignedTasks().section("-",0,0).toInt();
	cmd += " -s " + QString::number(mFrame);

	QString frameEnd = assignedTasks().section("-",1,1);
	if( frameEnd.isEmpty() )
		cmd += " -e " + QString::number(mFrame);
	else
		cmd += " -e " + frameEnd;

    cmd += " -n " + QString::number( mJob.assignmentSlots() );
	if( jm.width() > 0 )
		cmd += " -x " + QString::number(jm.width());
	if( jm.height() > 0 )
		cmd += " -y " + QString::number(jm.height());
	if( !jm.projectPath().isEmpty() )
		cmd += " -proj " + jm.projectPath();
	if( !jm.camera().isEmpty() )
		cmd += " -cam " + jm.camera();
	if( !jm.append().isEmpty() )
		cmd += " "+ jm.append();
	cmd += " -rd " + Path(jm.outputPath()).dirPath();
	cmd += " -r " + rendererFlag(jm.renderer());
	cmd += " -im '%s%_l%.4n%.e'";
	cmd += " " + jm.fileName();
	return cmd;
}

QString MayaBurner::buildCmdMaya() const
{
	JobMaya jm( mJob );
	QString cmd;

	cmd += " -s " + QString::number(mFrame);

	QString frameEnd = assignedTasks().section("-",1,1);
	if( frameEnd.isEmpty() )
		cmd += " -e " + QString::number(mFrame);
	else 
		cmd += " -e " + frameEnd;

    cmd += " -rt " + QString::number( mJob.assignmentSlots() );
	if( jm.width() > 0 )
		cmd += " -x " + QString::number(jm.width());
	if( jm.height() > 0 )
		cmd += " -y " + QString::number(jm.height());
	cmd += " -rd " + Path(jm.outputPath()).dirPath();
	if( !jm.projectPath().isEmpty() && !jm.append().contains("-proj") )
		cmd += " -proj " + jm.projectPath();
	if( !jm.camera().isEmpty() )
		cmd += " -cam " + jm.camera();
	if( !jm.append().isEmpty() )
		cmd += " "+ jm.append();
	cmd += " -renderer " + rendererFlag(jm.renderer());

	if( !jm.append().isEmpty() && jm.append().contains("defaultRenderLayer") )
		cmd += " -im %s%.4n%.e";
	else
		cmd += " -im %s%_l%.4n%.e";
	cmd += " " + jm.fileName();
	return cmd;
}

QString MayaBurner::rendererFlag(const QString & renderer) const
{
	if( renderer == "Maya" )
		return "sw";
	else if( renderer == "MentalRay" )
		return "mr";
	else
		return "sw";
}

QStringList MayaBurner::processNames() const
{
    return QStringList() << "maya.bin" << "maya";
}

QString MayaBurner::executable() const
{
	QString ret;
	QString jt = mJob.jobType().name(), key;
	if( jt == "Maya7" )
		ret = "su - "+mJob.user().name()+" -c \"/usr/aw/maya7.0/bin/Render ";
	else if ( jt == "Maya85" )
		ret = "su - "+mJob.user().name()+" -c \"/usr/aw/maya8.5/bin/Render ";
	else if ( jt == "Maya2008" )
		ret = "su - "+mJob.user().name()+" -c \"/usr/autodesk/maya2008-x64/bin/Render ";
	else
		ret = "su "+mJob.user().name()+" -c \"Render ";
	return ret;
}

void MayaBurner::startProcess()
{
	LOG_5( "starting" );

	QString jt = mJob.jobType().name();
	mCmdString = executable();

	QStringList env = environment();

	if( jt == "Maya7" ) {
		mCmdString += buildCmdMaya7();
		mCmdString += "\"";
		env << "MAYA_LOCATION=/usr/aw/maya7.0";
		env.replaceInStrings(QRegExp("^LD_LIBRARY_PATH=(.*)", Qt::CaseInsensitive), "LD_LIBRARY_PATH=\\1;/usr/aw/maya7.0/lib");
	 } else if ( jt == "Maya85" ) {
		mCmdString += buildCmdMaya();
		mCmdString += "\"";
		env << "MAYA_LOCATION=/usr/aw/maya8.5"; // Add an environment variable
		env.replaceInStrings(QRegExp("^LD_LIBRARY_PATH=(.*)", Qt::CaseInsensitive), "LD_LIBRARY_PATH=\\1;/usr/aw/maya8.5/lib");
	} else {
		mFrame = assignedTasks().section("-",0,0).toInt();
		mCmdString += buildCmdMaya();
		mCmdString += "\"";
	}

#ifdef USE_TIME_WRAP
	QString timeCmd = "/usr/bin/time --format=baztime:real:%e:user:%U:sys:%S:iowait:%w ";
	mCmdString = timeCmd + mCmdString;
#endif

	mCmd = new QProcess( this );
	connectProcess( mCmd );

	mJobAssignment.setCommand(mCmdString);

    //logMessage( "Environment: " + env.join("\n") );
	logMessage( "MB: Starting Cmd: " + mCmdString );
	mCmd->setEnvironment(env);
	mCmd->start( mCmdString );

	mCheckupTimer->start( 30 * 1000 );
}

void MayaBurner::slotProcessOutputLine( const QString & line, QProcess::ProcessChannel channel )
{
	// check for Errors first
	JobBurner::slotProcessOutputLine( line, channel );

	if( line.contains( mAssignedRE ) ) {
		logMessage( "MAYA: Starting frame: " + QString::number(mFrame) );
		taskStart( mFrame );
	}
	else if( line.contains( mCompleteRE ) ) {
		logMessage("MAYA: jobFinished()");
#ifndef Q_OS_WIN
		// sleep here cause Maya likes to dump memory reports and we don't want to force cleanup too early
		sleep(2);
#endif
		jobFinished();
	}
	else if( line.contains( mFrameCompleteRE ) ) {
		if( taskStarted() ) {
			QString framePath = makeFramePath( mJob.outputPath(), mFrame );
			logMessage("Checking for frame at " + framePath);
			QFileInfo fi( framePath );
			if( fi.exists() && fi.isFile() ) {
				emit fileGenerated( framePath );
				emit taskDone( mFrame );
				logMessage( "MAYA: Completed frame: " + QString::number(mFrame) );

				if (mJob.project().name() == "smallville" || mJob.project().name() == "harpers" ) {
					syncFile(framePath);
				}

				int frameNth = mJob.getValue("frameNth").toInt(), frameEnd = mJob.getValue("frameEnd").toInt();
				// Nth Frame Copy
				if( frameNth > 0 && mFrame < frameEnd )
					copyFrameNth( mFrame, frameNth, frameEnd );
				mFrame++;
			} else {
				mErrorOutput += framePath +" should exist, but dosn't; sucks!";
				jobErrored( mErrorOutput );
			}
		}
	}
}

void MayaBurner::cleanup()
{
#ifndef Q_OS_WIN
	// sleep here so we don't stomp on exiting maya's own cleanup
	sleep(2);
#endif

	JobBurner::cleanup();
}

QStringList MayaBurner::environment()
{
	if( mJob.environment().environment().isEmpty() )
		return QStringList();
	else
		return mJob.environment().environment().split("\n");
}

#endif
