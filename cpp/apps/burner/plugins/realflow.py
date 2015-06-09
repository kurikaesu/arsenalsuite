from blur.Stone import *
from blur.Classes import *
from blur.Burner import *
from PyQt4.QtCore import *
from PyQt4.QtSql import *
import re, traceback

class RealFlowBurner(JobBurner):
	def __init__(self,job,slave):
		JobBurner.__init__(self,job,slave)
		self.Job = job
		self.Slave = slave
		self.RenderStarted = False
		self.JobDone = False
		self.RealFlowService = None
		self.RealFlowSoftware = None
		self.RealFlowDir = None

	def __del__(self):
		# Nothing is required
		# self.cleanup() is explicitly called by the slave
		pass
	
	# Names of processes to kill after burn is finished
	def processNames(self):
		return QStringList() << "realflow.exe"
	
	def getRealFlowService(self):
		if self.RealFlowService is None:
			realflowServices = self.Job.jobServices().services().filter('service',QRegExp('^RealFlow'))
			if len(realflowServices) == 0:
				raise "Unable to find RealFlow service for job to determine render executable"
			if len(realflowServices) > 1:
				raise "Multiple RealFlow services listed, unable to determine which RealFlow installation to use"
			self.RealFlowService = realflowServices[0]
		return self.RealFlowService
	
	def getRealFlowSoftware(self):
		if self.RealFlowSoftware is None:
			service = self.getRealFlowService()
			self.RealFlowSoftware = service.software()
			if not self.RealFlowSoftware.isRecord():
				self.jobErrored( "service " + xsiService().service() + " has no software entry" );
		return self.RealFlowSoftware

	def getRealFlowDir(self):
		sw = self.getRealFlowSoftware()
		return sw.installedPath()
	
	def workingDirectory(self):
		return self.getRealFlowDir()
	
	def executable(self):
		try:
			return self.getRealFlowDir() + "RealFlow.exe"
		except:
			traceback.print_exc()
			self.jobErrored( "Unknown exception getting fusion path: " + traceback.format_exc() )
		
	def buildCmdArgs(self):
		# C:\Program Files (x86)\Next Limit\x64\RealFlow\RealFlow.exe -nogui -range 1 100 -mesh -useCache  "emitter.flw" 
		args = QStringList()
		args << "-nogui"
		args << "-range"
		tasks = self.assignedTasks().split("-")
		self.StartFrame = int(tasks[0])
		if len(tasks) == 2:
			args << str(int(tasks[0])-1) << tasks[1]
			self.EndFrame = int(tasks[1])
		elif len(tasks) == 1:
			args << str(int(tasks[0])-1) << tasks[0]
			self.EndFrame = int(tasks[0])
		if self.Job.simType() == 'Mesh':
			args << "-mesh"
			args << "-useCache"
		args << self.burnFile()
		return args
	
	def startProcess(self):
		Log( "RealFlowBurner::startBurn() called" )
		JobBurner.startProcess(self)
		self.BurnProcess = self.process()
		self.Started = QDateTime.currentDateTime()
		self.checkupTimer().start(2 * 60 * 1000) # Every two minutes
		Log( "RealFlowBurner::startBurn() done" )
	
	def cleanup(self):
		Log( "RealFlowBurner::cleanup() called" )
		JobBurner.cleanup(self)
		Log( "RealFlowBurner::cleaup() done" )
	
#C:\Program Files (x86)\Next Limit\x64\RealFlow4>RealFlow -nogui -range 1 2 "G:\F
#errari\01_Shots\Sc005\S0001.00\FX\LavaSplash\LavaSplashPreset\LavaSplashPreset-V
#3.flw"
#'import site' failed; use -v for traceback
#-----------------------------------------------------------------
  #1998-2007 (c) Next Limit - RealFlow(32bits) v4.3.8.0124
#-----------------------------------------------------------------

#Initializing Parameter Blocks ...
#Checking license (use -license to enter a valid license)...
#Initializing expressions engine ...
#Initializing Real Impact world ...
#Initializing Simulation Events dispatcher ...

#Loading plugins ...
#Loading workspace G:\Ferrari\01_Shots\Sc005\S0001.00\FX\LavaSplash\LavaSplashPre
#set\LavaSplashPreset-V3.flw ...

#>12:27:33: RealFlow error: Couldn't install license server. The UDP port 2222is
#used by another application. Probably another RealFlow instance.

#>>> Reading SD file
#>>> 50%
#>>> 100%



#>12:27:33: Workspace version: 2046
#>>> Setting up scene
#>>> 14%
#>>> 28%
#>>> 42%
#>>> 57%
#>>> 71%
#>>> 85%
#>>> 100%



#Using 2 threads for this simulation ...
#..................................................
#>12:27:51: Frame 2 finished.

#>12:27:51: Elapsed time: (0h0m18s)

#>12:27:51: =========================================

	def slotProcessOutputLine(self,line,channel):
		Log( "RealFlowBurner::slotReadOutput() called, ready to read output" )
		error = False
		
		# Job started
		if re.search( '100%', line ):
			self.RenderStarted = True
			self.taskStart(self.StartFrame)
		
		# Frames Rendered
		mo = re.search( r'Frame (\d+) finished.', line )
		if mo:
			frame = int(mo.group(1))
			
			self.taskDone(frame)
			
			if frame == self.EndFrame:
				self.JobDone = True
				QTimer.singleShot( 30000, self, SLOT('jobFinished()') )
			else:
				self.taskStart(frame + 1)
			return
		
		JobBurner.slotProcessOutputLine(self,line,channel)

	def slotProcessExited(self):
		if not self.JobDone:
			self.checkup()
		if self.JobDone:
			self.jobFinished()
			return
		JobBurner.slotProcessExited(self)

class RealFlowBurnerPlugin(JobBurnerPlugin):
	def __init__(self):
		JobBurnerPlugin.__init__(self)

	def jobTypes(self):
		return QStringList('RealFlow')

	def createBurner(self,job,slave):
		Log( "RealFlowBurnerPlugin::createBurner() called, Creating RealFlowBurner" )
		if job.jobType().name() == 'RealFlow':
			return RealFlowBurner(job,slave)
		return None

JobBurnerFactory.registerPlugin( RealFlowBurnerPlugin() )
