#!/usr/bin/python
from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *
import blur
import sys
import time
from blur.defaultdict import *
from math import ceil

if sys.argv.count('-daemonize'):
	from blur.daemonize import createDaemon
	createDaemon()

# First Create a Qt Application
app = QCoreApplication(sys.argv)

# Load database config
if sys.platform != 'win32':
	initConfig("/etc/unassign_tasks.ini",'/var/log/ab/unassign_tasks.log')
	# Read values from db.ini, but dont overwrite values from unassign_tasks.ini
	# This allows db.ini defaults to work even if unassign_tasks.ini is non-existent
	config().readFromFile( "/etc/db.ini", False )
else:
	initConfig("c:\\blur\\resin\\resin.ini")

blur.RedirectOutputToLog()

classes_loader()

#Database.instance().setEchoMode( Database.EchoUpdate | Database.EchoInsert )

# Automatically setup the AB_manager service record
# the hostservice record for our host, and enable it
# if no other hosts are enabled for this service
service = Service.ensureServiceExists('AB_unassign_tasks')
hostService = service.byHost(Host.currentHost(),True)
hostService.enableUnique()

def numberListToString(number_list):
	return ','.join([str(i) for i in number_list])

DRY_RUN = False

# Busy and idle refer to the scripts function(busy when hosts on the farm are idle), not the farm
# Having a longer delay keeps load off the server when the farm is busy
BUSY_DELAY = 5
IDLE_DELAY = 40

delay = BUSY_DELAY

class HostInfo(object):
	def __init__(self,host,hostStatus,assigned):
		self.host = host
		self.hostStatus = hostStatus
		self.assignedFrames = assigned

def unassign_tasks_from_job(job,surplusHosts):
	# Get the hosts, and the number of still assigned frames
	q_hosts = Database.current().exec_("SELECT fkeyhost, count(*) as asscnt FROM JobTask WHERE status='assigned' AND fkeyjob=" + QString.number(job.key()) + " group by fkeyhost")
	
	# Put the assigned count into the host record, and get the total
	# assigned count, and also the maximum assigned per single host
	hostInfoList = []
	total_ass = 0
	max_frames = 0
	while q_hosts.next():
		h = Host(q_hosts.value(0).toInt()[0])
		hs = h.hostStatus()
		hs.reload()
		cnt = q_hosts.value(1).toInt()[0]
		if h.isRecord():
			print "Host %s has %i assigned frames" % (h.name(),cnt)
			total_ass += cnt
			if cnt > max_frames:
				max_frames = cnt
			hostInfoList.append( HostInfo( h, hs, cnt ) )
	
	# If we get to a point where none are reassigned, then we can break out of the loop
	reass = True
	
	used_hosts = len(hostInfoList)
	# Here how many more hosts this job can use total
	max_hosts_to_use = total_ass - used_hosts
	if max_hosts_to_use > surplusHosts:
		max_hosts_to_use = surplusHosts
	
	if max_hosts_to_use <= 0:
		return 0
	
	to_reassign = (total_ass / (used_hosts + max_hosts_to_use)) * max_hosts_to_use
	if to_reassign > total_ass - used_hosts:
		to_reassign = total_ass - used_hosts
	
	to_reassign_orig = to_reassign
	
	# If this job doesn't qualify, go to the next
	if to_reassign == 0:
		return 0
	
	print "Reassigning %i Frames out of %i assigned from %i hosts" % (to_reassign,total_ass,used_hosts)
	
	# Each time through the loop, we reassign only the hosts with max_assigned frames
	# that was we don't reassign from a host that only has 1 task left, when there another with 10
	while reass and to_reassign > 0 and max_frames > 1:
		reass = False
		resetFrames = JobTaskList()
		for hostInfo in hostInfoList:
			frameList = hostInfo.hostStatus.slaveFrames()
			#print "Parsing frameList %s" % (frameList)
			frames = expandNumberList(frameList)[0]
			#print "Max Frame == %i, assigned frames == %i" % (max_frames, h.errorTempo())
			if hostInfo.assignedFrames == max_frames:
				foundReassign = False
				while not foundReassign and len(frames):
					frame = frames.pop()
					#print "Setting frameList %s" % (frameList)
					jtl = JobTask.select( "fkeyjob=? and jobtask=?", [QVariant(job.key()),QVariant(frame)] )
					frameList = numberListToString(frames)
					hostInfo.hostStatus.setSlaveFrames(frameList)
					if jtl.size() == 1:
						jt = jtl[0]
			#			print "JobTask host %s, Host %s" % (h.name(), jt.host().name())
						if jt.host().key() == hostInfo.host.key() and jt.status() == 'assigned':
							print "Reassigning Frame %i" % (frame)
							jt.setStatus('new')
							jt.setHost( Host() )
							hostInfo.assignedFrames -= 1
							resetFrames += jt
							to_reassign -= 1
							reass = True
							foundReassign = True
				
			# If we've already 
			if to_reassign == 0:
				break
				
		# Commit the reassigned frames, lower max_frames
		if not DRY_RUN:
			resetFrames.commit()
		max_frames -= 1
	return to_reassign_orig - to_reassign
	

def unassign_tasks(surplusHostsByJobType):
	# Find the jobs that can use more hosts with the highest priority
	# Once we get reaper as triggers, we'll have more accurate priority to work with
	jobs = Job.select( "INNER JOIN JobStatus AS js ON fkeyjob=keyjob WHERE job.status='started' and job.packetsize=0 and job.packettype!='preassigned' and coalesce(job.hostlist,'')='' and js.taskscount - js.tasksdone - js.hostsonjob > 0 and job.fkeyjobtype in (" + ','.join( map(lambda x: str(x), surplusHostsByJobType.keys()) ) + ") order by (js.tasksaveragetime*((js.taskscount - js.tasksdone) / (js.hostsonjob::float+0.0001)))/job.priority::float desc, job.submitted asc limit 5" )
	
	# Re-sort, in case we got jobs from different tables
	jobs = jobs.sorted("priority")
	
	for job in jobs:
		print "Trying to reassign frames for job %s" % (str(job.name()))
		# Only unassign from one job, then get new stats
		if unassign_tasks_from_job(job,surplusHostsByJobType[job.jobType().key()]):
			return True
	return False


class ReadyHost(object):
	def __init__(self, keyhost):
		self.host = Host(keyhost)
		self.jobtypes = []
	def addJobType(self, fkeyjobtype):
		if not fkeyjobtype in self.jobtypes:
			self.jobtypes.append( fkeyjobtype )

while True:
	# Update service pulse
	hostService.pulse()
	service.reload()
	
	if service.enabled() and hostService.enabled():
		# Collect all free hosts, and which job types they support
		q_freehosts = Database.current().exec_("select jobtype.keyjobtype, hoststatus.fkeyhost from hoststatus inner join hostservice on hostservice.fkeyhost=hoststatus.fkeyhost inner join jobtype on jobtype.fkeyservice=hostservice.fkeyservice inner join service on service.keyservice=hostservice.fkeyservice where hostservice.enabled=true and hoststatus.slavestatus='ready'" )
		
		readyHostDict = DefaultDict( ReadyHost, True )
		
		while q_freehosts.next():
			readyHostDict[q_freehosts.value(1).toInt()[0]].addJobType( q_freehosts.value(0).toInt()[0] )
		
		q_readytasks = Database.current().exec_("select sum(tasksunassigned), fkeyjobtype from job where status in ('started','ready') and tasksunassigned > 0 and packettype!='preassigned' and (hostlist is null or hostlist='') group by fkeyjobtype;")
		
		while q_readytasks.next():
			task_count = q_readytasks.value(0).toInt()[0]
			fkeyjobtype = q_readytasks.value(1).toInt()[0]
			
			for keyhost in readyHostDict.keys():
				if fkeyjobtype in readyHostDict[keyhost].jobtypes:
					del readyHostDict[keyhost]
					task_count = task_count - 1
					if task_count <= 0:
						break
		
		freeHostsByJobType = DefaultDict(int)
		
		for keyhost in readyHostDict.iterkeys():
			for fkeyjobtype in readyHostDict[keyhost].jobtypes:
				freeHostsByJobType[fkeyjobtype] += 1
		
		for fkeyjobtype in freeHostsByJobType.iterkeys():
			print "JobType:", JobType(fkeyjobtype).name(), " Ready Hosts:", freeHostsByJobType[fkeyjobtype]
			
		delay = IDLE_DELAY
		if not DRY_RUN and len(freeHostsByJobType) > 0:
			if unassign_tasks( freeHostsByJobType ):
				delay = BUSY_DELAY
	
	time.sleep(delay)

oldcode = \
"""	
	# Here we get a general idea of the state of the farm at the moment
	q_counter = Database.current().exec_("SELECT hostsTotal, hostsActive, hostsReady, jobsTotal, jobsActive, jobsDone FROM getcounterstate();")
	q_new = Database.current().exec_("SELECT count(*) FROM JobTask WHERE status='new' AND fkeyjob IN (SELECT keyjob FROM job WHERE status IN ('ready','started') and packettype!='preassigned' and (hostlist is null OR hostlist=''))")
	
	if q_counter.next() and q_new.next():
		hostsReady = q_counter.value(2).toInt()[0]
		newTasks = q_new.value(0).toInt()[0]
		print "Host Ready %i, New Tasks %i" % (hostsReady, newTasks)
		surplusHosts = hostsReady - newTasks
		
		# Do we have hosts just waiting around?
		if DRY_RUN or surplusHosts > 0:
			unassign_tasks(surplusHosts)
			delay = BUSY_DELAY
		else:
			delay = IDLE_DELAY
			print "Farm is busy, nothing to do"
			
"""
