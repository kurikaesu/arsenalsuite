from blur.Stone import *
from blur.Classes import *
from blur.Burner import *
from PyQt4.QtCore import *
from PyQt4.QtSql import *
import re, traceback, os

class BlenderBurner(JobBurner):
    def __init__(self,jobAssignment,slave):
        JobBurner.__init__(self,jobAssignment,slave)
        self.Job = JobBlender(jobAssignment.job().key())
        self.Slave = slave
        
        self.CurrentFrame = None
        self.StartFrame = None
        self.EndFrame = None
        
        self.OutputsExpected = self.Job.jobOutputs().size()
        self.OutputsReported = 0
        
    def __del__(self):
        pass
        
    def cleanup(self):
        Log( "BlenderBurner::cleanup() called" )
        if not self.process(): return
        
        mProcessId = self.processId()
        Log( "BlenderBurner::cleanup() Getting pid: %s" % (mProcessId))
        
        if mProcessId > 1:
            descendants = processChildrenIds( mProcessId, True )
            for processId in descendants:
                if processId > 1 and procesId != mProcessId:
                    Log( "BlenderBurner::cleanup() Killing child pid: %s" %(processId))
                    killProcess( processId )
            killProcess( mProcessId )
            
        JobBurner.cleanup(self)
        Log( "BlenderBurner::cleanup() done" )
        
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

        Log( "BlenderBurner::environment(): %s" % env )
        return env.split("\n")
        
    def executable(self):
        print "executable called"
        cmd = ""
        if os.name == 'nt':
            cmd = "\"C:\\Program Files\\Blender Foundation\\Blender\\blender.exe\" "
        else:
            timeCmd = "/usr/bin/time --format=baztime:real:%e:user:%Usys:%S:iowait:%w "
            cmd = timeCmd + "/bin/su %s -c \"$BLENDER/blender" % self.Job.user().name()
        
        args = QStringList()
        args << "-b" # --backround
        args << self.Job.fileName()
        args << "-t" # --threads
        args << QString.number(self.Job.assignmentSlots())
        #args << "-s" # --scene
        #args << self.Job.sceneName()
        #args << "-f" # --render-frame
        #args << "-o" # --render-output
        #args << self.Job.outputPath()
        if os.name == 'nt':
            cmd = cmd + args.join(" ")
        else:
            cmd = cmd + args.join(" ") + "\""
        print "Built exe string (%s)" % (cmd)
        return cmd
        
    def buildCmdArgs(self):
        print "buildCmdArgs called"
        args = QStringList()
        print self.CurrentFrame
        args << "-f"
        args << QString.number(self.CurrentFrame)
        print "cmd args created (%s)" % (args.join(" "))
        return args
        
    def startProcess(self):
        Log( "BlenderBurner::startBurn() called" )
        self.StartFrame = int(self.assignedTasks().section("-", 0,0))
        try:
            self.EndFrame = int(self.assignedTasks().section("-",1,1))
        except ValueError:
            self.EndFrame = self.StartFrame
        self.CurrentFrame = self.StartFrame
        JobBurner.startProcess(self)
        self.taskStart(self.CurrentFrame)
        Log( "BlenderBurner::startBurn() done" )
        
    def slotProcessOutputLine(self,line,channel):
        JobBurner.slotProcessOutputLine(self,line,channel)
        
class BlenderBurnerPlugin(JobBurnerPlugin):
    def __init__(self):
        JobBurnerPlugin.__init__(self)

    def jobTypes(self):
        return QStringList('Blender')

    def createBurner(self,jobAss,slave):
        Log( "BlenderBurnerPlugin::createBurner() called, Creating BlenderBurner" )
        if jobAss.job().jobType().name() == 'Blender':
            return BlenderBurner(jobAss,slave)

JobBurnerFactory.registerPlugin( BlenderBurnerPlugin() )