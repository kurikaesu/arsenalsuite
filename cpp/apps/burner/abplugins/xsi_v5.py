from blur.Stone import *
from blur.Classes import *
from blur.Burner import *
from PyQt4.QtCore import *
from PyQt4.QtSql import *
from PyQt4.QtGui import QApplication
qs = QString

#
#C:\softimage\XSI_5.0\Application\bin\xsibatch.bat -batchonly -batchuni -r -scene "<scene>" -pass "<PassName>" -startframe 1 -endframe 2
#
#c:/max8/\3dsmaxcmd.exe -frames:57 -outputName:S:\temp_AUTODELETED\dt\hair3_scanline\hairTest3_.tga -height:768 -width:1024 -v:5 -rfw:1 -videoColorCheck:0 -force2sided:0 -renderHidden:0 -atmospherics:1 -superBlack:0 -renderFields:0 -fieldOrder:odd -displacements:1 -effects:1 -ditherPaletted:0 -ditherTrueColor:0 c:\\max5\\assburner3\\maxhold113881.max
#
class XSIBurner(JobBurner):
	def __init__(self,job,slave):
		Log( "XSIBurner::ctor() called: " + str(self) )
		JobBurner.__init__(self,job,slave)
		self.Job = job
		self.Slave = slave
		self.CurrentFrame = None
		self.UseStatusFile = False
		self.XSIService = None
		killAll( "XSIBATCH.exe" )
		Log( "XSIBurner::ctor() done: " + str(self) )
	
	def __del__(self):
		# Nothing is required
		# self.cleanup() is explicitly called by the slave
		Log( "XSIBurner::dtor() done: " + str(self) )
		
	def xsiService(self):
		if self.XSIService is None:
			xsiservices = self.Job.jobServices().services().filter('service',QRegExp('XSI'))
			if xsiservices.size() != 1:
				self.jobErrored( "Unable to find XSI service" )
				return Service()
			self.XSIService = xsiservices[0]
		return self.XSIService
	
	def processNames(self):
		return QStringList() << "XSIBATCH.exe"
	
	def xsiBinPath(self):
		sw = self.xsiService().software()
		if not sw.isRecord():
			self.jobErrored( "service " + xsiService().service() + " has no software entry" );
			return ''
		return sw.installedPath()
		
	def spmHost(self):
		return Config.getString( 'slaveSPMHost', 'war' )
	
	def executable(self):
		useBatch = (self.Job.jobType().name() == 'XSIScript') and self.Job.scriptRequiresUi()
		if useBatch:
			return "cmd.exe"
		return  self.xsiBinPath() + 'xsibatch.exe'
	
	def generateFrameString(self):
		# Setup a frame list
		Log( "XSIBurner::buildCmdArgs() Expanding the number list" )
		(frameList,valid) = expandNumberList( self.assignedTasks() )
		Log( "XSIBurner::buildCmdArgs() Rendering frames " + str(frameList) )
		if not valid:
			self.jobErrored("The slaveFrame list was not valid: " + self.assignedTasks())
			return QStringList()
		Log( "XSIBurner::buildCmdArgs() Creating frame string" )
		frames = QStringList()
		for i in frameList:
			frames << QString.number( i )
		return frames.join(',')
		
	def xsiScriptArgs(self):
		args = QStringList()
		if self.Job.scriptRequiresUi():
			args << qs("-uiscript")
		else:
			args << qs("-script")
		args << qs(self.burnDir() + '/' + self.Job.scriptFile()).replace('/','\\')
		if not self.Job.scriptMethod().isEmpty():
			args << qs("-main") << self.Job.scriptMethod()
		args << qs("-args") << qs("-inFile") << qs(self.burnDir() + '/' + self.Job.xsiFile()).replace('/','\\')
		args << qs("-inFrames") << self.generateFrameString()
		args << qs("-jobId") << qs(str(self.Job.key()))
		return args
	
	# Generates a 2 line batch file that runs XSI_BIN_DIR/setenv.bat then XSI_BIN_DIR/xsi.exe -uiscript ...
	# Also prepares the status file by generating the path and ensuring that an existing one does not exist
	def generateXSIBatScript(self):
		batchPath = self.burnDir() + '/exec.bat'
		if QFile.exists(batchPath) and not QFile.remove(batchPath):
			self.jobErrored("Unable to remove exec.bat")
			return ""
		f = QFile( batchPath )
		if not f.open( QIODevice.WriteOnly ):
			self.jobErrored("Unable to open %s for writing" % batchPath)
			return ""
		txt_s = QTextStream(f)
		txt_s << ("call %s/setenv.bat\n" % self.xsiBinPath())
		txt_s << ("call %s/xsi.exe " % self.xsiBinPath()) << self.xsiScriptArgs().join(" ")
		del txt_s
		f.close()
		del f
		return batchPath
	
	# set SPM_HOST env var for license server setup
	def environment(self):
		ret = QProcess.systemEnvironment()
		ret.append( "SPM_HOST=%s" % self.spmHost() )
		return ret
	
	def buildCmdArgs(self):
		args = QStringList()
		jobTypeName = self.Job.jobType().name()
				
		# Regular Render Job
		if jobTypeName == 'XSI':
			args << qs("-render") << self.burnFile()
			pas = self.Job.getValue("pass").toString()
			if not pas.isEmpty():
				args << qs("-pass") << pas
			args << qs("-frames") << self.generateFrameString()
			
			# -scanline_type ["default" | "ogl" | "rapid"]
			#  default: uses software scanline rendering.
			#  ogl: uses OGL scanline rendering (requires certified graphics card).
			#  rapid: uses rapid scanline rendering which is designed for very large scenes.

			# -scanline boolean 
			#  Activates/deactivates scanline rendering. 
			#  When raytracing is off, scanline automatically is set to on. For more information on scanline rendering, see Scanline.
			
			# -trace boolean 
			#  Activates/deactivates raytracing.
			#  If set to off, scanline will automatically be set to on
			if self.Job.renderer() in ['default','ogl','rapid']:
				args << qs('-scanline') << qs('true') << qs('-scanline_type') << self.Job.renderType()
			elif self.Job.renderer() == 'trace':
				args << qs('-trace') << qs('true')
			
			# Returns QString('true') or QString('false')
			qstf = lambda x: QString(str(bool(x))).toLower()

			#-mb boolean 
			#  Activates/deactivates motion blurring. 
			#-mb_open integer
			#  Sets tyeahhe shutter open value.
			#-mb_close integer
			#  Sets the shutter close value.
			#-mb_steps integer
			#  Sets the number of Motion blur Interpolation steps.
			mb = self.Job.getValue("motionBlur")
			if not mb.isNull():
				args << qs("-mb") << qstf(mb.toBoolean())
			
			#-deform_mb boolean 
			#  Activates/deactivates deformation motion blur for selected passes.
			dmb = self.Job.getValue("deformMotionBlur")
			if not dmb.isNull():
				args << qs("-deform_mb") << qstf(mb.toBoolean())
			
			# -resolutionX integer and -resolutionY integer 
			#  Sets the render resolution in X and Y. When only X is supplied, Y will be automatically computed from the picture standard. See -pixel_ratio.
			if self.Job.resolutionX() > 0:
				args << qs('-resolutionX') << qs.number(self.Job.resolutionX())
			if self.Job.resolutionY() > 0:
				args << qs('-resolutionY') << qs.number(self.Job.resolutionY())

		# Script Job
		elif jobTypeName == 'XSIScript':
			# Insert task number into script template
			# In this case we are running xsi.exe through the generated batch file
			# which is returned by executable(), so we just need to pass args
			# for cmd.exe to run the batch file
			if self.Job.scriptRequiresUi():
				self.UseStatusFile = True
				self.StatusFile = qs(self.burnDir() + '/' + 'scriptstatus.txt')
				self.StatusLine = 0
				if QFile.exists(self.StatusFile) and not QFile.remove(self.StatusFile):
					self.jobErrored( "XSIBurner.generateXSIBatScript: Unable to remove status file at " + self.StatusFile )
					return ""
				args << "/c" << self.generateXSIBatScript()
				print "XSIBurner: Using statusFile", self.StatusFile
			else:
				args = self.xsiScriptArgs()
			
		print "XSIBurner.buildCmdArgs: Returning", args.join(' ')
		return args
	
	def startProcess(self):
		Log( "XSIBurner::startProcess() called" )
		for window in QApplication.instance().topLevelWidgets():
			if window.isWindow() and window.isVisible():
				window.showMinimized()
		JobBurner.startProcess(self)
		if self.UseStatusFile:
			self.BatchProcessId = qprocessId( self.process() )
		self.checkupTimer().start(2 * 1000) # Every two seconds
		Log( "XSIBurner::startProcess() done" )
	
	def cleanup(self):
		Log( "XSIBurner::cleanup() called" )
		JobBurner.cleanup(self)
		if self.UseStatusFile:
			xsiProcs = processChildrenIds( self.BatchProcessId )
			for xsiProcId in xsiProcs:
				Log( "XSIBurner.cleanup: Killing batch process child, id " + str(xsiProcId) )
				killProcess(xsiProcId)
		else:
			killAll("XSIBATCH.exe");
		Log( "XSIBurner::cleaup() done" )

	def slotProcessOutputLine(self,line,channel):
		frame = QRegExp("Rendering frame:? ([0-9]+)")
		error = QRegExp("ERROR")
		fatal = QRegExp("FATAL")
		unresolvedpath = QRegExp("WARNING : 3033")
		warning = QRegExp("WARNING")
		isError = False
		errorString = ''
		
		if frame.indexIn(line) >= 0:
			if self.CurrentFrame != None:
				self.taskDone( self.CurrentFrame )
			self.CurrentFrame = frame.cap(1).toInt()[0];
			if not self.taskStart( self.CurrentFrame ):
				return
		
		if line.contains("Rendering done") or line.contains("Render completed"):
			if self.CurrentFrame is not None:
				self.taskDone( self.CurrentFrame )
				self.CurrentFrame = None
			self.jobFinished()
				
		#if line.contains( error ):
		#	isError = True
		
		if line.contains( fatal ):
			isError = True
			
		#if line.contains( unresolvedpath ):
		#	isError = True
		
		#if line.contains( warning ):
		#	isError = True
			
		if isError:
			errorString += line
		
		if isError:
			self.jobErrored(errorString)

	def checkup(self,):
		Log("XSIBurner.checkup() called");
		if not JobBurner.checkup(self):
			return False
		
		# Check for license failure dialog, this should only occur when running xsi.exe
		# for -uiscript jobs
		if processHasNamedWindow( qprocessId( self.process() ), 'License failure\s*', True ):
			self.jobErrored( 'xsi.exe was unable to get an interactive license' )
			return False
		
		if not self.UseStatusFile or not QFile.exists( self.StatusFile ):
			return False
	
		status = QFile( self.StatusFile )
		if not status.open( QIODevice.ReadOnly ):
			self.jobErrored( "XSIBurner: Couldn't open status file for reading: " + self.StatusFile )
			return False
	
		errorString = QString()
		lineNumber = 0
	
		inStream = QTextStream( status )
		
		while not inStream.atEnd():
			line = inStream.readLine();
			# Keep track of where we're at
			if lineNumber < self.StatusLine:
				lineNumber += 1
				continue
	
			lineNumber += 1
			self.StatusLine = lineNumber
			
			self.logMessage( "XSIBurner: Read new line: " + line );
	
			starting = QRegExp( "^starting (\\d+)" )
			finished = QRegExp( "^finished (\\d+)" )
			success = QRegExp( "^success" )
			if starting.indexIn(line) >= 0:
				frame = starting.cap(1).toInt()[0]
				if not self.taskStart( frame ):
					return
			elif finished.indexIn(line) >= 0:
				self.taskDone( finished.cap(1).toInt()[0] )
			elif line.contains( success ):
				self.jobFinished()
				return True
			else:
				errorString += line.replace( "\\n", "\n" )
	
		if not errorString.isEmpty():
			self.jobErrored( errorString )
			return False
			
		Log("XSIBurner.checkup() done")
		return True

class XSIBurnerPlugin(JobBurnerPlugin):
	def __init__(self):
		JobBurnerPlugin.__init__(self)

	def __del__(self):
		Log( "XSIBurnerPlugin::dtor() called, XSIBurnerPlugin destroyed" )
		
	def jobTypes(self):
		return QStringList('XSI') << 'XSIScript'

	def createBurner(self,job,slave):
		Log( "XSIBurnerPlugin::createBurner() called, Creating XSIBurner" )
		return XSIBurner(job,slave)

JobBurnerFactory.registerPlugin( XSIBurnerPlugin() )
