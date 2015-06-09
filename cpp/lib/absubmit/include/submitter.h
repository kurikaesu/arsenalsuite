

#ifndef SUBMITTER_H
#define SUBMITTER_H

#include "employee.h"
#include "host.h"
#include "job.h"
#include "jobdep.h"
#include "joboutput.h"
#include "jobstat.h"
#include "jobservice.h"
#include "jobtask.h"
#include "jobtype.h"
#include "path.h"
#include "project.h"
#include "user.h"

#include "database.h"
#include "md5.h"
#include "table.h"

#include "absubmit.h"


class QFtp;
class QFile;

/// Returns the error number, where the error file is
/// c:/blur/absubmit/errors/ERROR_NUMBER.txt
AB_SUBMIT_EXPORT int writeErrorFile( const QString & error );

class AB_SUBMIT_EXPORT Submitter : public QObject
{
Q_OBJECT
public:
	Submitter( QObject * parent = 0 );

	~Submitter();

	enum State {
		New,
		Submitting,
		Success,
		Error
	};

	State state() const;

	/**
	 *  This function offers the primary functionality need to support command line absubmit.
	 *  It is also very useful for apps using pyabsubmit, or libabsubmit.
	 *  It will pass any job parameters to the job record, and offers extra functionality for the following parameters:
	 *     submitSuspended BOOL - Same as calling setSubmitSuspended( BOOL )
	 *     jobType JOBTYPE_NAME- Looks up the jobType record, and calls setJobType
	 *     noCopy IGNORED - Sets mUploadEnabled = false, sets mJob.uploadedFile and mJob.checkMd5Sum to false
	 *     user USERNAME - Looks up the User record and sets mJob.fkeyuser,  the user must be an active user(disabled=0)
	 *     job JOB_NAME - Sets mJob.name
	 *     projectName PROJECT_NAME - Looks up the project by name, and sets mJob.fkeyproject
	 *     host HOST_NAME - Looks up the host by name, and sets mJob.fkeyhost
	 *     jobParentId JOB_ID - Sets mJob.fkeyjob
	 *     services SERVICE_NAME_LIST - Comma separated list of service names are looked up and JobService records are added
	 *     deps JOB_ID_LIST - Comma separated list of job keys. A JobDep record is setup for each
	 *
	 *  Sets up frame lists properly with jobtasks and joboutputs, by passing the following parameters
	 *  outputPath
	 *  frameList or (frameStart and frameEnd)
	 *  optional taskLabels, which should be a comman separated list of labels, with the number of labels matching the number of frames in the frameList
	 *  optional frameNth and frameFill, these are documented in the Job schema
	 *
	 *  Allows passing a parameter through a file, the syntax for that is
	 *   paramName @filePath
	 *  It will open and read the full contents of filePath, and set the value of paramName to the contents.
	 *
	 *     sets flag_xo to default value of 0 for Max jobservice
	 *     sets default priority to 50 if not specified
	 *     startupScript StartupScriptPath - reads the contents of the file and sets the value of startupScript
	 * 
	 **/
	void applyArgs( const QMap<QString,QString> & args );

	Job job();
	void setJob( const Job & job );
	
	/** 
	 *   This function will look up the jobtype, and create set mJob to a new Job object from the appropriate table for the jobtype
	 **/
	void newJobOfType( const JobType & job );

	// Only use this if you have already added 1 or more job outputs, otherwise use addJobOutput
	void setFrameList( const QString & frameList, const QString & taskLabels = QString(), int frameNth = 0, bool frameFill = false );
	void addTasks( JobTaskList jtl );

	void addJobOutput( const QString & outputPath, const QString & outputName = QString(), const QString & frameList = QString(), const QString & taskLabels = QString(), int frameNth = 0, bool frameFill = false );

	/// jobs may need multiple JobService entries if they require licenses or the like
	void addServices( ServiceList services );
	void addJobDeps( JobDepList deps );

	/// adds an entry to the JobDep table, mapping two jobs together
	/// depType == 1 is for hard dependencies, when parent job is complete the child will be able to run
	/// depType == 2 is for soft or "linked" jobs, meaning when task N from the parent job is done
	///    then task N from the child job can run 
	void addDependencies( JobList deps, uint depType=1 );

	/// should we do a real exit(), or is this being used as a library where more submissions might follow
	void setExitAppOnFinish( bool );

	bool submitSuspended() const;

	State waitForFinished();

	QString errorText() const;
signals:
	/// clients of this library should hook into these signals to get notified when things are done
	void submitError( const QString & error );
	void submitSuccess();
	/// Substate includes "Setting up Job", "Computing Job File Checksum", "Uploading Job File", "Verifying Job"
	void stateChange( State, const QString & subState );
	/// Percentage uploaded
	void uploadProgress( int );
	
public slots:
	void submit();
	void setSubmitSuspended( bool );

protected slots:
	void ftpStateChange( int );
	void ftpDone(bool);
	void ftpTransferProgress(qint64,qint64);
	void ftpCommandStarted(int);
	void ftpCommandFinished(int,bool);
	void checkJobStatus();

protected:
	bool checkFarmReady();
	void startCopy();
	void postCopy();
	void checkFileFree();
	void checkMd5();
	void preCopy();
	void ftpCleanup();
	void issueFtpCommands( bool makeUserDir );

	void printJobInfo();

	int submitCheck();

	int createTasks( QList<int> taskNumbers, QStringList labels, JobOutputList outputs, int frameNth, bool frameFill );

	void _success();
	void exitWithError( const QString & );

	State mState;
	QString mErrorText;
	bool mUploadEnabled;
	Job mJob;
	JobOutputList mOutputs;
	JobDepList mJobDeps;
	JobTaskList mTasks;
	JobServiceList mServices;

	QFtp * mFtp;
	QFile * mFile;
	QFile * mFile2;

	QString mFtpHost, mFtpUser, mFtpPassword;
	int mFtpPort, mFtpRetries, mFtpTimeout;
	int mCdCmdId, mMkDirCmdId, mPutCmdId, mPutCmdId2;
	bool mCreateDirTried;
	bool mHaveJobInfoFile;
	bool mSubmitSuspended;
	QString mSrc, mFileName, mDest, mSrc2, mDest2;
	// Percentage
	int mProgress;
	QTimer * mVerifyTimer;
	bool mExitAppOnFinish;
	QString mInitialTaskStatus;
};

#endif // SUBMITTER_H

