
#include <qdir.h>
#include <qftp.h>
#include <qfileinfo.h>
#include <QFile>
#include <qtextstream.h>
#include <qtimer.h>
#include <QSqlDatabase>

#include "iniconfig.h"

#include "config.h"
#include "filetracker.h"
#include "hostservice.h"
#include "jobdep.h"
#include "jobenvironment.h"
#include "joberror.h"
#include "jobfilterset.h"
#include "jobservice.h"
#include "rangefiletracker.h"
#include "service.h"


#include "submitter.h"
#include "path.h"

QList<int> frameNthList( int start, int end, int nth, bool fill )
{
	QList<int> ret;
	for( int i=start; i<=end; i+=nth ) {
		if( fill ) {
			for( int x = i+1; x<=qMin(i+nth-1,end); x++ )
				ret += x;
		} else
			ret += i;
	}
	return ret;
}

static FileTracker getOutputFileTracker( const QString & path )
{
	if( path.isEmpty() ) return FileTracker();
	RangeFileTracker existing = FileTracker::fromPath( path );
	if( !existing.isRecord() ) {
		LOG_5( "Creating new RangeFileTracker for path: " + path );
		existing = RangeFileTracker();
		existing.setFilePath( path );
		existing.commit();
	} else
		LOG_5( "Returning existing RangeFileTracker for path: " + path );
	return existing;
}

Submitter::Submitter( QObject * parent )
: QObject(parent)
, mState( New )
, mUploadEnabled( true )
, mFtp( 0 )
, mFile( 0 )
, mFile2( 0 )
, mFtpPort( 0 )
, mFtpRetries( 0 )
, mFtpTimeout( 0 )
, mCdCmdId( 0 )
, mMkDirCmdId( 0 )
, mPutCmdId( 0 )
, mPutCmdId2( 0 )
, mCreateDirTried( false )
, mHaveJobInfoFile( false )
, mSubmitSuspended( false )
, mVerifyTimer( 0 )
, mExitAppOnFinish( false )
, mInitialTaskStatus("new")
{
	IniConfig & c = config();
	c.pushSection( "UploadSettings" );
	mFtpHost = Config::getString( "assburnerFtpHost", "stryfe" );
	mFtpPort = Config::getInt( "assburnerFtpPort", 21 );
	mFtpUser = Config::getString( "assburnerFtpUser", "assburner" );
	mFtpPassword = Config::getString( "assburnerFtpPassword", "assburner" );
	mFtpRetries = Config::getInt( "assburnerFtpRetries", 2 );
	mFtpTimeout = Config::getInt( "assburnerFtpTimeout", 60 );
	c.popSection();
}

Submitter::~Submitter()
{
	delete mVerifyTimer;
	mVerifyTimer = 0;
	ftpCleanup();
}

Submitter::State Submitter::state() const
{
	return mState;
}

void Submitter::setExitAppOnFinish( bool eaof )
{
	mExitAppOnFinish = eaof;
}

bool Submitter::submitSuspended() const
{
	return mSubmitSuspended;
}

void Submitter::setSubmitSuspended( bool ss )
{
	mSubmitSuspended = ss;
}

Job Submitter::job()
{
	return mJob;
}

void Submitter::setJob( const Job & job )
{
	mJob = job;
	mJob.setStatus( "submit" );
	mJob.setColumnLiteral( "submittedts", "now()" );
	if( !mJob.host().isRecord() )
		mJob.setHost( Host::currentHost() );
	if( !mJob.user().isRecord() )
		mJob.setUser( User::currentUser() );
	mJob.commit();
	LOG_5( "mJob committed" );
}

void Submitter::newJobOfType( const JobType & jobType )
{
	if( jobType.isRecord() ) {
		if( !mJob.isValid() || mJob.imp()->table() == Job::table() ) {
			LOG_5( "Setting mJob to new Job" + jobType.name() + " record" );
			Table * t = Database::current()->tableByName( "Job" + jobType.name() );
			if( t )
				mJob = t->load();
			else
				LOG_5( "Table Job" + jobType.name() + " not found" );
		}
		mJob.setJobType( jobType );
		setJob( mJob );
	} else
		LOG_5( "JobType is not a valid record" );
}

void Submitter::addTasks( JobTaskList jtl )
{
	mTasks += jtl;
}

int Submitter::createTasks( QList<int> taskNumbers, QStringList labels, JobOutputList outputs, int frameNth, bool frameFill )
{
	if( frameNth > 1 )
		taskNumbers = frameNthList( taskNumbers[0], taskNumbers.last(), frameNth, frameFill );

	JobTaskList tasks;
	foreach( JobOutput jo, outputs )
	{
		QStringList::const_iterator label_it = labels.begin();
		foreach( int number, taskNumbers ) {
			LOG_5( "Creating jobtask with taskNumber : " + QString::number( number ) + " output: " + jo.name() );
			JobTask jt;
			jt.setFrameNumber( number );
			jt.setJob( mJob );
			jt.setStatus( mInitialTaskStatus );
			jt.setJobOutput( jo );
			if( label_it != labels.end() ) {
				jt.setLabel( *label_it );
				++label_it;
			}
			tasks += jt;
		}
	}
	tasks.commit();

	mTasks += tasks;
	return tasks.size();
}

void Submitter::setFrameList( const QString & frameList, const QString & taskLabels, int frameNth, bool frameFill )
{
	QList<int> frames = expandNumberList( frameList );
	LOG_5( "Creating tasks for all outputs: " + frameList + " with labels: " + taskLabels );
	createTasks( frames, taskLabels.split(','), mOutputs, frameNth, frameFill );
}

void Submitter::addJobOutput( const QString & outputPath, const QString & outputName, const QString & frameList, const QString & taskLabels, int frameNth, bool frameFill )
{
	JobOutput jo;
	jo.setName( outputName.isEmpty() ? "Output " + QString::number(mOutputs.size()) : outputName );
	//jo.setFileTracker( getOutputFileTracker( outputPath ) );
	jo.setJob( mJob );
	jo.commit();
	mOutputs += jo;
	LOG_5( "Created Output " + jo.name() + " with path: " + jo.fileTracker().filePath() );
	createTasks( expandNumberList(frameList), taskLabels.split(','), JobOutputList(jo), frameNth, frameFill );
}

void Submitter::addServices( ServiceList services )
{
	JobServiceList jsl;
	foreach( Service s, services ) {
		JobService js;
		js.setService( s );
		jsl += js;
	}
	jsl.setJobs( mJob );
	jsl.commit();
}

void Submitter::addDependencies( JobList deps, uint depType )
{
	JobDepList jdl;
	foreach( Job j, deps ) {
		JobDep jd;
		jd.setDep( j );
		jd.setDepType( depType );
		jdl += jd;
	}
	jdl.setJobs( mJob );
	jdl.commit();
	mJobDeps += jdl;
}

void Submitter::addJobDeps( JobDepList deps )
{
	deps.commit();
	mJobDeps += deps;
}

QString readValueFromFile( const QString & path, const QString & field )
{
	QFile file(path);
	if( file.open(QIODevice::ReadOnly) )
		return QTextStream( &file ).readAll();
	else
		LOG_1( "Unable to open field " + field + " value file: " + path );
	return QString();
}

void Submitter::applyArgs( const QMap<QString,QString> & args )
{
	if( args.contains("submitSuspended") )
		mSubmitSuspended = QVariant(args["submitSuspended"]).toBool();

	// first thing we check for is the job type
	// this initialises the mJob member with the correct
	// class
	if( args.contains("jobType") ) {
		JobType jt = JobType::recordByName( args["jobType"] );
		if( jt.isRecord() ) {
			newJobOfType( jt );
		} else {
			exitWithError( "Could not find jobtype: " + args["jobType"] );
			return;
		}
		LOG_5( "Finished setting jobtype" );
	}

	/// filterSet determines which message filters
	/// are used to check for Errors.
	if( args.contains("filterSet") ) {
		JobFilterSet jfs = JobFilterSet::recordByName( args["filterSet"] );
		if( jfs.isRecord() ) {
			mJob.setFilterSet( jfs );
		}
	} else {
		/// look for a filterSet named the jobType
		JobFilterSet jfs = JobFilterSet::recordByName( mJob.jobType().name() );
		if( jfs.isRecord() ) {
			mJob.setFilterSet( jfs );
		}
	}
	LOG_5( "Finished setting filterSet" );

	if( args.contains("noCopy") ) {
		mUploadEnabled = false;
		mJob.setUploadedFile( false );
		mJob.setCheckFileMd5( false );
	}

	if( args.contains( "user" ) ) {
		User u = User::activeByUserName( args["user"] );
		if( u.isRecord() )
			mJob.setUser( u );
		else
			exitWithError( "user named: "+ args["user"] + " not valid for submitting the job" );
	} else if( !User::currentUser().isRecord() ) {
		exitWithError( "Current user not valid for submitting the job" );
	} else {
		mJob.setUser( User::currentUser() );
	}
	LOG_5( "finished setting user" );

	if( args.contains("job") ) {
		mJob.setName( args["job"] );
		LOG_5( "finishing setting job name" );
	}

	if( args.contains( "projectName" ) ) {
		Project p = Project::recordByName( args["projectName"] );
		if( !p.isRecord() )
			LOG_1( "Project not found: " + args["projectName"] );
		else
			mJob.setProject( p );
		LOG_5( "Finished setting project" );
	}

	if( args.contains( "host" ) ) {
		Host h = Host::recordByName( args["host"].toLower() );
		if( !h.isRecord() )
			LOG_1( "Host not found: " + args["host"] );
		else
			mJob.setHost( h );
		LOG_5( "finished setting host" );
	}

	if( mJob.jobType().name().contains("Max") && !args.contains( "flag_xo" ) )
		mJob.setValue( "flag_xo", QVariant(0) );

	if( args.contains("deps") ) {
		JobList jobs;
		foreach( QString id, args["deps"].split(",") ) {
			Job d = Job(id.toUInt());
			if( d.isRecord() )
				jobs += d;
			else
				LOG_1( "Dependency Job not found: " + id );
		}
		addDependencies( jobs );
	}

	if( args.contains("linked") ) {
		JobList jobs;
		foreach( QString id, args["linked"].split(",") ) {
			Job d = Job(id.toUInt());
			if( d.isRecord() ) {
				jobs += d;
				/// if there are linked parent jobs, then any tasks for this job start
				/// out "holding" for the parent tasks to complete
				mInitialTaskStatus = "holding";
			} else {
				LOG_1( "Linked Job with same task count not found: " + id );
			}
		}
		addDependencies( jobs, 2 /* depType */ );
	}

	if( args.contains("suspendDeps") ) {
		JobList jobs;
		foreach( QString id, args["suspendDeps"].split(",") ) {
			Job d = Job(id.toUInt());
			if( d.isRecord() ) {
				jobs += d;
			} else {
				LOG_1( "SuspendDep Job with same task count not found: " + id );
			}
		}
		addDependencies( jobs, 3 /* depType */ );
	}
		
	if( args.contains("deleteDeps") ) {
		JobList jobs;
		foreach( QString id, args["deleteDeps"].split(",") ) {
			Job d = Job(id.toUInt());
			if( d.isRecord() ) {
				jobs += d;
			} else {
				LOG_1( "deleteDep Job with same task count not found: " + id );
			}
		}
		addDependencies( jobs, 4 /* depType */ );
		}

	if( !args.contains( "priority" ) )
		mJob.setPriority( 50 );

	if( args.contains("startupScript") ) {
		QFile scriptFile(args["startupScript"]);
		if( scriptFile.open(QIODevice::ReadOnly) )
			mJob.setValue("startupScript", QTextStream(&scriptFile).readAll() );
		else
			LOG_1( "Unable to open startupScript file: " + args["startupScript"] );
	}

	QStringList argKeys = QStringList(args.keys());
	QRegExp outputPathRE( "outputPath(\\d+)" );
	QRegExp frameListRE( "frameList(\\d+)" );

	int frameNth = args["frameNth"].toInt(), frameFill = args["frameFill"].toInt();
	if( frameFill > 0 )
		mJob.setValue( "frameNth", -frameNth );

	QStringList outputs = argKeys.filter( outputPathRE );
	foreach( QString outputArg, outputs ) {
		QString path = args[outputArg];
		outputPathRE.exactMatch( outputArg );
		QString outputNumber = outputPathRE.cap(1);
		QString outputNameArg = "outputName" + outputNumber;
		QString outputFramesArg = "frameList" + outputNumber;
		QString outputLabelsArg = "taskLabels" + outputNumber;
		addJobOutput( path, args[outputNameArg], args[outputFramesArg], args[outputLabelsArg], frameNth, frameFill > 0 );
	}

	QString frameList = args["frameList"];
	if( !args["frameStart"].isEmpty() && !args["frameEnd"].isEmpty() && frameList.isEmpty() )
		frameList = args["frameStart"] + "-" + args["frameEnd"];

	// Create single output, backwards compatible
	if( mOutputs.isEmpty() )
		addJobOutput( args["outputPath"], "Output1", frameList, args.contains("taskLabels") ? args["taskLabels"] : "", frameNth, frameFill > 0 );
	//else if( !frameList.isEmpty() )
	//	setFrameList( frameList, args.contains("taskLabels") ? args["taskLabels"] : "" );

	if( args.contains( "jobParentId" ) )
		mJob.setJobParent( Job( args["jobParentId"].toInt() ) );

	if( args.contains("services" ) ) {
		ServiceList services;
		foreach( QString name, args["services"].split(",") ) {
			Service s = Service::recordByName(name);
			if( !s.isRecord() ) {
				LOG_5( "Service name does not exist: " + name );
				continue;
			}
			if( !services.contains(s) )
				services += s;
		}
		addServices( services );
	} else {
		// if they don't provide specific services then use the JobType service
		Service jts = mJob.jobType().service();
		if( jts.isRecord() )
			addServices( jts );
	}

	// create a JobEnvironment record if required
	if( args.contains("environment") ) {
		JobEnvironment jenv;
		jenv.setEnvironment( args["environment"] );
		jenv.commit();
		mJob.setEnvironment( jenv );
	}

    /// now we set any other argument assuming there is a database field with the arg name
	Table * t = mJob.imp()->table();
	for( QMap<QString,QString>::const_iterator it = args.begin(); it != args.end(); ++it )
	{
		bool valueFromFile = false;
		QString fieldName = it.key();
		if( fieldName.startsWith( "@" ) ) {
			valueFromFile = true;
			fieldName = fieldName.mid(1);
		}
		if( it.key() == "environment" ) continue;
		Field * f = t->schema()->field( it.key() );
		if( f && !f->flag(Field::ForeignKey) ) {
			LOG_5( "Setting " + it.key() + " to value " + it.value() );
			QVariant value = it.value();
			if( valueFromFile )
				value = QVariant( readValueFromFile( it.value(), fieldName ) );
			mJob.setValue( it.key(), QVariant( it.value() ) );
		}
	}
	
	LOG_5( "Finished applying args" );
}


void Submitter::submit()
{
	LOG_TRACE;
	if( mState != New ) {
		LOG_1( "submit called when state is not New" );
		return;
	}
	mState = Submitting;
	emit stateChange( Success, "Gathering Job Data" );

	if( !checkFarmReady() )
		return;

	// submitCheck() will return 0 if everything is ok
	if( submitCheck() ) return;

	bool jobHasFile = !mJob.fileName().isEmpty();
	if( jobHasFile && mUploadEnabled ) {
		checkFileFree();
		preCopy();
		startCopy();
	} else {
		//if( jobHasFile )
		//	checkMd5();
		mJob.setStatus( mSubmitSuspended ? "verify-suspended" : "verify" );
		mJob.commit();
		printJobInfo();
		_success();
	}
}

Submitter::State Submitter::waitForFinished()
{
	while( mState == Submitting ) {
		QCoreApplication::instance()->processEvents( QEventLoop::ExcludeUserInputEvents );
	}
	return mState;
}

bool Submitter::checkFarmReady()
{
	Service s = Service::recordByName( "AB_Reaper" );
	if( !s.enabled() || s.hostServices().filter( "enabled", true ).isEmpty() ) {
		exitWithError( "Assburner submission is currently disabled" );
		return false;
	}
	return true;
}

int Submitter::submitCheck()
{
	LOG_3( "Submitter::submitCheck" );

	//
	// Check required info
	//
	if( !mJob.jobType().isRecord() ) {
		LOG_1( "Missing JobType" );
		exitWithError( "Required parameter 'jobType' missing or invalid." );
		return 1;
	}

	if( !mJob.isValid() || mJob.imp()->table() == Job::table() ) {
		LOG_1( "Job not from valid table" );
		exitWithError( "Job not from valid table" );
		return 1;
	}

	if( !mJob.user().isRecord() ) {
		if( !User::currentUser().isRecord() ) {
			LOG_1( "Not valid user found for submitting the job" );
			exitWithError( "Not valid user found for submitting the job" );
			return 1;
		}
		mJob.setUser( User::currentUser() );
	}

	if( mJob.name().isEmpty() ) {
		LOG_1( "Job name is missing" );
		exitWithError( "Job name is missing" );
		return 1;
	}

	if( mTasks.isEmpty() ) {
		LOG_1( "Job requires a frame list" );
		exitWithError( "Job Requires a Frame List" );
		return 1;
	}

	mJob.commit();
	mTasks.setJobs( mJob );
	mTasks.commit();

	return 0;
}

void Submitter::checkFileFree()
{
	LOG_3( "Submitter::checkFileFree" );
	static int checkTime = 0;
	if( !Path::checkFileFree( mJob.fileName() ) ) {
		LOG_1( "Waiting 1 second for " + mJob.fileName() + " to become free (Path::checkFileFree)" );
		if( checkTime > 30 ) {
			LOG_1( "Timed out while waiting for " + mJob.fileName() + " to become free to move." );
			mJob.remove();
			exitWithError( "Some Program is keeping the job file open/locked" );
			return;
		}
		checkTime++;
		QTimer::singleShot( 1000, this, SLOT( checkFileFree() ) );
		return;
	}
}

void Submitter::checkMd5()
{
	Md5 md5;
	QString Md5Sum;
	LOG_3("Submitter::checkMd5: running md5.calcMd5 on " + mSrc );
	emit stateChange( Success, "Computing Job File Checksum" );

	Md5Sum = md5.calcMd5( mSrc );
	LOG_1("Submitter::checkMd5: fileMd5sum being set to " + Md5Sum );
	mJob.setFileMd5sum( Md5Sum );
	mJob.commit();
}

void Submitter::preCopy()
{
	LOG_3( "Submitter::preCopy" );
	QString fileName = mJob.fileName();
	QFileInfo fi( fileName );
	QString ext = "." + fi.suffix();
	QString name = fi.completeBaseName();
	QString path = fi.path() + QDir::separator();

	if( ext == ".mx" )
		ext = ".max";

	mSrc = fileName;
	mDest = name + QString::number( mJob.key() ) + ext;
	mFileName = "N:/" + mJob.user().name() + "/" + mDest;
	mDest2 = name + QString::number( mJob.key() ) + ".txt";
	mSrc2 = path + name + ".txt";

	mJob.setValue( "fileName", mFileName );
	mJob.commit();

	checkMd5();
}

void Submitter::startCopy()
{
	LOG_3( "Submitter::startCopy" );
	emit stateChange( Success, "Uploading Job File" );
	if( mFtp ) delete mFtp;
	mFtp = new QFtp( this );
	connect( mFtp, SIGNAL( stateChanged( int ) ), SLOT( ftpStateChange( int ) ) );
	connect( mFtp, SIGNAL( done( bool ) ), SLOT( ftpDone( bool ) ) );
	connect( mFtp, SIGNAL( dataTransferProgress( qint64, qint64 ) ), SLOT( ftpTransferProgress( qint64, qint64 ) ) );
	connect( mFtp, SIGNAL( commandStarted(int) ), SLOT( ftpCommandStarted(int) ) );
	connect( mFtp, SIGNAL( commandFinished(int,bool) ), SLOT( ftpCommandFinished(int,bool) ) );
	mProgress = 0;
	mFile = new QFile( mSrc, this );
	if( !mFile->open( QIODevice::ReadOnly ) ) {
		LOG_1( "Unable to open file " + mSrc + " for reading, error was: " + QString::number( (int)mFile->error() ) );
		mJob.remove();
		exitWithError( "Unable to open file " + mSrc + " for reading, error was: " + QString::number( (int)mFile->error() ) );
		return;
	}
	
	mFile2 = new QFile( mSrc2, this );
	if( !mFile2->open( QIODevice::ReadOnly ) ) {
		LOG_3("Submitter::startCopy do NOT have " + mSrc2 + ", will NOT try to send");
		mHaveJobInfoFile = false;
	} else {
		LOG_3("Submitter::startCopy have " + mSrc2 + ", will try to send");
		mHaveJobInfoFile = true;
	}
	LOG_3( "Connecting to Ftp Server for Job file upload: " + mFtpHost + ":" + QString::number( mFtpPort ) );
	if( !mFtp->connectToHost( mFtpHost, mFtpPort ) ) {
		LOG_1( "Unable to connect to ftp server: " + mFtpHost + ":" + QString::number( mFtpPort ) );
		exitWithError( "Unable to connect to ftp server: " + mFtpHost + ":" + QString::number( mFtpPort ) );
		return;
	}
	mFtp->login(mFtpUser, mFtpPassword);
	issueFtpCommands( false );
}

void Submitter::issueFtpCommands( bool makeUserDir )
{
	if( makeUserDir ) {
		mMkDirCmdId = mFtp->mkdir( mJob.user().name() );
		mCreateDirTried = true;
	}
	mCdCmdId = mFtp->cd( mJob.user().name() );
	mPutCmdId = mFtp->put( mFile, mDest );
	if (mHaveJobInfoFile) 
		mPutCmdId2 = mFtp->put( mFile2, mDest2 );
}

void Submitter::postCopy()
{
	mJob.setStatus( mSubmitSuspended ? "verify-suspended" : "verify" );
	mJob.commit();

	LOG_3( "Submitter::postCopy: starting  checkJobStatus timer" );
	mVerifyTimer = new QTimer(this);
	connect( mVerifyTimer, SIGNAL( timeout() ), SLOT( checkJobStatus() ) );
	mVerifyTimer->start(1000);
}

void Submitter::printJobInfo()
{
	LOG_3( "Submitter::printJobInfo()" );
	QFile file;
	QString msg = "Job Submitted: "+ QString::number(mJob.key())+"\n"; 
	file.open(stdout, QIODevice::WriteOnly);
	file.write(msg.toAscii(), qstrlen(msg.toAscii()));
	file.close();
}

void Submitter::checkJobStatus()
{
	mJob.reload();
	QString status = mJob.status();
	if( status.startsWith("verify") ) {
		LOG_3( "Submitter::checkJobStatus: waiting on Reaper" );
		return;
	}

	mVerifyTimer->stop();

	if( status == "holding" || status == "ready" || status == "started" ) {
		printJobInfo();
		_success();
		return;
	}

	// TODO: Alert IT
	// Set status to deleted, let the reaper clean up
	mJob.setStatus("deleted");
	mJob.commit();

	if( !mJob.isRecord() )
		exitWithError( "The job has disappeered from the database" );
	else {
		JobError error = mJob.verifyError();
		if( error.isRecord() ) {
			exitWithError( error.message() );
		} else
			exitWithError( "Unknown submission Error: The Job has status: " + mJob.status() );
	}
}

void Submitter::ftpStateChange( int state )
{
	QString t;
	switch( state ) {
		case QFtp::Unconnected:
			t = "Unconnected";
			break;
		case QFtp::HostLookup:
			t = "Host Lookup";
			break;
		case QFtp::Connecting:
			t = "Connecting";
			break;
		case QFtp::Connected:
			t = "Connected";
			break;
		case QFtp::LoggedIn:
			t = "Logged In";
			break;
		case QFtp::Closing:
			t = "Closing";
			break;
	}
	LOG_3( t );
}

void Submitter::ftpCommandStarted( int cmdId )
{
	//LOG_5( "Submitter::ftpCommandStarted: " + QString::number( cmdId ) );
	if( cmdId == mCdCmdId )
		LOG_3( "Sending CD command" );
	else if( cmdId == mMkDirCmdId )
		LOG_3( "Sending MKDIR command" );
	else if( cmdId == mPutCmdId )
		LOG_3( "Sending the file(PUT)" );
	else if( cmdId == mPutCmdId2 )
		LOG_3( "Sending the file(PUT #2)" );
}

void Submitter::ftpCommandFinished( int cmdId, bool error )
{
	//LOG_5( "Submitter::ftpCommandFinished: " + QString::number( cmdId ) + " error: " + QString(error ? "true" : "false") );
	if( cmdId == mCdCmdId ) {
		if( error ) {
			LOG_3( "CD command failed" );
			if( mCreateDirTried ) {
				// For some reason mkdir is not reporting an error even though it fails, so then the
				// subsequent cd cmd always fails.
				LOG_1( "Unable to create and navigate to users directory, aborting" );
				return;
			}
			mFtp->clearPendingCommands();
			issueFtpCommands( true );
		} else
			LOG_3( "CD successfull" );
	}
	if( cmdId == mMkDirCmdId ) {
		if( error ) {
			LOG_1( "Unable to create user directory, aborting" );
			exitWithError( "Unable to Create Directory On Server For User: " + mJob.user().name() );
			return;
		}
		LOG_3( "User Directory Created Successfully" );
	}
	if( cmdId == mPutCmdId ) {
		if( error ) {
			LOG_1( "PUT command failed" );
			return;
		}
		LOG_3( "PUT command finished successfully" );
	}
	if( cmdId == mPutCmdId2 ) {
		if( error ) {
			LOG_1( "PUT (2nd) command failed" );
			return;
		}
		LOG_3( "PUT (2nd) comand finished successfully" );
	}
}

void Submitter::ftpDone( bool error )
{
	mFile->close();
	mFile2->close();
	mFtp->disconnect( this );
	mFtp->close();

	if( error ) {
		LOG_3( "Error during ftp upload: " + mFtp->errorString() );
		mJob.remove();
		exitWithError( "Error Uploading File(Out of Disk Space?): " + mFtp->errorString() );
		return;
	}

	LOG_3( "Ftp upload successful" );
	postCopy();
}

void Submitter::ftpTransferProgress( qint64 done, qint64 total )
{
	int progress = int( done * 100.0 / double(total) );
	if( progress - mProgress >= 5 ) {
		LOG_3( "Uploaded " + QString::number( progress ) + "%" );
		mProgress = progress;
		emit uploadProgress( progress );
	}
}

void Submitter::ftpCleanup()
{
	delete mFtp;
	mFtp = 0;
	delete mFile;
	mFile = 0;
	delete mFile2;
	mFile2 = 0;
}

void Submitter::_success()
{
	mState = Success;
	emit stateChange( Success, QString() );
	emit submitSuccess();
	if( mExitAppOnFinish ) {
		foreach( QString dbName, QSqlDatabase::connectionNames() )
			QSqlDatabase::database( dbName, false ).close();
		qApp->exit( 0 );
	}
}

void Submitter::exitWithError( const QString & error )
{
	mState = Error;
	emit stateChange( Error, QString() );
	int number = writeErrorFile( error );
	mErrorText = error;
	emit submitError( error );
	LOG_1("ERROR: "+error);
	if( mExitAppOnFinish ) {
		foreach( QString dbName, QSqlDatabase::connectionNames() )
			QSqlDatabase::database( dbName, false ).close();
		qApp->exit( 1 );
	}
}

QString Submitter::errorText() const
{
	return mErrorText;
}

static int getNextErrorNumber( const QString & directory )
{
	int max = 0;
	QRegExp errorFileRE( "^(\\d+)\\.txt$" );
	QStringList files = QDir(directory).entryList( QDir::Files );
	foreach( QString file, files ) {
		if( errorFileRE.exactMatch( file ) ) {
			int number = errorFileRE.cap(1).toInt();
			max = qMax(max,number);
		}
	}
	return max + 1;
}

int writeErrorFile( const QString & error )
{
	QString errorsPath("c:/blur/absubmit/errors/");
	if( !QDir(errorsPath).exists() && !QDir().mkpath(errorsPath) ) {
		LOG_1( "Error creating error directory: " + errorsPath );
		return -1;
	}
	int number = getNextErrorNumber(errorsPath);
	QFile errorFile(errorsPath + "/" + QString::number(number) + ".txt");
	errorFile.open( QIODevice::WriteOnly );
	QTextStream out(&errorFile);
	out << error;
	errorFile.close();
	return number;
}

