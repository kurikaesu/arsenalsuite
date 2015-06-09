#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.uic import *
from blur.Stone import *
from blur.Classes import *
from blur.absubmit import Submitter
from blur.Classesui import *
import sys, os

class Cinema4DRenderDialog(QDialog):
	def __init__(self,parent=None):
		QDialog.__init__(self,parent)
		loadUi("cinema4drenderdialogui.ui",self)
		self.connect( self.mAutoPacketSizeCheck, SIGNAL('toggled(bool)'), self.autoPacketSizeToggled )
		self.connect( self.mChooseFileNameButton, SIGNAL('clicked()'), self.chooseFileName )
		self.connect( self.mFrameStartSpin, SIGNAL('valueChanged(int)'), self.frameStartChanged )
		self.connect( self.mFrameEndSpin, SIGNAL('valueChanged(int)'), self.frameEndChanged )
		self.connect( self.mAllHostsCheck, SIGNAL('toggled(bool)'), self.allHostsToggled )
		self.connect( self.mHostListButton, SIGNAL('clicked()'), self.showHostSelector )
		self.layout().setSizeConstraint(QLayout.SetFixedSize);
		self.mProjectCombo.setSpecialItemText( 'None' )
		self.mProjectCombo.setStatusFilters( ProjectStatusList(ProjectStatus.recordByName( 'Production' )) )
		self.Cinema4DServiceDict = { 'Cinema 4D R10' : 'cinema4d', 'Cinema 4D R11' : 'cinema4dr11' }
		self.mVersionCombo.addItems( self.Cinema4DServiceDict.keys() )
		self.Format = None
		self.OutputPath = None
		self.MultipassFormat = None
		self.MultipassOutputPath = None
		self.PassNames = []
		self.HostList = ''
		self.loadSettings()
	
	def loadSettings(self):
		c = userConfig()
		c.pushSection( "LastSettings" )
		project = Project.recordByName( c.readString( "Project" ) )
		if project.isRecord():
			self.mProjectCombo.setProject( project )
		aps = c.readBool( "AutoPacketSize", True )
		self.mAutoPacketSizeCheck.setChecked( aps )
		if not aps:
			self.mPacketSizeSpin.setValue( c.readInt( "PacketSize", 10 ) )
		self.mJobNameEdit.setText( c.readString( "JobName" ) )
		self.mFileNameEdit.setText( c.readString( "FileName" ) )
		self.mJabberErrorsCheck.setChecked( c.readBool( "JabberErrors", False ) )
		self.mJabberCompletionCheck.setChecked( c.readBool( "JabberCompletion", False ) )
		self.mEmailErrorsCheck.setChecked( c.readBool( "EmailErrors", False ) )
		self.mEmailCompletionCheck.setChecked( c.readBool( "EmailCompletion", False ) )
		self.mPrioritySpin.setValue( c.readInt( "Priority", 50 ) )
		self.mDeleteOnCompleteCheck.setChecked( c.readBool( "DeleteOnComplete", False ) )
		c.popSection()
		
	def saveSettings(self):
		c = userConfig()
		c.pushSection( "LastSettings" )
		c.writeString( "Project", self.mProjectCombo.project().name() )
		c.writeBool( "AutoPacketSize", self.mAutoPacketSizeCheck.isChecked() )
		c.writeInt( "PacketSize", self.mPacketSizeSpin.value() )
		c.writeString( "JobName", self.mJobNameEdit.text() )
		c.writeString( "FileName", self.mFileNameEdit.text() )
		c.writeBool( "JabberErrors", self.mJabberErrorsCheck.isChecked() )
		c.writeBool( "JabberCompletion", self.mJabberCompletionCheck.isChecked() )
		c.writeBool( "EmailErrors", self.mEmailErrorsCheck.isChecked() )
		c.writeBool( "EmailCompletion", self.mEmailCompletionCheck.isChecked() )
		c.writeInt( "Priority", self.mPrioritySpin.value() )
		c.writeBool( "DeleteOnComplete", self.mDeleteOnCompleteCheck.isChecked() )
		c.popSection()
	
	def allHostsToggled(self,allHosts):
		self.mHostListButton.setEnabled( not allHosts )
	
	def showHostSelector(self):
		hs = HostSelector(self)
		hs.setServiceFilter( ServiceList(Service.recordByName('cinema4d')) )
		hs.setHostList( self.HostList )
		if hs.exec_() == QDialog.Accepted:
			self.HostList = hs.hostStringList()
		del hs

	def frameStartChanged( self, val ):
		self.mFrameEndSpin.setMinimum( val )
	
	def frameEndChanged( self, val ):
		self.mFrameStartSpin.setMaximum( val )
	
	def autoPacketSizeToggled(self,autoPacketSize):
		self.mPacketSizeSpin.setEnabled(not autoPacketSize)
	
	def chooseFileName(self):
		fileName = QFileDialog.getOpenFileName(self,'Choose Scene To Render', QString(), 'Cinema4D scenes (*.c4d)' )
		if not fileName.isEmpty():
			self.mFileNameEdit.setText(fileName)
		
	def packetSize(self):
		if self.mAutoPacketSizeCheck.isChecked():
			return 0
		return self.mPacketSizeSpin.value()
	
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
	
	def c4difyPath(self,outputPath,format):
		opfi = QFileInfo(outputPath)
		ret = opfi.path() + "/" + opfi.completeBaseName()
		if ret.size() and ret.right(1).at(0).isDigit():
			ret += "_"
		ret += "." + format
		return ret
	
	def buildAbsubmitArgList(self):
		sl = {}
		notifyError, notifyComplete = self.buildNotifyStrings()
		
		sl['jobType'] 			= 	'Cinema4D'
		sl['packetType'] 		=	'continuous'
		sl['priority'] 			=	str(self.mPrioritySpin.value())
		sl['user'] 				=	str(getUserName())
		sl['packetSize'] 		=	str(self.packetSize())
		sl['frameList'] 		=	(str(self.mFrameStartSpin.value()) + '-' + str(self.mFrameEndSpin.value()))
		sl['fileName'] 			=	str(self.mFileNameEdit.text())
		sl['notifyOnError']		=	notifyError
		sl['notifyOnComplete']	=	notifyComplete
		sl['job'] 				=	self.mJobNameEdit.text()
		sl['deleteOnComplete'] 	= 	str(int(self.mDeleteOnCompleteCheck.isChecked()))
		sl['services'] 			= 	self.Cinema4DServiceDict[str(self.mVersionCombo.currentText())]
		
		if self.mProjectCombo.project().isRecord():
			sl['projectName'] = self.mProjectCombo.project().name()
		
		passNum = 0
		if self.MultipassOutputPath and self.MultipassFormat and len(self.PassNames):
			opfi = QFileInfo(self.MultipassOutputPath)
			ext = opfi.suffix()
			for passName in self.PassNames:
				sl[('outputPath%i' % passNum)] = (opfi.filePath() + '_' + passName + '.' + self.MultipassFormat)
				sl[('outputName%i' % passNum)] = passName
				passNum += 1
		
		if self.OutputPath and self.Format:
			if passNum > 0:
				sl[('outputName%i' % passNum)] = 'Regular Output'
				sl[('outputPath%i' % passNum)] = self.c4difyPath(self.OutputPath,self.Format)
			else:
				sl['outputPath'] = self.c4difyPath(self.OutputPath,self.Format)
			
		if not self.mAllHostsCheck.isChecked() and len(self.HostList):
			sl['hostList'] = str(self.HostList)

		return sl
	
	def accept(self):
		if self.mJobNameEdit.text().isEmpty():
			QMessageBox.critical(self, 'Missing Job Name', 'You must choose a name for this job' )
			return
		
		if not QFile.exists( self.mFileNameEdit.text() ):
			QMessageBox.critical(self, 'Invalid File', 'You must choose an existing cinema4d file' )
			return
		
		self.saveSettings()
		
		submitter = Submitter(self)
		self.connect( submitter, SIGNAL( 'submitSuccess()' ), self.submitSuccess )
		self.connect( submitter, SIGNAL( 'submitError( const QString & )' ), self.submitError )
		submitter.applyArgs( self.buildAbsubmitArgList() )
		submitter.submit()
		
	def submitSuccess(self):
		Log( 'Submission Finished Successfully' )
		QDialog.accept(self)
		
	def submitError(self,errorMsg):
		QMessageBox.critical(self, 'Submission Failed', 'Submission Failed With Error: ' + errorMsg)
		Log( 'Submission Failed With Error: ' + errorMsg )
		QDialog.reject(self)
	
if __name__ == "__main__":
	if sys.platform == 'win32':
		os.chdir("c:\\blur\\absubmit\\cinema4d\\")
	app = QApplication(sys.argv)
	initConfig("../absubmit.ini","cinema4dsubmit.log")
	if sys.platform == 'win32':
		cp = "h:/public/" + getUserName() + "/Blur";
		if not QDir( cp ).exists():
			cp = "C:/Documents and Settings/" + getUserName();
		initUserConfig( cp + "/c4dsubmit.ini" );
	else:
		initUserConfig( QDir.homePath() + "/.c4dsubmit" );

	initStone(sys.argv)
	blurqt_loader()
	dialog = Cinema4DRenderDialog()
	opts = []
	try:
		opts = open("current_options.txt").read().split('\n')
	except:
		pass
	
	Log( "Passed options: \n" + '\n'.join(opts) )
	
	# Filename
	if len(opts) >= 1:
		dialog.mFileNameEdit.setText(opts[0])
		dialog.mJobNameEdit.setText(QFileInfo(opts[0]).completeBaseName())
		path = Path(opts[1])
		if path.level() >= 1:
			p = Project.recordByName( path[1] )
			if p.isRecord():
				dialog.mProjectCombo.setProject( p )
				
	# Framestart/FrameEnd
	if len(opts) >= 3:
		dialog.mFrameStartSpin.setValue(int(float(opts[1])))
		dialog.mFrameEndSpin.setValue(int(float(opts[2])))
	
	# Format
	if len(opts) >= 4:
		if opts[3] != 'NONE':
			dialog.Format = opts[3]
			Log( "Format: " + dialog.Format )
	
	# Output Path
	if len(opts) >= 5:
		if opts[4] != 'NONE':
			dialog.OutputPath = opts[4]
			Log( "Output Path:" + dialog.OutputPath )
	
	# Multi-pass format
	if len(opts) >= 6:
		dialog.MultipassFormat = opts[5]
		Log( "Multipass Format: " + dialog.MultipassFormat )
	
	# Multi-pass filename
	if len(opts) >= 7 and len(opts[6]):
		dialog.MultipassOutputPath = opts[6]
		Log( "Multipass output path: " + dialog.MultipassOutputPath )
	
	# If multi-pass, comma separated list of pass names
	if len(opts) >= 8 and len(opts[7]):
		for pn in opts[7].split(','):
			if len(pn):
				dialog.PassNames.append(pn)
		Log( "Pass Names: " + ','.join(dialog.PassNames) )
	
	# If no outputs are set, give a message box and quite
	if not dialog.OutputPath and not dialog.MultipassOutputPath:
		QMessageBox.critical( None, 'No outputs defined', 'Cannot submit render because no render output files were defined.' )
	
	else:
		#if len(sys.argv) > 3:
		#	dialog.mOutputPathCombo.addItems( sys.argv[3:] )
		dialog.show()
		app.exec_()
		shutdown()
		
