from blur.Stone import *
from blur.Classes import *
from blur.Burner import *
from PyQt4.QtCore import *
from PyQt4.QtSql import *
import re, traceback

class FusionBurner(JobBurner):
	def __init__(self,job,slave):
		JobBurner.__init__(self,job,slave)
		self.Job = job
		self.Slave = slave
		self.CurrentFrame = None
		self.RenderStarted = False
		self.LastRenderedFrame = None
		self.FusionDir = None
		self.OutputsReported = 0

	def __del__(self):
		# Nothing is required
		# self.cleanup() is explicitly called by the slave
		pass
	
	# Names of processes to kill after burn is finished
	def processNames(self):
		return QStringList() << "fusion.exe"
	
	def getFusionDir(self):
		if self.FusionDir is None:
			
			fusionServices = self.Job.jobServices().services().filter('service',QRegExp('^Fusion'))
			if len(fusionServices) == 0:
				raise "Unable to find fusion service for job to determine render executable"
			if len(fusionServices) > 1:
				raise "Multiple fusion services listed, unable to determine which fusion installation to use"
			
			fusionService = fusionServices[0]
			
			configKey = 'assburner%sRenderDir' % fusionService.service()
			self.FusionDir = Config.getString( configKey, "" )
			
			if self.FusionDir.isEmpty():
				raise ("Empty or Invalid Fusion Path listed under config key %s" % configKey)
		
		return self.FusionDir
	
	def environment(self):
		# Set the license server environment variable if it exists in the database
		fusionLicenseServer = Config.getString('fusionLicenseServer')
		if not fusionLicenseServer.isEmpty():
			# Remove existing license server env var, if it exists
			envs = QProcess.systemEnvironment().filter( '^(?!EYEON_LICENSE_FILE=)' )
			# Add new one
			envs << ('EYEON_LICENSE_FILE=@' + fusionLicenseServer)
			return envs
		return QStringList()
	
	def executable(self):
		try:
			return self.getFusionDir() + "ConsoleSlave.exe"
		except:
			traceback.print_exc()
			self.jobErrored( "Unknown exception getting fusion path: " + traceback.format_exc() )
		
	def buildCmdArgs(self):
		args = QStringList()
		args << self.burnFile()
		args << "/status"
		if not self.Job.allFramesAsSingleTask():
			args << "/frames" << self.assignedTasks().replace("-","..")
		return args
	
	def startProcess(self):
		Log( "FusionBurner::startBurn() called" )
		JobBurner.startProcess(self)
		self.BurnProcess = self.process()
		self.Started = QDateTime.currentDateTime()
		self.checkupTimer().start(2 * 60 * 1000) # Every two minutes
		Log( "FusionBurner::startBurn() done" )
	
	def cleanup(self):
		Log( "FusionBurner::cleanup() called" )
		JobBurner.cleanup(self)
		killAll("ConsoleSlave.exe");
		Log( "FusionBurner::cleaup() done" )
	
	def slotProcessOutputLine(self,line,channel):
		Log( "FusionBurner::slotReadOutput() called, ready to read output" )
		error = False
		
		# Job started
		if re.match( 'Render started', line ):
			self.RenderStarted = True
			if self.Job.allFramesAsSingleTask():
				self.taskStart(1)
		
		# Frames Rendered
		mo = re.match( r'^Rendered frame (\d+) \(\d+\sof\s\d+\), took ([\d\.]+)', line )
		if mo and not self.Job.allFramesAsSingleTask():
			frame = int(mo.group(1))
			taskTime = int(round(float(mo.group(2))))
			
			self.OutputsReported += 1
			
			# Got new frame when we were expecting more done messages from current
			if self.CurrentFrame is not None and frame != self.CurrentFrame:
				self.taskDone(self.CurrentFrame)
				self.LastRenderedFrame = self.CurrentFrame
				self.CurrentFrame = None
				self.OutputsReported = 1
				
			# New frame is started, got first done message
			if self.CurrentFrame is None and frame != self.LastRenderedFrame:
				if not self.taskStart(frame, QString(), taskTime):
					return
				self.CurrentFrame = frame
			
			# Got unexpected done message from previous frame
			elif frame == self.LastRenderedFrame:
				# Add time to the task
				Database.current().exec_( "UPDATE JobTask SET endedts=endedts+'%f seconds'::interval, ended=started + extract(epoch from (endedts-startedts+'%f seconds'::interval)) WHERE fkeyjob=%i AND jobtask=%i" % (taskTime, taskTime, self.Job.key(), frame) )
				self.OutputsReported = 0
			
			# Got last expected done message
			if frame == self.CurrentFrame and self.OutputsReported >= self.Job.outputCount():
				self.taskDone(self.CurrentFrame)
				self.LastRenderedFrame = self.CurrentFrame
				self.CurrentFrame = None
				self.OutputsReported = 0
			
			return
		
		# Job completion
		if re.match( '^Render completed', line ):
			if self.CurrentFrame is not None:
				if not self.taskDone(self.CurrentFrame):
					return
			if self.Job.allFramesAsSingleTask():
				self.taskDone(1)
			self.jobFinished()
			return
		
		# Errors
		# Not sure if we should check this one,
		# seems failed to load catches it, and gets
		# more info.
		#if re.search( 'cannot get .+ at time', line ):
		#	error = True
		if re.search( 'failed to (load|write)', line ):
			error = True
		if re.search( 'failed at time', line ):
			error = True
		if re.search( 'Render failed at', line ):
			error = True
		if re.search( 'License Error:', line ):
			error = True
		
		# Hmm, gives this if invalid file name, i wonder about other errors...
		# Also gives this during normal render, but gives the render started
		# output first
		if re.match( '^Listening for commands...', line ):
			if not self.RenderStarted:
				self.jobErrored( "Unknown Error During Fusion Rendering\n" )
				return
		
		if error:
			self.jobErrored( line )

from blur import blurFusion

avi_codec_dict = {
	'Cinepack' : 0,
	'Indeo Video R3.2' : 1,
#	'Indeo Video R3.2' : 2, # Fusion has these two listed the same...
	'Indeo Video R4.5' : 3,
	'Indeo Video R5.10' : 4,
	'Intel IYUV' : 5,
	'Microsoft Video 1' : 6,
	'DivX 6.0' : 7,
	'Techsmith Screen Capture' : 8,
	'Uncompressed' : 9
	}

qt_codec_dict = {
	'MPEG-4 Video' : 'MPEG-4 Video_mp4v',
	'Sorenson Video' : 'Sorenson Video 3_SVQ1',
	'Sorenson Video 3' : 'Sorenson Video 3_SVQ3',
	'Cinepack' : ''
	}

class FusionVideoMakerBurner( FusionBurner ):
	def __init__(self,job,slave):
		FusionBurner.__init__(self,job,slave)
		
	# Returns a frame path with 0000 frame, ex. 'c:/temp/test_jpg0120.jpg' -> 'c:/temp/test_jpg0000.jpg'
	def zeroFramePath(self,framePath):
		inputPathBase = framePathBaseName( framePath )[0] # returns a_frame_sequence_.jpeg
		return Path(framePath).dirPath() + '/' + makeFramePath( inputPathBase, 0 ) # G:/temp/a_frame_sequence_0000.jpeg
	
	def burnFile(self):
		return self.BurnFile
	
	def startProcess(self):
		compTemplatePath = "C:/blur/assburner/plugins/fusion_video_maker_template.comp"
		self.BurnFile = self.burnDir() + '/fusion_video_maker.comp'
		frameStart = self.Job.sequenceFrameStart()
		frameEnd = self.Job.sequenceFrameEnd()
		
		# Parse Comp Template
		comp = blurFusion.parseEyeonFile(compTemplatePath)
		
		# Setup global and render range
		comp.GlobalRange = blurFusion.FusionTable(Array=[frameStart,frameEnd])
		comp.RenderRange = blurFusion.FusionTable(Array=[frameStart,frameEnd])
		comp.CurrentTime = frameStart
		
		# Get input and output paths in fusion format
		zeroPath = self.zeroFramePath(self.Job.inputFramePath()) # Should be something like G:/temp/a_frame_sequence_0111.jpeg
		outputPath = self.Job.outputPath()
		comp.OutputClips = blurFusion.FusionTable(Array=[str(outputPath).replace('\\','/')])
		
		# Setup Input Frames
		clip = comp.Tools.Loader1.Clips[0]
		clip.Filename = str(zeroPath).replace('/','\\')
		clip.FormatID = blurFusion.getFusionFormat(QFileInfo(zeroPath).suffix())
		clip.TrimIn = 0
		clip.TrimOut = frameEnd - frameStart
		clip.GlobalStart = clip.StartFrame = frameStart
		clip.GlobalEnd = frameEnd
		clip.Length = frameEnd - frameStart + 1
		
		# Setup Video Output
		comp.Tools.Saver1.Inputs.Clip.Value.Filename = str(outputPath).replace( "\\", "/" )
		
		if outputPath.toLower().endsWith( 'avi' ):
			codec = 7 # DivX as default
			if str(self.Job.codec()) in avi_codec_dict:
				codec = avi_codec_dict[str(self.Job.codec())]
			comp.Tools.Saver1.Inputs["AVIFormat.Compression"].Value = codec
		elif outputPath.toLower().endsWith( 'mov' ):
			comp.Tools.Saver1.Inputs.Clip.Value.FormatID = "QuickTimeMovies"
			comp.Tools.Saver1.Inputs.OutputFormat.Value[0] = "QuickTimeMovies"
			# TODO - Support Multiple codecs
			codec = "Sorenson Video 3"
			if self.Job.codec() in qt_codec_dict:
				codec = self.Job.codec()
			if codec == 'Cinepack':
				del comp.Tools.Saver1.Inputs["QuickTimeMovies.Compression"]
			else:
				comp.Tools.Saver1.Inputs["QuickTimeMovies.Compression"].Value[0] = qt_codec_dict[codec]
		
		if QFile( self.BurnFile ).exists():
			QFile.remove( self.BurnFile )
		
		blurFusion.writeEyeonFile( comp, self.BurnFile )
		
		FusionBurner.startProcess(self)
	
	def cleanup(self):
		#QFile.remove( self.BurnFile )
		FusionBurner.cleanup(self)

class FusionBurnerPlugin(JobBurnerPlugin):
	def __init__(self):
		JobBurnerPlugin.__init__(self)

	def jobTypes(self):
		return QStringList('Fusion') << 'FusionVideoMaker'

	def createBurner(self,job,slave):
		Log( "FusionBurnerPlugin::createBurner() called, Creating FusionBurner" )
		if job.jobType().name() == 'Fusion':
			return FusionBurner(job,slave)
		elif job.jobType().name() == 'FusionVideoMaker':
			return FusionVideoMakerBurner(job,slave)

JobBurnerFactory.registerPlugin( FusionBurnerPlugin() )
