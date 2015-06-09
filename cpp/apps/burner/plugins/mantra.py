
from blur.Stone import *
from blur.Classes import *
from blur.Burner import *
from PyQt4.QtCore import *
from PyQt4.QtSql import *
import re, traceback, os

class MantraBurner(JobBurner):
    def __init__(self,jobAss,slave):
        JobBurner.__init__(self,jobAss,slave)
        self.Job = jobAss.job()
        self.jobAss = jobAss
        self.Slave = slave
        self.CurrentFrame = None
        self.LastRenderedFrame = None
        self.OutputsReported = 0
        self.cmdline = None
        self.workingDir = None
        self.frameList = expandNumberList( self.assignedTasks() )[0]

        self.frameStart = QRegExp("^Generating[\w\s]+[Ii]mage: (.*\.)(\d\d\d\d)(\.\w+)\s*(\(\d+x\d+\))?")
        self.jobDone = QRegExp("^Render Time:")
        self.jobProgress = QRegExp("^ALF_PROGRESS (\\d+)")

        self.progress = 0

    def __del__(self):
        # Nothing is required
        # self.cleanup() is explicitly called by the slave
        pass

    # Names of processes to kill after burn is finished
    def processNames(self):
        return QStringList()

    def environment(self):
        env = self.Job.environment().environment()
        try:
            self.cmdline = env.split("\n").filter(QRegExp("^EPA_CMDLINE=")).replaceInStrings("EPA_CMDLINE=","")[0]
        except:
            Log("could not deduce executable")

        try:
            self.workingDir = env.split("\n").filter(QRegExp("^EPA_WORKINGDIR=")).replaceInStrings("EPA_WORKINGDIR=","")[0]
        except:
            Log("could not deduce working dir")

        Log( "MantraBurner::environment(): %s" % env )
        return env.split("\n")

    def executable(self):
        self.environment()
        timeCmd = "/usr/bin/time --format=baztime:real:%e:user:%U:sys:%S:iowait:%w ";
        cmd = timeCmd + "/bin/su %s -c \"mantra " % self.Job.user().name()

        args = QStringList()
        args << "-V"
        args << "2a"
        args << "-j"
        args << str(self.jobAss.assignSlots())
        if self.Job.outputPath().isEmpty():
            args << "-F"
            #args << self.burnFile().replace("..ifd", (".%s.ifd" % self.frameList[0]))
            args << self.burnFile().replace("..ifd", (".%04d.ifd" % self.frameList[0]))
        else:
            args << "-f"
            args << self.burnFile().replace("..ifd", (".%04d.ifd" % self.frameList[0]))
            #args << self.Job.outputPath().replace("..", (".%04d." % self.frameList[0]))

        cmd = cmd + args.join(" ") + "\""
        return cmd

    def buildCmdArgs(self):
        return QStringList()

    def startProcess(self):
        Log( "MantraBurner::startBurn() called" )
        JobBurner.startProcess(self)
        Log( "MantraBurner::startBurn() done" )

    def cleanup(self):
        Log( "MantraBurner::cleanup() called" )
        if not self.process():
            Log( "MantraBurner::cleanup() no process, uh oh" )
            return
        mProcessId = self.process().pid()
        Log( "MantraBurner::cleanup() Getting pid: %s" % mProcessId )

        # Need to find the correct PID space ..
        if mProcessId > 10:
            descendants = processChildrenIds( mProcessId, True )
            for processId in descendants:
                Log( "MantraBurner::cleanup() Killing child pid: %s" % processId )
                killProcess( processId )
            killProcess( mProcessId )

        JobBurner.cleanup(self)
        Log( "MantraBurner::cleanup() done" )

    def slotProcessExited(self):
        if self.progress == 100:
            self.taskDone(self.CurrentFrame)
            self.jobFinished()
        else:
            self.jobErrored("process exited before frame was complete")
        
    def slotProcessOutputLine(self,line,channel):
        JobBurner.slotProcessOutputLine(self,line,channel)

        # Frame status
        if self.jobProgress.indexIn(line) >= 0:
            if self.CurrentFrame == None:
                self.jobErrored("output path is improperly formed")

            self.progress = int(self.jobProgress.cap(1))
            tasks = self.currentTasks()
            tasks.setProgresses(self.progress)
            tasks.commit()
        elif self.frameStart.indexIn(line) >= 0:
            self.progress == 0
            Log("MantraBurner: start frame %s" % self.frameStart.cap(2))
            frame = int(self.frameStart.cap(2))
            if not self.taskStarted():
                self.taskStart(frame)
                self.CurrentFrame = frame

            if self.Job.outputPath().isEmpty() or self.Job.outputPath().endsWith(".rat"):
                outputPath = self.frameStart.cap(1) + self.frameStart.cap(3)
                Log("MantraBurner: updating outputPath based on IFD info to %s" % outputPath)
                self.Job.setOutputPath(outputPath)
                self.Job.commit()

class MantraBurnerPlugin(JobBurnerPlugin):
    def __init__(self):
        JobBurnerPlugin.__init__(self)

    def jobTypes(self):
        return QStringList('Mantra95') << 'Mantra100'

    def createBurner(self,jobAss,slave):
        Log( "MantraBurnerPlugin::createBurner() called, Creating MantraBurner" )
        if jobAss.job().jobType().name() == 'Mantra95':
            return MantraBurner(jobAss,slave)
        if jobAss.job().jobType().name() == 'Mantra100':
            return MantraBurner(jobAss,slave)

JobBurnerFactory.registerPlugin( MantraBurnerPlugin() )

