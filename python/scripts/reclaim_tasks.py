#!/usr/bin/python

from PyQt4.QtCore import *
from blur.Stone import *
from blur.Classes import *
import sys, os, re, traceback, time

if sys.argv.count('-daemonize'):
	from blur.daemonize import createDaemon
	createDaemon()

recConfig = '/etc/ab/reclaim_tasks.ini'
try:
	pos = sys.argv.index('-config')
	recConfig = sys.argv[pos+1]
except: pass
dbConfig = '/etc/ab/db.ini'
try:
	pos = sys.argv.index('-dbconfig')
	dbConfig = sys.argv[pos+1]
except: pass

# First Create a Qt Application
app = QCoreApplication(sys.argv)

# Load database config
if sys.platform=='win32':
	initConfig("c:\\blur\\resin\\resin.ini")
else:
	initConfig(recConfig, "/var/log/ab/reclaim_tasks.log")
	# Read values from db.ini, but dont overwrite values from reclaim_tasks.ini
	# This allows db.ini defaults to work even if reclaim_tasks.ini is non-existent
	config().readFromFile( dbConfig, False )

initStone(sys.argv)
classes_loader()

#Database.current().setEchoMode( Database.EchoUpdate | Database.EchoInsert | Database.EchoDelete )

class RTConfig:
	def __init__(self):
		pass
	def update(self):
		self.pulse_period = Config.getInt('assburnerPulsePeriod', 600) # default 10 minutes
		self.loop_time = int(Config.getInt('assburnerLoopTime', 5000) / 1000.0 + .999)
		
		self.pulse_period += self.loop_time + 20 # It can take up to assburnerPulsePeriod + assburnerLoopTime to update the pulse
		
# Returns true if the host responds to a ping
def ping(hostName):
	pingcmd = "ping -c 3 %s 2>&1 > /dev/null" % hostName
	return os.system(pingcmd) == 0

# check for dead pulse/hung status
# 	any host that hasn't given pulse or changed status in 10 minutes, 
# 	reset host and reclaim job frames and send note to it@

total_hosts = 0

# Automatically setup the AB_manager service record
# the hostservice record for our host, and enable it
# if no other hosts are enabled for this service
service = Service.ensureServiceExists('AB_reclaim_tasks')
hostService = service.byHost(Host.currentHost(),True)
hostService.enableUnique()

config = RTConfig()
lastPingSweepTime = QDateTime.currentDateTime().addDays(-1)

def dbTime():
	q = Database.current().exec_("SELECT now();")
	if q.next():
		return q.value(0).toDateTime()
	Log( "Database Time select failed" )
	return QDateTime.currentDateTime()

while True:
    hostService.pulse()
    service.reload()
    config.update()

    Log( "=== retrieving wayward assignments ===" )
    waywardAssignments = JobAssignment.select( "WHERE fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus from jobassignmentstatus WHERE status in ('ready','copy','busy')) AND (fkeyjob NOT IN (SELECT keyjob from job where status IN ('ready','started')) OR fkeyhost NOT IN (SELECT fkeyhost from hoststatus WHERE slavestatus IN ('ready')))" )

    for ass in waywardAssignments:
        job = ass.job()
        host = ass.host()
        print ("Assignment is wayward, %i:%s for job %i:%s assigned to host %i:%s" % ( ass.key(), ass.jobAssignmentStatus().status(), job.key(), job.status(), host.key(), host.hostStatus().slaveStatus() ) )
        Database.current().exec_( "SELECT cancel_job_assignment(%i)" % ass.key() )

    Log( "=== retrieving wayward task assignments ===" )
    waywardTaskAssignments = JobTaskAssignment.select( """
    JOIN jobassignment ON fkeyjobassignment=keyjobassignment AND jobassignment.fkeyjobassignmentstatus > 3 
    WHERE jobtaskassignment.fkeyjobassignmentstatus < 4""")

    for taskAss in waywardTaskAssignments:
        ass = taskAss.jobAssignment()
        job = ass.job()
        host = ass.host()
        print ("Task Assignment is wayward, %i:%s for job %i:%s assigned to host %i:%s" % ( taskAss.key(), taskAss.jobAssignmentStatus().status(), job.key(), job.status(), host.key(), host.hostStatus().slaveStatus() ) )
        taskAss.setJobAssignmentStatus( JobAssignmentStatus.recordByName( 'cancelled' ) )

    Log( "=== retrieving wayward tasks ===" )
    waywardTasks = JobTask.select( """
    JOIN jobtaskassignment ON keyjobtaskassignment = fkeyjobtaskassignment 
    WHERE status IN ('assigned','busy') 
    AND fkeyjobtaskassignment NOT IN (SELECT keyjobtaskassignment 
                                      FROM jobtaskassignment 
                                      WHERE fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus 
                                      FROM jobassignmentstatus 
                                      WHERE status in ('ready','copy','busy')))""" )

    waywardTasksByJob = waywardTasks.groupedBy("fkeyjob")
    for jobKey, tasks in waywardTasksByJob.iteritems():
        job = Job(int(jobKey))
        tasks = JobTaskList(tasks)
        msg = "Returning %i tasks from job %s" % (tasks.size(),job.name())
        print msg
        Notification.create('reclaim_tasks','hung frames', ('Job %s has %i hung frames' % (job.name(), tasks.size())), msg)
        tasks.setStatuses('new')
        if job.packetType() != 'preassigned':
            tasks.setHosts( Host() )
        tasks.commit()

    Log( "=== retrieving wayward hosts ===" )
    selectStartTime = dbTime()
    q = Database.current().exec_("SELECT * FROM get_wayward_hosts_2('%ss'::interval,'%ss'::interval)" % (config.pulse_period, config.loop_time))
    while q.next():
        fkeyhost = q.value(0).toInt()[0]
        reason = q.value(1).toInt()[0] # 1 - Stale status, 2 - Stale pulse

        host = Host(fkeyhost)
        status = host.hostStatus().reload()

        # Double check to see if the problem host is already up-to-date
        if reason == 1:
            if status.lastStatusChange() > selectStartTime:
                continue
        else:
            if status.slavePulse() > selectStartTime:
                continue

        print "host %s looks fucked, returning tasks - slave last pulsed at %s" % (host.name(), status.slavePulse().toString())

        lastPulseInterval = Interval(status.slavePulse(),selectStartTime)
        msg = QStringList()
        msg << ("Host: " + host.name())
        msg << ("Assburner Version:" + host.abVersion())
        msg << ("keyHost: " + str(host.key()))
        msg << ("Status: " + status.slaveStatus())
        msg << ("Time since status change: " + Interval(status.lastStatusChange(),selectStartTime).toString())
        msg << ("Time since pulse: " + lastPulseInterval.toString())
        alive = ping(host.name())
        pingText = "Yes"
        if not alive: pingText = "No"
        msg << ("Ping Response: " + pingText)

        assignments = host.activeAssignments()
        for ja in assignments:
            job = ja.job()
            msg << ("Job: " + str(job.key()) + ", " + job.name())
            msg << ("Job Type: " + job.jobType().name())
        status.returnSlaveFrames(True)

        nextStatus = 'no-ping'
        if alive:
            nextStatus = 'no-pulse'

        msg << ""
        msg << ("Setting status: " + nextStatus)

        n = Notification.create( 'reclaim_tasks', 'wayward host', '%s is wayward' % host.name(), msg.join("\n") )

        status.setSlaveStatus(nextStatus).commit()

        total_hosts += 1

    Log("total_hosts: %i" % total_hosts)

    cdt = QDateTime.currentDateTime()
    if lastPingSweepTime.daysTo( cdt ) > 0:
        lastPingSweepTime = cdt

        for hs in HostStatus.select( "slavestatus in ('no-pulse','no-ping') or slavestatus IS NULL" ):
            hostName = hs.host().name()
            alive = ping(hostName)
            nextStatus = 'no-ping'
            if alive:
                nextStatus = 'no-pulse'
            # Ensure it's status hasn't already changed
            if nextStatus != hs.slaveStatus() and str(hs.reload().slaveStatus()) in ('no-pulse','no-ping',''):
                print "Updating %s slave status from %s to %s" % (hostName, hs.slaveStatus(), nextStatus)
                hs.setSlaveStatus(nextStatus).commit()

    # Run every 15 min
    time.sleep(60 * 10)

