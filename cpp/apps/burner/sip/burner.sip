
%Module blur.Burner 0

%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip
%Import blur/blurqt.sip
%Import blur/classes.sip

%DefaultSupertype sip.wrapper

%VirtualErrorHandler JobBurnerVirtualErrorHandler
        throw PythonException();
%End

%ModuleHeaderCode
	class JobBurner;
	void JobBurnerVirtualErrorHandler( const JobBurner *, sipSimpleWrapper * );
%End

%ModuleCode
	void JobBurnerVirtualErrorHandler( const JobBurner * burner, sipSimpleWrapper * /*pyObject*/ )
	{
		SIP_BLOCK_THREADS
		PyErr_Print();
		SIP_UNBLOCK_THREADS
		const_cast<JobBurner*>(burner)->jobErrored( "Exception occurred in python burner: Please contact IT" ); //runPythonFunction( "traceback", "format_exc", VarList() ).toString() );
	}
%End



class Slave : QObject
{
%TypeHeaderCode
#include <slave.h>
%End
public:
	Slave(QObject * parent /TransferThis/ =0);
	~Slave();

	HostStatus hostStatus() const;
	Host host() const;

	QString status() const;

	void clientUpdate( bool onlineAfterUpdate = true );

	Spooler * spooler();

	bool usingGui() const;

	bool inOwnLogonSession();

	void setInOwnLogonSession( bool iols );

	QString logRootDir() const;

	QString currentPriorityName() const;

	QList<JobBurner*> activeBurners();
	JobAssignmentList activeAssignments();

public slots:
	void setStatus( const QString & status, bool force = false );

	void setUseGui( bool );

	void slotError( const QString & text );
	void slotBurnFinished();

	void online();
	void offline();
	void toggle();

	void offlineFromAboutToQuit();

	void showRemapWarning(const QString & drive);

	void setIsMapped( bool );

signals:
	void statusChange( const QString & status );

	void copyProgressChange( int );

	void activeAssignmentsChange( JobAssignmentList activeAssignments, JobAssignmentList oldActiveAssignments );
};

class Spooler : QObject
{
%TypeHeaderCode
#include <spooler.h>
%End
public:
	Spooler( QObject * parent /TransferThis/ );
	~Spooler();

	QString spoolDir() const;
};


class JobBurner : QObject /VirtualErrorHandler=JobBurnerVirtualErrorHandler/
{
%TypeHeaderCode
#include <jobburner.h>
#include "pyembed.h"
%End
public:
	JobBurner( const JobAssignment & job, Slave * slave /TransferThis/ );
	~JobBurner();

	enum State {
		StateNew,
		StateStarted,
		StateDone,
		StateError
	};

	enum Options {
		OptionCheckDone = 1,
		OptionIgnoreStdError = 2,
		OptionMergeStdError = 4,
		OptionProcessIOReadAll = 8
	};

	State state() const;
	bool isActive() const;
	Options options() const;

	JobAssignment jobAssignment() const;

	QString assignedTasks() const;
	JobTaskList currentTasks() const;
	bool taskStarted() const;
	bool exceededMaxTime() const;

	QDateTime startTime() const;
	QDateTime taskStartTime() const;

	virtual QStringList processNames() const;

	virtual QString executable();

	virtual QString workingDirectory();

	virtual QStringList buildCmdArgs();

	virtual QStringList environment();

	QProcess * process() const;

	QString burnFile() const;

	QString burnDir() const;

	static void logError( const Job & j, const QString & msg, const JobTaskList & jtl = JobTaskList() );

protected:

	bool taskStart( int frame, const QString & outputName = QString(), int secondsSinceStarted = 0 );
	void taskDone( int frame );
    void jobErrored( const QString &, bool timeout=false, const QString & nextstate="new" );

	QTimer * checkupTimer();

	void logMessage( const QString &, QProcess::ProcessChannel = QProcess::StandardOutput );

	virtual void connectProcess( QProcess * );

	virtual void startProcess();

	void copyFrameNth( int frame, int frameNth, int frameRangeEnd );

public slots:
	virtual void start();

	virtual void startBurn();

	virtual void cleanup();

signals:
	void finished();
	void errored( const QString & );

	void fileGenerated( const QString & );

protected slots:
	void jobFinished();

	virtual bool checkup();

	virtual void slotReadStdOut();
	virtual void slotReadStdError();
	virtual void slotProcessExited();
	virtual void slotProcessStarted();

	virtual void slotProcessError( QProcess::ProcessError );

	virtual void slotProcessOutput( const QByteArray & output, QProcess::ProcessChannel );

	virtual void slotProcessOutputLine( const QString & output, QProcess::ProcessChannel );

	void syncFile( const QString & );

	void updateOutput();

	virtual void checkMemory();

	void slotCopyFinished();
	void slotCopyError( const QString & text, int );

protected:
	void deliverOutput( QProcess::ProcessChannel );
	bool checkIgnoreLine( const QString & );
};

class JobBurnerPlugin
{
%TypeHeaderCode
#include <jobburnerplugin.h>
%End
public:
	JobBurnerPlugin();
	virtual ~JobBurnerPlugin();
	virtual QStringList jobTypes()=0;
	virtual JobBurner * createBurner( const JobAssignment & job, Slave * slave )=0;
};

class JobBurnerFactory
{
%TypeHeaderCode
#include <jobburnerfactory.h>
%End
	public:
	static bool supportsJobType( const Job & job );
	static JobBurner * createBurner( const JobAssignment & job, Slave * slave, QString * errMsg );
	static void registerPlugin( JobBurnerPlugin * bp /Transfer/, bool overrideExisting = false );
};


