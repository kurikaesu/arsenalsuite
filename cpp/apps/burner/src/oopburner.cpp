
#include "process.h"

#include "oopburner.h"

OOPBurner::OOPBurner( const JobAssignment & jobAssignment, Slave * slave )
: JobBurner( jobAssignment, slave )
, mProcess( 0 )
{}

OOPBurner::~OOPBurner()
{}

QStringList OOPBurner::processNames() const
{
	return QStringList() << "assburner.exe";
}

QString OOPBurner::executable()
{
	return workingDirectory() + "assburner.exe";
}

QString OOPBurner::workingDirectory()
{
	return "c:/blur/assburner/";
}

QStringList OOPBurner::buildCmdArgs()
{
	return QStringList() << "-burn" << "-in-own-logon-session";
}

QStringList OOPBurner::environment()
{
	return QStringList();
}

void OOPBurner::startProcess()
{
	mProcess = new Win32Process(this);
	connect( mProcess, SIGNAL(started()), SLOT(slotProcessStarted()) );
	connect( mProcess, SIGNAL(error(QProcess::ProcessError)), SLOT(slotProcessError(QProcess::ProcessError)) );
	connect( mProcess, SIGNAL(finished(int)), SLOT(slotProcessExited()) );
	if( mState == StateError ) return;
	QString cmd = executable();
	if( mState == StateError ) return;
	QStringList args = buildCmdArgs();
	if( mState == StateError ) return;
	QString wholeCmd = cmd + " " + args.join(" ");
	QString wd = workingDirectory();
	if( mState == StateError ) return;
	QString userString = mJob.domain().isEmpty() ? mJob.userName() : mJob.domain() + "/" + mJob.userName();
	LOG_5( "Starting command: " + wholeCmd + (wd.isEmpty() ? QString() : QString(" in directory " + wd)) + " as user " + userString );
	if( !wd.isEmpty() )
		mProcess->setWorkingDirectory( workingDirectory() );
	QStringList env = environment();
	if( !env.isEmpty() )
		mProcess->setEnvironment( env );
	mProcess->setLogon( mJob.userName(), mJob.password(), mJob.domain() );
	mProcess->start( cmd, args );
}

void OOPBurner::start()
{
	// Go directly to startBurn, skip the copy step as this will
	// be done by the spawned assburner
	startBurn();
}

void OOPBurner::startBurn()
{
	// Skip drive mapping, JCH setup, memory and output timers
	startProcess();
}

void OOPBurner::cleanup()
{
	JobBurner::cleanup();
}

bool OOPBurner::checkup()
{
	return true;
}

void OOPBurner::slotProcessExited()
{
}

void OOPBurner::slotProcessStarted()
{
	LOG_5( "Process started with pid: " + QString::number(qpidToId(mProcess->pid())) );
}

void OOPBurner::slotProcessError( QProcess::ProcessError )
{
}
