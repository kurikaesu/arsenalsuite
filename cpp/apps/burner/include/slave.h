
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


#ifndef SLAVE_H
#define SLAVE_H

#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <qpointer.h>
#include <qsocketnotifier.h>

#include "host.h"
#include "hoststatus.h"
#include "job.h"
#include "jobassignment.h"
#include "jobtask.h"
#include "spooler.h"
#include "service.h"

class QProcess;
class QTimer;
class JobBurner;
class Service;
class Idle;
class KillDialog;
class QMessageBox;

/// \ingroup ABurner
/// \brief Creates a SysLog entry for the error and sets the HostService::sysLog (fkeysyslog) to it.
void host_error( const QString & text, const QString & method, const Service & service );

struct AccountingInfo {
    AccountingInfo() : pid(0), ppid(0), cpuTime(0),
        memory(0),
        bytesRead(0),
        bytesWrite(0),
        opsRead(0),
        opsWrite(0),
        ioWait(0) {}

    uint pid,
        ppid,
        realTime,
        cpuTime,
        memory,
        bytesRead,
        bytesWrite,
        opsRead,
        opsWrite,
        ioWait;
};

/// \ingroup ABurner
/// @{

/**
 * \brief Controls Burner Rendering
 * * Keeps track of host's status. 
 * * Queries the database for new job assignments.
 * * Updates the database when the user sets
 *   offline via the GUI. 
 * * Starts copies and burners.
 */

class Slave : public QObject
{
Q_OBJECT
public:
	/// If jobAssignmentKey!=0 then we are in burn only mode.  This slave will create a jobburner
	/// for the assignment and when finished burning will close this instance of assburner
	Slave(bool useGui=true, bool autoRegister=false, int jobAssignmentKey=0, QObject * parent=0);
	~Slave();

	HostStatus hostStatus() const;
	Host host() const;

	/// Returns the status field of the host status record.
	QString status() const;

	/// Runs a program specified by 'assburnerInstallerPath'
	/// in the config table, then quits.
	void clientUpdate( bool onlineAfterUpdate = true );

	/// Returns the spooler used to download and keep track of files for the jobs
	/// Can be 0 if not using a spool
	Spooler * spooler() { return mSpooler; }

	bool usingGui() const { return mUseGui; }

    /// Windows specific functionality
	bool inOwnLogonSession() { return mInOwnLogonSession; }
	void setInOwnLogonSession( bool iols );
	QString currentPriorityName() const;

    /// returns the path to the log root dir, used if you don't
    /// want to log to the database, set in the Config table
	QString logRootDir() const;

    /// a single Slave can run multiple jobs at the same time
    /// these lists keep track of assignments from the scheduler
    /// and the Burner process responsible for running them
	QList<JobBurner*> activeBurners();
	JobAssignmentList activeAssignments();

    /// set via Config::abMemCheckPeriod 
    ///- time between check for job memory usage
    int memCheckPeriod() const;

    AccountingInfo parseTaskLoggerOutput( const QString & );

    // Unix signal handlers.
    static void intSignalHandler(int unused);
    static void termSignalHandler(int unused);

public slots:
	/// Updates the status in the database to status
	/// If force is true, any status change that happens before
	/// the new status is committed will be ignored.  If force
	/// is false, the new status will be handled and status will
	/// be ignored.
	bool setStatus( const QString & status, bool force = false );

    /// do we want to run as a daemon or a user controllable interface
	void setUseGui( bool );

	/// 
	/// These are the 3 slots that need to be called
	/// from the jobburner to tell the slave what
	/// is going on currently
	/// 
	void slotError( const QString & text );
	void slotBurnFinished();

	/// Interface for gui that is controlling us
	void online();
	void offline();
	void toggle();

	/// for debugging
	void offlineFromAboutToQuit();

    // windows drive mapping support
	void showRemapWarning(const QString & drive);
	void setIsMapped( bool );

#ifndef Q_OS_WIN
    // Qt signal handlers
	void handleSigInt();
	void handleSigTerm();
#endif

    /// Update memory usage of running and finished jobs
    void setAvailableMemory();

signals:
	/// For the gui to show status.
	void statusChange( const QString & status );

	void copyProgressChange( int );

	void activeAssignmentsChange( JobAssignmentList activeAssignments, JobAssignmentList oldActiveAssignments );

protected slots:
	/// Call via singleshot timer from the ctor
	/// We dont want any db access in the ctor,
	/// because that is called from the mainwindow
	/// ctor, if the db connection gets lost
	/// before the mainwindow is up, then qt4
	/// will exit once the dialog is closed
	void startup();

	/// Called once every 5 seconds to check database
	/// status
	void loop();

    /// Cleans up the currently running jobs
    void cleanup();

	/// Resets assburner, stopping and cleaning up any currently
	/// running job by calling cleanup.  Status will be set to ready if offline is false
	void reset( bool offline=false );

	/// This restarts assburner. It actually just exits and expects a system process
    /// to restart it.
	void restart();

	/// This reboots the host machine
	void reboot();

    /// This shuts the host machine down
    void shutdown();

	/// Shuts down the machine and sets the host's slaveStatus to 'sleeping'
	void goToSleep();

	/// This restarts assburner when it loses its DB connection
	/// (doesn't return frames, etc)
	void restartFromLostConnectionTimer();

	/// Connected to mIdle's secondsIdle signal
	/// This is used to go offline automatically
	/// after a period of machine inactivity
	void slotSecondsIdle( int );

	/// These are here to moniter the database connection
	/// If we cant connect for a minute, then we'll try to restart
	void slotConnectionLost();
	void slotConnected();

    /// windows specific
	void execPriorityMode();
	void restoreDefaultMappings();

    /// Called once every 5 minutes to update logged in users
    void updateLoggedUsers();

protected:
	void handleStatusChange( const QString & status, const QString & oldStatus );

	/// Attempts to start a job burner for a currently assigned job.
	void burn( const JobAssignment & );
	void handleJobAssignmentChange( const JobAssignment & );
	JobBurner * setupJobBurner( const JobAssignment & );
	void cleanupJobBurner( JobBurner * burner );
	void setActiveAssignments( JobAssignmentList );

    /// The admin can decide what "services" a host can run via the
    /// HostService table. This keeps a list of allowed Services.
	ServiceList enabledServiceList();

    /// A job type can register a list of processes that can't be running for
    /// the Burner to start up. This is useful for artist workstations where
    /// AB may start up via an idle timer.
	bool checkForbiddenProcesses();
	QStringList mForbiddenProcesses;
	void loadForbiddenProcesses();

    /// tell the database we are still alive
	void pulse();

    /// TODO: remove?
	bool checkFusionRunning();

//	void checkDriveMappingAvailability();

    /// initialise the Python interpreter for Python Burner plug-ins.
	void loadEmbeddedPython();

	Host mHost;
	HostStatus mHostStatus;
	QDateTime mLastAssignmentChange;
	JobAssignmentList mActiveAssignments;
	QList<JobBurner*> mActiveBurners;
	QList<JobBurner*> mBurnersToDelete;

	QTimer * mTimer;
	int mPulsePeriod, mLoopTime, mQuickLoopTime, mQuickLoopCount, mMemCheckPeriod;
	Service mService;
	QDateTime mLastPulse;

    QTimer * mUserTimer;

	Idle * mIdle;
	int mIdleDelay;

	Spooler * mSpooler;

    /// a dialog used to show forbidden processes, and let the artist
    /// kill them. This is useful for things like 3dsmax which likes
    /// to crash and leave ghost processes around.
	KillDialog * mKillDialog;

    /// inside the main process loop?
	bool mInsideLoop;
    
    /// if we loose connection to the database
	QTimer * mLostConnectionTimer;

    /// windows drive mapping
	QPointer<QMessageBox> mDriveRemapWarningDialog;
	bool mIsMapped;

    /// are we a daemon or gui?
	bool mUseGui;

    /// if the host doesn't exist in the database, should we create a host
    /// record automatically?
	bool mAutoRegister;

	// Specifies that assburner is running in it's own windows logon sesssion
	// and therefore has it's own drive mappings.
	bool mInOwnLogonSession;

	int mBurnOnlyJobAssignmentKey;

    /// windows priority stuff
	void updatePriorityMode( int );
	bool mBackgroundMode;
	bool mBackgroundModeEnabled;

private:
    static int sigintFd[2];
    static int sigtermFd[2];
 
    QSocketNotifier *snInt;
    QSocketNotifier *snTerm;
};

/// @}

#endif // SLAVE_H

