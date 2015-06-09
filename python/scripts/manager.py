#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *
import blur
import sys, time
from math import ceil
from blur.defaultdict import DefaultDict
from exceptions import Exception
import traceback

import signal
import cProfile
import lsprofcalltree

if sys.argv.count('-daemonize'):
    from blur.daemonize import createDaemon
    createDaemon()

managerConfig = '/etc/ab/manager.ini'
try:
    pos = sys.argv.index('-config')
    managerConfig = sys.argv[pos+1]
except: pass
dbConfig = '/etc/ab/db.ini'
try:
    pos = sys.argv.index('-dbconfig')
    dbConfig = sys.argv[pos+1]
except: pass
scheduler = None
try:
    pos = sys.argv.index('-scheduler')
    if pos > 0:
        scheduler = sys.argv[pos+1]
except: pass
servicesInclude = None
try:
    pos = sys.argv.index('-servicesinclude')
    if pos > 0:
        servicesInclude = str.split(sys.argv[pos+1],",")
except: pass
servicesExclude = None
try:
    pos = sys.argv.index('-servicesexclude')
    if pos > 0:
        servicesExclude = str.split(sys.argv[pos+1],",")
except: pass

app = QCoreApplication(sys.argv)

initConfig( managerConfig, "/var/log/ab/manager.log" )
# Read values from db.ini, but dont overwrite values from manager.ini
# This allows db.ini defaults to work even if manager.ini is non-existent
config().readFromFile( dbConfig, False )

initStone(sys.argv)

blur.RedirectOutputToLog()

# Load all the database tables
classes_loader()

VERBOSE_DEBUG = False

if VERBOSE_DEBUG:
    Database.current().setEchoMode( Database.EchoUpdate | Database.EchoDelete | Database.EchoInsert | Database.EchoSelect )

# Connect to the database
Database.current().connection().reconnect()

# Automatically setup the AB_manager service record
# the hostservice record for our host, and enable it
# if no other hosts are enabled for this service
service = Service.ensureServiceExists('AB_manager')
hostService = service.byHost(Host.currentHost(),True)
hostService.enableUnique()

# Stores/Updates Config vars needed for manager
class ManagerConfig:
    def update(self):
        # Sort parameters
        if scheduler:
            self._SORT_METHOD = 'key_'+scheduler
        else:
            self._SORT_METHOD = Config.getString( 'arsenalSortMethod', 'key_default' )
        try:
            JobAssign.SortMethod = getattr(JobAssign, str(self._SORT_METHOD))
            #print ("Using %s job sort method" % self._SORT_METHOD)
        except:
            print "Config sort method %s not found, using key_default" % self._SORT_METHOD
            JobAssign.SortMethod = JobAssign.key_default

        self._PRIO_SORT_MUL = Config.getFloat( 'assburnerPrioSort', 100 )
        self._ERROR_SORT_MUL = Config.getFloat( 'assburnerErrorSort', 10 )
        self._SUB_TIME_SORT_MUL = Config.getFloat( 'assburnerSubmissionSort', 3 )
        self._HOSTS_SORT_MUL = Config.getFloat( 'assburnerHostsSort', 100 )

        # Mapserver weight parameters
        self._MAPS_WEIGHT_MUL = Config.getFloat( 'assburnerMapWeightMultiply', 1.0 )
        self._PC_WEIGHT_MUL = Config.getFloat( 'assburnerPointCacheWeightMultiply', 0.2 )
        self._XREF_WEIGHT_MUL = Config.getFloat( 'assburnerXrefWeightMultiply', 1.0 )
        self._WEIGHT_DIVIDER = Config.getFloat( 'assburnerWeightDivider', 1024 * 1024 * 50 )

        # Rate of assignment parameters
        self._ASSIGN_RATE = Config.getFloat( 'assburnerAssignRate', 15 )
        self._ASSIGN_PERIOD = Config.getFloat( 'assburnerAssignPeriod', 10 )

        self._HOST_ERROR_LIMIT = Config.getFloat( 'assburnerHostErrorLimit', 2 )

        # target is the number of seconds for an assignment to run based on a priority 10 job
        # default is the default number of seconds for a jobtask if there's no previous done tasks to get averagetasktime
        # so if targer is 600(10min) and a job is taking 30 seconds per task, it should get 20 tasks assigned
        # but it's scaled by the priority, so a priority 70 job will have higher packet sizes than a priority 1 job
        # idea is that more packets mean more time spent loading the files and resources, so is less efficient.
        # but for jobs that are high priority(low #), we sacrafice efficiency in order to get the job done quicker
        # it would be better to have some code in max to estimate the task time, or to remember from previous renders, but we've never gotten around to implementing that
        # It would also be nice to be able to estimate load time vs task time so we can actually calculate the efficiency
        self._AUTOPACKET_TARGET = Config.getFloat( 'assburnerAutoPacketTarget', 600 )
        self._AUTOPACKET_DEFAULT = Config.getFloat( 'assburnerAutoPacketDefault', 60 )

        # this is the max percentage of slots that will get assigned in a single
        # assignment loop.
        # so priorities may not be targeted correctly for up to this % of total slots on
        # the farm
        self._ASSIGN_SLOPPINESS = Config.getFloat( 'arsenalAssignSloppiness', 2.0 )

        self._ASSIGN_MAXHOSTS = Config.getFloat( 'arsenalAssignMaxHosts', 0.1 ) # 10 percent default

# Single Instance
config = ManagerConfig()

# Utility Functions
def numberListToString(number_list):
    return ','.join([str(i) for i in number_list])

def unique(alist):
    return [alist[i] for i in range(0,len(alist)) if alist.index(alist[i])==i]

# Python2.3 compat
if not hasattr(__builtins__,'sorted'):
    def sorted(alist):
        alist.sort()
        return alist

# Wraps a single job that needs tasks assigned
class JobAssign:
    def __init__( self, job ):
        self._hostOk = None
        self.ErroredHosts = None
        self.sortKey = None
        self.Job = job
        self.JobStatus = job.jobStatus()
        self.lastPriorityHostsOnJob = None
        self.tasksUnassigned = self.JobStatus.tasksUnassigned()
        self.servicesRequired = ServiceList()
        self.preassignedHosts = None
        self.logString = ''

    def loadHosts( self ):
        if self._hostOk == None:
            self._hostOk = [s for s in str.split(str(self.Job.hostList()),',') if len(s)]

    def loadErroredHosts( self ):
        # Get list of errored hosts
        if self.ErroredHosts == None:
            self.ErroredHosts = {}
            q = Database.current().exec_("SELECT fkeyhost, sum(count) FROM JobError WHERE fkeyjob=%i AND cleared=false GROUP BY fkeyhost HAVING sum(count) > %i" % (self.Job.key(), config._HOST_ERROR_LIMIT))
            while q.next():
                self.ErroredHosts[q.value(0).toInt()[0]] = q.value(1).toInt()[0]

    def loadPreassignedHosts( self ):
        if self.preassignedHosts == None:
            self.preassignedHosts = Host.select("INNER JOIN JobTask ON fkeyhost=keyhost WHERE JobTask.status='new' AND JobTask.fkeyJob=?", [QVariant(self.Job.key())])

    # Return value of None means there is no white list, all non-blacklisted hosts that 
    # satisfy this jobs service list are valid.
    def hostWhiteList( self ):
        ret = HostList()
        if self.Job.packetType() == 'preassigned':
            self.loadPreassignedHosts()
            ret = self.preassignedHosts
        else:
            self.loadHosts()
            if len(self._hostOk)==0:
                return None
            for hostName in self._hostOk:
                ret.append( Host.recordByName( hostName ) )
        self.loadErroredHosts()
        for fkeyhost in self.ErroredHosts.keys():
            ret.remove( Host(fkeyhost) )
        return ret

    # Returns the list of hosts that cannot be assigned to this job
    # Currently this is only errored out hosts
    def hostBlackList( self ):
        ret = HostList()
        self.loadErroredHosts()
        for fkeyhost in self.ErroredHosts.keys():
            ret.append( Host(fkeyhost) )
        return ret

    def hostMemoryOk( self, hostStatus ):
        # We need an idea of how much memory we require, in kb
        if self.Job.minMemory() > 0:
            memoryRequired = self.Job.minMemory()
        else:
            memoryRequired = self.JobStatus.averageMemory()

        if memoryRequired == 0:
            if VERBOSE_DEBUG: Log( 'Job has unknown memory requirements, not assigning.' )
            return False

        # job memory 16GB  = 16000000
        # host memory 24GB =    24147
        if memoryRequired > (hostStatus.availableMemory() * 1000):
            if VERBOSE_DEBUG: Log( 'Not enough memory, %i required, %i available on %s' % (memoryRequired, hostStatus.availableMemory() * 1000, hostStatus.host().name()) )
            return False
        if self.Job.maxMemory() > (hostStatus.host().memory()*1024):
            if VERBOSE_DEBUG: Log( 'Not enough memory, %i maximum, %i physical on %s' % (self.Job.maxMemory(), hostStatus.host().memory() * 1024, hostStatus.host().name()) )
            return False

        # Only if workstation - to be save for now
        #if hostStatus.host().name().startsWith("om"):
        #    if self.Job.maxMemory() > (hostStatus.availableMemory() * 1024):
        #        Log( 'Not enough memory on workstation.' )
                #return False

        # memory is ok to assign
        return True

    def hostOk( self, hostStatus, snapshot ):
        host = hostStatus.host()

        if FarmResourceSnapshot.hostsUnused.get(hostStatus, 0) - self.Job.assignmentSlots() < 0:
            if VERBOSE_DEBUG: Log( 'Job requires more slots (%s) than host (%s) has available (%s)' % (self.Job.assignmentSlots(), host.name(), FarmResourceSnapshot.hostsUnused.get(hostStatus, 0) - self.Job.assignmentSlots() ) )
            return False

        if hostStatus.activeAssignmentCount() > 0:
            # Check exclusive bit
            if self.Job.exclusiveAssignment():
                if VERBOSE_DEBUG: Log( 'Cant assign exclusive job to a host that has an assignment' )
                return False

        if not self.hostMemoryOk( hostStatus ):
            return False

        # Check for preassigned job list
        if self.Job.packetType() == 'preassigned':
            self.loadPreassignedHosts()
            if not host in self.preassignedHosts:
                return False

        # Check in host list
        self.loadHosts()
        if len(self._hostOk) > 0 and not host.name() in self._hostOk:
            #if VERBOSE_DEBUG: print "Skipping Host, Not In Host List: ", host.name()
            return False

        # Check for errored hosts
        self.loadErroredHosts()
        if host.key() in self.ErroredHosts:
            #if VERBOSE_DEBUG: print "Job %i errored out on host %s with %i errors." % (self.Job.key(),host.name(),self.ErroredHosts[host.key()])
            return False

        return True

    def priority( self ):
        if self.lastPriorityHostsOnJob != self.JobStatus.hostsOnJob():
            if self.Job.prioritizeOuterTasks() and not self.Job.outerTasksAssigned():
                self._priority = 1
            else:
                self._priority = \
                    self.Job.priority() 		* config._PRIO_SORT_MUL \
                    + self.JobStatus.errorCount() 	* config._ERROR_SORT_MUL \
                    + self.Job.submittedts().toTime_t()	* config._SUB_TIME_SORT_MUL \
                    + self.JobStatus.hostsOnJob()	* config._HOSTS_SORT_MUL
            self.lastPriorityHostsOnJob = self.JobStatus.hostsOnJob()
        return self._priority

    def __cmp__( self, other ):
        if id(self) == id(other): return 0
        #if self.Job.user() == other.Job.user():
        #    if self.Job.priority() == other.Job.priority():
        #        diff = self.Job.personalPriority() - other.Job.personalPriority()
        #        if diff > 0: return 1
        #        elif diff < 0: return -1
        # bone sez... if the sort key is == then the sort will never stabalize
        #             so we must check for equivalence
        my_key    = JobAssign.SortMethod(self)
        other_key = JobAssign.SortMethod(other)
        if my_key == other_key: return 0
        if my_key  > other_key: return 1
        return -1

    def key_default( self ):
        sortKey = self.priority()
        #if VERBOSE_DEBUG: print 'job %s has key %s' % (self.Job.name(), sortKey)
        return sortKey

    def key_even_by_priority( self ):
        hasHost = 0
        if self.JobStatus.hostsOnJob() > 0: hasHost = 1
        sortKey = '%03d-%01d-%04d-%04d-%10d' % (self.Job.priority(), hasHost, int(self.JobStatus.errorCount()/5.0), self.JobStatus.hostsOnJob(), self.Job.submittedts().toTime_t())
        self.sortKey = sortKey
        return sortKey

    def key_user_project_soft_reserves( self ):
        user_deficit = self.Job.user().arsenalSlotReserve() - FarmResourceSnapshot.slotsByUser.get(self.Job.user().name(),0)
        if(user_deficit < 0): user_deficit = 0
        project_deficit = self.Job.project().arsenalSlotReserve() - FarmResourceSnapshot.slotsByProject.get(self.Job.project().name(),0)
        if(project_deficit < 0): project_deficit = 0
        urgent = 99
        if(self.Job.priority() <= 20): urgent = self.Job.priority()
        sortKey = '%02d-%05d-%05d-%03d-%04d-%04d' % (urgent, int(99999-user_deficit), int(99999-project_deficit), self.Job.priority(), int(self.JobStatus.errorCount()/5.0), self.JobStatus.hostsOnJob())

        self.sortKey = sortKey
        return sortKey

    def key_darwin( self ):
        # bone sez... priority should still be taken into account so that the wranglers still have a way to push something important to the front
        urgent = 99
        if(self.Job.priority() <= 20): urgent = self.Job.priority()

        #project_deficit = self.Job.project().arsenalSlotReserve() - FarmResourceSnapshot.slotsByProject.get(self.Job.project().name(),0)
        reserve_used = min(100, int(float(FarmResourceSnapshot.slotsByProject.get(self.Job.project().name(),0)) / float(max(1,self.Job.project().arsenalSlotReserve())) * 100))
        if not self.Job.project().isRecord(): reserve_used = 999

        # important things will take precedence within a department, but not over everything
        important = 55
        if(self.Job.priority() <= 30): important = self.Job.priority()
        if(self.Job.priority() >= 60): important = self.Job.priority()

        shotTimeKey = "%s-%s" % (self.Job.shotName(),  self.Job.project().name())
        #avgTime = min(1, int(self.JobStatus.errorCount()/5.0)) * FarmResourceSnapshot.shotTimes.get(shotTimeKey, 99999999)
        avgTime = FarmResourceSnapshot.shotTimes.get(shotTimeKey, 99999999)

        hasTensRunning = 0
        tensRunning = self.JobStatus.tasksDone() + self.JobStatus.tasksAssigned() + self.JobStatus.tasksBusy()
        # if we have some done then include the cancelled ones in this sum as the job is at least partially good
        # and therefore should not get stuck trying to reach the 10% mark
        if( self.JobStatus.tasksDone() > 0):
            tensRunning += self.JobStatus.tasksCancelled()
        if( float(tensRunning) / float(self.JobStatus.tasksCount()) >= 0.1 ):
            hasTensRunning = 1

        hasTensComplete = 1
        tensComplete = self.JobStatus.tasksDone()
        # if we have some done then include the cancelled ones in this sum as the job is at least partially good
        # and therefore should not get stuck trying to reach the 10% mark
        if( self.JobStatus.tasksDone() > 0):
            tensComplete += self.JobStatus.tasksCancelled()
        if( float(tensComplete) / float(self.JobStatus.tasksCount()) >= 0.1 ):
            hasTensComplete = 0

        # bone sez... within a particular shot, prefer slower passes first
        # ideally this would also include the time from jobs that this job blocks so
        # that we favour doing stuff deep in the chain
        shotAvgTime = 999999 - self.JobStatus.tasksAverageTime()

        sortKey = '%02d-%03d-%02d-%01d-%01d-%08d-%06d' % ( urgent, reserve_used, important, hasTensRunning, hasTensComplete, avgTime, shotAvgTime )
        self.sortKey = sortKey
        return sortKey

    def calculateAutoPacketSize( self, totalHosts, totalTasks ):
        return self.calculateAutoPacketSizeByAvgTime( totalHosts, totalTasks )

    def calculateAutoPacketSizeByAvgTime( self, totalHosts, totalTasks ):
        tasksByHost = ceil( totalTasks / totalHosts )
        maxSize = 50
        # Target packet time duration, smaller time for higher(lower #) priority jobs
        target = config._AUTOPACKET_TARGET * max(self.Job.priority(),3) / 10
        idealSize = ceil(target / self.calculateAverageTaskTime())
        packetSize = min( idealSize, tasksByHost )
        return max(packetSize,1)

    def calculateAverageTaskTime( self ):
        if self.JobStatus.tasksAverageTime() > 0 and self.JobStatus.tasksDone() > 0:
            return self.JobStatus.tasksAverageTime()
        return config._AUTOPACKET_DEFAULT

    def retrieveRandomTasks(self, limit):
        self.logString += 'Random [limit=%i] ' % limit
        return JobTask.select("fkeyjob=%i AND status='new' ORDER BY random() ASC LIMIT %i" % (self.Job.key(),limit))

    def retrieveSequentialTasks(self, limit):
        self.logString += 'Sequential [limit=%i] ' % limit
        return JobTask.select("fkeyjob=%i AND status='new' ORDER BY jobtask ASC LIMIT %i" % (self.Job.key(),limit))

    def retrieveContinuousTasks(self, limit):
        self.logString += 'Continuous [limit=%i] ' % limit
        return JobTask.select("keyjobtask IN (SELECT * FROM get_continuous_tasks(%i,%i))" % (self.Job.key(),limit))

    def retrievePreassignedTasks(self, hostStatus):
        self.logString += 'Preassigned '
        return JobTask.select("fkeyjob=%i AND fkeyhost=%i AND status='new'" % (self.Job.key(),hostStatus.getValue("fkeyhost").toInt()[0]))

    def retrieveOuterTasks(self,limit=None):
        q = Database.current().exec_('SELECT min(jobtask), max(jobtask), avg(jobtask) FROM jobtask WHERE fkeyjob=%i' % (self.Job.key()))
        if not q.next(): return
        tasks = [ q.value(0).toInt()[0], q.value(1).toInt()[0] ]
        midGuess = q.value(2).toDouble()[0]

        # Get the closest numbered task
        q = Database.current().exec_('SELECT jobtask, abs(%g-jobtask) as distance from jobtask where fkeyjob=%i order by distance asc limit 1' % (midGuess,self.Job.key()) )
        if q.next():
            tasks.append( q.value(0).toInt()[0] )

        # Make Unique
        tasks = unique(tasks)
        self.logString += ('Outer Tasks [%s] ' % str(tasks))
        if limit is None or limit < 1:
            limit = 3
        return JobTask.select( "jobtask IN (%s) AND fkeyjob=%i AND status='new' LIMIT %i" % ( numberListToString(tasks), self.Job.key(), limit ) )

    def retrieveIterativeTasks(self, limit, stripe, strictOnTens):
        self.logString += 'Iterative [limit=%i,stripe=%i,strict=%s] ' % (limit, stripe,strictOnTens)
        #return JobTask.select("keyjobtask IN (SELECT * FROM get_iterative_tasks(%i,%i,%i))" % (self.Job.key(),limit,stripe))
        return JobTask.select("keyjobtask IN (SELECT * FROM get_iterative_tasks_2(%i,%i,%i,%s))" % (self.Job.key(),limit,stripe,strictOnTens))

    def assignHostFunc( self, hostStatus, totalHosts, totalTasks ):
        q = Database.current().exec_('SELECT * FROM assign_single_host_2(%s,%s,%s)' % (self.Job.key(), hostStatus.host().key(), self.Job.packetSize()))
        tasks = []
        while q.next():
            tasks.append(q.value(0).toInt()[0])

        Log( "Assigned tasks " + numberListToString(tasks) + " from job " + str(self.Job.key()) + " to host " + hostStatus.host().name() )
        #time.sleep(10)
        return len(tasks)

    def assignHost( self, hostStatus, totalHosts, totalTasks ):
        tasks = None
        packetSize = self.Job.packetSize()

        self.logString = ' PT: '
        if self.Job.prioritizeOuterTasks() and not self.Job.outerTasksAssigned():
            tasks = self.retrieveOuterTasks(limit=packetSize)
            self.Job.setOuterTasksAssigned(True)
            self.Job.commit()
        else:
            if packetSize < 1:
                packetSize = self.calculateAutoPacketSize( totalHosts, totalTasks )
            packetType = self.Job.packetType()
            if VERBOSE_DEBUG: print "Finding Tasks For PacketType:", packetType, " packetSize: ", packetSize
            if packetType == 'random':
                tasks = self.retrieveRandomTasks(limit = packetSize)
            elif packetType == 'preassigned':
                tasks = self.retrievePreassignedTasks(hostStatus)
            elif packetType == 'continuous':
                tasks = self.retrieveContinuousTasks(limit=packetSize)
            elif packetType == 'iterative':
                strictOnTens = 'true'
                # shouldn't this also include tasksSuspended()?
                tensRunning = self.JobStatus.tasksDone() + self.JobStatus.tasksAssigned() + self.JobStatus.tasksBusy() + self.JobStatus.tasksCancelled()
                if( float(tensRunning) / float(self.JobStatus.tasksCount()) >= 0.1 ):
                    strictOnTens = 'false'
                tasks = self.retrieveIterativeTasks(limit=packetSize,stripe=10,strictOnTens=strictOnTens)
            else: # sequential
                tasks = self.retrieveSequentialTasks(limit=packetSize)

        if not tasks or tasks.isEmpty():
            if VERBOSE_DEBUG: print "No tasks returned to assign"
            return 0

        jtal = JobTaskAssignmentList()
        for jt in tasks:
            jta = JobTaskAssignment()
            jta.setJobTask( jt )
            jtal += jta
        jtal.setJobAssignmentStatuses( JobAssignmentStatus.recordByName( 'ready' ) )

        ja = JobAssignment()
        ja.setJob( self.Job )
        ja.setJobAssignmentStatus( JobAssignmentStatus.recordByName( 'ready' ) )
        ja.setHost( hostStatus.host() )
        ja.commit()

        jtal.setJobAssignments( ja )
        jtal.commit()

        # deduct memory requirement from available so we don't over allocate the host
        if( hostStatus.host().os().startsWith("Linux") ):
            min = self.Job.minMemory()
            if( min == 0 ): min = self.Job.jobStatus().averageMemory()
            hostStatus.setAvailableMemory( (hostStatus.availableMemory()*1024 - min) / 1024 )
            if( hostStatus.activeAssignmentCount() > 0 and hostStatus.availableMemory() <= 0 ):
                Database.current().rollbackTransaction();
                print "Host %s does not have the memory required returning" % (hostStatus.host().name())
                return 0
            hostStatus.commit()

        tasks = JobTaskList()
        for jta in jtal:
            task = jta.jobTask()
            task.setJobTaskAssignment( jta )
            task.setStatus( 'assigned' )
            task.setHost( hostStatus.host() )
            tasks += task
        tasks.commit()

        # Keep hostsOnJob up to date while assigning.  It is accurately updated periodically by the reaper
        if self.Job.maxHosts() > 0:
            Database.current().exec_("UPDATE jobstatus SET hostsOnJob=hostsOnJob+1 WHERE fkeyjob=%i" % self.Job.key())

        # Increment hosts on job count,  this is taken care of by a trigger at the database level
        # but we increment here to get more accurate calculated priority until the next refresh of
        # the job list
        self.JobStatus.setHostsOnJob( self.JobStatus.hostsOnJob() + 1 )

        Log( "Assigned tasks " + numberListToString(tasks.frameNumbers()) + " from job " + str(self.Job.key()) + " to host " + hostStatus.host().name() + self.logString )
        return tasks.size()

def updateProjectTempo():
    Database.current().exec_( "SELECT * from update_project_tempo()" )

# Returns [hostsTotal int, hostsActive int, hostsReady int, jobsTotal int, jobsActive int, jobsDone int]
def getCounter():
    q = Database.current().exec_( "SELECT hostsTotal, hostsActive, hostsReady FROM getcounterstate()" )
    if not q.next(): return map(lambda x:0,range(0,3))
    return map(lambda x: q.value(x).toInt()[0], range(0,3))

timer = QTime()

class FreeHost(object):
    def __init__(self,hostStatus):
        self.hostStatus = hostStatus
        self.host = hostStatus.host()
        self.hostServices = HostServiceList()
        self.services = ServiceList()

    def addService(self,hostService):
        self.hostServices.append(hostService)
        self.services.append(hostService.service())

    def handlesServiceList(self,serviceList):
        for service in serviceList:
            if service not in self.services:
                return False
        return True

    def handlesJob(self,jobAssign):
        return handlesServiceList(jobAssign.servicesRequired)

class ThrottleLimitException(Exception): pass
class AllHostsAssignedException(Exception): pass
class AllJobsAssignedException(Exception): pass
class NonCriticalAssignmentError(Exception): pass

class FarmResourceSnapshot(object):
    slotsByUserAndService = {}
    limitsByUserAndService = {}
    slotsByUser = {}
    limitsByUser = {}
    slotsByProject = {}
    limitsByProject = {}
    shotTimes = {}
    hostsUnused = {}
    iteration = 10

    def __init__(self):
        # Regular Job/Task Info
        self.jobList = JobList()
        self.jobAssignByJob = {}

        # Service info
        self.servicesNeeded = ServiceList()
        self.jobsByService = DefaultDict(JobList)

        # Host Info
        self.freeHosts = {}
        self.hostStatuses = HostStatusList()
        self.hostStatusesByService = DefaultDict(HostStatusList)

        self.licCountByService = {}
        self.potentialSlotsAvailable = 0

    def reset(self):
        # Regular Job/Task Info
        self.jobList.clear()
        self.jobAssignByJob.clear()

        # Service info
        self.servicesNeeded.clear()
        self.jobsByService.clear()

        # Host Info
        self.freeHosts.clear()
        self.hostStatuses.clear()
        self.hostStatusesByService.clear()
        self.hostsUnused.clear()

        self.licCountByService.clear()
        self.potentialSlotsAvailable = 0

    def refresh(self):
        self.reset()

        # Project Info
        self.refreshProjectUsage()

        # Shot Info
        self.refreshShotTimes()

        # User Info
        # no point refresh User usage if not using that scheduler..
        #self.refreshUserUsage()

        self.refreshJobList()
        if self.jobList.isEmpty():
            return
        self.refreshServiceData()
        self.refreshHostData()

    def refreshUserUsage(self):
        self.slotsByUserAndService.clear()
        self.slotsByUser.clear()
        self.limitsByUserAndService.clear()
        self.limitsByUser.clear()

        q = Database.current().exec_("""
SELECT * from user_service_current
                                    """)
        while q.next():
            key = q.value(0).toString()
            value = q.value(1).toInt()[0]
            self.slotsByUserAndService[key] = value

        q2 = Database.current().exec_("""
SELECT * from user_service_limits
                                    """)
        while q2.next():
            key = q2.value(0).toString()
            value = q2.value(1).toInt()[0]
            self.limitsByUserAndService[key] = value

        q3 = Database.current().exec_("""
SELECT * from user_slots_current
                                    """)
        while q3.next():
            key = q3.value(0).toString()
            value = q3.value(1).toInt()[0]
            self.slotsByUser[key] = value

        q4 = Database.current().exec_("""
SELECT * from user_slots_limits
                                    """)
        while q4.next():
            key = q4.value(0).toString()
            limit = q4.value(1).toInt()[0]
            reserve = q4.value(2).toInt()[0]
            self.limitsByUser[key] = [limit, reserve]

    def refreshProjectUsage(self):
        self.slotsByProject.clear()
        self.limitsByProject.clear()

        q = Database.current().exec_("""
SELECT * from project_slots_current
                                    """)
        while q.next():
            key = q.value(0).toString()
            current = q.value(1).toInt()[0]
            reserve = q.value(2).toInt()[0]
            limit = q.value(3).toInt()[0]
            self.slotsByProject[key] = current
            self.limitsByProject[key] = [limit, reserve]

    def refreshShotTimes(self):
        self.shotTimes.clear()

        q = Database.current().exec_("""
SELECT * from running_shots_averagetime_4
                                    """)
        while q.next():
            shot = q.value(0).toString()
            project = q.value(1).toString()
            value = q.value(2).toInt()[0]
            key = "%s-%s" % (shot, project)
            self.shotTimes[key] = int(value)

    def refreshJobList(self):
        # Gather the jobs 
        self.jobList = Job.select( """WHERE status IN ('ready','started') 
                                        AND ((SELECT count(*) FROM jobtask WHERE fkeyjob = keyjob AND status = 'new') > 0)""")
        statuses = JobStatusList()
        if self.jobList.size() > 0:
            #statuses = JobStatus.select("fkeyjob IN("+self.jobList.keyString()+")")
            statuses = self.jobList.jobStatuses()
        for job in self.jobList:
            # Create Job Assign class
            jobAssign = JobAssign(job)
            self.jobAssignByJob[job] = jobAssign

        if VERBOSE_DEBUG: Log( "Found %i jobs to assign" % len(self.jobList) )

    def addJobService(self,job,service):
        jobAssign = self.jobAssignByJob[job]
        jobAssign.servicesRequired.append(service)
        self.jobsByService[service] += job
        if not service in self.servicesNeeded:
            self.servicesNeeded += service

    def refreshServiceData(self):
        # Gather required services
        for jobService in JobService.select( "JobService.fkeyJob IN (" + self.jobList.keyString() + ")" ):
            if servicesInclude:
                if jobService.service().service() in servicesInclude:
                    self.addJobService(jobService.job(), jobService.service())
            elif servicesExclude:
                if not jobService.service().service() in servicesExclude:
                    self.addJobService(jobService.job(), jobService.service())
            else:
                self.addJobService(jobService.job(), jobService.service())

        # Filter out services that have no available licenses
        self.licCountByService.clear()

        q = Database.current().exec_("""SELECT * from license_usage_2""")
        while q.next():
            key = str(q.value(0).toString())
            limit = q.value(1).toInt()[0]
            current = q.value(2).toInt()[0]
            self.licCountByService[key] = int(limit - current)
            #Log( "Service %s has limit: %s, current: %s" % (key, limit, current) )

    def refreshHostData(self):
        hostServices = HostServiceList()
        if VERBOSE_DEBUG: Log( "Fetching Hosts for required services, %s" % self.servicesNeeded.services().join(',') )
        #if VERBOSE_DEBUG: Log( "Fetching Hosts for required services, %s" % self.servicesNeeded.keyString() )
        if not self.servicesNeeded.isEmpty():
            hostServices = HostService.select( """INNER JOIN HostStatus ON HostStatus.fkeyHost=HostService.fkeyHost 
                                                  INNER JOIN Host ON HostStatus.fkeyhost=Host.keyhost 
                                                  WHERE HostStatus.slaveStatus='ready' 
                                                    AND HostStatus.activeAssignmentCount < coalesce(Host.maxAssignments,8)
                                                    AND HostService.enabled=true AND HostService.fkeyService IN (%s)""" % self.servicesNeeded.keyString()  )
        hosts = hostServices.hosts().unique()

        if len(hosts):
            self.hostStatuses = HostStatus.select("fkeyhost IN (" + hosts.keyString() + ")")
        else:
            self.hostStatuses = HostStatusList()
        for hostService in hostServices:
            host = hostService.host()
            service = hostService.service()
            hostStatus = host.hostStatus()
            if not hostStatus in self.freeHosts:
                self.freeHosts[hostStatus] = FreeHost(hostStatus)
            self.freeHosts[hostStatus].addService(hostService)
            self.hostStatusesByService[service] += hostStatus
            self.hostsUnused[hostStatus] = host.maxAssignments() - hostStatus.activeAssignmentCount()

        for host in hosts:
            self.potentialSlotsAvailable =  self.potentialSlotsAvailable + host.maxAssignments()

        if VERBOSE_DEBUG:
            for service in self.servicesNeeded:
                print "%i hosts available for service %s" % (len(self.hostStatusesByService[service]),service.service())
        #	for hs in self.hostsUnused:
        #		Log( "Host is Ready %s" % (hs.host().name()) )

    def availableHostsByService(self,service):
        if service in self.hostStatusesByService:
            ret = HostStatusList(self.hostStatusesByService[service])
            #if len(ret) == 0:
            #	if VERBOSE_DEBUG: print ("No Hosts Left for Service %s, Removing" % service.service())
            #	self.removeService(service)
            return ret
        if VERBOSE_DEBUG: print "Service Not available", service.service()
        return HostStatusList()

    # Get a list of host statuses that have all services required
    def availableHosts( self, jobAssign ):
        requiredServices = jobAssign.servicesRequired
        hostStatuses = HostStatusList()

        # Services are always required
        if requiredServices.isEmpty():
            if VERBOSE_DEBUG: print "*** Job has no service requirements, skipping"
            return hostStatuses

        # Start the list off with the first service
        # Only return hosts that provide ALL required services
        # Make sure to take a copy so we don't modify the original
        hostStatuses += self.hostStatusesByService[requiredServices[0]]
        for service in requiredServices:
            # Check if there are licenses available for the service
            if service in self.licCountByService:
                if VERBOSE_DEBUG: print ("%i licenses available for service %s" % (self.licCountByService[service], service.service()))
                if self.licCountByService[service] <= 0:
                    return HostStatusList()
            hostStatuses &= self.availableHostsByService(service)
            if hostStatuses.isEmpty():
                if VERBOSE_DEBUG: print "** No hosts available with required services"
                break

        # We have the hosts that match the service requirements, now
        # we can check against the job's white/black list of hosts
        ret = HostStatusList()
        for hostStatus in hostStatuses:
            if jobAssign.hostOk(hostStatus,self):
                ret += hostStatus
            else:
                if VERBOSE_DEBUG: print "** hostOk is NOT ok!"

        # Assign hosts with 0 jobs first
        #ret = ret.sorted( "activeassignmentcount" )
        return ret

    # Always call canReserveLicenses first
    def reserveLicenses(self,services):
        for service in services:
            serviceName = str(service.service())
            if self.licCountByService.has_key(serviceName) and self.licCountByService[serviceName] > 0:
                self.licCountByService[serviceName] -= 1

    def canReserveLicenses(self,services):
        for service in services:
            serviceName = str(service.service())
            #Log( "canReserveLicense for %s? %s left" % ( serviceName, self.licCountByService.get(serviceName, 9999) ) )
            if self.licCountByService.has_key(serviceName) and (self.licCountByService[serviceName]) <= 0:
                return False
        return True

    def jobListByServiceList( self, serviceList ):
        jobs = JobList()
        for service in serviceList:
            for job in self.jobsByService[service]:
                jobs += job
        return jobs.unique()

    def taskCountByServices( self, serviceList ):
        taskCount = 0
        for job in self.jobListByServiceList(serviceList):
            if job in self.jobAssignByJob:
                taskCount += self.jobAssignByJob[job].tasksUnassigned
        return taskCount

    # Remove the job as a job that needs more hosts assigned
    def removeJob( self, job ):
        self.jobList.remove( job )
        if job in self.jobAssignByJob:
            del self.jobAssignByJob[job]

    # Removes the host from available hosts to assign to
    # Removes all services that have no remaining hosts
    def removeHostStatus( self, hostStatus, slots ):
        hostStatus.setActiveAssignmentCount( hostStatus.activeAssignmentCount() + slots )

        if hostStatus in self.hostsUnused:
            self.hostsUnused[hostStatus] = self.hostsUnused[hostStatus] - slots
            if self.hostsUnused[hostStatus] <= 0:
                # host has no slots left so remove it
                del self.hostsUnused[hostStatus]
                if hostStatus in self.freeHosts:
                    for service in self.freeHosts[hostStatus].services:
                        self.hostStatusesByService[service].remove(HostStatus(hostStatus.key()))
                        if len(self.hostStatusesByService[service]) == 0:
                            self.removeService(service)
                    del self.freeHosts[hostStatus]

    # Called to remove the service and all jobs that require it
    def removeService( self, service ):
        if service in self.hostStatusesByService:
            del self.hostStatusesByService[service]
        if service in self.jobsByService:
            for job in self.jobsByService[service]:
                self.removeJob( job )
            del self.jobsByService[service]
        self.servicesNeeded.remove( service )

    # Returns (assignedCount, tasksUnassigned)
    def assignSingleHost(self, jobAssign, hostStatus):
        job = jobAssign.Job

        # Make sure this host hasn't been used yet
        # This should not be triggered
        if not hostStatus in self.hostsUnused:
            raise NonCriticalAssignmentError("Host %s missing from self.hostsUnused" % hostStatus.host().name())

        # This should already be checked, but should be free from selects, so do it again here
        #if not jobAssign.hostOk( hostStatus, self ):
        #    raise NonCriticalAssignmentError("Host %s failed redundant jobAssign.hostOk check" % hostStatus.host().name())

        # Account for licenses used by each service
        # This is just a temporary local count to avoid checking the
        # database every assignment.
        if not self.canReserveLicenses(jobAssign.servicesRequired):
            raise NonCriticalAssignmentError("Service %s out of licenses" % "something")
        self.reserveLicenses(jobAssign.servicesRequired)

        # check for project limits on total slot use
        key = job.project().name()
        if self.limitsByProject.has_key(key):
            if self.slotsByProject.has_key(key):
                self.slotsByProject[key] = self.slotsByProject[key] + job.assignmentSlots()
            else:
                self.slotsByProject[key] = job.assignmentSlots()

            if self.limitsByProject[key][0] > -1 and self.slotsByProject[key] > self.limitsByProject[key][0]:
                raise NonCriticalAssignmentError("Project %s slot limit of %s reached" % (key, self.limitsByProject[key][0]))

        # check for per user limits on total slot use
        key = job.user().name()
        if self.limitsByUser.has_key(key):
            if self.slotsByUser.has_key(key):
                self.slotsByUser[key] = self.slotsByUser[key] + job.assignmentSlots()
            else:
                self.slotsByUser[key] = job.assignmentSlots()

            if self.limitsByUser[key][0] > -1 and self.slotsByUser[key] > self.limitsByUser[key][0]:
                raise NonCriticalAssignmentError("User %s slot limit of %s reached" % (key, self.limitsByUser[key][0]))

        # check for per user limits on slot use per service
        for service in jobAssign.servicesRequired:
            key = job.user().name() +":"+ service.service()

            if self.limitsByUserAndService.has_key(key):
                if self.slotsByUserAndService.has_key(key):
                    self.slotsByUserAndService[key] = self.slotsByUserAndService[key] + job.assignmentSlots()
                else:
                    self.slotsByUserAndService[key] = job.assignmentSlots()

                #Log( "slotsByUserAndService: key %s, value %s" % (key,  self.slotsByUserAndService[key]))
                #if self.limitsByUserAndService.has_key(key):
                #    Log( "limitsByUserAndService: key %s, value %s" % (key,  self.limitsByUserAndService[key]))

                if self.slotsByUserAndService[key] > self.limitsByUserAndService[key]:
                    raise NonCriticalAssignmentError("Service %s exceeds user limit" % service.service())

        # If the host_assign fails, then skip this job
        # There are valid reasons for this to fail, such as the host going offline
        tasksAssigned = jobAssign.assignHost(hostStatus, len(self.hostsUnused), self.taskCountByServices(jobAssign.servicesRequired))
        if tasksAssigned < 1:
            #if VERBOSE_DEBUG: print "JobAssign.assignHost failed"
            Log( "JobAssign.assignHost failed" )
            raise NonCriticalAssignmentError("Job out of tasks")
            return (0,jobAssign.tasksUnassigned)

        jobAssign.tasksUnassigned -= tasksAssigned

        # Remove the job from the list if there are no tasks left to assign
        if jobAssign.tasksUnassigned <= 0:
            if VERBOSE_DEBUG: print "Removing job from list, all tasks are assigned"
            self.removeJob( job )

        # Host no longer ready
        self.removeHostStatus(hostStatus, job.assignmentSlots())

        return (tasksAssigned,jobAssign.tasksUnassigned)

    def assignSingleJob(self,jobAssign):
        #print 'job %s has sortKey %s' % (jobAssign.Job.name(), jobAssign.key_even_by_priority())
        #if VERBOSE_DEBUG: print 'job %s has sortKey %s' % (jobAssign.Job.name(), jobAssign.key_even_by_priority())

        # Check maxhosts
        if jobAssign.Job.maxHosts() > 0 and jobAssign.JobStatus.hostsOnJob() >= jobAssign.Job.maxHosts():
            self.removeJob(jobAssign.Job)
            return False

        hostStatuses = self.availableHosts( jobAssign )
        if hostStatuses.isEmpty():
            if VERBOSE_DEBUG: print "*** No hosts available for this job - returning"
            return False

        assignedHosts = 0
        maxAssignedHosts = int(jobAssign.JobStatus.tasksCount() * config._ASSIGN_MAXHOSTS)

        # if less than 10% of a job is started, don't overrun 10%, for Darwin purposes
        tenPercentComplete = float(jobAssign.JobStatus.tasksCount()-jobAssign.JobStatus.tasksUnassigned()) / float(jobAssign.JobStatus.tasksCount())
        if tenPercentComplete < 0.1:
            maxAssignedHosts = int(jobAssign.JobStatus.tasksCount() * 0.1) - (jobAssign.JobStatus.tasksCount()-jobAssign.JobStatus.tasksUnassigned())

        # Gather available hosts for this job
        if VERBOSE_DEBUG: print "Finding Hosts to Assign to %i tasks to Job %s, possible: %s" % (jobAssign.JobStatus.tasksUnassigned(), jobAssign.Job.name(), hostStatuses.size())
        print "Finding Hosts to Assign to %i out of %i tasks to Job %s, possible: %s" % (maxAssignedHosts, jobAssign.tasksUnassigned, jobAssign.Job.name(), hostStatuses.size())

        assignSuccess = False
        Database.current().beginTransaction()
        for hostStatus in hostStatuses:
            tasksAssigned, tasksLeft = self.assignSingleHost( jobAssign, hostStatus )

            # Return so that we can recalculate assignment priorities
            if tasksAssigned > 0:
                assignSuccess = True
                assignedHosts = assignedHosts + 1
                self.slotAssignCount = self.slotAssignCount + jobAssign.Job.assignmentSlots()

                if ((float(self.slotAssignCount) / self.potentialSlotsAvailable)*100) > config._ASSIGN_SLOPPINESS:
                    Database.current().exec_("SELECT update_job_task_counts(%i)" % jobAssign.Job.key())
                    print "enough hostStatuses assigned, resort job list and also update_job_task_counts(%i)" % jobAssign.Job.key()
                    Database.current().commitTransaction()
                    return True

                if (assignedHosts > maxAssignedHosts):
                    #Database.current().exec_("SELECT update_job_task_counts(%i)" % jobAssign.Job.key())
                    Database.current().commitTransaction()
                    return True
            else:
                # If we get to here there's no hosts to assign to this job, so ignore it
                self.removeJob( jobAssign.Job )

            # This shouldn't get hit, because there should be tasks available if we
            # are assigning, and we should already break above if they get assigned
            if tasksLeft <= 0:
                Database.current().commitTransaction()
                raise NonCriticalAssignmentError("Job %s has tasksLeft <= 0 before any assignments made" % jobAssign.Job.name())

        Database.current().commitTransaction()
        return assignSuccess

    def performNormalAssignments(self):
        # We loop while there are hosts, and we are assigning hosts
        # Its possible to have free hosts that can't provide a service
        # for our current jobs
        while True:
            self.slotAssignCount = 0
            if len(self.hostsUnused) < 1:
                raise AllHostsAssignedException()

            # Assign from all ready jobs
            jobAssignList = []
            for ja in self.jobAssignByJob.values():
                if ja.tasksUnassigned > 0:
                    jobAssignList.append(ja)

            if len(jobAssignList) == 0:
                raise AllJobsAssignedException()

            Project().select()
            Log("re-sorting job priorities, %s jobs to consider" % len(jobAssignList))
            jobAssignList.sort()

            if( FarmResourceSnapshot.iteration % 10 == 0 ):
                Log("clearing queueOrder")
                Database.current().exec_("UPDATE jobstatus SET queueorder = 9999 WHERE queueorder < 9999")

            queueOrder = 1
            for jobAssign in jobAssignList:
                #print "job %s has key %s" % ( jobAssign.Job.name(), jobAssign.sortKey )
                try:
                    if( FarmResourceSnapshot.iteration % 10 == 0 ):
                        Database.current().exec_("UPDATE jobstatus SET queueorder = %s WHERE fkeyjob = %s" % (queueOrder, jobAssign.Job.key()))
                    queueOrder = queueOrder + 1
                    self.assignSingleJob(jobAssign)
                    # Recalc priority and resort job list after assignments.
                    # Use the "sloppiness" attribute to determine how many slots
                    # to assign in a single loop.
                    if ((float(self.slotAssignCount) / self.potentialSlotsAvailable)*100) > config._ASSIGN_SLOPPINESS:
                        Log( "%s hostStatuses out of %s assigned, resort job list" % (self.slotAssignCount, self.potentialSlotsAvailable) )
                        return
                except NonCriticalAssignmentError:
                    Database.current().commitTransaction()
                    if VERBOSE_DEBUG: traceback.print_exc()

            if self.slotAssignCount == 0:
                break

    def performAssignments(self):
        try:
            timer.start()
            self.performNormalAssignments()
        except ThrottleLimitException, e:
            print "Assignment has been throttled."
        except AllHostsAssignedException:
            if VERBOSE_DEBUG: print "All relevant 'ready' hosts have been assigned"
        except AllJobsAssignedException:
            # Catch this exception, no need to print the time if this is the case
            if VERBOSE_DEBUG: print "All jobs have been assigned"
        except Exception, e:
            # Print the time but let the exception pass through
            if VERBOSE_DEBUG: print "Unknown error while assigning"
            traceback.print_exc()
            raise e
        finally:
            print "Finished assigning jobs, took %i" % (timer.elapsed())
            if( timer.elapsed() < 10 ): time.sleep(1)

def run_loop():
    config.update()

    # Complete Job / Host snapshot
    snapshot = FarmResourceSnapshot()
    print "Manager: Beginning Loop. (%i)" % snapshot.iteration
    snapshot.refresh()
    snapshot.performAssignments()
    FarmResourceSnapshot.iteration += 1
    if FarmResourceSnapshot.iteration > 110:
        sys.exit(0)

def manager2():
    print "Manager: Starting up"
    while True:
        hostService.pulse()
        service.reload()
        if service.enabled() and hostService.enabled():
            run_loop()
            if VERBOSE_DEBUG: print "Loop Assigning Jobs, took: %i, Waiting 1 second" % (timer.elapsed())
            #time.sleep( 1 )

def manager3():
    print "Manager: Starting up"
    while True:
        run_loop()

if VERBOSE_DEBUG:
    profile = cProfile.Profile()

def handler(signum):
    print 'Signal handler called with signal', signum
    profileName = "/tmp/manager.profile"
    kProfile = lsprofcalltree.KCacheGrind(profile)
    kFile = open(profileName, 'w+')
    kProfile.output(kFile)
    kFile.close()

if __name__ == "__main__":
    if VERBOSE_DEBUG:
        #signal.signal(signal.SIGHUP, handler)
        #profile.runctx('manager3()', globals(), locals())
        manager3()
    else:
        manager3()

