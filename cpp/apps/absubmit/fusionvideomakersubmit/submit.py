#!/usr/bin/env python

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.uic import *
from blur.Stone import *
from blur.Classes import *
from blur.Classesui import HostSelector
from blur.absubmit import Submitter
from blur import RedirectOutputToLog
import sys

class FusionVideoMakerDialog(QDialog):
	def __init__(self,parent=None):
		QDialog.__init__(self,parent)
		loadUi("fusionvideomakerdialogui.ui",self)
		self.connect( self.mChooseFrameSequenceButton, SIGNAL('clicked()'), self.chooseFrameSequence )
		self.connect( self.mAllHostsCheck, SIGNAL('toggled(bool)'), self.allHostsToggled )
		self.connect( self.mHostListButton, SIGNAL('clicked()'), self.showHostSelector )
		self.connect( self.mFormatCodecCombo, SIGNAL('activated(int)'), self.formatCodecChanged )
		
	 	#self.layout().setSizeConstraint(QLayout.SetFixedSize);
		self.mProjectCombo.setSpecialItemText( 'None' )
		self.mProjectCombo.setStatusFilters( ProjectStatusList(ProjectStatus.recordByName( 'Production' )) )
		self.OutputPath = None
		self.HostList = ''
		self.Version = '5.2'
		self.Platform = 'IA32'
		self.Services = []
		for format in JobFusionVideoMaker.outputFormats():
			for codec in JobFusionVideoMaker.outputCodecs(format):
				self.mFormatCodecCombo.addItem( format + ' - ' + codec )
		self.loadSettings()
		
	def loadSettings(self):
		c = userConfig()
		c.pushSection( "LastSettings" )
		project = Project.recordByName( c.readString( "Project" ) )
		if project.isRecord():
			self.mProjectCombo.setProject( project )
		self.mSequencePathEdit.setText( c.readString( "SequencePath" ) )
		self.mFrameListEdit.setText( c.readString( "FrameList" ) )
		self.mJabberErrorsCheck.setChecked( c.readBool( "JabberErrors", False ) )
		self.mJabberCompletionCheck.setChecked( c.readBool( "JabberCompletion", False ) )
		self.mEmailErrorsCheck.setChecked( c.readBool( "EmailErrors", False ) )
		self.mEmailCompletionCheck.setChecked( c.readBool( "EmailCompletion", False ) )
		self.mPrioritySpin.setValue( c.readInt( "Priority", 10 ) )
		self.mDeleteOnCompleteCheck.setChecked( c.readBool( "DeleteOnComplete", False ) )
		self.mSubmitSuspendedCheck.setChecked( c.readBool( "SubmitSuspended", False ) )
		c.popSection()
		
	def saveSettings(self):
		c = userConfig()
		c.pushSection( "LastSettings" )
		c.writeString( "Project", self.mProjectCombo.project().name() )
		c.writeString( "SequencePath", self.mSequencePathEdit.text() )
		c.writeString( "FrameList", self.mFrameListEdit.text() )
		c.writeBool( "JabberErrors", self.mJabberErrorsCheck.isChecked() )
		c.writeBool( "JabberCompletion", self.mJabberCompletionCheck.isChecked() )
		c.writeBool( "EmailErrors", self.mEmailErrorsCheck.isChecked() )
		c.writeBool( "EmailCompletion", self.mEmailCompletionCheck.isChecked() )
		c.writeInt( "Priority", self.mPrioritySpin.value() )
		c.writeBool( "DeleteOnComplete", self.mDeleteOnCompleteCheck.isChecked() )
		c.writeBool( "SubmitSuspended", self.mSubmitSuspendedCheck.isChecked() )
		c.popSection()
	
	def updateOutputPath(self,path = ''):
		format = self.mFormatCodecCombo.currentText().section(" - ",0,0)
		if not format.isEmpty():
			if path is None or not len(path):
				path = self.mOutputFileEdit.text()
			self.mOutputFileEdit.setText( JobFusionVideoMaker.updatePathToFormat( path,format ) )

	def formatCodecChanged(self):
		self.updateOutputPath()
		
	def allHostsToggled(self,allHosts):
		self.mHostListButton.setEnabled( not allHosts )
	
	def showHostSelector(self):
		hs = HostSelector(self)
		hs.setServiceFilter( ServiceList(Service.recordByName( 'Fusion5.2' )) )
		hs.setHostList( self.HostList )
		if hs.exec_() == QDialog.Accepted:
			self.HostList = hs.hostStringList()
		del hs
		
	def chooseFrameSequence(self):
		fileName = QFileDialog.getOpenFileName(self,'Choose Frame Sequence', QString(), 'Image Files (*.jpg, *.jpeg, *.tif, *.tiff, *.tga, *.targa, *.exr)' )
		if not fileName.isEmpty():
			self.mSequencePathEdit.setText(fileName)
		
	def checkFrameList(self):
		(frames, valid) = expandNumberList( self.mFrameListEdit.text() )
		previous = None
		for frame in frames:
			# Check for non-continuous frame list
			if previous is not None and previous+1 != frame:
				return False
			previous = frame
		return True
		
	def buildNotifyString(self,jabber,email):
		ret = ''
		if jabber or email:
			ret = getUserName() + ':'
			if jabber:
				ret += 'j'
			if email:
				ret += 'e'
		return ret
	
	# Returns tuple (notifyOnErrorString,notifyOnCompleteString)
	def buildNotifyStrings(self):
		return (
			self.buildNotifyString(self.mJabberErrorsCheck.isChecked(), self.mEmailErrorsCheck.isChecked() ),
			self.buildNotifyString(self.mJabberCompletionCheck.isChecked(), self.mEmailCompletionCheck.isChecked() ) )
	
	def buildAbsubmitArgs(self):
		sl = {}
		sl['jobType'] = 'FusionVideoMaker'
		sl['packetType'] = 'sequential'
		sl['priority'] = str(self.mPrioritySpin.value())
		sl['user'] = getUserName()
		sl['packetSize'] = '1'
		sl['frameList'] = str('1')
		frameList = self.mFrameListEdit.text().split('-')
		if len(frameList) == 1:
			frameList = [frameList[0],frameList[0]]
		if len(frameList) == 2:
			sl['sequenceFrameStart'] = str(frameList[0])
			sl['sequenceFrameEnd'] = str(frameList[1])
		else:
			raise "Invalid frame list"
		format, codec = self.mFormatCodecCombo.currentText().split(" - ")
		sl['format'] = format
		sl['codec'] = codec
		sl['inputFramePath'] = self.mSequencePathEdit.text()
		notifyError, notifyComplete = self.buildNotifyStrings()
		sl['notifyOnError'] = notifyError
		sl['notifyOnComplete'] = notifyComplete
		sl['job'] = self.mJobNameEdit.text()
		sl['deleteOnComplete'] = str(int(self.mDeleteOnCompleteCheck.isChecked()))
		if self.mProjectCombo.project().isRecord():
			sl['projectName'] = self.mProjectCombo.project().name()
		sl['outputPath'] = str(self.mOutputFileEdit.text())
		if not self.mAllHostsCheck.isChecked() and len(self.HostList):
			sl['hostList'] = str(self.HostList)
		sl['services'] = 'Fusion5.2'
		if self.mSubmitSuspendedCheck.isChecked():
			sl['submitSuspended'] = '1'
		Log("Applying Absubmit args: %s" % str(sl))
		return sl
	
	def accept(self):
		if self.mJobNameEdit.text().isEmpty():
			QMessageBox.critical(self, 'Missing Job Name', 'You must choose a name for this job' )
			return
		
		#if not QFile.exists( self.mSequencePathEdit.text() ):
		#	QMessageBox.critical(self, 'Invalid File', 'You must choose an existing fusion flow' )
		#	return
		
		if not self.checkFrameList():
			QMessageBox.critical(self, 'Invalid Frame List', 'Frame Lists are comma separated lists of either "XXX", or "XXX-YYY"' )
			return
		
		self.saveSettings()

		submitter = Submitter(self)
		self.connect( submitter, SIGNAL( 'submitSuccess()' ), self.submitSuccess )
		self.connect( submitter, SIGNAL( 'submitError( const QString & )' ), self.submitError )
		submitter.applyArgs( self.buildAbsubmitArgs() )
		submitter.submit()
	
	def submitSuccess(self):
		Log( 'Submission Finished Successfully' )
		QDialog.accept(self)
		
	def submitError(self,errorMsg):
		QMessageBox.critical(self, 'Submission Failed', 'Submission Failed With Error: ' + errorMsg)
		Log( 'Submission Failed With Error: ' + errorMsg )
		QDialog.reject(self)
	
if __name__ == "__main__":
	app = QApplication(sys.argv)
	initConfig("../absubmit.ini","fusionvideomakersubmit.log")
	initStone(sys.argv)
	RedirectOutputToLog()
	cp = 'h:/public/' + getUserName() + '/Blur'
	if not QDir( cp ).exists():
		cp = 'C:/Documents and Settings/' + getUserName()
	initUserConfig( cp + "/fusionvideomakersubmit.ini" );
	blurqt_loader()
	dialog = FusionVideoMakerDialog()
	Log( "Parsing args: " + ','.join(sys.argv) )
	outputPath = 'S:/'
	for i, key in enumerate(sys.argv[1::2]):
		val = sys.argv[(i+1)*2]
		if key == 'sequencePath':
			dialog.mSequencePathEdit.setText(val)
			jobName = QFileInfo(val).completeBaseName().replace(QRegExp('[/\\\\]'),'_').replace(QRegExp('^.:'),'').replace(QRegExp('((^_)|(_$))'),'')
			dialog.mJobNameEdit.setText(jobName)
			path = Path(val)
			if path.level() >= 1:
				p = Project.recordByName( path[1] )
				if p.isRecord():
					dialog.mProjectCombo.setProject( p )
					outputPath += path[1]
		elif key == 'frameList':
			dialog.mFrameListEdit.setText(val)
		elif key == 'services':
			dialog.Services += val.split(',')
		elif key == 'outputPath':
			pass #dialog.mOutputPathCombo.addItems( QString(val).split(',') )
		elif key == 'version':
			dialog.Version = val
		elif key == 'platform':
			dialog.Platform = val
	
	print "Output Path:",outputPath
	if not 'outputPath' in sys.argv:
		outputPath += '/temp_AUTODELETED/' + str(Path(dialog.mSequencePathEdit.text()).fileName())
		dialog.updateOutputPath(outputPath)
	
	ret = dialog.exec_()
	shutdown()
	sys.exit( ret )
