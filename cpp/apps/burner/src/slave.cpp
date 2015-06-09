
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

#include "Python.h"

#include <qapplication.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qmessagebox.h>
#include <qprocess.h>
#include <qregexp.h>
#include <qtcpserver.h>
#include <qtimer.h>
#include <qsocketnotifier.h>

#ifndef Q_OS_WIN
#include <sys/socket.h>
#endif

#include "common.h"
#include "idle.h"
#include "killdialog.h"
#include "maindialog.h"
#include "mapwarningdialog.h"
#include "slave.h"
#include "spooler.h"

#include "blurqt.h"
#include "freezercore.h"
#include "config.h"
#include "database.h"
#include "path.h"
#include "process.h"
#include "remotelogserver.h"

#include "builtinburnerplugin.h"
#include "employee.h"
#include "group.h"
#include "groupmapping.h"
#include "hostservice.h"
#include "hoststatus.h"
#include "job.h"
#include "jobassignmentstatus.h"
#include "jobburner.h"
#include "jobburnerfactory.h"
#include "jobtype.h"
#include "jobtypemapping.h"
#include "jobhistory.h"
#include "joberror.h"
#include "jobstatus.h"
#include "mapping.h"
#include "oopburner.h"
#include "syslog.h"
#include "user.h"
#include "usermapping.h"
#include "usergroup.h"


#ifdef Q_OS_WIN
#include "windows.h"
#include "reason.h"
#endif // Q_OS_WIN

int Slave::sigintFd[2];
int Slave::sigtermFd[2];

Slave::Slave( bool gui, bool autoRegister, int jobAssignmentKey, QObject * parent )
: QObject( parent )
, mPulsePeriod( 60 )
, mIdle( 0 )
, mIdleDelay( -1 )
, mSpooler( 0 )
, mKillDialog( 0 )
, mInsideLoop( false )
, mLostConnectionTimer( 0 )
, mBackgroundMode( false )
, mIsMapped( false )
, mUseGui( gui )
, mAutoRegister( autoRegister )
, mInOwnLogonSession( false )
, mBurnOnlyJobAssignmentKey( jobAssignmentKey )
{
    QTimer::singleShot( 10, this, SLOT( startup() ) );

#ifndef Q_OS_WIN
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigintFd))
        qFatal("Couldn't create TERM socketpair");

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermFd))
        qFatal("Couldn't create TERM socketpair");

    snTerm = new QSocketNotifier(sigtermFd[1], QSocketNotifier::Read, this);
    connect(snTerm, SIGNAL(activated(int)), this, SLOT(handleSigTerm()));

    snInt = new QSocketNotifier(sigintFd[1], QSocketNotifier::Read, this);
    connect(snInt, SIGNAL(activated(int)), this, SLOT(handleSigInt()));
#endif
}

#ifndef Q_OS_WIN
void Slave::intSignalHandler(int)
{
    char a = '2';
    ::write(sigintFd[0], &a, sizeof(a));
}

void Slave::termSignalHandler(int)
{
    char a = '2';
    ::write(sigtermFd[0], &a, sizeof(a));
}

void Slave::handleSigInt()
{
    snInt->setEnabled(false);
    char tmp;
    ::read(sigintFd[1], &tmp, sizeof(tmp));
 
    setStatus("stopping", true);
    handleStatusChange( "offline", "stopping" );
    exit(0);
}

void Slave::handleSigTerm()
{
    snTerm->setEnabled(false);
    char tmp;
    ::read(sigtermFd[1], &tmp, sizeof(tmp));
 
    setStatus("stopping", true);
    handleStatusChange( "offline", "stopping" );
    exit(0);
}
#endif

void Slave::startup()
{
    registerBuiltinBurners();
    loadEmbeddedPython();

    LOG_5( "Slave::startup()" );
    mHost = Host::currentHost();

    mSpooler = new Spooler( this );

    if( !mHost.isRecord() ) {
        if( mAutoRegister ) {
            LOG_3( "Slave::startup: no host record, auto-registering" );
            mHost = Host::autoRegister();
        } else {
            LOG_3( "Slave::startup: no host record, uh oh!" );
            exit(-1);
        }
    } else
        mHost.updateHardwareInfo();

    mHostStatus = mHost.hostStatus();

    if( !mHostStatus.isRecord() ) {
        if( mAutoRegister ) {
            LOG_3( "Slave::startup: No host status record(are all your triggers installed?), creating one" );
            mHostStatus.setHost( mHost );
            mHostStatus.setOnline( 1 );
        } else {
            LOG_3( "Slave::startup: no host status record, uh oh!" );
            exit(-1);
        }
    }
    mHostStatus.setAvailableMemory( mHost.memory() );
    mHostStatus.commit();

    connect( Database::current()->connection(), SIGNAL( connectionLost() ), SLOT( slotConnectionLost() ) );
    connect( Database::current()->connection(), SIGNAL( connected() ), SLOT( slotConnected() ) );

    // The rest of the initialization is only for normal mode where we moniter and manipulate
    // the hosts status.  In burn only mode we simply execute a job and exit
    if( mBurnOnlyJobAssignmentKey ) {
        burn( JobAssignment(mBurnOnlyJobAssignmentKey) );
        return;
    }

    mService = Service::ensureServiceExists("Assburner");

    // Start remote log server and set port in our Assburner HostService record
    RemoteLogServer * rls = new RemoteLogServer();
    HostService hs = HostService::recordByHostAndService( mHost, mService );
    if( hs.isRecord() && rls->tcpServer()->isListening() ) {
        hs.setRemoteLogPort( rls->tcpServer()->serverPort() );
        hs.commit();
    }

    // Set host version string.
    mHost.setAbVersion( "v" + QString(VERSION));
    mHost.commit();

    // Reset any frame assignments and set our host status
    // to ready.  Do we need this anymore?  The reaper
    // should now detect and re-assign frames if we are
    // offline, and this prevents a client-update from
    // happening when assburner starts.
    //
    // Nope, this doesn't fuck with the status, so it is
    // safe.  It just clears fkeyJob to slaveFrames.  And
    // returns any frames that have fkeyHost=this
    mHostStatus.returnSlaveFrames();

    loadForbiddenProcesses();

    mTimer = new QTimer( this );
    connect( mTimer, SIGNAL( timeout() ), SLOT( loop() ) );

    // Default 5000ms, min 200ms, max 600,000ms - 10 minutes
    mLoopTime = qMax( 200, qMin( 1000 * 60 * 10, Config::getInt( "assburnerLoopTime", 5000 ) ) );

    // Default mLoopTime / 2,  min 100ms, max mLoopTime
    mQuickLoopTime = qMax( 100, qMin( mLoopTime, Config::getInt( "assburnerQuickLoopTime", mLoopTime / 2 ) ) );

    // Default 60 seconds, min 10 seconds.
    mPulsePeriod = qMax( 10, Config::getInt( "arsenalPulsePeriod", 600 ) );

    // Default 60 seconds, min 10 seconds.
    mMemCheckPeriod = qMax( 10, Config::getInt( "abMemCheckPeriod", 60 ) );

    IniConfig & c = config();

    c.pushSection( "BackgroundMode" );
    mBackgroundModeEnabled = c.readBool( "Enabled", true );
    c.popSection();

    // Pulse right away.
    pulse();

    // We need to set our host status to offline when assburner quits
    connect( qApp, SIGNAL( aboutToQuit() ), SLOT( offlineFromAboutToQuit() ) );

    if( mUseGui ) {
        mIdle = new Idle();
        connect( mIdle, SIGNAL( secondsIdle( int ) ), SLOT( slotSecondsIdle( int ) ) );
        mIdle->start();
    }

    if( mHostStatus.slaveStatus() == "client-update-offline" )
        offline();
    else if( mHostStatus.slaveStatus() == "client-update" )
        clientUpdate();
    else {
        // Store the current status
        QString currentStatus = mHostStatus.slaveStatus();

        // We run the host through the online method first
        online();

        // Check if the old host status wasn't empty
        if( !currentStatus.isEmpty() ){
            // Resume the state the host was in.
            LOG_3("Slave::startup() Resuming previous status of " + currentStatus);
            handleStatusChange(currentStatus, "");
        }
    }

    // Create a timer for logged in user checks
    mUserTimer = new QTimer( this );
    connect( mUserTimer, SIGNAL( timeout() ), SLOT( updateLoggedUsers() ) );

    // Set it to trigger every 5 mins
    mUserTimer->start(300000);

    LOG_5( "Slave::startup() done" );
}

Slave::~Slave()
{
    LOG_5("Slave::dtor() called");
    foreach( JobBurner * burner, (mActiveBurners + mBurnersToDelete) )
        delete burner;
    mActiveBurners.clear();
    if( mUserTimer ){
        mUserTimer->disconnect(this);
        delete mUserTimer;
        mUserTimer = 0;
    }
    if( mTimer ) {
        mTimer->disconnect(this);
        delete mTimer;
        mTimer = 0;
    }
    if ( mLostConnectionTimer ) {
        delete mLostConnectionTimer;
        mLostConnectionTimer = 0;
    }
    LOG_5("Slave::dtor() done");
}

#ifdef Q_OS_WIN
extern "C" void initsip(void);
extern "C" void initStone(void);
extern "C" void initClasses(void);
#endif
extern "C" void initBurner(void);

void Slave::loadEmbeddedPython()
{
    LOG_5( "Loading Python" );
    /*
     * This structure specifies the names and initialisation functions of
     * the builtin modules.
     */
    struct _inittab builtin_modules[] = {
#ifdef Q_OS_WIN
        {"sip", initsip},
        {"blur.Stone",initStone},
        {"blur.Classes",initClasses},
#endif
        {"blur.Burner", initBurner},
        {NULL, NULL}
    };

    PyImport_ExtendInittab(builtin_modules);
    
    Py_Initialize();

    const char * builtinModuleImportStr = 
#ifdef Q_OS_WIN
        "import imp,sys\n"
        "class MetaLoader(object):\n"
        "\tdef __init__(self):\n"
        "\t\tself.modules = {}\n"

        "\t\tself.modules['blur.Stone'] = imp.load_module('blur.Stone',None,'',('','',imp.C_BUILTIN))\n"
        "\t\tself.modules['blur.Classes'] = imp.load_module('blur.Classes',None,'',('','',imp.C_BUILTIN))\n"

        "\t\tself.modules['blur.Burner'] = imp.load_module('blur.Burner',None,'',('','',imp.C_BUILTIN))\n"
        "\tdef find_module(self,fullname,path=None):\n"
//      "\t\tprint 'MetaLoader.find_module: ', fullname, path\n"
        "\t\tif fullname in self.modules:\n"
        "\t\t\treturn self.modules[fullname]\n"
//      "\t\tprint 'MetaLoader.find_module: Returning None'\n"
        "sys.meta_path.append(MetaLoader())\n"
        "import blur\n"
        "blur.RedirectOutputToLog()\n";
#else
        "import imp,sys\n"
        "class MetaLoader(object):\n"
        "\tdef __init__(self):\n"
        "\t\tself.modules = {}\n"
//  blur.Stone and blur.Classes dynamically loaded
        "\t\tself.modules['blur.Burner'] = imp.load_module('blur.Burner',None,'',('','',imp.C_BUILTIN))\n"
        "\tdef find_module(self,fullname,path=None):\n"
//      "\t\tprint 'MetaLoader.find_module: ', fullname, path\n"
        "\t\tif fullname in self.modules:\n"
        "\t\t\treturn self.modules[fullname]\n"
//      "\t\tprint 'MetaLoader.find_module: Returning None'\n"
        "sys.meta_path.append(MetaLoader())\n"
        "import blur\n"
        "blur.RedirectOutputToLog()\n";
#endif
    PyRun_SimpleString(builtinModuleImportStr);

    LOG_5( "Loading python plugins" );
    
    QDir plugin_dir( "plugins" );
    QStringList el = plugin_dir.entryList(QStringList() << "*.py" << "*.pys" << "*.pyw", QDir::Files);
    foreach( QString plug, el ) {
        QString name("plugins/" + plug);
        LOG_5( "Loading plugin: " + name );
        QFile f( name );
        f.open( QIODevice::ReadOnly );
        QTextStream ts(&f);
        QString contents = ts.readAll();
        contents.replace("\r","");
        PyRun_SimpleString(contents.toLatin1());
    }
}

void Slave::pulse()
{
    // Burner pulse
    QDateTime cdt( QDateTime::currentDateTime() );
    if( !mLastPulse.isValid() || mLastPulse.secsTo(cdt) >= mPulsePeriod ) {
        LOG_3( "Slave::pulse(): Emitting pulse at " + cdt.toString(Qt::ISODate) );
        //mService.pulse();
        mHostStatus.setColumnLiteral("slavePulse", "now()");
        mHostStatus.commit();
        mLastPulse = cdt;
    }
}

void Slave::setAvailableMemory()
{
    LOG_3("Updating available memory.");

    int avail = mHostStatus.host().memory() * 1024;

    foreach( JobBurner * burner, mActiveBurners ) {
        int mem_job_needs = burner->jobAssignment().job().minMemory();
        if( burner->jobAssignment().assignMinMemory() > 0 )
            mem_job_needs = burner->jobAssignment().assignMinMemory();

        int mem_job_using = burner->jobAssignment().maxMemory();

        mem_job_needs = qMax( mem_job_needs, mem_job_using );
        if ( mem_job_needs == 0 )
            mem_job_needs = burner->jobAssignment().job().jobStatus().averageMemory();

        avail -= mem_job_needs;
        LOG_3(QString("Job %1 requires memory %2 available is %3").arg(burner->jobAssignment().job().key()).arg(mem_job_needs).arg(avail));
    }

    avail = qMax( avail, 0 );
    mHostStatus.setAvailableMemory( avail / 1024 );
    mHostStatus.commit();
}

bool Slave::checkFusionRunning()
{
    /*
    // don't check for fusion running if we're in the middle of a batch job
    if (mJob.isRecord() && mJob.jobType().name() == "Batch") {
        return false;
    }

    // don't check for fusion running if we're in the middle of a batch job
    if (mJob.isRecord() && mJob.jobType().name() == "Fusion") {
        return false;
    }

    // if Fusion4 or 5 flow rendering, go offline
    bool fusionRunning = false;
    if (pidsByName("eyeonServer.exe") > 0) {
        LOG_3("Slave::checkFusionRunning deteced eyeon Fusion5 render process, going offline");
        fusionRunning = true;
    }
    if (pidsByName("Overseer.exe") > 0 || pidsByName("overseer.exe") > 0 ) { 
        LOG_3("Slave::checkFusionRunning detected eyeon Fusion4 render process, going offline");
        fusionRunning = true;
    }
    return fusionRunning;
    */
    return false;
}

void Slave::loop()
{
    if( mInsideLoop ) return;
    mInsideLoop = true;

    LOG_6( "Slave::loop called");
    foreach( JobBurner * burner, mBurnersToDelete )
        delete burner;
    mBurnersToDelete.clear();

    LOG_6("Checking for mKillDialog..");
    if( mKillDialog ) {
        LOG_3( "Slave::loop: Kill Dialog is active, returning without new status" );
        mInsideLoop = false;
        return;
    }
    LOG_6("Done checking for mKillDialog..");

    // Fetch new status from the database
    QString oldStatus = mHostStatus.slaveStatus();
    mHostStatus.reload();
    QString status = mHostStatus.slaveStatus();

    pulse();

    handleStatusChange( status, oldStatus );

    if( mHostStatus.lastAssignmentChange() != mLastAssignmentChange ) {
        mLastAssignmentChange = mHostStatus.lastAssignmentChange();

        QString assignmentsByKey;
        if( mActiveAssignments.size() )
            assignmentsByKey += " OR keyjobassignment IN (" + mActiveAssignments.keyString() + ")";

        JobAssignmentList assignments = JobAssignment::select( "fkeyhost=? AND (fkeyjobassignmentstatus=? " + assignmentsByKey + ")",
            VarList() << mHost.key() << JobAssignmentStatus::recordByName("ready").key() );

        foreach( JobAssignment ja, assignments )
            handleJobAssignmentChange( ja );
    }

    // Decrement the mQuickLoopCount each time through the loop,
    // when it reaches 0, go back to normal loop time
    if( mQuickLoopCount > 0 ) {
        mQuickLoopCount--;
        if( mQuickLoopCount == 0 )
            mTimer->start( mLoopTime );
    }

    // attempt to cleanup any files in list
    mSpooler->deleteFiles();
    mInsideLoop = false;
}

void Slave::handleStatusChange( const QString & status, const QString & oldStatus )
{
    // Return if nothing has changed
    if( oldStatus == status ) {
        LOG_6("oldStatus same as mHost.slaveStatus, returning");
        // why was this getting set here?
        mInsideLoop = false;
        return;
    }

    // Check for a user logged in status upon change of status
    updateLoggedUsers();

    LOG_3( "Got new status: " + status );

    if ( status == "client-update" ){
        emit statusChange( "busy" );
        clientUpdate( oldStatus != "offline" );
    } else if( status == "offline" || status == "stopping" || status == "maintenance" ) {
        // When we get set offline remotely, don't go online again
        // Until someone presses Start the burn, or until
        // we are set online remotely.  The idledelay
        // will be reset when we go online again
        mIdleDelay = -1;
        offline();
    } else if( status == "starting" ) { 
        if( oldStatus != "ready" )
            online();
    } else if( status == "restart" ) {
        restart();
    } else if( status == "restart-when-done" ) {
        mHostStatus.reload();
        if( mHostStatus.activeAssignmentCount() == 0 )
            restart();
        else
            return;
    } else if( status == "reboot" ) {
        reboot();
    } else if( status == "reboot-when-done" ) {
        mHostStatus.reload();
        if( mHostStatus.activeAssignmentCount() == 0 )
            reboot();
        else
            return;
    } else if( status == "offline-when-done" ) {
        mHostStatus.reload();
        if( mHostStatus.activeAssignmentCount() == 0 )
            emit statusChange( "offline ");
        else
            return;
    } else if( status == "shutdown" ) {
        shutdown();
    } else if( status == "shutdown-when-done" ) {
        mHostStatus.reload();
        if( mHostStatus.activeAssignmentCount() == 0 )
            shutdown();
        else
            return;
    } else if( status == "sleep" ) {
        goToSleep();
    } else if( status == "busy" ) {
        // When using OOPBurner the child process sets the status to busy
        emit statusChange( status );
    } else {
        emit statusChange( "error" );
        // Go to ready if status is unknown.
        reset();
    }
}

void Slave::restartFromLostConnectionTimer()
{
    LOG_5("Slave::restartFromLostConnectionTimer() called");

    if (mLostConnectionTimer)
        mLostConnectionTimer->disconnect(this);

    cleanup();

    LOG_5("Slave::restartFromLostConnectionTimer() calling qApp->disconnect(this)");
    qApp->disconnect( this );

    LOG_5("Slave::restartFromLostConnectionTimer() done, calling qApp->quit()");
    qApp->quit();

    LOG_5("Slave::restartFromLostConnectionTimer() done, calling exit");
    exit(-1);
}

void Slave::restart()
{
    LOG_5("Slave::restart() called");
    // Cleanup any current jobs
    cleanup();
    // Return any job assignments
    LOG_5("Slave::restart() calling mHostStatus.returnSlaveFrames()");
    mHostStatus.returnSlaveFrames();
    LOG_5("Slave::restart() call to mHostStatus.returnSlaveFrames() done");
    // Disconnect this, so that we dont set the status to offline
    // the status will stay at restart
    LOG_5("Slave::restart() calling qApp->disconnect(this)");
    qApp->disconnect( this );

#ifdef Q_OS_WIN
    LOG_5( "Closing mutex" );
    CloseHandle( hMutex );
#endif

    // Restart with the same arguments
    QProcess::startDetached( "assburner.exe", QCoreApplication::instance()->arguments() );

    LOG_5("Slave::restart() done, calling qApp->quit()");
    qApp->quit();
}

void Slave::reboot()
{
    LOG_5("Slave::reboot() called");

    // Cleanup any current jobs
    cleanup();

    // Return any job assignments
    LOG_5("Slave::reboot() calling mHostStatus.returnSlaveFrames()");
    mHostStatus.returnSlaveFrames();
    LOG_5("Slave::reboot() call to mHostStatus.returnSlaveFrames() done");

    // Disconnect this, so that we dont set the status to offline
    // the status will stay at restart
    LOG_5("Slave::reboot() calling qApp->disconnect(this)");
    qApp->disconnect( this );

    // Since we're rebooting, set the status to starting instead.
    setStatus( "starting" , true );
    systemShutdown(/*reboot=*/true);
#ifndef Q_OS_WIN
    sleep(60);
#endif
}

void Slave::shutdown()
{
    LOG_5("Slave::shutdown() called");

    // Cleanup any current jobs
    cleanup();

    // Return any job assignments
    LOG_5("Slave::reboot() calling mHostStatus.returnSlaveFrames()");
    mHostStatus.returnSlaveFrames();
    LOG_5("Slave::reboot() call to mHostStatus.returnSlaveFrames() done");

    // Disconnect this, so that we dont set the status to maintenance
    // the status will stay at restart
    LOG_5("Slave::reboot() calling qApp->disconnect(this)");
    qApp->disconnect( this );

    setStatus( "offline" , true );
    systemShutdown(/*reboot=*/false);
#ifndef Q_OS_WIN
    sleep(60);
#endif
}

void Slave::goToSleep()
{
    LOG_5("Slave::goToSleep() called");
    // Cleanup any current jobs
    cleanup();

    // Return any job assignments
    if( !setStatus( "sleeping" ) )
        return;

    // Disconnect this, so that we dont set the status to offline
    // the status will stay at sleeping
    LOG_5("Slave::restart() calling qApp->disconnect(this)");
    qApp->disconnect( this );

    if( systemShutdown() ) {
        killAll( "abpsmon.exe" );
        LOG_5("Slave::goToSleep() done, calling qApp->quit()");
        qApp->quit();
    } else
        reset();
}

void Slave::clientUpdate( bool onlineAfterUpdate )
{
    // Go offline right now
    offline();
    mTimer->stop();

    // Dont want offline to get called again when we quit
    qApp->disconnect( this );

    if( !onlineAfterUpdate )
        setStatus( "client-update-offline" );

    // Reload the config entry just in case it has changed
    Config c = Config::recordByName( "assburnerInstallerPath" );
    c.reload();

    QString install = c.value();
    //QFileInfo fi( install );
    //QString tp( "c:\\temp\\" + fi.fileName() );
    //Path::copy( install, tp );
    LOG_3( "Slave::clientUpdate() running " + install );
    QProcess::startDetached( install );
    LOG_5( "Slave::clientUpdate() quit()" );
    qApp->quit();
}

QList<JobBurner*> Slave::activeBurners()
{
    return mActiveBurners;
}

JobAssignmentList Slave::activeAssignments()
{
    return mActiveAssignments;
}

JobBurner * Slave::setupJobBurner( const JobAssignment & jobAssignment )
{
    JobBurner * burner = 0;
    QString err;
#ifdef Q_OS_WIN
    if( !mBurnOnlyJobAssignmentKey && !jobAssignment.job().userName().isEmpty() ) {
        burner = new OOPBurner( jobAssignment, this );
    } else
#endif // Q_OS_WIN
    {
        burner = JobBurnerFactory::createBurner( jobAssignment, this, &err );
    }

    if( !burner ) {
        Job job = jobAssignment.job();
        JobBurner::logError( job, QString("Couldn't Create Burner for Job %1, Job Type %2: ").arg( job.key() ).arg( job.jobType().name() ) + err );
        return 0;
    }

    if( !burner->isActive() ) {
        LOG_5( "Burner already inactive, adding to mBurnersToDelete" );
        cleanupJobBurner(burner);
        return 0;
    }

    connect( burner, SIGNAL( errored( const QString & ) ), SLOT( slotError( const QString & ) ) );
    connect( burner, SIGNAL( finished() ), SLOT( slotBurnFinished() ) );

    return burner;
}

void Slave::burn( const JobAssignment & jobAssignment )
{
    LOG_TRACE

    LOG_3( "Slave: Starting the burn" );
    Job job = jobAssignment.job();

    if( !JobBurnerFactory::supportsJobType( job ) ) {
        slotError( QString("JobType %1 not supported by this host for job %2").arg( job.jobType().name() ).arg( job.key() ) );
        return;
    }

    JobBurner * burner = setupJobBurner( jobAssignment );
    if( burner ) {
        mActiveBurners += burner;
        setActiveAssignments( mActiveAssignments + jobAssignment );
        burner->start();
        emit statusChange( "busy" );
    }
}

void Slave::setActiveAssignments( JobAssignmentList activeAssignments )
{
    JobAssignmentList oldActiveAssignments( mActiveAssignments );
    mActiveAssignments = activeAssignments;
    emit activeAssignmentsChange( activeAssignments, oldActiveAssignments );
}

void Slave::handleJobAssignmentChange( const JobAssignment & ja )
{
    QString status = ja.jobAssignmentStatus().status();
    if( status == "ready" ) {
        burn(ja);
    }
    else if( status == "cancelled" || status == "suspended" ) {
        foreach( JobBurner * burner, mActiveBurners ) {
            if( burner->jobAssignment() == ja ) {
                cleanupJobBurner(burner);
                break;
            }
        }
    }

    /*
    if( (status == "assigned" || status == "copy" || status == "busy") && checkFusionRunning() ) {
        offline();
        mIdleDelay = 1;
        mInsideLoop = false;
        return;
    }

    if( status == "assigned" ){
        if( !checkForbiddenProcesses() )
            reset();
        burn();
    }
    */
}

Host Slave::host() const
{
    return mHost;
}

HostStatus Slave::hostStatus() const
{
    return mHostStatus;
}

QString Slave::status() const
{
    return mHostStatus.slaveStatus();
}

/*
static bool burnerDone( JobBurner * burner )
{
    return !burner || burner->state() == JobBurner::StateDone || burner->state() == JobBurner::StateError;
}
*/

void Slave::slotBurnFinished()
{
    JobBurner * burner = qobject_cast<JobBurner*>(sender());
    if( burner ) cleanupJobBurner(burner);

    mHostStatus.reload();
    if( status() == "restart-when-done" ) {
        if( mHostStatus.activeAssignmentCount() == 0 )
            restart();
    }
    if( status() == "reboot-when-done" ) {
        if( mHostStatus.activeAssignmentCount() == 0 )
            reboot();
    }
    if( status() == "offline-when-done" ) {
        if( mHostStatus.activeAssignmentCount() == 0 )
            emit statusChange( "offline" );
    }
    if( status() == "shutdown-when-done" ) {
        if( mHostStatus.activeAssignmentCount() == 0 )
            shutdown();
    }
}

void Slave::slotError( const QString & /*msg*/ )
{
    JobBurner * burner = qobject_cast<JobBurner*>(sender());
    if( burner ) cleanupJobBurner(burner);

    mHostStatus.reload();
    if( status() == "restart-when-done" ) {
        if( mHostStatus.activeAssignmentCount() == 0 )
            restart();
    }
    if( status() == "reboot-when-done" ) {
        if( mHostStatus.activeAssignmentCount() == 0 )
            reboot();
    }
    if( status() == "offline-when-done" ) {
        if( mHostStatus.activeAssignmentCount() == 0 )
            emit statusChange( "offline" );
    }
    if( status() == "shutdown-when-done" ) {
        if( mHostStatus.activeAssignmentCount() == 0 )
            shutdown();
    }
}

bool Slave::checkForbiddenProcesses()
{
    bool ret = true;
    if( mUseGui && !mKillDialog ) {
        QStringList procsToKill;
        LOG_3( "Checking Forbidden process list: " + mForbiddenProcesses.join(",") );
        foreach( QString procName, mForbiddenProcesses )
            if( pidsByName( procName ) > 0 )
                procsToKill << procName;

        if( procsToKill.size() > 0 ) {
            LOG_3( "Showing killdialog, forbidden processes running: " + procsToKill.join(",") );
            mTimer->stop();
            if( mIdle )
                mIdle->stop();
            mKillDialog = new KillDialog(procsToKill);
            mKillDialog->show();
            if( mKillDialog->exec() != QDialog::Accepted )
                ret = false;
            delete mKillDialog;
            mKillDialog = 0;
            mTimer->start( mLoopTime );
            if( mIdle )
                mIdle->start();
            LOG_3( "Slave::killLeftover: mIdle->start()'d, killdialog closed" );
        }
    }
    LOG_3("Slave::killLeftover() is about to return " + ret);
    return ret;
}

void Slave::cleanupJobBurner( JobBurner * burner )
{
    LOG_TRACE
    burner->disconnect(this);
    burner->cleanup();
    mBurnersToDelete += burner;
    mActiveBurners.removeAll(burner);
    setActiveAssignments( mActiveAssignments - burner->jobAssignment() );
    setAvailableMemory();
    if( mActiveBurners.size() == 0 && this->status() == "ready" )
        emit statusChange( "ready" );
}

// Cleans up any currently active job(s)
void Slave::cleanup()
{
    LOG_TRACE
    foreach( JobBurner * burner, mActiveBurners )
        cleanupJobBurner(burner);
}

// Resets assburner to offline or online
void Slave::reset( bool offline )
{
    // This will be called when the burner(s) are done
    if( mBurnOnlyJobAssignmentKey ) {
        qApp->exit(0);
        return;
    }

    LOG_5("Slave::reset called, current status is " + QString(status()) + ". Slave::reset() called with " + QString(offline ? "true" : "false"));

    if( status() == "restart-when-done" ) {
        setStatus( "restart", true );
        restart();
        return;
    }
    if( status() == "reboot-when-done" ) {
        reboot();
        return;
    }
    if( status() == "shutdown-when-done") {
        shutdown();
        return;
    }

    cleanup();

  // This makes sure assburner doesn't pick up a job when a conflicting program is running
    if( !offline && !checkForbiddenProcesses() )
        offline = true;
    
    if( !offline ) {
        IniConfig & c = config();
        c.pushSection( "Assburner" );
        mIdleDelay = c.readInt( "OnlineAfterIdleMinutes", -1 );
        c.popSection();
        if( mHost.allowMapping()
         && !mInOwnLogonSession
         && mUseGui
         && mHostStatus.slaveStatus() == "offline" 
         && Employee(User::currentUser()).isRecord() 
         && userConfig().readBool( "WarnBeforeMapping", true ) ) {
            MapWarningDialog * mwd = new MapWarningDialog(qobject_cast<QWidget*>(parent()));
            int result = mwd->exec();
            delete mwd;
            if( result == QDialog::Rejected ) {
                this->offline();
                return;
            }
        }
    }

    // Do this before the exclusive row lock, because we want to hold that lock
    // for as little time as possible.
    pulse();

    // Since we are going to return the frames no matter what, there is no reason to do
    // this inside the transaction.
    mHostStatus.returnSlaveFrames();

    if( mHostStatus.slaveStatus() != "maintenance" )
        setStatus( offline ? "offline" : "ready", /*force=*/offline );
    else
        emit statusChange( "maintenance" );

    if( offline && !mInOwnLogonSession && mHost.allowMapping() )
        restoreDefaultMappings();

    // No need for quick loop time if we are going offline
    if( !offline ) {
        // Anytime we are reset, use quick loop time to check if we get new
        // status soon.  If we don't, then fall back to regular loop time.
        mQuickLoopCount = 2;
        mTimer->start( mQuickLoopTime );
    }

    LOG_5( "Slave::reset: done. returning" );
}

QString Slave::currentPriorityName() const
{
    QString ret;
    if( mBackgroundModeEnabled ) {
        IniConfig & c = config();
        c.pushSection( "BackgroundMode" );
        QString key(mBackgroundMode ? "BackgroundPriority" : "ForegroundPriority");
        QString def(mBackgroundMode ? "idle" : "normal");
        ret = c.readString( key, def );
        c.popSection();
    }
    return ret;
}

void Slave::execPriorityMode()
{
    QString priorityName = currentPriorityName();
    if( !priorityName.isEmpty() ) {
        LOG_3( "Slave::execPriorityMode: Setting priority to: " + priorityName );
        foreach( JobBurner * burner, mActiveBurners )
            burner->updatePriority( priorityName );
    }
}

void Slave::updatePriorityMode( int secondsIdle )
{
    LOG_6( "Slave::updatePriorityMode: seconds idle: " + QString::number(secondsIdle) );
    IniConfig & c = config();
    c.pushSection( "BackgroundMode" );
    if( c.readBool( "Enabled", true ) ) {
        int cutoff = c.readInt( "CutoffTime", 60 );
        if( (secondsIdle < cutoff) != mBackgroundMode ) {
            mBackgroundMode = (secondsIdle < cutoff);
            LOG_3( "Slave::updatePriorityMode: Setting background mode: " + QString(mBackgroundMode ? "true" : "false") );
            execPriorityMode();
        }
    }
    c.popSection();
}

void Slave::slotSecondsIdle( int idle )
{
    LOG_6("Slave::slotSecondsIdle called");
    updatePriorityMode(idle);
    if( mIdleDelay > 0 && idle > 60 * mIdleDelay && mHost.hostStatus().slaveStatus() == "offline" )
        if( checkFusionRunning() )
            mIdleDelay = 1;
        else
            reset();
    LOG_6("Slave::slotSecondsIdle done");
}

void Slave::online()
{
    LOG_5("Slave::online() called");
    reset( false );
    LOG_5("Slave::online() done");
}

void Slave::offlineFromAboutToQuit() 
{
    LOG_5("Slave::offlineFromAboutToQuit() signal/slot called. calling real offline method");
    offline();
    setStatus( "no-pulse", true );
}

void Slave::offline()
{
    LOG_5("Slave::offline() called");
    reset( true );
    LOG_5("Slave::offline() done");
}

void Slave::toggle()
{
    LOG_5("Slave::toggle() called");
    reset( status() != "offline" );
    LOG_5("Slave::toggle() done");
}

bool Slave::setStatus( const QString & nextStatus, bool force )
{
    LOG_3( "Slave: Setting Status: " + nextStatus );
    QString status = mHostStatus.slaveStatus();
    Database::current()->beginTransaction();
    // Locks the row, and updates mHost
    mHostStatus.reload( true /* Lock For Update */ );
    if( !mHostStatus.isRecord() ) {
        LOG_1( "Slave::reset: HostStatus record is now missing!!!!!!" );
        Database::current()->rollbackTransaction();
        qApp->exit(1);
        return false;
    }
    QString newStatus = mHostStatus.slaveStatus();
    if( newStatus != status && newStatus != nextStatus && !force ) {
        // Send offline as last status if that's what we were trying to do
        // That way if client-update is the newStatus, it'll go offline after the
        // client update is done
        Database::current()->commitTransaction();
        if( !mBurnOnlyJobAssignmentKey )
            handleStatusChange( newStatus, nextStatus == "offline" ? nextStatus : status );
        return false;
    }

    mHostStatus.setSlaveStatus( nextStatus );
    mHostStatus.commit();
    Database::current()->commitTransaction();
    emit statusChange( nextStatus );
    LOG_5( "Slave::setStatus returning after setting status " + nextStatus );
    return true;
}

void Slave::setUseGui( bool gui )
{
    mUseGui = gui;
}

void Slave::slotConnectionLost()
{
    LOG_5( "Slave::slotConnectionLost() called");
    if (mTimer) {
        mTimer->stop();
    }
    if( mLostConnectionTimer ) return;
    mLostConnectionTimer = new QTimer( this );
    connect( mLostConnectionTimer, SIGNAL( timeout() ), SLOT( restartFromLostConnectionTimer() ) );
    mLostConnectionTimer->start( 1000 * 120 ); // 120 seconds = 2 minutes
    LOG_5( "Slave::slotConnectionList() mLostConnectinTimer created, started. returning from slotConnectionLost()");
}

void Slave::slotConnected()
{
    LOG_5( "Slave::slotConnected() called");
    if (mTimer) {
        mTimer->start(mLoopTime);
    }
    if( mLostConnectionTimer ) {
        LOG_5( "Slave::slotConnected() mLostConnectionTimer->stopped, deleting");
        mLostConnectionTimer->stop();
        delete mLostConnectionTimer;
        mLostConnectionTimer = 0;
    }
    LOG_5( "Slave::slotConnected() done");
}

void Slave::restoreDefaultMappings()
{
    if( mIsMapped && !mInOwnLogonSession ) {
        if( mIsMapped 
                && Employee(User::currentUser()).isRecord() 
                && userConfig().readBool( "WarnBeforeReMapping", true ) 
                && mUseGui ) {
            ReMapWarningDialog * mwd = new ReMapWarningDialog(qobject_cast<QWidget*>(parent()));
            mwd->exec();
            delete mwd;
        }
        User u = User::currentUser();
        GroupList gl = u.userGroups().groups();
        MappingList mappings;
        foreach( Group g, gl )
            mappings += g.groupMappings().mappings();
    
        QString err;
        // Restore the group mappings, then the user mappings
        // The user mappings are done second so they can override
        // the group mappings
        for( int i=0; i<2; i++ ) {
            foreach( Mapping mapping, mappings ) {
                if( !mapping.map(true, &err) )
                    LOG_1( err );
            }
            mappings = u.userMappings().mappings();
        }
        mIsMapped = false;
    }
}

void Slave::showRemapWarning( const QString & drive )
{
    if( mUseGui && !mDriveRemapWarningDialog ) {
        mDriveRemapWarningDialog = new QMessageBox( "Unable to re-map drive " + drive,
            "Assburner is unable to re-map drive " + drive + " because it is being used by another program.\n"
            "Please close any programs using this drive.",
            QMessageBox::Warning, QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
        mDriveRemapWarningDialog->show();
        mDriveRemapWarningDialog->setAttribute( Qt::WA_DeleteOnClose, true );
    }
}

void host_error( const QString & text, const QString & method, const Service & service )
{
    SysLog sl;
    sl.setMessage( text );
    sl.set_class( "Assburner" );
    sl.setCount( 1 );
    sl.setCreated( QDateTime::currentDateTime() );
    sl.setHost( Host::currentHost() );
    sl.setValue( "fkeySysLogRealm", 6 ); // Render
    sl.setValue( "fkeySysLogSeverity", 4 ); // Major
    sl.setLastOccurrence( QDateTime::currentDateTime() );
    sl.setMethod( method );
    sl.commit();
    HostService hs = HostService::recordByHostAndService( Host::currentHost(), service );
    hs.setSysLog( sl );
    hs.commit();
    return;
}

ServiceList Slave::enabledServiceList()
{
    return Service::select(
        "WHERE enabled=true AND keyservice IN (SELECT fkeyservice FROM hostservice WHERE fkeyhost=? and enabled=true)",
        VarList() << Host::currentHost().key() );
}

void Slave::loadForbiddenProcesses()
{
    mForbiddenProcesses.clear();
    ServiceList enabledServices = enabledServiceList();
    foreach( QString forbiddenProcesses, enabledServices.forbiddenProcesses() )
        foreach( QString procName, forbiddenProcesses.split(",") )
            mForbiddenProcesses << procName;
}
/*
void Slave::checkDriveMappingAvailability()
{
    ServiceList enabledServices = enabledServiceList();
    foreach( 
}*/

void Slave::setInOwnLogonSession( bool iols )
{
    mInOwnLogonSession = iols;
}

void Slave::setIsMapped( bool isMapped )
{
    mIsMapped |= isMapped;
}

QString Slave::logRootDir() const
{
    return Config::getString("assburnerLogRootDir");
}

int Slave::memCheckPeriod() const
{
    return mMemCheckPeriod;
}

AccountingInfo Slave::parseTaskLoggerOutput( const QString & line )
{
    AccountingInfo info;
#ifdef USE_ACCOUNTING_INTERFACE
    // parse the line
    QStringList parts = line.split("\t");
    if( parts.size() < 13 )
        return info;

    info.pid = parts[0].toInt();
    info.ppid = parts[1].toInt();

    // times are reported in usecs, but we want to store them as msecs
    // which is what we record from baztime too
    info.realTime = int(parts[3].toInt()/1000);
    info.cpuTime = int((parts[4].toInt() + parts[5].toInt()) / 1000);

    info.memory = parts[6].toInt();
    info.bytesRead = parts[8].toInt();
    info.bytesWrite = parts[9].toInt();
    info.opsRead = parts[10].toInt();
    info.opsWrite = parts[11].toInt();
    info.ioWait = parts[13].toInt();
#endif

    return info;
}

void Slave::updateLoggedUsers()
{
    LOG_5("Checking if user is logged in");
    QStringList users = getLoggedInUsers();

    foreach(QString user, users)
        LOG_5(QString("User %1 is logged in").arg(user));

    for (QStringList::Iterator it = users.begin(); it != users.end(); ++it) {
        User user = User::recordByUserName((*it));
        if (user.isRecord()) {
            if (user == mHost.user()){
                mHost.setUserIsLoggedIn(true);
                mHost.commit();
                return;
            }
        }
    }

    mHost.setUserIsLoggedIn(false);
    mHost.commit();
}

