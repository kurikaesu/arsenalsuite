#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.uic import *
from blur.Stone import *
from blur.Classes import *
from blur.Classesui import HostSelector
from blur.absubmit import Submitter
from blur import RedirectOutputToLog
import sys
import time
import os.path
import random
import win32api

#


class RFRenderDialog(QDialog):
	def __init__(self,parent=None):
		QDialog.__init__(self,parent)
		loadUi( "realFlowrenderdialogui.ui",self)
		#self.connect( self.mAutoPacketSizeCheck, SIGNAL('toggled(bool)'), self.autoPacketSizeToggled )
		self.connect( self.mChooseFileNameButton, SIGNAL('clicked()'), self.chooseFileName )
		self.connect( self.mAllHostsCheck, SIGNAL('toggled(bool)'), self.allHostsToggled )
		self.connect( self.mHostListButton, SIGNAL('clicked()'), self.showHostSelector )
		self.layout().setSizeConstraint(QLayout.SetFixedSize);
		self.mProjectCombo.setSpecialItemText( 'None' )
		self.mProjectCombo.setStatusFilters( ProjectStatusList(ProjectStatus.recordByName( 'Production' )) )
		self.OutputPath = None
		self.HostList = ''
		self.Version = '4'
		self.Platform = 'IA32'
		self.Services = []
		self.loadSettings()
		
	def loadSettings(self):
		c = userConfig()
		c.pushSection( "LastSettings" )
		project = Project.recordByName( c.readString( "Project" ) )
		if project.isRecord():
			self.mProjectCombo.setProject( project )
		self.mFileNameEdit.setText( c.readString( "FileName" ) )
		self.mOutputNameEdit.setText( c.readString( "OutputName" ) )
		self.mStartFrameSpin.setValue( c.readInt( "StartFrame" ) )
		self.mEndFrameSpin.setValue( c.readInt( "EndFrame" ) )
		self.mJabberErrorsCheck.setChecked( c.readBool( "JabberErrors", False ) )
		self.mJabberCompletionCheck.setChecked( c.readBool( "JabberCompletion", False ) )
		self.mEmailErrorsCheck.setChecked( c.readBool( "EmailErrors", False ) )
		self.mEmailCompletionCheck.setChecked( c.readBool( "EmailCompletion", False ) )
		self.mPrioritySpin.setValue( c.readInt( "Priority", 9 ) )
		self.mDeleteOnCompleteCheck.setChecked( c.readBool( "DeleteOnComplete", False ) )
		self.mSubmitSuspendedCheck.setChecked( c.readBool( "SubmitSuspended", False ) )
		c.popSection()
		
	def saveSettings(self):
		c = userConfig()
		c.pushSection( "LastSettings" )
		c.writeString( "Project", self.mProjectCombo.project().name() )
		# c.writeBool( "AutoPacketSize", self.mAutoPacketSizeCheck.isChecked() )
		# c.writeInt( "PacketSize", self.mPacketSizeSpin.value() )
		c.writeString( "FileName", self.mFileNameEdit.text() )
		c.writeString( "OuputName", self.mOutputNameEdit.text() )
		c.writeInt( "StartFrame", self.mStartFrameSpin.value() )
		c.writeInt( "EndFrame", self.mEndFrameSpin.value() )
		# c.writeString( "PacketType", {True : "sequential", False: "random"}[self.mSequentialRadio.isChecked()] )
		c.writeBool( "JabberErrors", self.mJabberErrorsCheck.isChecked() )
		c.writeBool( "JabberCompletion", self.mJabberCompletionCheck.isChecked() )
		c.writeBool( "EmailErrors", self.mEmailErrorsCheck.isChecked() )
		c.writeBool( "EmailCompletion", self.mEmailCompletionCheck.isChecked() )
		c.writeInt( "Priority", self.mPrioritySpin.value() )
		c.writeBool( "DeleteOnComplete", self.mDeleteOnCompleteCheck.isChecked() )
		c.writeBool( "SubmitSuspended", self.mSubmitSuspendedCheck.isChecked() )
		c.popSection()
		
	def allHostsToggled(self,allHosts):
		self.mHostListButton.setEnabled( not allHosts )
	
	def showHostSelector(self):
		hs = HostSelector(self)
		hs.setServiceFilter( Service.select( "service ~ 'RealFlow'" ))
		hs.setHostList( self.HostList )
		if hs.exec_() == QDialog.Accepted:
			self.HostList = hs.hostStringList()
		del hs
		
	def chooseFileName(self):
		fileName = QFileDialog.getOpenFileName(self,'Choose File to Submit', QString(), 'RealFlow (*.flw)' )
		if not fileName.isEmpty():
			self.mFileNameEdit.setText(fileName)
		
	def checkFrameList(self):
		return self.mStartFrameSpin.value() <= self.mEndFrameSpin.text()
	
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
		
		# Basic RealFlow Command Line Syntax to Make Mesh
		# C:\Program Files (x86)\Next Limit\x64\RealFlow4 RealFlow -nogui -range 1 100 -mesh -useCache  "emitter.flw" 
		# Basic RealFlow Command Line Syntax to Simulate
		#C:\Program Files (x86)\Next Limit\x64\RealFlow4 RealFlow -nogui -range 1 100 "emitter.flw" 
		
		sl = {}
		sl['jobType'] = 'RealFlow'
		sl['noCopy'] = 'true'
		sl['priority'] = str(self.mPrioritySpin.value())
		sl['user'] = getUserName()
		startFrame = self.mStartFrameSpin.value()
		endFrame = self.mEndFrameSpin.value()
		sl['packetSize'] = str(endFrame - startFrame + 1)
		sl['packetType'] = 'continuous'
		
		############## New Flags Just for Real Flow #####################
		# 
		sl['frameStart'] = str(startFrame)
		sl['frameEnd'] = str(endFrame)
		sl['fileName'] = self.mFileNameEdit.text()
		sl['threads'] = str(self.mThreadCountSpin.value())
		
		# if the Mesh box is checke we need to add the -mesh flag to the command
		SimOutputType = None
		if self.mSim.isChecked():
			SimOutputType = "Sim"
		if self.mMesh.isChecked():
			SimOutputType = "Mesh"
		
		sl['SimType'] = SimOutputType
				
		# Creates Output Folder 
		simOutput = "S:\\" + self.mProjectCombo.project().name() +"\\FX\\Cache_RealFlow\\" + self.mOutputNameEdit.text() + "\\"	+ SimOutputType + "\\"
		sl['outputPath'] = simOutput
		############### End New SL flags #####################
		
		notifyError, notifyComplete = self.buildNotifyStrings()
		sl['notifyOnError'] = notifyError
		sl['notifyOnComplete'] = notifyComplete
		sl['job'] = self.mJobNameEdit.text()
		sl['deleteOnComplete'] = str(int(self.mDeleteOnCompleteCheck.isChecked()))
		if self.mProjectCombo.project().isRecord():
			sl['projectName'] = self.mProjectCombo.project().name()
		if not self.mAllHostsCheck.isChecked() and len(self.HostList):
			sl['hostList'] = str(self.HostList)
		if self.Version:
			service = 'RealFlow' + self.Version
			if not Service.recordByName( service ).isRecord():
				QMessageBox.critical(self, 'Service %s not found' % service, 'No service found for %s, please contact IT' % service )
				raise ("Invalid RealFlow Version %s" % service)
			self.Services.append( service )
		if len(self.Services):
			sl['services'] = ','.join(self.Services)
		if self.mSubmitSuspendedCheck.isChecked():
			sl['submitSuspended'] = '1'
		Log("Applying Absubmit args: %s" % str(sl))
		return sl
	
	def accept(self):
		
		if self.mJobNameEdit.text().isEmpty():
			QMessageBox.critical(self, 'Missing Job Name', 'You must choose a name for this job' )
			return
		
		if not QFile.exists( self.mFileNameEdit.text() ):
			QMessageBox.critical(self, 'Invalid File', 'You must choose an existing RealFlow File' )
			return
		
		if not self.checkFrameList():
			QMessageBox.critical(self, 'Invalid Start and End Frame', 'End Frame must be greater then Start Frame' )
			return
		
		self.saveSettings()

		if self.mDeleteFramesBeforeSubmitCheck.isChecked():
			tFileName = str(self.mOutputPathCombo.currentText())
			tFileDelete = os.path.dirname(tFileName) + "/*" + os.path.splitext(os.path.basename(tFileName))[1]
			self.__specialDeleteMsg(tFileDelete)
			time.sleep(60)
			
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

#================================================================================================

	def __sendMessage ( self, msgPath , msg ):
		# we use a random number and username to create an unique file
		msgFileNamePath = msgPath # TBR: hard coded path!! what happens with offsite people!
		msgFileNameFile = win32api.GetUserName() + str( random.randint(0, 65535) )
		msgFileNameExtTemp = ".tmp"
		msgFileNameExt = ".msg"
		msgFile = file (msgFileNamePath + msgFileNameFile + msgFileNameExtTemp, "w") 

		if msgFile:
			msgFile.write ( msg + "\n" )
			msgFile.close()
			os.rename( (msgFileNamePath + msgFileNameFile + msgFileNameExtTemp) , (msgFileNamePath + msgFileNameFile + msgFileNameExt) )
			return True
		
		return False

	def __specialDeleteMsg ( self, inPathToDelete ):
		msgPath = '//thor/spool/new/'
		#pathToDelete = blurFile.ConvertToUNC ( inPathToDelete )
		pathToDelete = inPathToDelete.replace("G:", "//thor/animation").replace("Q:", "//cougar/compOutput").replace("S:", "//cheetah/renderOutput").replace("U:", "//goat/renderOutput")
		msg = "     {                   \n\
				action => rm,		\n\
				data => 						\n\
				{								\n\
					dir =>  '%s',		    	\n\
					verbose => 1				\n\
				}, 								\n\
				info => { user => '%s' }	   	\n\
			}" 
		msg = msg % ( pathToDelete , win32api.GetUserName() )
		return self.__sendMessage ( msgPath , msg )
	
if __name__ == "__main__":
	os.chdir('c:\\blur\\absubmit\\realflow\\')
	app = QApplication(sys.argv)
	initConfig("../absubmit.ini", "rfsubmit.log")
	RedirectOutputToLog()
	cp = 'h:/public/' + getUserName() + '/Blur'
	if not QDir( cp ).exists():
		cp = 'C:/Documents and Settings/' + getUserName()
	initUserConfig( cp + "/RFsubmit.ini" );
	blurqt_loader()
	dialog = RFRenderDialog()
	Log( "Parsing args: " + ','.join(sys.argv) )
	
	for i, key in enumerate(sys.argv):
		if key == 'fileName':
			val = sys.argv[(i+1)]
			dialog.mFileNameEdit.setText(val)
			dialog.mJobNameEdit.setText(QFileInfo(val).completeBaseName())
			dialog.mOutputNameEdit.setText(QFileInfo(val).completeBaseName())
			path = Path(val)
			if path.level() >= 1:
				p = Project.recordByName( path[1] )
				if p.isRecord():
					dialog.mProjectCombo.setProject( p )
		elif key == 'mStartFrame':
			val = sys.argv[(i+1)]
			dialog.mStartFrameSpin.setValue(int(val))
		elif key == 'mEndFrame':
			val = sys.argv[(i+1)]
			dialog.mEndFrameSpin.setValue(int(val))
		elif key == 'mThreads':
			val = sys.argv[(i+1)]
			dialog.mThreadCountSpin.setValue(int(val))
		elif key == 'version':
			val = sys.argv[(i+1)]
			dialog.Version = val
	ret = dialog.exec_()
	shutdown()
	sys.exit( ret )
