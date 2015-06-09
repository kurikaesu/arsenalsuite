
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

#ifdef COMPILE_MAXSCRIPT_BURNER

#include <qprocess.h>
#include <qtimer.h>
#include <qregexp.h>
#include <qfile.h>
#include <qfileinfo.h>

#include "path.h"
#include "process.h"

#include "jobmaxscript.h"
#include "jobservice.h"

#include "config.h"
#include "maxscriptburner.h"
#include "slave.h"


// TODO -- MSB should remove/cleanup the unzip of the job after its done

MaxScriptBurner::MaxScriptBurner( const JobAssignment & jobAssignment, Slave * slave )
: JobBurner( jobAssignment, slave )
, mStatusLine( 0 )
{
}

MaxScriptBurner::~MaxScriptBurner()
{
}

QStringList MaxScriptBurner::processNames() const
{
    return QStringList() << "3dsmax.exe";
}

QString MaxScriptBurner::runScriptDest()
{
	return maxDir() + "/scripts/startup/runScriptJob.ms";
}

QString MaxScriptBurner::maxDir()
{
	ServiceList sl = mJob.jobServices().services().filter( "service", QRegExp("^MAX\\d") );
	if( sl.size() != 1 ) {
		jobErrored( "Couldn't find valid max service" );
		return QString();
	}
	QString configName = "slave" + sl[0].service().replace("MAX","Max") + "Dir";
	Config cfg = Config::recordByName( configName );
	if( !cfg.isValid() ) {
		jobErrored( "Couldn't find config key for " + configName );
		return QString();
	}
	return cfg.value();
}

void MaxScriptBurner::cleanup()
{
	LOG_5("MSB::cleanup() called");
	// We don't want this file hanging around, it keeps max7burner
	// from working.
	if( QFile::exists( mRunScriptDest ) && !QFile::remove( mRunScriptDest ) ) {
		LOG_1( "MSB: Couldn't remove runscript file at: " + mRunScriptDest );
	}

	JobBurner::cleanup();

	killAll("3dsmax.exe", 10);
	LOG_5("MSB::cleanup() done");
}

void MaxScriptBurner::startProcess()
{
	LOG_5("MSB::startBurn() called");
	JobMaxScript jms( mJob );

	mStatusFile = burnDir() + "\\scriptstatus.txt";

	// Check for error after calling burnDir, in case the services aren't setup properly
	if( state() == StateError ) return;

	QString runScriptSrc = "runScriptJob.ms";

	if( !QFile::exists( runScriptSrc ) ) {
		jobErrored( "MSB: Missing file required for maxscript jobs: " + runScriptSrc );
		return;
	}

	mRunScriptDest = runScriptDest();
	QFile::remove( mRunScriptDest );

	{
		QFile src( runScriptSrc );
		if( !src.open( QIODevice::ReadOnly ) ) {
			jobErrored( "MSB: Couldnt open " + runScriptSrc + " for reading" );
			return;
		}
		QTextStream ts( &src );
		QString txt = ts.readAll();
		txt.replace( "%BURN_DIR%", burnDir() + "/" );
		txt.replace( "%STATUS_FILE%", mStatusFile );
		txt.replace( "%JOB_PARENT_ID%", QString::number( jms.key() ) );
		QFile dest( mRunScriptDest );
		if( !dest.open( QIODevice::WriteOnly ) ) {
			jobErrored( "MSB: Couldnt open " + mRunScriptDest + " for writing" );
			return;
		}
		QTextStream os( &dest );
		os << txt;
	}
	
	// Make sure there are no other 3dsmaxcmd.exe or 3dsmax.exe running
	if( !killAll( "3dsmaxcmd.exe", 10 ) || !killAll( "3dsmax.exe", 10 ) ) {
		jobErrored( "MSB: Couldn't kill existing 3dsmax processes" );
		return;
	}

	// Make sure there are no '3ds Max Error Report' dialogs up
	if( !killAll( "SendDmp.exe", 10) ) {
		jobErrored( "M7B: Couldn't kill '3ds Max Error Report' dialog" );
		return;
	}


	// Write frame list file for maxscript to read
	{
		bool validNumberList = true;
		QList<int> frames = expandNumberList( assignedTasks(), &validNumberList );

		if( !validNumberList ) {
			jobErrored( "MSB: Invalid number list: " + assignedTasks() );
			return;
		}

		QStringList sl;
		foreach( int frame, frames )
			sl += QString::number( frame );

		QFile frameFile( "c:/temp/runscriptframes.txt" );
		if( !frameFile.open( QIODevice::WriteOnly ) ) {
			jobErrored( "MSB: Unabled to write to frames file at: c:/temp/runscriptframes.txt" );
			return;
		}
	
		QTextStream ts(&frameFile);
		ts << sl.join(",");
		frameFile.close();
	}
	
	QFile::remove( mStatusFile );
	
	QString cmd = maxDir() + "\\3dsmax.exe";
	if( !QFile::exists( cmd ) ) {
		jobErrored( "MSB: Couldn't find 3dsmax.exe at: " + cmd );
		return;
	}

	mCmd = new QProcess();

	if( jms.silent() )
		cmd += " -silent";

	connectProcess( mCmd );
	
	mJobAssignment.setCommand( cmd );
	mCmd->start( cmd );

	mCheckupTimer->start( 200 );

	LOG_5("MSB::startProcess() done");
}

// Returns true if the job is done
bool MaxScriptBurner::checkup()
{
	LOG_6("MSB::checkup() called");
	if( !JobBurner::checkup() )
		return false;

	if( !QFile::exists( mStatusFile ) )
		return false;

	QFile status( mStatusFile );
	if( !status.open( QIODevice::ReadOnly ) ) {
		jobErrored( "MSB: Couldn't open status file for reading: " + mStatusFile );
		return false;
	}

	QString line;
	QString errorString;
	int lineNumber = 0;

	QTextStream inStream( &status );
	
	while( !inStream.atEnd() ) {
		line = inStream.readLine();
		// Keep track of where we're at
		if( lineNumber++ < mStatusLine )
			continue;

		mStatusLine = lineNumber;
		logMessage( "MSB: Read new line: " + line );

		QRegExp starting( "^starting (\\d+)" ), finished( "^finished (\\d+)" ), success( "^success" );
		if( starting.indexIn(line) >= 0 ) {
			int frame = starting.cap(1).toInt();
			taskStart( frame );
		} else if( finished.indexIn(line) >= 0 ) {
			taskDone( finished.cap(1).toInt() );
		} else if( line.contains( success ) ) {
			jobFinished();
			return true;
		} else {
			errorString += line.replace( "\\n", "\n" );
		}
	}

	if( !errorString.isEmpty() ) {
		jobErrored( errorString );
		return false;
	}

/* TODO: Merge killWindows implementation from blur's repo
	// Kill 3dsmax.exe if these windows popup
	QStringList titles, procs;
	titles += "Microsoft Visual C++ Runtime Library";
	titles += "DLL";
	titles += "DbxHost Message";
	titles += "Fatal Error";
	titles += "3ds Max Error Report";
	titles += "glu3D";
	procs += "3dsmax.exe";
	procs += "3dsmaxcmd.exe";
	procs += "SendDmp.exe";
	LOG_6( "Calling killWindows" );
	QString windowFound;
	if( killWindows( titles, procs, &windowFound ) ) {
		jobErrored( "MaxScriptBurner::checkup: Found window: " + windowFound + ", killed processes with names: " + procs.join(", ") );
		return false;
	}
	LOG_6( "killWindows Finished" );
*/

	LOG_6("MSB::checkup() done");
	return true;
}

#endif
