
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

#include <qapplication.h>
#include <qprocess.h>
#include <qtimer.h>
#include <qregexp.h>
#include <qdir.h>

#include "config.h"
#include "database.h"
#include "path.h"
#include "process.h"

#include "hostservice.h"
#include "jobassignmentstatus.h"
#include "joberror.h"
#include "jobtype.h"
#include "jobtypemapping.h"
#include "joboutput.h"
#include "jobcommandhistory.h"
#include "location.h"
#include "mapping.h"
#include "project.h"
#include "service.h"
#include "syslog.h"
#include "jobfilterset.h"
#include "jobfiltertype.h"
#include "jobenvironment.h"

#include "jobburner.h"
#include "slave.h"
#include "spooler.h"

#ifdef Q_OS_WIN
#include "windows.h"
#endif // Q_OS_WIN

JobBurner::JobBurner( const JobAssignment & jobAssignment, Slave * slave, int options )
: QObject( slave )
, mSlave( slave )
, mCmd( 0 )
, mJobAssignment( jobAssignment )
, mJob( jobAssignment.job() )
, mLoaded( false )
, mOutputTimer( 0 )
, mMemTimer( 0 )
, mLogFlushTimer( 0 )
, mCheckupTimer( 0 )
, mState( StateNew )
, mOptions( Options(options) )
, mCurrentCopy( 0 )
, mLogFilesReady( false )
, mLogFile( 0 )
, mLogStream( 0 )
, mCmdPid( 0 )
{
	mOutputTimer = new QTimer( this );
	connect( mOutputTimer, SIGNAL( timeout() ), SLOT( updateOutput() ) );

	mMemTimer = new QTimer( this );
	connect( mMemTimer, SIGNAL( timeout() ), SLOT( checkMemory() ) );

	mCheckupTimer = new QTimer( this );
	connect( mCheckupTimer, SIGNAL( timeout() ), SLOT( checkup() ) );
    mCheckupTimer->start(30000);

	/* Ensure we are actually assigned some tasks to work on */
	mTaskAssignments = mJobAssignment.jobTaskAssignments();
	if( mTaskAssignments.isEmpty() ) {
		jobErrored( QString("JobAssignment has no assigned tasks, Job Assignment key: %1").arg(mJobAssignment.key()) );
		return;
	}

	// Double check that we are still assigned
	mJobAssignment.reload();
	if( mJobAssignment.jobAssignmentStatus().status() != "ready" ) {
		LOG_1( "JobAssignment no longer ready, cancelling the burn" );
		cancel();
		return;
	}
    // get any changed Job params ( like environment, command, etc )
    mJob.reload();

	/* Make sure each of the tasks are still valid, some could have already been unassigned or cancelled */
	/* Also verify that the jobtask record matches the assignment */
	mTasks = JobTask::table()->records( mTaskAssignments.keys( JobTaskAssignment::schema()->field("fkeyjobtask")->pos() ), /*select=*/true, /*useCache=*/false );
	foreach( JobTaskAssignment jta, mTaskAssignments ) {
		JobTask task = jta.jobTask();
		if( jta.jobAssignmentStatus().status() != "ready" || task.status() != "assigned" || task.host() != Host::currentHost() ) {
			LOG_1( QString("JobTask no longer assigned, discarding. keyJobTask: %1  keyJobTaskAssignment: %2  jobtask status: %3 jobtaskassignment status: %4")
				.arg(task.key()).arg(jta.key()).arg(task.status()).arg(jta.jobAssignmentStatus().status()) );
			mTaskAssignments -= jta;
			mTasks -= task;
		}
	}

	if( mTasks.isEmpty() ) {
		LOG_1( "No Tasks still assigned, cancelling the burn" );
		cancel();
		return;
	}

	mTaskList = compactNumberList( mTasks.frameNumbers() );
	//connect( this, SIGNAL( fileGenerated(const QString &) ), SLOT( syncFile(const QString &) ) );

	mProgressRE = QRegExp("\\(?(\\d+)\\%\\)?$");
	mProgressUpdate = QDateTime::currentDateTime();

    mJobFilterMessages = mJob.filterSet().jobFilterMessages();
    mJobFilterMessages.reload();

    foreach( JobFilterMessage jfm, mJobFilterMessages )
        if( jfm.jobFilterType().name() == "Ignore" )
            mIgnoreREs += QRegExp(jfm.regex());
}

JobBurner::~JobBurner()
{
	if( !mSlave->logRootDir().isEmpty() && mLogFile ) {
		mLogFile->flush();
		mLogFile->close();
	}
	delete mCmd;
	delete mOutputTimer;
	delete mMemTimer;
	delete mCheckupTimer;
	delete mLogFlushTimer;
	delete mCurrentCopy;
	delete mLogFile;
	delete mLogStream;
}

JobAssignment JobBurner::jobAssignment() const
{
	return mJobAssignment;
}

QString JobBurner::executable()
{
	jobErrored( "JobBurner::executable() Not Implemented" );
	return QString();
}

QStringList JobBurner::buildCmdArgs()
{
	return QStringList();
}

QStringList JobBurner::environment()
{
	return QStringList();
}

QString JobBurner::workingDirectory()
{
	return QString();
}

void JobBurner::connectProcess( QProcess * process )
{
	mCmd = process;
	connect( process, SIGNAL( readyReadStandardOutput() ), SLOT( slotReadStdOut() ) );
	if( mOptions & OptionMergeStdError ) {
		process->setProcessChannelMode( QProcess::MergedChannels );
	} else {
		connect( process, SIGNAL( readyReadStandardError() ), SLOT( slotReadStdError() ) );
	}
	connect( process, SIGNAL( started() ), SLOT( slotProcessStarted() ) );
	connect( process, SIGNAL( finished(int) ), SLOT( slotProcessExited() ) );
	connect( process, SIGNAL( error( QProcess::ProcessError ) ), SLOT( slotProcessError( QProcess::ProcessError ) ) );
}

void JobBurner::startProcess()
{
	mCmd = new QProcess( this );
	connectProcess( mCmd );
	if( !isActive() ) return;
	QString cmd = executable();
	if( !isActive() ) return;
	if( cmd.isEmpty() ) {
		jobErrored( "Job has no command to run =(" );
		return;
	}
	QStringList args = buildCmdArgs();
	if( !isActive() ) return;
	QString wholeCmd = cmd + " " + args.join(" ");
	mJobAssignment.setCommand( wholeCmd );
	QString wd = workingDirectory();
	if( !isActive() ) return;
	if( !wd.isEmpty() )
		mCmd->setWorkingDirectory( wd );
	QStringList env = environment();
	if( !isActive() ) return;
	LOG_3( "Starting command: " + wholeCmd + (wd.isEmpty() ? QString() : QString(" in directory " + wd)) );
    env << "ARSENAL_JOBKEY=" + QString::number(mJob.key());
	if( !env.isEmpty() ) {
		mCmd->setEnvironment( env );
		LOG_5( "Environment: " + env.join("\n") );
	}

	if( args.isEmpty() )
		mCmd->start( cmd );
	else
		mCmd->start( cmd, args );

    mCmdPid = qprocessId(mCmd);
}

void JobBurner::start()
{
	if( mJob.uploadedFile() )
		startCopy();
	else {
		mBurnFile = mJob.fileName();
		startBurn();
	}
}

void JobBurner::startCopy()
{
	if( !mSlave->spooler() ) {
		jobErrored( "Spooler is missing" );
		return;
	}
	mJobAssignment.setJobAssignmentStatus( JobAssignmentStatus::recordByName( "copy" ) );
	mJobAssignment.commit();
	mCurrentCopy = new SpoolRef(mSlave->spooler()->startCopy( mJob ));
	connect( mCurrentCopy, SIGNAL( completed() ), SLOT( slotCopyFinished() ) );
	connect( mCurrentCopy, SIGNAL( errored( const QString &, int ) ), SLOT( slotCopyError( const QString &, int ) ) );
	connect( mCurrentCopy, SIGNAL( copyProgressChange( int ) ), mSlave, SIGNAL( copyProgressChange( int ) ) );
	mCurrentCopy->start();
}

void JobBurner::slotCopyFinished()
{
	if( mCurrentCopy ) {
		SpoolItem * item = mCurrentCopy->spoolItem();
		mBurnFile = item->isUnzipped() ? item->zipFolder() : item->destFile();
		mCurrentCopy->disconnect( this );
		startBurn();
	} else
		jobErrored( "slotCopyFinished called while mCurrentCopy is null" );
}

void JobBurner::slotCopyError( const QString & error, int )
{
	mCurrentCopy->disconnect( this );
	jobErrored( error );
}

void JobBurner::startBurn()
{
	if( !isActive() ) return;
	QString err;
	if( mSlave->inOwnLogonSession() || mSlave->host().allowMapping() ) {
		MappingList mappings = mJob.jobType().jobTypeMappings().mappings();
		mMappingRef = Win32ShareManager::instance()->map( mappings, mSlave->inOwnLogonSession(), &err );
	
		mSlave->setIsMapped( mMappingRef.isValid() );
	
		// If mapping fails, then we cannot render this job at this time
		// return the job assignment and go to ready state.
		if( mappings.size() && !mMappingRef.isValid() ) {
			jobErrored( "Unable to map drive: " + err );
			return;
		}
	}

	mStart = QDateTime::currentDateTime();
	mLastOutputTime = QDateTime::currentDateTime();

	mJobAssignment.setJobAssignmentStatus( JobAssignmentStatus::recordByName( "busy" ) );
	mJobAssignment.setStarted( mStart );
	// Make sure the fields get set to empty strings(not null), so that a later || works
	mJobAssignment.setStdOut( mStdOut.isNull() ? QString("") : mStdOut );
	mJobAssignment.commit();

	startProcess();
	logMessage( "startBurn: pid: " + QString::number(mCmdPid) );
	updateOutput();

	if( !isActive() ) return;
	mOutputTimer->start( 30000 );
	mMemTimer->start( 10000 );
	updatePriority( mSlave->currentPriorityName() );
}

QStringList JobBurner::processNames() const
{
    return QStringList();
}

bool JobBurner::taskStart( int task, const QString & outputName, int secondsSinceStarted )
{
	mLoaded = true;
	mState = StateStarted;
	mTaskStart = QDateTime::currentDateTime();
	QString query = "fkeyjob=? and jobtask=?";
	VarList vl;
	vl += mJob.key();
	vl += task;

	if( !outputName.isEmpty() ) {
		JobOutput jo = JobOutput::recordByJobAndName( mJob, outputName );
		if( jo.isRecord() ) {
			query += " and fkeyjoboutput=?";
			vl += jo.key();
		}
	}
	mCurrentTasks = JobTask::select( query, vl );
	if( mCurrentTasks.isEmpty() ) {
		LOG_1( QString("JobBurner::slotTaskStart: Couldn't find jobtask record where fkeyjob=%1 and frame=%2").arg(mJob.key()).arg(task) );
		return false;
	}

	mCurrentTaskAssignments = mCurrentTasks.jobTaskAssignments();
	foreach( JobTaskAssignment jta, mCurrentTaskAssignments ) {
		JobTask jt = jta.jobTask();
		if( jt.status() != "assigned" || jt.host() != Host::currentHost() || jta.jobAssignmentStatus().status() != "ready" ) {
			// If we are starting a task that is no longer assigned to us, then some of our
			// tasks have been re-assigned.  We currently are forced to quit and return the remaining
			// frames that are assigned to us, because none of the burners can alter the task
			// list after the first task has been started.  Someday we may be able to do this with
			// certain burners.  But the assignment process will probably be improved by then anyway.
			LOG_1("JobBurner::taskStart() no longer assigned to task, returning");
			jobFinished();
			return false;
		}
	}

	LOG_3( "Starting task " + QString::number(task) + " task ids: " + mCurrentTasks.keyString() );
    Database::current()->beginTransaction();
	mCurrentTasks.setStatuses( "busy" );
	mCurrentTaskAssignments.setJobAssignmentStatuses( JobAssignmentStatus::recordByName("busy") );
	mCurrentTaskAssignments.setColumnLiteral( "started", "now()" + QString(secondsSinceStarted > 0 ? " - '" + QString::number(secondsSinceStarted) + " seconds'::interval" : "") );
	updateOutput();
	mCurrentTaskAssignments.commit();
	mCurrentTasks.commit();
    Database::current()->commitTransaction();
	return true;
}

void JobBurner::taskDone( int task )
{
    if( mState == StateError || mState == StateCancelled ) {
        LOG_3( "JobBurner: ERROR an error has occurred, can't be done" );
        return;
    }

	bool needMemoryCheck = false;
	foreach( JobTaskAssignment jta, mCurrentTaskAssignments )
		if( jta.memory() == 0 ) needMemoryCheck = true;
	if( needMemoryCheck )
		checkMemory();

	LOG_3( "Task Done " + QString::number(task) + " task ids: " + mCurrentTasks.keyString() );
    Database::current()->beginTransaction();
	mCurrentTaskAssignments.setJobAssignmentStatuses( JobAssignmentStatus::recordByName("done") );
	mCurrentTaskAssignments.setColumnLiteral( "ended", "now()" );
	mCurrentTaskAssignments.commit();
	mCurrentTasks.setStatuses( "done" );
	mCurrentTasks.commit();
    Database::current()->commitTransaction();

    mCurrentTaskAssignments.clear();
    mCurrentTasks.clear();

	// Reset this, so that we can keep track of last frame finished until next frame started time
	mTaskStart = QDateTime::currentDateTime();
}

QString JobBurner::assignedTasks() const
{
	return mTaskList;
}

JobTaskList JobBurner::currentTasks() const
{
	return mCurrentTasks;
}

bool JobBurner::taskStarted() const
{
	return mCurrentTasks.size();
}

QDateTime JobBurner::startTime() const
{
	return mStart;
}

QDateTime JobBurner::taskStartTime() const
{
	return mTaskStart;
}

bool JobBurner::exceededMaxTime() const
{
	if( mLoaded )
		return (mJob.maxTaskTime() > 0 && mTaskStart.secsTo( QDateTime::currentDateTime() ) > int(mJob.maxTaskTime()));
	return mJob.maxLoadTime() > 0 && mStart.secsTo( QDateTime::currentDateTime() ) > mJob.maxLoadTime();
}

bool JobBurner::exceededQuietTime() const
{
    return (mJob.maxQuietTime() > 0 && mLastOutputTime.secsTo( QDateTime::currentDateTime() ) > int(mJob.maxQuietTime()));
}

bool JobBurner::checkup()
{
	if( exceededMaxTime() ) {
		// reload and check again in case they changed it
		mJob.reload();
		if( exceededMaxTime() ) {
			QString action( taskStarted() ? "Task" : "Load" );
			QString msg = action + " exceeded max " + action + " time of " + Interval( mLoaded ? mJob.maxTaskTime() : mJob.maxLoadTime() ).toDisplayString();
			jobErrored( msg, /*timeout=*/ true );
			return false;
		}
	}
	if( exceededQuietTime() ) {
		// reload and check again in case they changed it
		mJob.reload();
		if( exceededQuietTime() ) {
			QString msg = "Task exceeded max quiet time of " + Interval( mJob.maxQuietTime() ).toDisplayString();
			jobErrored( msg, /*timeout=*/ true );
			return false;
		}
	}
	return true;
}

JobError JobBurner::logError( const Job & j, const QString & msg, const JobTaskList & tasks, bool timeout )
{
	JobError je;
	Host host( Host::currentHost() );
	// Try to find an existing job error record for this error and host
	{
		VarList v;
		v += j.key();
		v += msg;
		v += host.key();
		JobErrorList jel = JobError::select( "fkeyJob=? and message=? and fkeyhost=? and cleared=false", v );
		if( jel.size() >= 1 )
			je = jel[0];
	}

	je.setJob( j );
	je.setMessage( msg );
	if( tasks.size() )
		je.setFrames( compactNumberList( expandNumberList( je.frames() ) + tasks.frameNumbers() ) );
	je.setColumnLiteral( "lastOccurrence", "now()" );
	je.setErrorTime( QDateTime::currentDateTime().toTime_t() );
	je.setHost( host );
	je.setCount( je.count() + 1 );
	je.setTimeout( timeout );
	je.commit();

	return je;
}

void JobBurner::jobErrored( const QString & msg, bool timeout, const QString & nextstate )
{
	LOG_3( "Got Error: " + msg );
	logMessage( "JobBurner: Got Error: " + msg );
    if( mState == StateError ) {
        LOG_3( "ERROR an error has already occurred" );
        return;
    }
    mState = StateError;

	LOG_3( "firing error slot in 1000 msecs" );
    // delay the actual error firing to grab any final output from the process
    //QTimer::singleShot(1000, this, SLOT(slotJobErrored(msg, timeout, nextstate)));
#ifndef Q_OS_WIN
    sleep(1);
#endif
    slotJobErrored(msg, timeout, nextstate);
}

void JobBurner::slotJobErrored( const QString & msg, bool timeout, const QString & nextstate )
{
    LOG_3( "JobBurner: ERROR! log error and cleanup" );
	// error will be logged by the slave that is monitoring this jobburner
	JobError je = logError( mJob, msg, mCurrentTasks, timeout );

    // an update trigger on JobAssignment will update all JTAs which in turn will
    // reset their respective JobTasks
    mJobAssignment.setJobError( je );
    //mJobAssignment.setJobAssignmentStatus( JobAssignmentStatus::recordByName( "error" ) );
    mJobAssignment.commit();
	Database::current()->exec("SELECT cancel_job_assignment(?,?,?)", VarList() << mJobAssignment.key() << "error" << nextstate );

	cleanup();
	slotReadStdOut();
	slotReadStdError();
	updateOutput();
	emit errored( msg );
}

void JobBurner::cancel()
{
	mState = StateCancelled;
	cleanup();
	updateOutput();
	// This will reset jobtasks and cancel the jobtaskassignments
	Database::current()->exec("SELECT cancel_job_assignment(?,?,?)", VarList() << mJobAssignment.key() << "cancelled" << "new" );
	emit finished();
}

void JobBurner::jobFinished()
{
    if( mState == StateError || mState == StateCancelled ) {
        logMessage( "JobBurner: ERROR an error has occurred, job can't be finished" );
        return;
    }

	if( isActive() )
		mState = StateDone;

	if( mMemTimer )
		mMemTimer->stop();

    mJobAssignment.setJobAssignmentStatus( JobAssignmentStatus::recordByName( "done" ) );
    mJobAssignment.commit();

	cleanup();
	slotReadStdOut();
	slotReadStdError();
	updateOutput();

	emit finished();
}

void JobBurner::cleanup()
{
	if( mCurrentCopy ) {
		delete mCurrentCopy;
		mCurrentCopy = 0;
	}

	if( mCheckupTimer )
		mCheckupTimer->stop();
	if( mOutputTimer )
		mOutputTimer->stop();

	if( mCmd ) {
		mCmd->disconnect( this );
		// if it is Done let the process exit naturally
		if( mCmd->state() != QProcess::NotRunning && mState != StateDone ) {
            if (qprocessId(mCmd) > 1) {
                QList<int> descendants = processChildrenIds( qprocessId(mCmd), true );
                foreach( int processId, descendants ) {
                    logMessage( "JobBurner::cleanup() Killing child pid: "+ processId );
                    if( processId > 1 )
                        killProcess( processId );
                }
            }
			logMessage("attempting to kill process "+ QString::number(qprocessId(mCmd)));
			mCmd->kill();
		}
	}

	slotReadStdOut();
	slotReadStdError();
	updateOutput();

	mJobAssignment.commit();
}

void JobBurner::updateOutput()
{
	if( !mJob.loggingEnabled() ) return;

	if( mSlave->logRootDir().isEmpty() ) {
		// Append fields to limit amount of data uploaded
		VarList vl;
		QStringList colUpdates;
		if( !mStdOut.isEmpty() ) {
			vl += mStdOut;
			colUpdates += "\"stdout\" = \"stdout\" || E?";
		}
		if( !mStdErr.isEmpty() ) {
			vl += mStdErr;
			colUpdates += "\"stderr\" = \"stderr\" || E?";
		}
		if( colUpdates.size() ) {
			vl += mJobAssignment.key();
			Database::current()->exec( "UPDATE jobassignment SET " + colUpdates.join(", ") + " WHERE keyjobassignment = ?", vl );
		}
	} else {
		// write to log file
		if(!mLogFilesReady)
			openLogFiles();
        *mLogStream << mStdOut.toUtf8();
	}

	mStdOut.clear();
	mStdErr.clear();
}

void JobBurner::checkMemory()
{
	if( mCmd ) {
		//LOG_5( "Checking memory for pid: " + QString::number(mCmdPid) );
		ProcessMemInfo pmi = processMemoryInfo( mCmdPid, /*recursive=*/ true );
		uint mem = 0, maxMem = 0;
		if( pmi.caps & ProcessMemInfo::CurrentSize )
			mem = pmi.currentSize;
		maxMem = qMax(mem,mJobAssignment.maxMemory());

		LOG_3("process "+QString::number(mCmdPid)+" is using memory: "+QString::number(maxMem) + "Kb");
		mJobAssignment.setMaxMemory(maxMem);
		if( !mCurrentTaskAssignments.isEmpty() ) {
			mCurrentTaskAssignments.setMemories(mem);
			mCurrentTaskAssignments.commit();
		}

        uint memoryLimit = mJob.maxMemory();
        if( mJobAssignment.assignMaxMemory() > 0 )
            memoryLimit = mJobAssignment.assignMaxMemory();

		if( memoryLimit > 0 && mem > memoryLimit ) {
			// reload and check again in case they changed it
			mJob.reload();
			if( mJob.maxMemory() > 0 && mem > mJob.maxMemory() ) {
                SystemMemInfo mi = systemMemoryInfo();
                float freeMem = mi.freeMemory + mi.cachedMemory;
                if( freeMem / (float)(mi.totalMemory) < 0.10 ) {
                    logMessage("process table at time of error: ");
                    logMessage(backtick("ps auxwww"));

                    QString msg = QString("Process exceeded max memory of %1 at %2, with %3 free")
                        .arg(mJob.maxMemory())
                        .arg(mem)
                        .arg(freeMem);
                    jobErrored( msg );
                }
            }
		}

#ifdef USE_ACCOUNTING_INTERFACE
        updateAssignmentAccountingInfo();
#endif

        // update the Hosts available memory ( it might have gone down )
        mSlave->setAvailableMemory();
	}

	// check 10 seconds in, and then every N seconds ( default 60 )
	if( mMemTimer->interval() == 10000 )
		mMemTimer->setInterval( mSlave->memCheckPeriod() * 1000);
}

void JobBurner::logMessage( const QString & msg, QProcess::ProcessChannel channel )
{
	QString ts = QDateTime::currentDateTime().toString("hh:mm:ss") + ": ";
	QStringList lines = msg.split("\n");
	foreach( QString line, lines ) {
		if( line.trimmed().isEmpty() ) continue;
		//foreach( QRegExp re, mIgnoreREs )
		//	if( line.contains( re ) )
		//		continue;
		QString log = ts + line + "\n";
		if( mJob.loggingEnabled() ) {
			if( channel == QProcess::StandardOutput )
				mStdOut += log;
			else
				mStdErr += log;
		}
		LOG_3( QString(channel == QProcess::StandardOutput ? "STDOUT: " : "STDERR: ") + line );
	}
}

void JobBurner::syncFile( const QString & path )
{
	QFile::setPermissions( path, QFile::ReadUser | QFile::WriteUser | QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther | QFile::WriteOther );

	// copy this file to all locations that the Host isn't in	
	// this is a first pass to get things working, and will
	// make it more generic with some refactoring
	Host curHost = Host::currentHost();

	QString destPath(path);
	if( curHost.location().name() == "VCO" ) {
		destPath = "rsync://pollux/root";
	} else if ( curHost.location().name() == "SMO" ) {
		destPath = "rsync://northlic01/root";
	}
	QString cmd = "rsync -a --numeric-ids --relative "+path+" "+destPath;
	logMessage("syncFile(): "+cmd);
	QString result = backtick(cmd);
	logMessage("syncFile(): "+result);
}

void JobBurner::copyFrameNth( int frame, int frameNth, int frameRangeEnd )
{
	int frameStart = frame + 1;
	int frameEnd = qMin( frameRangeEnd, frame + frameNth - 1 );

	LOG_3( QString("JB: nth frame is on, copying out dupes %1 to %2").arg(frameStart).arg(frameEnd) );
	QString curFrame = mJob.outputPath();
	QRegExp pe( "^(.*)(\\.\\w\\w\\w)$" );
	if( curFrame.contains( pe ) ) {
		QString path = pe.cap(1), ext = pe.cap(2);
		curFrame = path + QString::number(frame).rightJustified(4,'0') + ext;
		for( int i = frameStart; i<=frameEnd; i++ ) {
			QString newFrame = path + QString::number(i).rightJustified(4,'0') + ext;
			Path::copy( curFrame, newFrame );
			LOG_3( QString("JB: Copying %1 to %2").arg(curFrame).arg(newFrame) );
		}
	} else {
		LOG_3( "JB: Couldn't parse outputpath for nth frame copy: " + mJob.outputPath() );
	}
}

#ifdef Q_OS_WIN
DWORD getPrio( const QString & name )
{
	QString n = name.toLower();
	if( n == "low" || n == "idle" )
		return IDLE_PRIORITY_CLASS;
	else if( n == "belownormal" )
		return BELOW_NORMAL_PRIORITY_CLASS;
	else if( n == "normal" )
		return NORMAL_PRIORITY_CLASS;
	else if( n == "abovenormal" )
		return ABOVE_NORMAL_PRIORITY_CLASS;
	else if( n == "high" )
		return HIGH_PRIORITY_CLASS;
	else if( n == "realtime" )
		return REALTIME_PRIORITY_CLASS;
	return NORMAL_PRIORITY_CLASS;
}
#endif // Q_OS_WIN

void JobBurner::updatePriority( const QString & priorityName )
{
#ifdef Q_OS_WIN
	DWORD prio = getPrio( priorityName);
	foreach( QString processName, processNames() )
		setProcessPriorityClassByName( processName, prio );
#endif
}

void JobBurner::slotReadStdOut()
{
	deliverOutput( QProcess::StandardOutput );
}

void JobBurner::slotReadStdError()
{
	deliverOutput( QProcess::StandardError );
}

bool JobBurner::checkIgnoreLine( const QString & line )
{
	foreach( QRegExp re, mIgnoreREs )
		if( line.contains( re ) )
			return true;
	return false;
}

void JobBurner::deliverOutput( QProcess::ProcessChannel channel )
{
	mCmd->setReadChannel( channel );
	if( mOptions & OptionProcessIOReadAll ) {
		slotProcessOutput( mCmd->readAll(), channel );
	} else {
		while( mCmd->canReadLine() ) {
			QString line = QString::fromAscii( mCmd->readLine() );
			if( checkIgnoreLine( line ) ) continue;
			logMessage( line, channel );
			slotProcessOutputLine( line, channel );
            // No need to process output if the job execution is already done or errored.
            // really?
            // if( mState == StateError || mState == StateDone ) break;
		}
	}
}

void JobBurner::slotProcessOutput( const QByteArray & output, QProcess::ProcessChannel channel )
{
	QStringList lines = QString::fromAscii( output ).split('\n');
	foreach( QString line, lines ) {
		if( !checkIgnoreLine( line ) )
			slotProcessOutputLine( line, channel );
	}
}

void JobBurner::slotProcessOutputLine( const QString & line, QProcess::ProcessChannel )
{
    mLastOutputTime = QDateTime::currentDateTime();

    // if the job supports progress messages, see if there's an update
	if( mJob.hasTaskProgress() && mProgressRE.indexIn(line) >= 0 )
		setProgress( mProgressRE.cap(1).toInt() );

#ifdef USE_TIME_WRAP
	// # baztime:real:%e:user:%U:sys:%S:iowait:%w
	if( line.startsWith("baztime:") ) {
        logMessage("found baztime line, updating accounting info");
		QStringList jobStats = line.split(":");
        // time reports things in decimal seconds, but we want to store
        // them in msecs
		mJobAssignment.setRealtime( int(jobStats[2].toFloat() * 1000) );
		mJobAssignment.setUsertime( int(jobStats[4].toFloat() * 1000) );
		mJobAssignment.setSystime( int(jobStats[6].toFloat() * 1000) );
        //mJobAssignment.setEfficiency( (mJobAssignment.usertime() + mJobAssignment.systime()) / (mJobAssignment.realtime() > 0 ? mJobAssignment.realtime() : 1) );
		mJobAssignment.commit();
	}
#endif

    // check all error messages against current line
	foreach( QRegExp re, mErrorREs ) {
		if( line.contains( re ) ) {
			jobErrored( line );
			return;
		}
    }
    foreach( JobFilterMessage jfm, mJobFilterMessages ) {
        if( jfm.enabled() && line.contains( QRegExp(jfm.regex()) ) ) {
            logMessage( QString("JobBurner: JFM id: %1, type: %2 produced an error with regex %2 ").arg(jfm.key()).arg(jfm.jobFilterType().name()).arg(jfm.regex()));
            if( jfm.jobFilterType().name() == "Error-TaskCancel" )
                jobErrored( line, false, "cancelled" );
            else if( jfm.jobFilterType().name() == "Error-TaskRetry" )
                jobErrored( line, false, "new" );
            else if( jfm.jobFilterType().name() == "Error-JobSuspend" ) {
                jobErrored( line, false, "suspended" );
                //suspend the job itself
            }
            else if( jfm.jobFilterType().name() == "Error-HostOffline" ) {
                jobErrored( line, false, "new" );
                // set host offline
            }
            else if( jfm.jobFilterType().name() == "Notify-Message" ) {
                // is notifying from within a CPP burner possible?
            }
            return;
        }
    }
}

void JobBurner::slotProcessExited()
{
	slotReadStdOut();
	slotReadStdError();
	if( isActive() )
		jobErrored( "process exited unexpectedly" );
}

void JobBurner::slotProcessStarted()
{
}

static QString stringForProcessError( QProcess::ProcessError error )
{
	if( error == QProcess::FailedToStart )
		return "Failed To Start";
	else if( error == QProcess::Crashed )
		return "The process crashed some time after starting successfully.";
	else if( error == QProcess::Timedout )
		return "Timed Out";
	else if( error == QProcess::WriteError )
		return "Error Writing To Process";
	else if( error == QProcess::ReadError )
		return "An error occurred when attempting to read from the process.";
	return "An unknown error occurred.";
}

void JobBurner::slotProcessError( QProcess::ProcessError error )
{
	slotReadStdOut();
	slotReadStdError();
	if( isActive() ) return;
		jobErrored( "Got QProcess error: (error code #" + QString::number( (int)error ) + ") " + stringForProcessError( error ));
}

QString JobBurner::burnDir() const
{
	return QFileInfo( mBurnFile ).path();
}

void JobBurner::slotFlushOutput()
{
    mLogStream->flush();
}

void JobBurner::openLogFiles()
{
    // log file path is:
    // logRootDir / right three digits of job id / job id _ hostname _ unique assignment id . out
    QString logDir = mSlave->logRootDir() + "/" + QString::number(mJob.key()).right(3) + "/" + QString::number(mJob.key());
    QDir dir;
    dir.mkdir(logDir);

    QString stdOutPath = logDir + "/" + QString::number(mJob.key())+"_"+Host::currentHost().name()+"_"+QString::number(mJobAssignment.key()) + ".out";

    LOG_3("opening log file: "+stdOutPath);
    mLogFile = new QFile(stdOutPath);
    if(!mLogFile->open( QIODevice::Append | QIODevice::Text )) {
        LOG_1("error opening log file: "+stdOutPath + " error: "+QString::number(mLogFile->error()));
        return;
    }

    mLogStream = new QTextStream(mLogFile);

    mJobAssignment.setStdOut( stdOutPath );
    mJobAssignment.commit();

    // the log files should be flushed to disk every N seconds
    mLogFlushTimer = new QTimer( this );
    connect( mLogFlushTimer, SIGNAL( timeout() ), SLOT( slotFlushOutput() ) );

    mLogFlushTimer->setInterval(30000);
    mLogFlushTimer->start();

    mLogFilesReady = true;
}

void JobBurner::setProgress(int progress)
{
	mCurrentTasks.setProgresses(progress);
	// limit database update to this field to once per N(30) seconds
	if( mProgressUpdate.secsTo(QDateTime::currentDateTime()) > 29 ) {
		mProgressUpdate = QDateTime::currentDateTime();
		mCurrentTasks.commit();
	}
}

void JobBurner::addAccountingData( const AccountingInfo & info )
{
#ifdef USE_ACCOUNTING_INTERFACE
    mAccountingData = sumAccountingData( mAccountingData, info );
#endif
}

AccountingInfo JobBurner::sumAccountingData( const AccountingInfo & left, const AccountingInfo & right ) const
{
    AccountingInfo ret;
#ifdef USE_ACCOUNTING_INTERFACE
    ret.realTime = left.realTime + right.realTime; //realTime is special case?
    ret.cpuTime = left.cpuTime + right.cpuTime;
    ret.memory = left.memory + right.memory;
    ret.bytesRead = left.bytesRead + right.bytesRead;
    ret.bytesWrite = left.bytesWrite + right.bytesWrite;
    ret.opsRead = left.opsRead + right.opsRead;
    ret.opsWrite = left.opsWrite + right.opsWrite;
    ret.ioWait = left.ioWait + right.ioWait;
#endif
    return ret;
}

AccountingInfo JobBurner::checkResourceUsage()
{
    // for processes that have already ended
    // we initialise our counts to include them
    AccountingInfo ret;
#ifdef USE_ACCOUNTING_INTERFACE
    ret = sumAccountingData( ret, mAccountingData );

    QList<int> descendants = processChildrenIds( mCmdPid, true );
    foreach( int pid, descendants ) {
        QString line = backtick("/usr/local/bin/tasklogger -d -p "+ QString::number(pid));
        AccountingInfo info = mSlave->parseTaskLoggerOutput(line);
        ret = sumAccountingData( ret, info );
    }

    // uh I'm not sure this is quite right. We don't want to sum all
    // children's wallclock time as they overlap, but is this right?
    QString line = backtick("/usr/local/bin/tasklogger -d -p "+ QString::number(mCmdPid));
    AccountingInfo info = mSlave->parseTaskLoggerOutput(line);
    ret = sumAccountingData( ret, info );
    ret.realTime = info.realTime;
#endif

    return ret;
}

void JobBurner::updateAssignmentAccountingInfo()
{
#ifdef USE_ACCOUNTING_INTERFACE
    // simple database field updates
    AccountingInfo info = checkResourceUsage();
    mJobAssignment.setRealtime(info.realTime);
    mJobAssignment.setUsertime(info.cpuTime);
    mJobAssignment.setIowait(info.ioWait);
    mJobAssignment.setBytesRead(info.bytesRead);
    mJobAssignment.setBytesWrite(info.bytesWrite);
    mJobAssignment.setOpsRead(info.opsRead);
    mJobAssignment.setOpsWrite(info.opsWrite);
    //mJobAssignment.setEfficiency(info.cpuTime / (info.realTime > 0 ? info.realTime : 1));
#endif
}

