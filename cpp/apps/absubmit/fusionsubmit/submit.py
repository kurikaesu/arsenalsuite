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
if sys.platform == 'win32':
	import win32api

class FusionRenderDialog(QDialog):
	def __init__(self,parent=None):
		QDialog.__init__(self,parent)
		loadUi("fusionrenderdialogui.ui",self)
		self.connect( self.mAutoPacketSizeCheck, SIGNAL('toggled(bool)'), self.autoPacketSizeToggled )
		self.connect( self.mChooseFileNameButton, SIGNAL('clicked()'), self.chooseFileName )
		self.connect( self.mAllHostsCheck, SIGNAL('toggled(bool)'), self.allHostsToggled )
		self.connect( self.mHostListButton, SIGNAL('clicked()'), self.showHostSelector )
		self.layout().setSizeConstraint(QLayout.SetFixedSize);
		self.mProjectCombo.setSpecialItemText( 'None' )
		self.mProjectCombo.setStatusFilters( ProjectStatusList(ProjectStatus.recordByName( 'Production' )) )
		self.OutputPath = None
		self.HostList = ''
		self.Version = '5.2'
		self.Platform = 'IA32'
		self.Services = []
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
		self.mFileNameEdit.setText( c.readString( "FileName" ) )
		self.mFrameListEdit.setText( c.readString( "FrameList" ) )
		self.mSequentialRadio.setChecked( c.readString( "PacketType", "random" ) == "sequential" )
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
		c.writeBool( "AutoPacketSize", self.mAutoPacketSizeCheck.isChecked() )
		c.writeInt( "PacketSize", self.mPacketSizeSpin.value() )
		c.writeString( "FileName", self.mFileNameEdit.text() )
		c.writeString( "FrameList", self.mFrameListEdit.text() )
		c.writeString( "PacketType", {True : "sequential", False: "random"}[self.mSequentialRadio.isChecked()] )
		c.writeBool( "JabberErrors", self.mJabberErrorsCheck.isChecked() )
		c.writeBool( "JabberCompletion", self.mJabberCompletionCheck.isChecked() )
		c.writeBool( "EmailErrors", self.mEmailErrorsCheck.isChecked() )
		c.writeBool( "EmailCompletion", self.mEmailCompletionCheck.isChecked() )
		c.writeInt( "Priority", self.mPrioritySpin.value() )
		c.writeBool( "DeleteOnComplete", self.mDeleteOnCompleteCheck.isChecked() )
		c.writeBool( "SubmitSuspended", self.mSubmitSuspendedCheck.isChecked() )
		c.popSection()
		
	def autoPacketSizeToggled(self,autoPacketSize):
		self.mPacketSizeSpin.setEnabled(not autoPacketSize)
	
	def allHostsToggled(self,allHosts):
		self.mHostListButton.setEnabled( not allHosts )
	
	def showHostSelector(self):
		hs = HostSelector(self)
		hs.setServiceFilter( ServiceList(Service.recordByName( 'Fusion5.2' )) )
		hs.setHostList( self.HostList )
		if hs.exec_() == QDialog.Accepted:
			self.HostList = hs.hostStringList()
		del hs
		
	def chooseFileName(self):
		fileName = QFileDialog.getOpenFileName(self,'Choose Flow To Render', QString(), 'Fusion Flows (*.comp)' )
		if not fileName.isEmpty():
			self.mFileNameEdit.setText(fileName)
		
	def checkFrameList(self):
		(frames, valid) = expandNumberList( self.mFrameListEdit.text() )
		return valid
	
	def packetTypeString(self):
		if self.mSequentialRadio.isChecked():
			return 'sequential'
		return 'random'
	
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
	
	def buildAbsubmitArgs(self):
		sl = {}
		sl['jobType'] = 'Fusion'
		sl['packetType'] = self.packetTypeString()
		sl['priority'] = str(self.mPrioritySpin.value())
		sl['user'] = getUserName()
		sl['packetSize'] = str(self.packetSize())
		if self.mAllFramesAsSingleTaskCheck.isChecked():
			sl['allframesassingletask'] = 'true'
			sl['frameList'] = str('1')
		else:
			sl['frameList'] = self.mFrameListEdit.text()
		sl['fileName'] = self.mFileNameEdit.text()
		notifyError, notifyComplete = self.buildNotifyStrings()
		sl['notifyOnError'] = notifyError
		sl['notifyOnComplete'] = notifyComplete
		sl['job'] = self.mJobNameEdit.text()
		sl['deleteOnComplete'] = str(int(self.mDeleteOnCompleteCheck.isChecked()))
		if self.mProjectCombo.project().isRecord():
			sl['projectName'] = self.mProjectCombo.project().name()
		if not self.mOutputPathCombo.currentText().isEmpty():
			sl['outputPath'] = str(self.mOutputPathCombo.currentText())
		if not self.mAllHostsCheck.isChecked() and len(self.HostList):
			sl['hostList'] = str(self.HostList)
		sl['outputCount'] = str(self.mOutputPathCombo.count())
		if self.Version:
			# Cut off to Major.Minor version, ex. 5.21 -> 5.2
			self.Version = '%.01f' % float(self.Version)
			service = 'Fusion' + self.Version
			if self.Platform == 'IA64' or self.Platform == 'X64':
				service += 'x64'
			dialog.Services.append( service )
			if not Service.recordByName( service ).isRecord():
				QMessageBox.critical(self, 'Service %s not found' % service, 'No service found for %s, please contact IT' % service )
				raise ("Invalid Fusion Version %s" % service)
		
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
			QMessageBox.critical(self, 'Invalid File', 'You must choose an existing fusion flow' )
			return
		
		if not self.checkFrameList():
			QMessageBox.critical(self, 'Invalid Frame List', 'Frame Lists are comma separated lists of either "XXX", or "XXX-YYY"' )
			return
		
		self.saveSettings()

		if self.mDeleteFramesBeforeSubmitCheck.isChecked():
##			tFileName = str(self.mOutputPathCombo.currentText())
##			tFileDelete = os.path.dirname(tFileName) + "/*" + os.path.splitext(os.path.basename(tFileName))[1]
##			self.__specialDeleteMsg(tFileDelete)
##			time.sleep(60)
			for loop in (range(self.mOutputPathCombo.count())):
				tFileName = str(self.mOutputPathCombo.itemText(loop))
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
		msgFileNameFile = getUserName() + str( random.randint(0, 65535) )
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
		#----------------------------------------------------------------------------
		#	Prototype:
		#				function syncPCDirectory ( inPathToDelete ):
		#
		#	Remarks:
		#				syncs pc directories
		#	Parameters:
		#				<string> inPathToDelete 
		#	Returns:
		#				<bool> true if the message to IT could be sent, false otherwise
		#----------------------------------------------------------------------------
		
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
		localName = "fusion-job"
		if sys.platform == 'win32':
			localName = win32api.GetUserName()
		msg = msg % ( pathToDelete , localName)
		return self.__sendMessage ( msgPath , msg )
	
if __name__ == "__main__":
	app = QApplication(sys.argv)
	initConfig("../absubmit.ini","fusionsubmit.log")
	RedirectOutputToLog()
	cp = 'h:/public/' + getUserName() + '/Blur'
	if not QDir( cp ).exists():
		cp = 'C:/Documents and Settings/' + getUserName()
	initUserConfig( cp + "/fusionsubmit.ini" );
	blurqt_loader()
	dialog = FusionRenderDialog()
	Log( "Parsing args: " + ','.join(sys.argv) )
	for i, key in enumerate(sys.argv[1::2]):
		val = sys.argv[(i+1)*2]
		if key == 'fileName':
			dialog.mFileNameEdit.setText(val)
			dialog.mJobNameEdit.setText(QFileInfo(val).completeBaseName())
			path = Path(val)
			if path.level() >= 1:
				p = Project.recordByName( path[1] )
				if p.isRecord():
					dialog.mProjectCombo.setProject( p )
		elif key == 'frameList':
			dialog.mFrameListEdit.setText(val)
		elif key == 'services':
			dialog.Services += val.split(',')
		elif key == 'outputs':
			dialog.mOutputPathCombo.addItems( QString(val).split(',') )
		elif key == 'version':
			dialog.Version = val
		elif key == 'platform':
			dialog.Platform = val
	ret = dialog.exec_()
	shutdown()
	sys.exit( ret )
