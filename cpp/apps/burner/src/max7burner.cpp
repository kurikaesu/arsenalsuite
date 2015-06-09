
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

#ifdef COMPILE_MAX7_BURNER

#include <qbuffer.h>
#include <qdir.h>
#include <qprocess.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qfile.h>
#include <qfileinfo.h>

#include "path.h"
#include "process.h"

#include "jobmax.h"
#include "jobtype.h"
#include "jobservice.h"
#include "service.h"

#include "config.h"
#include "slave.h"
#include "max7burner.h"

#ifdef Q_OS_WIN
#include "windows.h"
#endif // Q_OS_WIN

Max7Burner::Max7Burner( const JobAssignment & jobAssignment, Slave * slave )
: JobBurner( jobAssignment, slave )
, mFrame( 0 )
{
	mErrorREs += QRegExp( "Error" );
	mIgnoreREs += QRegExp( "MENTAL\\s+RAY\\s+LOG:.+progr:" );
}

Max7Burner::~Max7Burner()
{}

QString nts( int n ) { return n ? "1" : "0"; }

QString Max7Burner::buildCmd() const
{
	JobMax jm( mJob );
	QString cmd;
	cmd += " -frames:" + assignedTasks().replace( ' ', "" );
	// Should probably warn for this somehow, but it is possible to render fine without this
	// the default output path for the file will be used, and i guess the file could theoretically
	// be setup to not output frames at all
	if( !jm.outputPath().isEmpty() )
		cmd += " -outputName:" + jm.outputPath().replace( "/", "\\\\" );
	cmd += " -height:" + QString::number(jm.flag_h());
	cmd += " -width:" + QString::number(jm.flag_w());
	cmd += " -v:5";
	cmd += " -rfw:1";
	cmd += " -videoColorCheck:" +	nts( jm.flag_xv() );
	cmd += " -force2sided:" 	+	nts( jm.flag_x2() );
	cmd += " -renderHidden:" 	+	nts( jm.flag_xh() );
	cmd += " -atmospherics:" 	+	nts( !jm.flag_xa() );
	cmd += " -superBlack:" 		+	nts( jm.flag_xk() );
	cmd += " -renderFields:" 	+	nts( jm.flag_xf() );
	cmd += " -fieldOrder:" 	+	QString( jm.flag_xo() ? "even" : "odd");
	cmd += " -displacements:" 	+	nts( !jm.flag_xd() );
	cmd += " -effects:" 		+	nts( !jm.flag_xe() );
	cmd += " -ditherPaletted:" +	nts( jm.flag_xp() );
	cmd += " -ditherTrueColor:"+	nts( jm.flag_xc() );
	return cmd;
}

QStringList Max7Burner::processNames() const
{
    return QStringList() << "3dsmax.exe" << "3dsmaxcmd.exe";
}

QString Max7Burner::executable() const
{
	return maxDir() + "\\3dsmaxcmd.exe" + buildCmd();
}

QString Max7Burner::startupScriptDest()
{
	return maxDir() + "/scripts/startup/assburnerStartupScript.ms";
}

QString Max7Burner::maxDir() const
{
	QString jt = mJob.jobType().name();
	Service maxService = mJob.jobServices().services().filter( "service", QRegExp("^MAX\\d") )[0];
	if( maxService.isRecord() && maxService.service().endsWith( "_64" ) )
		jt += "_64";
	return Config::getString( "slave" + jt + "Dir" );
}

void Max7Burner::startProcess()
{
	LOG_5( "starting" );

	// Make sure there are no other 3dsmaxcmd.exe or 3dsmax.exe running
/*	if( !killAll( "3dsmaxcmd.exe", 10 ) || !killAll( "3dsmax.exe", 10 ) ) {
		jobErrored( "M7B: Couldn't kill existing 3dsmax processes" );
		return;
	} */

	// Make sure there are no '3ds Max Error Report' dialogs up
	if( !killAll( "SendDmp.exe", 10) ) {
		jobErrored( "M7B: Couldn't kill '3ds Max Error Report' dialog" );
		return;
	}

	QString cmd = executable();

	generateStartupScript();

	cmd += " " + mBurnFile.replace( "/", "\\\\" );

	mJobAssignment.setCommand( cmd );

	mCmd = new QProcess( this );
	connectProcess(mCmd);
	
	LOG_3( "M7B: Starting Cmd: " + cmd );
	mCmd->start( cmd );

	mCheckupTimer->start( 3000 );
}

void Max7Burner::slotProcessOutputLine( const QString & line, QProcess::ProcessChannel channel )
{
	QRegExp complete( "Job Complete - Results in" ), assigned( "Frame (\\d+) assigned" ), framecomplete( "Frame (\\d+) completed" );
	JobMax j( mJob );

	bool ass = assigned.indexIn(line) >= 0;
	bool com = complete.indexIn(line) >= 0;
	bool framecom = framecomplete.indexIn(line) >= 0;

	// Certain versions of max will not report finished frames,
	// they will just report that the next is started, so we
	// assume the current frame has finished when we get
	// frame finished, frame started, or job complete
	if( ass || com || framecom ) {
		if( taskStarted() ) {
			taskDone( mFrame );

			// Nth Frame Copy
			if( j.frameNth() > 0 && mFrame < (int)j.frameEnd() )
				copyFrameNth( mFrame, j.frameNth(), (int)j.frameEnd() );
		}
		if( ass ) {
			mFrame = assigned.cap( 1 ).toInt();
			taskStart( mFrame );
		}
		if( com )
			jobFinished();
		return;
	}

	JobBurner::slotProcessOutputLine( line, channel );
}

void Max7Burner::slotProcessStarted()
{
	mProcessId = qprocessId(mCmd);
}

bool Max7Burner::checkup()
{
	if( !JobBurner::checkup() )
		return false;

/*	// Kill 3dsmax.exe if these windows popup
	QStringList titles, procs;
	titles += "Microsoft Visual C++ Runtime Library";
	titles += "DLL";
	titles += "DbxHost Message";
	titles += "Fatal Error";
	titles += "3ds Max Error Report";
	titles += "glu3D";
	titles += "PNG Plugin";
	titles += "Brazil r/s Error";
	titles += "command line renderer";
	procs += "3dsmax.exe";
	procs += "3dsmaxcmd.exe";
	procs += "SendDmp.exe";
	LOG_6( "Calling killWindows" );
	QString windowFound;
	if( killWindows( titles, procs, &windowFound ) )
		jobErrored( "M7B::checkup: Found window: " + windowFound + ", killed processes with names: " + procs.join(", ") );
*/
	return true;
}

void Max7Burner::cleanupTempFiles()
{
	QRegExp tempFileDelMatch( "(\\.tmp|\\.ac\\$|^cr(\\d+)|^st(\\d+)|^mzptmp(\\d+))$" );
	QFileInfoList fil = QDir( "C:/Documents And Settings/" ).entryInfoList( QDir::Dirs | QDir::NoDotAndDotDot );
	foreach( QFileInfo fi, fil ) {
		LOG_5( "Searching dir " + fi.path() + "/Temp/ for temp files to delete" );
		QDir tempDir( fi.path() + "/Temp/" );
		QFileInfoList tempFileList = tempDir.entryInfoList( QDir::Files );
		foreach( QFileInfo tempFile, tempFileList ) {
			if( tempFile.fileName().contains( tempFileDelMatch ) ) {
				LOG_5( "Deleting temp file: " + tempFile.filePath() );
				QFile::remove( tempFile.filePath() );
			}
		}
	}
}

void Max7Burner::cleanup()
{
	JobBurner::cleanup();
	
	QList<qint32> descendants = processChildrenIds( mProcessId, /* recursive = */ true );
	foreach( qint32 processId,descendants )
		killProcess( processId );
 
	if( !mStartupScriptPath.isEmpty() )
		QFile::remove( mStartupScriptPath );

	cleanupTempFiles();
}


bool Max7Burner::generateStartupScript()
{
	JobMax jm(mJob);
	QString path = maxDir() + "/scripts/startup/assburnerStartupScript.ms";
	QBuffer buffer;
	buffer.open( QIODevice::WriteOnly );

	{
		// Scrope for startupScript for proper flushing
		QTextStream startupScript( &buffer );
	
		if( mJob.fileName().right(4) == ".zip" ) {
			// Set sysinfo.currentdir for renderballs
			// so that the point cache files can be found
			startupScript << ("sysinfo.currentdir = \"" + path.replace("/","\\\\") + "\"\n");
			mBurnFile += "/maxhold.mx";
		}
		
		if( jm.outputPath().endsWith( "exr", Qt::CaseInsensitive ) ) {

			startupScript << "function restoreExrSettings =\n(\n\n";

			startupScript << "\tMaxOpenExr.SetSaveUseDefaults false\n\n";

			if( !jm.exrChannels().isEmpty() ) {
				startupScript << "\tMaxOpenEXR.RemoveAllChannels()\n";
				int channel = 1;
				foreach( QString exrChannel, jm.exrChannels().split(";") ) {
					QStringList parts = exrChannel.split(",");
					if( parts.size() == 4 ) {
						startupScript << QString( "\tMaxOpenEXR.AddChannel %1\n" ).arg(parts[0]);
						startupScript << QString( "\tMaxOpenEXR.SetChannelActive %1 %2\n" ).arg(channel).arg(parts[1]);
						startupScript << QString( "\tMaxOpenEXR.SetChannelFileTag %1 \"%2\"\n" ).arg(channel).arg(parts[2]);
						startupScript << QString( "\tMaxOpenEXR.SetChannelDataType %1 %2\n" ).arg(channel).arg(parts[3]);
						channel++;
					}
				}
				startupScript << "\n";
			}
		
			if( !jm.exrAttributes().isEmpty() ) {
				startupScript << "\tMaxOpenEXR.RemoveAllAttribs()\n";
				int channel = 1;
				foreach( QString exrAttrib, jm.exrAttributes().split(";") ) {
					QStringList parts = exrAttrib.split(",");
					if( parts.size() == 4 ) {
						startupScript << QString( "\tMaxOpenEXR.AddAttrib %1\n" ).arg(parts[0]);
						startupScript << QString( "\tMaxOpenEXR.SetAttribActive %1 %2\n" ).arg(channel).arg(parts[1]);
						startupScript << QString( "\tMaxOpenEXR.SetAttribFileTag %1 \"%2\"\n" ).arg(channel).arg(parts[2]);
						startupScript << QString( "\tMaxOpenEXR.SetAttribContent %1 \"%2\"\n" ).arg(channel).arg(parts[3]);
						channel++;
					}
				}
				startupScript << "\n";
			}

			startupScript << "\tMaxOpenExr.SetSaveBitDepth " << jm.exrSaveBitDepth() << "\n";

			// This applies the exr settings changes
			startupScript << "\trof = rendOutputFilename\n";
			startupScript << "\tprint (\"Reloading Output Settings: \" + rof)\n";
			startupScript << "\trendOutputFilename = rof\n)\n\n";
			startupScript << "callbacks.addScript #filePostOpen \"restoreExrSettings()\"\n";
			
		}

		startupScript << jm.startupScript();
	}

	if( buffer.size() ) {
		logMessage( "Writing startup script to " + path );
		logMessage( "Contents:\n" + QString::fromUtf8(buffer.data()) + "\n\n" );
		QFile preRenderScriptFile( path );
		if( !preRenderScriptFile.open( QIODevice::WriteOnly ) ) {
			jobErrored( "Unable to write startup script to " + path );
			return false;
		}
		preRenderScriptFile.write( buffer.data() );
		preRenderScriptFile.close();
		mStartupScriptPath = path;
		return true;
	}

	return false;
}

#endif
