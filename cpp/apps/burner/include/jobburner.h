
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

#ifndef JOB_BURNER_H
#define JOB_BURNER_H

#include <qobject.h>
#include <qpointer.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qmap.h>
#include <qprocess.h>

#include "blurqt.h"

#include "job.h"
#include "jobassignment.h"
#include "jobtaskassignment.h"
#include "joberror.h"
#include "jobtask.h"
#include "jobfiltermessage.h"

#include "spooler.h"
#include "slave.h"
#include "win32sharemanager.h"

class Slave;
class QTimer;
class QFile;
class QTextStream;

/// \ingroup ABurner
/// @{

class JobBurner : public QObject
{
Q_OBJECT
public:
	/// created by the Slave
	JobBurner( const JobAssignment & jobAssignment, Slave * slave, int options = OptionMergeStdError );
	virtual ~JobBurner();

	enum State {
		StateNew,
		StateStarted,
		StateDone,
		StateError,
		StateCancelled
	};

	enum Options {
		OptionCheckDone = 1,
		OptionIgnoreStdError = 2,
		OptionMergeStdError = 4,
		/// If this option is set, all data is read and delivered to slotProcessOutput
		/// otherwise data is read and delivered line by line.
		OptionProcessIOReadAll = 8
	};

	State state() const { return mState; }
	bool isActive() const { return mState == StateNew || mState == StateStarted; }
	Options options() const { return mOptions; }

    /// the Assignment from the Slave
	JobAssignment jobAssignment() const;

    /// a compressed representation of task numbers
    /// ie.   1-40;  1,5,9;  50-55,57
	QString assignedTasks() const;

	/// Returns an null record if the job is still loading(ie. no task has started)
	JobTaskList currentTasks() const;
	/// Returns true if a task has started, false if the job is still loading
	bool taskStarted() const;
	/// Returns true if a task has taken longer then it's allowed
	bool exceededMaxTime() const;
	bool exceededQuietTime() const;

	/// what time was the burn started
	QDateTime startTime() const;
	/// what time was the burn for the current task started
	QDateTime taskStartTime() const;

	/// what processes will the burner spawn?
	virtual QStringList processNames() const;

	/// Returns the process executable
	virtual QString executable();

	/// The processes working directory is set to the returned string unless it is empty or null
	/// Default implementation returns an empty string
	virtual QString workingDirectory();

	/// Returns arguments to start the process
	virtual QStringList buildCmdArgs();

	virtual QStringList environment();

	QProcess * process() const { return mCmd; }

	QString burnFile() const { return mBurnFile; }

	QString burnDir() const;

	static JobError logError( const Job & j, const QString & msg, const JobTaskList & jtl = JobTaskList(), bool timeout=false );

	/// to be called by a burner if an error is raised
	void jobErrored( const QString &, bool timeout=false, const QString & nextstate="new" );

	void cancel();

	/// call this to set mCurrentTasks progress field
	void setProgress(int);
    void setProgressMessage( const QRegExp & );

    void addErrorMessage( const QRegExp & );
    void addIgnoreMessage( const QRegExp & );


protected:
	/// to be called by a burner when a task starts
	/// Returns true if the task status changed to 'busy', returns false if the task is not found or no longer assigned
	bool taskStart( int frame, const QString & outputName = QString(), int secondsSinceStarted = 0 );
	/// to be called by a burner when a task is done
	void taskDone( int frame );

	QTimer * checkupTimer() { return mCheckupTimer; }

	void logMessage( const QString &, QProcess::ProcessChannel = QProcess::StandardOutput );

	/// Connects process to slots
	/// slotReadStdOut
	/// slotReadStdError
	/// slotProcessExited
	/// slotProcessError
	virtual void connectProcess( QProcess * );

	/// Responsibilities
	///  1) Create mCmd
	///  2) Connect mCmd's signals
	///  3) Set mCommandHistory.command
	///  4) Start mCmd
	
	/// Default implementation does the following
	///  1) Creates mCmd
	///  2) Calls connectProcess on mCmd
	///  3) Gets cmd via executable and buildCmdArgs()
	///  4) Sets mCommandHistory.commad to cmd + " " + args.join(" ")
	///  5) Starts mCmd with start( cmd, args )
	virtual void startProcess();

	/// Copies 'frame' (path generated using mJob.outputPath and makeFramePath function),
	/// to all subsequent frames in the frameNth range, up to frameRangeEnd
	void copyFrameNth( int frame, int frameNth, int frameRangeEnd );

	void updatePriority( const QString & priorityName );
	
public slots:
	virtual void start();
	/// Called by slave to start the job
	/// Shouldn't need to override this, override startProcess instead.
	/// This calls startProcess, sets up JobCommandHistory, starts
	/// the output timer and the memory timer
	virtual void startBurn();
	
	virtual void startCopy();

	/// Called by Slave
	/// Default implementation disconnects mCmd and kills the process
	/// Override to do other cleanup, or prevent mCmd from being killed
	virtual void cleanup();

    void addAccountingData( const AccountingInfo & );

signals:
	/// emited so the Slave knows the Burn is done
	void finished();
	/// emited so the Slave knows the Burn errored out
	void errored( const QString & );

	/// emitted whenever a file is generated by a Burner that should have perms set,
	/// copied somewhere, etc.
	void fileGenerated( const QString & );

protected slots:
	/// to be called by a burner when the job has successfully finished
	void jobFinished();

	/// Default implementation checks to make sure maximum load time 
	/// and maximum task time haven't been exceeded.
	virtual bool checkup();

	/// Process slots
	virtual void slotReadStdOut();
	virtual void slotReadStdError();
	virtual void slotProcessExited();
	virtual void slotProcessStarted();

	/// Calls jobErrored with the error code and error description
	virtual void slotProcessError( QProcess::ProcessError );

	/// Default imp calls slotProcessOutputLine for each line
	virtual void slotProcessOutput( const QByteArray & output, QProcess::ProcessChannel );

	/// Default imp does nothing
	virtual void slotProcessOutputLine( const QString & output, QProcess::ProcessChannel );

    /// originally added for syncing frames cross site
	void syncFile( const QString & );

	/// pushes changes to mStdOut and mStdErr to the database
	void updateOutput();

	/// see how much memory the spawned process tree is using, makes sure it dosn't go over the allowed amount.
	virtual void checkMemory();

	void slotCopyFinished();
	void slotCopyError( const QString & text, int );

    /// called by a timer to flush log output to disk
    void slotFlushOutput();

	/// internally called by jobburner if an error is raised
	void slotJobErrored( const QString &, bool timeout=false, const QString & nextstate="new" );

protected:
	void deliverOutput( QProcess::ProcessChannel );
	bool checkIgnoreLine( const QString & );
	void setTaskList( const QString & );
	void openLogFiles();

	Slave * mSlave;
	QProcess * mCmd;
    uint mCmdPid;
	JobAssignment mJobAssignment;
	Job mJob;
	JobTaskAssignmentList mTaskAssignments, mCurrentTaskAssignments;
	JobTaskList mTasks, mCurrentTasks;
	bool mLoaded;
	QDateTime mStart, mTaskStart;
	QString mBurnFile, mTaskList;
	QTimer * mOutputTimer, * mMemTimer, * mCheckupTimer, * mLogFlushTimer;
	QString mStdOut, mStdErr;

	State mState;
	Options mOptions;
	QList<QRegExp> mIgnoreREs, mErrorREs;
	SpoolRef * mCurrentCopy;
	Win32ShareManager::MappingRef mMappingRef;

	bool mLogFilesReady;
	QFile * mLogFile;
    QTextStream * mLogStream;

	friend class Slave;

	QRegExp mProgressRE;
	QDateTime mProgressUpdate;

    AccountingInfo mAccountingData;
    AccountingInfo sumAccountingData( const AccountingInfo & left, const AccountingInfo & right ) const;
    AccountingInfo checkResourceUsage();
    void updateAssignmentAccountingInfo();

    JobFilterMessageList mJobFilterMessages;

	QDateTime mLastOutputTime;
};

/// @}

#endif // JOB_BURNER_H

