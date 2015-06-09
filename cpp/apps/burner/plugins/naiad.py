
from blur.Stone import *
from blur.Classes import *
from blur.Burner import *
from PyQt4.QtCore import *
from PyQt4.QtSql import *
import traceback, os

class NaiadBurner(JobBurner):
    def __init__(self,jobAss,slave):
        JobBurner.__init__(self,jobAss,slave)
        self.Job = jobAss.job()
        self.CurrentFrame = None
        self.frameList = []
        self.StartFrame = None
        self.EndFrame = None

        # Naiad: Solving frame 26
        self.frameStart = QRegExp("^Naiad: Solving frame (\d+)")
        self.jobDone = QRegExp("^baztime:")

    def __del__(self):
        # Nothing is required
        # self.cleanup() is explicitly called by the slave
        pass

    def cleanup(self):
        Log( "NaiadBurner::cleanup() called" )
        if not self.process():
            Log( "NaiadBurner::cleanup() process doesn't even exist" )
            return
        mProcessId = self.process().pid()
        Log( "NaiadBurner::cleanup() Getting pid: %s" % mProcessId )

        # Need to find the correct PID space ..
        if mProcessId > 10:
            descendants = processChildrenIds( mProcessId, True )
            for processId in descendants:
                Log( "NaiadBurner::cleanup() Killing child pid: %s" % processId )
                killProcess( processId )
            killProcess( mProcessId )

        JobBurner.cleanup(self)
        Log( "NaiadBurner::cleanup() done" )

    # Names of processes to kill after burn is finished
    def processNames(self):
        return QStringList()

    def environment(self):
        env = self.Job.environment().environment()
        Log( "NaiadBurner::environment(): %s" % env )
        return env.split("\n")

    def buildCmdArgs(self):
        return QStringList()

    def executable(self):
        self.frameList = expandNumberList( self.assignedTasks() )[0]
        if not len( self.frameList ) > 0:
            Log( "NaiadBurner::executable(): assigned %s" % self.assignedTasks() )
            self.jobErrored("got no assigned tasks =(")
            return ""
        self.StartFrame = self.frameList[0]
        self.EndFrame = self.frameList[-1]

        timeCmd = "/usr/bin/time --format=baztime:real:%e:user:%U:sys:%S:iowait:%w ";
        cmd = timeCmd + "/bin/su %s -c \"/drd/software/int/farm/naiadstub.sh " % (self.Job.user().name())

        args = QStringList()
        args << "--verbose"
        args << "--threads"
        args << str(self.Job.threads())
        args << "--frames"
        args << str(self.StartFrame) << str(self.EndFrame)

        if not self.Job.append().isEmpty:
            args << self.Job.append()

        # if the frame is the first frame in the job, do NOT use the restartCache!
        frames = self.Job.jobTasks().frameNumbers()
        frames.sort()
        if self.Job.forceRestart() or (not self.Job.restartCache().isEmpty() and self.StartFrame != frames[0]):
            self.logMessage("using restartCache %s" % self.Job.restartCache())
            restartParam = self.Job.restartCache()
            #restartParam.replace("####", "%04d" % int(int(self.StartFrame)-1))
            # Naiad implementation has changed, now use a single # instead of a frame number
            restartParam.replace("####", "#")
            args << restartParam

        args << self.burnFile()

        cmd = cmd + args.join(" ") + "\""
        return cmd

    def startProcess(self):
        #Log( "NaiadBurner::startProcess: clearing license locks " )
        #os.system("/drd/software/int/farm/naiadinit.sh &> /tmp/naiadlic.out")
        JobBurner.startProcess(self)

    def slotProcessOutputLine(self,line,channel):
        JobBurner.slotProcessOutputLine(self,line,channel)

        # Frame status
        if self.frameStart.indexIn(line) >= 0:
            if self.CurrentFrame:
                self.taskDone(self.CurrentFrame)

            self.CurrentFrame = int(self.frameStart.cap(1))
            self.taskStart(self.CurrentFrame)

        elif line.contains(self.jobDone):
            if self.CurrentFrame:
                self.taskDone(self.CurrentFrame)
            self.jobFinished()

class NaiadBurnerPlugin(JobBurnerPlugin):
    def __init__(self):
        JobBurnerPlugin.__init__(self)

    def jobTypes(self):
        return QStringList('Naiad')

    def createBurner(self,jobAss,slave):
        Log( "NaiadBurnerPlugin::createBurner() called, Creating NaiadBurner" )
        if jobAss.job().jobType().name() == 'Naiad':
            return NaiadBurner(jobAss,slave)

JobBurnerFactory.registerPlugin( NaiadBurnerPlugin() )

