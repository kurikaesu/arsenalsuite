from blur.Stone import *
from blur.Classes import *
from blur.Burner import *
from PyQt4.QtCore import *
from PyQt4.QtSql import *
import re, traceback

class Cinema4DBurner(JobBurner):
	def __init__(self,job,slave):
		JobBurner.__init__(self,job,slave)
		self.Job = job
		self.Slave = slave
		self.CurrentFrame = None
		self.FileServerClockSkew = 0
		self.RenderStarted = False
		self.CurrentFrame = None
		self.Cinema4DDir = self.Cinema4DExe = ''
		self.JobDone = False
	
	def __del__(self):
		# Nothing is required, qt's ownership model will delete BurnProcess and CheckupTimer
		# self.cleanup() is explicitly called by the slave
		pass
	
	def executable(self):
		return self.workingDirectory() + self.Cinema4DExe
	
	def workingDirectory(self):
		c4dservices = self.Job.jobServices().services().filter('service',QRegExp('cinema4d'))
		if c4dservices.size() != 1:
			self.jobErrored( 'Unable to find single c4d service for job to determine the correct cinema4d version to run' )
			return ''
		c4dservice = c4dservices[0]
		if c4dservice.service() == 'cinema4d':
			self.Cinema4DDir = Config.getString( 'cinema4DDir', "C:/Program Files (x86)/Maxon/Cinema 4D R10/" )
			self.Cinema4DExe = Config.getString( 'cinema4DExe', 'Cinema 4d.exe' )
		elif c4dservice.service() == 'cinema4dr11':
			self.Cinema4DDir = Config.getString( 'cinema4DR11Dir', "C:/Program Files (x86)/Maxon/Cinema 4D R11/" )
			self.Cinema4DExe = Config.getString( 'cinema4DR11Exe', 'Cinema 4d.exe' )
		return self.Cinema4DDir
	
	def buildCmdArgs(self):
		args = QStringList()
		
		# Hack for how c4d handles output paths, S:/frame_02.tif will output S:/frame_02_0001.tif, S:/frame_02_0002.tif, etc
		opfi = QFileInfo(self.Job.outputPath())
		op = opfi.completeBaseName()
		if op.size() and op.at(op.size()-1).isDigit():
			op += '_'
		self.OutputPath = opfi.path() + QDir.separator() + op + "." + opfi.suffix()
		
		if not QFileInfo(self.Cinema4DDir).isDir():
			self.jobErrored( "Cinema4d dir does not exist at: " + self.Cinema4DDir )
			return args
		self.logMessage ("Cinema4d dir exists at %s " % self.Cinema4DDir)
		
		args << '-nogui'
		args << '-render'
		args << self.burnFile().replace('/','\\')
		args << '-frame'
		try:
			frameList = expandNumberList( self.assignedTasks() )[0]
			self.StartFrame = frameList[0]
			self.EndFrame = frameList[-1]
			print frameList
			args << str(self.StartFrame) << str(self.EndFrame)
		except:
			self.jobErrored('Unable to parse frame list: ' + str(self.assignedTasks()) )
		
		try:
			# Determine the file server clock skew
			tasks = JobTask.select( "fkeyjob=? AND jobtask=? LIMIT 1", [QVariant(self.Job.key()),QVariant(self.StartFrame)] )
			if tasks.size() == 1:
				path = QFileInfo(self.taskPath( tasks[0] )).path() + '/clock_skew_test.txt'
				csf = QFile(path)
				if not csf.open( QIODevice.WriteOnly ):
					self.jobErrored('Unable to open clock skew test file at %s' % path)
					return args
				csf.write('clock skew test')
				csf.close()
				self.FileServerClockSkew = QDateTime.currentDateTime().secsTo( QFileInfo(path).lastModified() )
				self.logMessage('File server clock skew set to %i' % self.FileServerClockSkew)
				QFile.remove(path)
			else:
				self.jobErrored('Unable to find a task with frameNumber %i to determine fileserver clock skew' % self.StartFrame)
				return args
		except:
			self.jobErrored('Exception occured while trying to determine file server clock skew\n%s'%traceback.format_exc())

		return args
		
	# Names of processes to kill after burn is finished
	def processNames(self):
		return QStringList() << self.Cinema4DExe
	
	def startProcess(self):
		Log( "Cinema4DBurner::startBurn() called" )
		JobBurner.startProcess(self)
		self.checkupTimer().start(1000) # Every five seconds, since we rely on this instead of stdout to mark frames busy/done
		Log( "Cinema4DBurner::startBurn() done" )
	
	def cleanup(self):
		Log( "Cinema4DBurner::cleanup() called" )
		JobBurner.cleanup(self)
		if self.Cinema4DExe:
			killAll(self.Cinema4DExe)
		Log( "Cinema4DBurner::cleaup() done" )
	
	def startTimeUnskewed(self):
		return self.startTime().addSecs( self.FileServerClockSkew )
	
	def taskPath(self,task):
		path = None
		if task.jobOutput().isRecord() and RangeFileTracker(task.jobOutput().fileTracker()).isRecord():
			path = task.jobOutput().fileTracker().filePath(task.frameNumber())
		else:
			path = makeFramePath( self.OutputPath, task.frameNumber() )
		return path
	
	def startFrame(self,frameNumber):
		if not self.taskStart(frameNumber):
			return False
		self.CurrentFrame = frameNumber
		self.OutputPaths = {}
		for task in self.currentTasks():
			# RangeFileTracker::filePath( int frameNumber ), returns full frame path
			path = self.taskPath(task)
			if path is not None:
				self.logMessage( 'Adding path to check for current frame output ' + path )
				self.OutputPaths[path] = False
			else:
				self.jobErrored( 'Unable to get valid output path for frame %i, ID: %i' % (task.frameNumber(), task.key()) )
				return False
		return True
	
	def checkup(self):
		Log( "Cinema4DBurner::checkup() called" )
		if self.JobDone: return True
		while self.CurrentFrame is not None:
			allFramesDone = True
			for path, checked in self.OutputPaths.iteritems():
				if not checked:
					self.logMessage( "Checking Path: " + path )
					allFramesDone = False
					fi = QFileInfo( path )
					if not fi.exists():
						self.logMessage( "File not found" )
						break
					if not fi.isFile():
						self.logMessage( "Path does not point to a file" )
						break
					if not Path.checkFileFree( path ):
						self.logMessage( "File is not free" )
						break
					if not (fi.lastModified() > self.startTimeUnskewed()):
						self.logMessage( "File is old, last modified: %s" % fi.lastModified().toString() )
						break
					# Mark this frame as finished
					allFramesDone = True
					self.OutputPaths[path] = True
					self.logMessage( 'Frame Output Finished: ' + path )
			
			if allFramesDone:
				self.logMessage( 'All outputs finished for frame: ' + str(self.CurrentFrame) )
				self.taskDone( self.CurrentFrame )
				if self.CurrentFrame == self.EndFrame:
					# Give 5 seconds for the object buffer passes to be written since we dont
					# know about them or check for them yet
					self.JobDone = True
					QTimer.singleShot( 30000, self, SLOT('jobFinished()') )
					return True
				if not self.startFrame( self.CurrentFrame + 1 ):
					return False
			else:
				break
		return JobBurner.checkup(self)
	
	def slotProcessStarted(self):
		Log( "Cinema4DBurner::slotStarted" )
		self.startFrame( self.StartFrame )
	
	def slotProcessExited(self):
		if not self.JobDone:
			self.checkup()
		if self.JobDone:
			self.jobFinished()
			return
		JobBurner.slotProcessExited(self)
	
class Cinema4DBurnerPlugin(JobBurnerPlugin):
	def __init__(self):
		JobBurnerPlugin.__init__(self)

	def jobTypes(self):
		return QStringList('Cinema4D') << 'Cinema4D R11'

	def createBurner(self,job,slave):
		Log( "Cinema4DBurnerPlugin::createBurner() called, Creating Cinema4DBurner" )
		return Cinema4DBurner(job,slave)

JobBurnerFactory.registerPlugin( Cinema4DBurnerPlugin() )
