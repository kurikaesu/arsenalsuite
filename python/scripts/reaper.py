#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *
import blur.email, blur.jabber
import sys, time, re, os, gc
from math import ceil
import traceback
import unicodedata

try:
    import popen2
except: pass

if sys.argv.count('-daemonize'):
    from blur.daemonize import createDaemon
    createDaemon()

reaperConfig = '/etc/ab/reaper.ini'
try:
    pos = sys.argv.index('-config')
    reaperConfig = sys.argv[pos+1]
except: pass
dbConfig = '/etc/ab/db.ini'
try:
    pos = sys.argv.index('-dbconfig')
    dbConfig = sys.argv[pos+1]
except: pass

app = QCoreApplication(sys.argv)

initConfig( reaperConfig, "/var/log/ab/reaper.log" )
# Read values from db.ini, but dont overwrite values from reaper.ini
# This allows db.ini defaults to work even if reaper.ini is non-existent
config().readFromFile( dbConfig, False )

blur.RedirectOutputToLog()

initStone( sys.argv )

classes_loader()

VERBOSE_DEBUG = True

if False and VERBOSE_DEBUG:
    Database.current().setEchoMode( Database.EchoUpdate | Database.EchoDelete | Database.EchoSelect )

Database.current().connection().reconnect()

# Stores/Updates Config vars needed for reaper
class ReaperConfig:
    def update(self):
        self.managerDriveLetter = str(Config.getString('managerDriveLetter')).lower()
        self.spoolDir = Config.getString('managerSpoolDir')
        self.requiredSpoolSpacePercent = Config.getFloat('assburnerRequiredSpoolSpacePercent',10.0)
        self.totalFailureThreshold = Config.getInt('assburnerTotalFailureErrorThreshold',30)
        self.permissableMapDrives = Config.getString('assburnerPermissableMapDrives','G:,V:')
        self.assburnerForkJobVerify = Config.getBool('assburnerForkJobVerify',True)
        self.jabberDomain = Config.getString('jabberDomain','jabber.com')
        self.jabberSystemUser = Config.getString('jabberSystemUser','farm')
        self.jabberSystemPassword = Config.getString('jabberSystemPassword','farm')
        self.jabberSystemResource = Config.getString('jabberSystemResource','Reaper')

def execReturnStdout(cmd):
    p = popen2.Popen3(cmd)
    while True:
        try:
            p.wait()
            break
        except OSError:
            print 'EINTR during popen2.wait, ignoring.'
            pass
    return p.fromchild.read()

def getLocalFilePath(job):
    if config.spoolDir != "":
        baseName = QFileInfo(job.fileName()).fileName()
        userName = job.user().name()
        userDir = config.spoolDir + "/" + userName + "/"
        filePath = userDir + baseName
        return str(filePath)
    else: return str(job.fileName())

def cleanupJob(job):
    print "cleanupJob called on %s jobID: %i" % (job.name(), job.key())
    if not job.user().isRecord():
        print "Job::cleanup cannot clean up a job that isn't assigned a user!\n"
        return

    if job.uploadedFile():
        filePath = getLocalFilePath(job)
        filePathExtra = re.sub('(\.zip|\.max)','.txt',filePath)

        if config.spoolDir != "":
            QFile.remove(filePath)
            QFile.remove(filePathExtra)

def diskFree(path):
    if len(path) < 1:
        path = '/'
    dfcmd = '/bin/df'
    inp = []
    for line in execReturnStdout('%s %s 2>&1' % (dfcmd, path)).splitlines():
        line = line.strip()
        if line.find('Filesystem') >= 0:
            continue
        inp.append(line)
    parts = re.split('\s+',' '.join(inp))
    device, blocks, usedblocks, freeblocks, percent_used, mountpoint = parts
    percent_used = int(re.sub('\%','',percent_used))
    return (blocks,usedblocks,freeblocks,percent_used)

# Usage: checkSpoolSpace( SpoolDir, RequiredPercent )
# Returns: 1 for sufficient space, else 0
def checkSpoolSpace(requiredPercent):
    (blocks,usedblocks,freeblocks,percent_used) = diskFree(config.spoolDir)
    if (100 - percent_used) >= requiredPercent: return True
    print "checkSpoolSpace found required percent free disk space on %s insufficient (%i %% used)!" % (config.spoolDir, percent_used)
    return False

def cleanupJobs():
    if VERBOSE_DEBUG: print "cleanupJobs called, retrieving cleanable jobs"
    jobsCleaned = 0
    for job in retrieveCleanable():
        print "Found cleanable Job: %s %i" % (job.name(), job.key() )
        cleanupJob(job)
        job.setCleaned(True)
        job.commit()
        jobsCleaned+=1
    if VERBOSE_DEBUG: print "Job::cleanupJobs done, returning,", jobsCleaned
    return jobsCleaned

def ensureSpoolSpace(requiredPercent):
    triedCleanup = False
    while not checkSpoolSpace( requiredPercent ):
        print "ensureSpoolSpace -- checkSpool indicates not enough free space, attemping to free space!"
        if not triedCleanup:
            triedCleanup = True
            if cleanupJobs() > 0:
                continue
        jobs = Job.select("fkeyjobtype=9 and status='done' ORDER BY endedts asc limit 1")
        #only auto-deleting max8 jobs for now, some batch jobs will be really old and ppl will want them saved 
        #no matter what disk space they use
        if not jobs.isEmpty():
            job = jobs[0]
            print "Setting Job %s to deleted and removing files to free disk space" % (job.name())
            job.setStatus('deleted')
            cleanupJob(job)
            job.setCleaned(True)
            job.commit()
            notifylist = "it:e"
            if job.user().isRecord():
                notifylist += "," + job.user().name() + ":je" 
            notifySend( notifyList = notifylist, subject = 'Job auto-deleted: ' + job.name(),
                        body = "Your old max8 job was automatically deleted to free up space on the Assburner job spool. Job: " + job.name() )
        else:
            print "Failed to ensure proper spool space, no more done jobs to delete and cleanup."
            return False
    return True

def getSlotCount(job):
    q = Database.current().exec_("""
     SELECT sum(assignslots)
     FROM JobAssignment ja
     WHERE fkeyjob = %i
     AND fkeyjobassignmentstatus < 4""" % job.key())

    if q.next():
        return q.value(0).toInt()[0]
    return 0

def getErrorCount(job):
    q = Database.current().exec_("SELECT sum(count) FROM JobError WHERE cleared=false AND fkeyJob=%i" % (job.key()) )
    if q.next():
        return q.value(0).toInt()[0]
    return 0

def getTimeoutCount(job):
    q = Database.current().exec_("SELECT count(*) FROM JobError WHERE timeout=true AND cleared=false AND fkeyjob=%i GROUP BY fkeyhost" % job.key() )
    if q.next():
        return q.value(0).toInt()[0]
    return 0

def retrieveReapable():
    jobList = Job.select("status IN ('ready','started')")
    if not jobList.isEmpty():
        JobStatus.select("fkeyjob IN ("+jobList.keyString()+")")
    return jobList

def retrieveCleanable():
    return Job.select("status IN ('deleted') AND (cleaned IS NULL OR cleaned=0)")

def reaper():
    #   Config: managerDriveLetter, managerSpoolDir, assburnerErrorStep
    print "Reaper is starting up"

    errorStep = Config.getInt('assburnerErrorStep')

    # Make sure that people get notified at most once per error
    if not errorStep or errorStep < 1:
        errorStep = 1

    # Terminal coloring filter
    colorFilter = re.compile("(\x1b|\x1B)\[[0-9];?[0-9]{0,}m")

    # Log reaper in once to the jabber server
    sender = config.jabberSystemUser +'@'+ config.jabberDomain +'/'+config.jabberSystemResource
    password = str(config.jabberSystemPassword)
    jabberBot = blur.jabber.farmBot(sender, password)

    while True:
        # Process any incoming messages (discard them)
        jabberBot.process()

        t1 = time.time()
        service.pulse()
        config.update()
        if config.spoolDir != "":
            ensureSpoolSpace(config.requiredSpoolSpacePercent)

        for job in retrieveReapable():
            #if VERBOSE_DEBUG: Log( "Checking Job %i %s" % (job.key(), job.name()) )
            status = str(job.status())

            Database.current().exec_('SELECT update_job_soft_deps(%i)' % job.key())
            Database.current().exec_("SELECT update_job_task_counts_2(%i)" % job.key())
            Database.current().exec_('SELECT update_job_stats(%i)' % job.key())

            js = JobStatus.recordByJob(job)
            unassigned  = js.tasksUnassigned()
            done        = js.tasksDone()
            tasks       = js.tasksCount()
            cancelled   = js.tasksCancelled()
            suspended   = js.tasksSuspended()
            assigned    = js.tasksAssigned()
            busy        = js.tasksBusy()

            hostCount = getSlotCount(job)
            #print "host has %s slots active" % hostCount

            js.setHostsOnJob(hostCount)

            errorCount = getErrorCount(job)
            js.setErrorCount(errorCount)

            suspend = False
            suspendTitle = ''
            suspendMsg = ''
            if errorCount > 3:
                timeoutCount = getTimeoutCount(job)
                if timeoutCount > 3:
                    suspend = True
                    suspendMsg = 'Job %s (%i) has been suspended.  The job has timed out on %i hosts.' % (job.name(), job.key(), timeoutCount)
                    suspendTitle = 'Job %s (%i) has been suspended(Timeout limit reached).' % (job.name(), job.key())

            hasErrors = False
            # do we have a job that's got everything done except suspended frames?
            if ((done + cancelled + suspended) == tasks) and (suspended > 0):
                js.reload()
                cancelled = js.tasksCancelled()
                suspended = js.tasksSuspended()
                if ((done + cancelled + suspended) == tasks):
                    suspend = True
                    jobErrors = job.jobErrors()
                    for error in jobErrors:
                        if not error.cleared():
                            hasErrors = True
                            break

            # If job is erroring check job and global error thresholds. Immediately refresh the job data to check for an increase in max errors since the records are cached.
            if ((errorCount > job.maxErrors()) and (job.reload()) and (errorCount > job.maxErrors())) or (done == 0 and errorCount > config.totalFailureThreshold) or (hasErrors):
                suspend = True
                suspendMsg = 'Job %s (%i) has been suspended.  The job has produced %i errors.' % (job.name(), job.key(), errorCount)
                jobErrors = job.jobErrors()
                messages = []
                for i in range (0, min(5, len(jobErrors))):
                    if not jobErrors[i].cleared():
                        messages.append(colorFilter.sub("",str(jobErrors[i].message())).strip().encode('ascii','ignore'))

                suspendMsg += "\nThe last %d errors produced were:\n" % (min(5, len(jobErrors)))
                suspendMsg += "\n".join(messages)

                suspendTitle = 'Job %s (%i) suspended.' % (job.name(), job.key())

            if suspend:
                # Return tasks and hosts
                Job.updateJobStatuses( JobList(job), 'suspended', False )
                notifyList = str(job.notifyOnError())
                # If they don't have notifications on for job errors, then
                # notify them via email and jabber
                if len(notifyList) == 0:
                    notifyList = job.user().name() + ':je'
                notifySend(jabberBot, notifyList, suspendMsg, suspendTitle )
                continue

            # send error notifications
            lastErrorCount = js.lastNotifiedErrorCount()

            # If some or all of the errors were cleared
            if errorCount < lastErrorCount:
                js.setLastNotifiedErrorCount( errorCount )

            # Notify about errors if needed
            if job.notifyOnError() and errorCount - lastErrorCount > errorStep:
                notifyOnErrorSend(jabberBot, job,errorCount,lastErrorCount)
                js.setLastNotifiedErrorCount( errorCount )

            # now we set the status started unless it's new
            if busy + assigned > 0:
                if job.status() == 'ready':
                    job.setStatus('started')
                if job.startedts().isNull():
                    job.setColumnLiteral( "startedts", "now()" )

            # done if no more tasks
            if done + cancelled >= tasks:
                job.setColumnLiteral("endedts","now()")
                if job.notifyOnComplete():
                    notifyOnCompleteSend(jabberBot, job)
                if job.deleteOnComplete():
                    job.setStatus( 'deleted' )
                else:
                    job.setStatus( 'done' )

                job.commit()
                # go through every job that is waiting on this one to finish
                # then if that job has other dependencies, make sure they are complete
                # too. If so then it is good to go.
                Database.current().exec_('SELECT update_job_hard_deps(%i)' % job.key())

            '''
            if False and (float(done) / float(tasks) >= 0.10) and job.autoAdaptSlots() == 0 and job.assignmentSlots() == 8:
                if js.averageMemory() < (job.maxMemory() / 6):
                    print "job %s has poor memory utilisation %s" % ( job.name(), js.averageMemory() )
                    job.setAssignmentSlots( int(job.assignmentSlots() / 2) )
                    job.setAutoAdaptSlots( job.autoAdaptSlots() + 1 )
                    job.setMinMemory( int(job.minMemory()/2) )
                    job.setMaxMemory( int(job.maxMemory()/2) )
                    job.commit()
                    job.addHistory( "Assignment slots auto-adapted in half due to low memory %s" % js.averageMemory() )

            if False and (float(done) / float(tasks) >= 0.15) and job.autoAdaptSlots() == 0 and job.assignmentSlots() == 8:
                # 20% of tasks complete and never auto-adjusted
                if (js.efficiency() <= 0.6) and (js.averageMemory() < (job.maxMemory() / 4)):
                    print "job %s has bad efficiency %s" % ( job.name(), js.efficiency() )
                    job.setAssignmentSlots( int(job.assignmentSlots() / 2) )
                    job.setAutoAdaptSlots( job.autoAdaptSlots() + 1 )
                    job.setMaxMemory( int(job.maxMemory()/2) )
                    job.commit()
                    job.addHistory( "Assignment slots auto-adapted in half due to poor efficiency %s" % js.efficiency() )

            if False and (float(done) / float(tasks) >= 0.40) and job.autoAdaptSlots() == 0 and job.assignmentSlots() == 8:
                if (js.efficiency() <= 0.75) and (js.averageMemory() < (job.maxMemory() / 4)):
                    print "job %s has not great efficiency %s" % ( job.name(), js.efficiency() )
                    job.setAssignmentSlots( int(job.assignmentSlots() / 2) )
                    job.setAutoAdaptSlots( job.autoAdaptSlots() + 1 )
                    job.setMaxMemory( int(job.maxMemory()/2) )
                    job.commit()
                    job.addHistory( "Assignment slots auto-adapted in half due to poor efficiency %s" % js.efficiency() )
            '''

            job.commit()
            js.commit()

        t2 = time.time()
        if VERBOSE_DEBUG: 
            Log( "*** Reaping complete in %0.3f ms" % ((t2-t1) * 1000.0))
            Log( "*** Objects in memory %d" % (len(gc.get_objects())) )
            print # formatting
        time.sleep(1)

def notifyOnCompleteSend(jabberBot, job):
    if VERBOSE_DEBUG:
        print 'notifyOnCompleteSend(): Job %s is complete.' % (job.name())
    msg = 'Job %s (%i) is complete.' % (job.name(), job.key())
    if not job.notifyCompleteMessage().isEmpty():
        msg = job.notifyCompleteMessage()
    notifySend( jabberBot, job.notifyOnComplete(), msg, msg )

def notifyOnErrorSend(jabberBot, job,errorCount,lastErrorCount):
    if VERBOSE_DEBUG:
        print 'notifyOnErrorSend(): Job %s has errors.' % (job.name())
    msg = 'Job %s (%i) for user %s has %i errors.' % (job.name(), job.key(), job.user().name(), errorCount)
    if not job.notifyErrorMessage().isEmpty():
        msg = job.notifyErrorMessage()

    jobErrors = job.jobErrors()
    messages = []
    colorFilter = re.compile("(\x1b|\x1B)\[[0-9];?[0-9]{0,}m")
    for i in range ( 0, min( 5, min( errorCount - lastErrorCount, len(jobErrors) ) ) ):
        messages.append(colorFilter.sub("", str(jobErrors[i].message())).strip().encode('ascii','ignore'))

    msg += "\nThe last %d errors produced were:\n" % (min(5, errorCount - lastErrorCount))
    msg += "\n".join(messages)

    notifySend(jabberBot,  job.notifyOnError(), msg, msg, True )

def notifySend(jabberBot, notifyList, body, subject, noEmail = False ):
    messages = body.split('\n')
    messages[len(messages)-1] += "\n"
    if VERBOSE_DEBUG:
        print 'NOTIFY: %s' % body
    sender = config.jabberSystemUser +'@'+ config.jabberDomain +'/'+config.jabberSystemResource
    for notify in str(notifyList).split(','):
        sendType = 'chat'
        if not len(notify.split(':')) == 2: # Incorrectly formatted notify entries are skipped
            if VERBOSE_DEBUG:
                print 'bad formatting in notifyList %s' % notify
            continue

        recipient, method = notify.split(':')
        if 'e' in method and not noEmail:
            blur.email.send(sender = 'no-reply@drdstudios.com', recipients = [recipient], subject = subject, body = body )
        if 'j' in method:
            if not recipient.find('@') > -1:
                recipient += '@'+config.jabberDomain
            else:
                sendType = 'groupchat'
                jabberBot.joinRoom(recipient)
            if VERBOSE_DEBUG:
                print 'JABBER: %s %s %s %s' % (sender, config.jabberSystemPassword, recipient, sendType)
            for message in messages:
                jabberBot.send(str(recipient), ur'%s' % str(message), sendType)

    print # formatting

config = ReaperConfig()
config.update()

service = Service.ensureServiceExists('AB_Reaper')

# Holds PID of checker:Job
newJobCheckers = {}

reaper()
