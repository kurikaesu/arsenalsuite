
from blur.Stone import *
from blur.Classes import *
from blur.Burner import *
from PyQt4.QtCore import *
from PyQt4.QtSql import *
import re, traceback, os

class NukeBurner(JobBurner):
    def __init__(self,jobAss,slave):
        JobBurner.__init__(self,jobAss,slave)
        self.Job = jobAss.job()
        self.Slave = slave

        self.CurrentFrame = None
        self.StartFrame = None
        self.EndFrame = None
        self.FramePendingComplete = None

        self.LastRenderedFrame = None

        self.OutputsExpected = self.Job.jobOutputs().size()
        self.OutputsReported = 0

        self.frameDone = QRegExp("^Writing .*\.(\d{4,})\.\w+ took")
        self.frameStart = QRegExp("^Writing .*\.(\d{4,})\.\w+")
        self.jobDone = QRegExp("^Total render time:")

    def __del__(self):
        # Nothing is required
        # self.cleanup() is explicitly called by the slave
        pass

    def cleanup(self):
        Log( "NukeBurner::cleanup() called" )
        if not self.process(): return

        mProcessId = self.process().pid()
        Log( "NukeBurner::cleanup() Getting pid: %s" % mProcessId )

        # Need to find the correct PID space ..
        if mProcessId > 10:
            descendants = processChildrenIds( mProcessId, True )
            for processId in descendants:
                Log( "NukeBurner::cleanup() Killing child pid: %s" % processId )
                killProcess( processId )
            killProcess( mProcessId )

        JobBurner.cleanup(self)
        Log( "NukeBurner::cleanup() done" )

    # Names of processes to kill after burn is finished
    def processNames(self):
        return QStringList()

    def slotProcessExited(self):
        if self.CurrentFrame:
            self.taskDone(self.CurrentFrame)
            self.jobFinished()
        else:
            self.jobErrored("process exited before frame was complete")

    def environment(self):
        env = self.Job.environment().environment()

        Log( "NukeBurner::environment(): %s" % env )
        return env.split("\n")

    def executable(self):
        timeCmd = "/usr/bin/time --format=baztime:real:%e:user:%U:sys:%S:iowait:%w ";
        cmd = timeCmd + "/bin/su %s -c \"$NUKE/Nuke$NUKE_MAJOR " % self.Job.user().name()

        args = QStringList()
        # verbose
        args << "-V"
        # execute
        args << "-x"
        # terminal only
        # behavior change between Nuke5 and Nuke6
        #args << "-t"
        # full-size - no proxy
        args << "-f"
        # cache limit
        args << "-c"
        args << str(int(self.Job.maxMemory() / 1024 / 1024))+"G"
        # # of threads
        args << "-m"
        args << str(self.Job.assignmentSlots())
        # view?
        if not self.Job.viewName().isEmpty():
            args << "-view"
            args << self.Job.viewName()
        # limit nodes to execute
        if not self.Job.nodes().isEmpty():
            args << "-X"
            args << self.Job.nodes()
        # frame range
        args << "-F"
        args << self.assignedTasks()
        args << self.burnFile()

        cmd = cmd + args.join(" ") + "\""
        return cmd

    def startProcess(self):
        Log( "NukeBurner::startBurn() called" )
        JobBurner.startProcess(self)
        self.StartFrame = int(self.assignedTasks().section("-",0,0))
        try:
            self.EndFrame = int(self.assignedTasks().section("-",1,1))
        except ValueError:
            self.EndFrame = self.StartFrame
        self.CurrentFrame = self.StartFrame

        self.logMessage("starting render - expecting %s outputs" % self.OutputsExpected)
        self.taskStart(self.CurrentFrame)
        Log( "NukeBurner::startBurn() done" )

    def slotProcessOutputLine(self,line,channel):
        JobBurner.slotProcessOutputLine(self,line,channel)
        #Log( "NukeBurner::slotReadOutput() called, ready to read output" )
        # Frame status
        if self.frameDone.indexIn(line) >= 0:
        #elif self.frameStart.indexIn(line) >= 0:
            self.OutputsReported = self.OutputsReported + 1
            self.logMessage( "NukeBurner::slotReadOutput() output detected %s of %s" % (self.OutputsReported, self.OutputsExpected) )
            if self.OutputsReported == self.OutputsExpected:
                self.logMessage( "NukeBurner::slotReadOutput() all outputs done for frame" )
                # all outputs have been accounted for, set the task to complete
                self.taskDone(self.CurrentFrame)

                #if self.FramePendingComplete:
                #    self.taskDone(self.CurrentFrame)

                self.CurrentFrame = self.CurrentFrame + 1
                #frame = int(self.frameStart.cap(1))
                if self.CurrentFrame <= self.EndFrame:
                    self.taskStart(self.CurrentFrame)

                # reset to zero for next task
                self.OutputsReported = 0

class NukeBurnerPlugin(JobBurnerPlugin):
    def __init__(self):
        JobBurnerPlugin.__init__(self)

    def jobTypes(self):
        return QStringList('Nuke51') << 'Nuke52' << 'Nuke'

    def createBurner(self,jobAss,slave):
        Log( "NukeBurnerPlugin::createBurner() called, Creating NukeBurner" )
        if jobAss.job().jobType().name() == 'Nuke51':
            return NukeBurner(jobAss,slave)
        if jobAss.job().jobType().name() == 'Nuke52':
            return NukeBurner(jobAss,slave)
        if jobAss.job().jobType().name() == 'Nuke':
            return NukeBurner(jobAss,slave)

JobBurnerFactory.registerPlugin( NukeBurnerPlugin() )

