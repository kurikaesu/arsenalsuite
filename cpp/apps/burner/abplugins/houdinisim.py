
from blur.Stone import *
from blur.Classes import *
from blur.Burner import *
from PyQt4.QtCore import *
from PyQt4.QtSql import *
import traceback, os

class HoudiniSimBurner(JobBurner):
	def __init__(self,jobAss,slave):
		JobBurner.__init__(self, jobAss, slave)
		self.Job = jobAss.job()
		self.CurrentFrame = None
		self.frameList = []
		self.StartFrame = None
		self.EndFrame = None

		self.jobDone = QRegExp("^baztime:")

		self.errors = []
		self.errors.append(QRegExp("cannot open output file"))
		self.errors.append(QRegExp("3DL SEVERE ERROR L2033"))
		self.errors.append(QRegExp("Command exited with non-zero status"))

	def __del__(self):
		# Nothing is required
		# self.cleanup() is explicitly called by the slave
		pass
	
	# Names of processes to kill after burn is finished
	def processNames(self):
		return QStringList()

	def environment(self):
		env = self.Job.environment().environment()
		Log( "HoudiniSimBurner::environment(): %s" % env )
		return env.split("\n")

	def buildCmdArgs(self):
		return QStringList()

	def executable(self):

		timeCmd = "/usr/bin/time --format=baztime:real:%e:user:%U:sys:%S:iowait:%w ";
		cmd = timeCmd + "/bin/su %s -c \"exec hython /drd/software/int/farm/hbatchrendersim.py " % self.Job.user().name()

		args = QStringList()
		args << str(self.Job.fileName())
		args << str(self.Job.nodeName())
		args << str(self.Job.frameStart())
		args << str(self.Job.frameEnd())
		args << str(self.Job.packetSize())
		args << str(self.assignedTasks())
		#args << str(self.tracker())

		cmd = cmd + args.join(" ") + "\""

		return cmd

	def startProcess(self):
		Log( "HoudiniSimBurner::startBurn() called" )
		JobBurner.startProcess(self)
		Log( "HoudiniSimBurner::startBurn() done" )

	def cleanup(self):
		Log( "HoudiniSimBurner::cleanup() called" )

		mProcessId = self.process().pid()

		Log( "HoudiniSimBurner::cleanup() Getting pid: %s" % mProcessId )

    # Need to find the correct PID space ..
        if mProcessId > 6000:
            descendants = processChildrenIds( mProcessId, True )
            for processId in descendants:
                Log( "HoudiniSimBurner::cleanup() Killing pid: %s" % processId )
                killProcess( processId )

		JobBurner.cleanup(self)
		Log( "HoudiniSimBurner::cleaup() done" )

	def slotProcessOutputLine(self,line,channel):
		JobBurner.slotProcessOutputLine(self,line,channel)

class HoudiniSimBurnerPlugin(JobBurnerPlugin):
	def __init__(self):
		JobBurnerPlugin.__init__(self)

	def jobTypes(self):
		return QStringList('HoudiniSim10')

	def createBurner(self,jobAss,slave):
		Log( "HoudiniSimBurnerPlugin::createBurner() called" )
		if jobAss.job().jobType().name() == 'HoudiniSim10':
			return HoudiniSimBurner(jobAss,slave)

JobBurnerFactory.registerPlugin( HoudiniSimBurnerPlugin() )

