#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *
import blur.email, blur.jabber
import sys, time, re, os, gc
from math import ceil
import traceback
from verifier_plugin_factory import *
import verifier_plugins

try:
	import popen2
except: pass

if sys.argv.count('-daemonize'):
	from blur.daemonize import createDaemon
	createDaemon()

reaperConfig = '/etc/ab/verifier.ini'
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

initConfig( reaperConfig, "/var/log/ab/verifier.log" )
# Read values from db.ini, but dont overwrite values from verifier.ini
# This allows db.ini defaults to work even if verifier.ini is non-existent
config().readFromFile( dbConfig, False )

blur.RedirectOutputToLog()

initStone( sys.argv )

classes_loader()

VERBOSE_DEBUG = False

if VERBOSE_DEBUG:
	Database.current().setEchoMode( Database.EchoUpdate | Database.EchoDelete | Database.EchoSelect )

Database.current().connection().reconnect()

# Stores/Updates Config vars needed for reaper
class VerifierConfig:
	def update(self):
		self.managerDriveLetter = str(Config.getString('managerDriveLetter')).lower()
		self.spoolDir = Config.getString('managerSpoolDir')
		self.requiredSpoolSpacePercent = Config.getFloat('assburnerRequiredSpoolSpacePercent',10.0)
		self.assburnerForkJobVerify = Config.getBool('assburnerForkJobVerify',True)

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

def checkNewJob(job):
    # make sure the job file exists
    print 'Checking new job %i, %s' % ( job.key(), job.name() )

    if not job.user().isRecord():
        print "Job missing fkeyUsr"
        return False

    if job.table().schema().field( 'frameStart' ) and job.table().schema().field( 'frameEnd' ):
        minMaxFrames = Database.current().exec_('SELECT min(jobtask), max(jobtask) from JobTask WHERE fkeyjob=%i' % job.key())
        if minMaxFrames.next():
            job.setValue( 'frameStart', minMaxFrames.value(0) )
            job.setValue( 'frameEnd', minMaxFrames.value(1) )

    print "checking job dependencies"
    status = 'ready'
    for jobDep in JobDep.recordsByJob( job ):
        if jobDep.depType() == 1 and jobDep.dep().status() != 'done' and jobDep.dep().status() != 'deleted':
            status = 'holding'
    if job.status() == 'verify-suspended':
        status = 'suspended'
    job.setStatus(status)

    print "New Job %s appears ready, running through plug-ins" % ( job.name() )
    for key in sorted(VerifierPluginFactory().sVerifierPlugins.keys()):
        function = VerifierPluginFactory().sVerifierPlugins[key]
        result = False
        try: result = function(job)
        except:
            traceback.print_exc()
            result = True
        if not result: return False

    createJobStat(job)
    job.commit()
    return True

def retrieveVerifiable():
	jobList = Job.select("status IN ('verify','verify-suspended')")
	if not jobList.isEmpty():
		JobStatus.select("fkeyjob IN ("+jobList.keyString()+")")
	return jobList

def verifier():
    #	Config: managerDriveLetter, managerSpoolDir, assburnerErrorStep
    print "Verifier is starting up"

    while True:
        # plug-ins can be hot-loaded
        try:
            reload(verifier_plugins)
        except: traceback.print_exc()

        # Remove checker from dict if the process has exited, so
        # we can recheck the job if it failed
        try:
            for pid in newJobCheckers.keys():
                result = os.waitpid(pid,os.WNOHANG)
                if result and result[0] == pid:
                    del newJobCheckers[pid]
                    print 'Job Checker Child Process: %i Exited with code: %i' % result
        except:
            print "Exception During newJobCheckers waitpid checking."
            traceback.print_exc()

        service.pulse()
        config.update()
        if config.spoolDir != "":
            ensureSpoolSpace(config.requiredSpoolSpacePercent)

        for job in retrieveVerifiable():
            if VERBOSE_DEBUG: print "Checking Job %i %s" % (job.key(), job.name())
            status = str(job.status())

            if status.startswith('verify'):
                if config.assburnerForkJobVerify:
                    if VERBOSE_DEBUG: print "Forking verify thread for %s" % (job.name())
                    if not job in newJobCheckers.values():
                        pid = os.fork()
                        Database.current().connection().reconnect()
                        if pid == 0:
                            checkNewJob(job)
                            os._exit(0)
                        else:
                            newJobCheckers[pid] = job
                else:
                    checkNewJob(job)

        if VERBOSE_DEBUG: 
            print "Sleeping for 5 seconds"

        print "Objects in memory %d" % ( len( gc.get_objects() ) )
        time.sleep(5)

def createJobStat( mJob ):
	stat = JobStat()
	stat.setName( mJob.name() )
	stat.setElement( mJob.element() )
	stat.setProject( mJob.project() )
	stat.setUser( mJob.user() )
	stat.commit()
	mJob.setJobStat( stat )
	mJob.commit()

config = VerifierConfig()
config.update()

service = Service.ensureServiceExists('AB_Verifier')

# Holds PID of checker:Job
newJobCheckers = {}

verifier()

