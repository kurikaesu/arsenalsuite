#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.uic import *
from blur.Stone import *
from blur.Classes import *
from blur.Classesui import *
from blur.absubmit import Submitter
import pumpThread as pt
import sys
import os
import time
import maya.cmds as cmds
import maya.mel

dialog = None

def imageNameParts():
  # 0 baseName
  # 1 version
  # 2 extension

	imageNames = cmds.renderSettings(firstImageName=True)
	parts = imageNames[0].split(".")
	ret = [None,None,None]
	ret[0] = parts[0]

	ret[1] = maya.mel.eval('match "v[0-9][0-9][0-9]" "'+ret[0]+'"')
	if ret[1] == "":
		ret[1] = "v000"

	ret[2] = parts[2]

	r = maya.mel.eval("currentRenderer()")
	if( ret[2] == "sgi" and r == "mentalRay" ): # and int(cmds.about(version=True)) == 8 ):
		ret[2] = "rgb"

	return ret

def version():
    v = os.environ["MAYA_VER"]
    v = v.replace(".SP1", "")
    return v

class MayaRenderDialog(QDialog):
	def __init__(self,parent=None):
		QDialog.__init__(self,parent)
		loadUi(os.environ["ABSUBMIT"]+"/mayarenderdialogui.ui",self)
		self.connect( self.mAutoPacketSizeCheck, SIGNAL('toggled(bool)'), self.autoPacketSizeToggled )
		self.connect( self.mChooseFileNameButton, SIGNAL('clicked()'), self.chooseFileName )
		self.connect( self.mAllHostsCheck, SIGNAL('toggled(bool)'), self.allHostsToggled )
		self.connect( self.mHostListButton, SIGNAL('clicked()'), self.showHostSelector )
		self.layout().setSizeConstraint(QLayout.SetFixedSize)
		self.mProjectCombo.setSpecialItemText( 'None' )
		#self.mProjectCombo.setStatusFilters( ProjectStatusList(ProjectStatus.recordByName( 'Production' )) )
		self.OutputPath = None
		self.HostList = ''
		self.Services = []
		#self.loadSettings()

	def initFields(self):
		global saveNeeded
		scenePath = cmds.file(query=True, sceneName=True)
		sceneName = cmds.file(query=True, shortName=True, sceneName=True)
		renderableCameras = 0
		cameras = cmds.ls(type="camera")
		for cName in cameras:
			if cmds.getAttr(cName+".renderable") == 1:
				renderableCameras = renderableCameras + 1

		#if( renderableCameras != 1 ):
		#	error("Only scenes with one renderable camera allowed. Please save your scene after fixing.");

		prevFilePrefix = cmds.getAttr("defaultRenderGlobals.imageFilePrefix")
		filePrefix = "%s%.4n%.e"
		renderLayers = cmds.ls( type="renderLayer" )
		for lName in renderLayers:
			if( cmds.getAttr(lName+".renderable") and lName.find(":") == -1):
				if( lName != "defaultRenderLayer" ):
					filePrefix = "%/l%s%_l%.4n%.e"

		saveNeeded = False
		if filePrefix != prevFilePrefix:
			saveNeeded = True

		cmds.setAttr("defaultRenderGlobals.imageFilePrefix", filePrefix, type="string")
		cmds.setAttr("defaultRenderGlobals.extensionPadding", 4)
		#cmds.setAttr("defaultRenderGlobals.imageFormat", 5)
		maya.mel.eval('setMayaSoftwareFrameExt(3,0)')
		#renderDir = cmds.workspace(q=True, dir=True)
		#renderDir += "images/"
		self.mFileNameEdit.setText(scenePath)
		self.mJobNameEdit.setText(sceneName)

		renderDir = os.environ["DRD_FARM_PIC"] + "/" + sceneName;
		self.mOutputDirEdit.setText(renderDir)

		if self.mTimelineRadio.isChecked():
			start = int(maya.mel.eval('playbackOptions -q -min'))
			end = int(maya.mel.eval('playbackOptions -q -max'))
		else:
			start = int(maya.mel.eval('getAttr "defaultRenderGlobals.startFrame"'))
			end = int(maya.mel.eval('getAttr "defaultRenderGlobals.endFrame"'))
		self.mFrameStartEdit.setText(QString.number(start))
		self.mFrameEndEdit.setText(QString.number(end))

		project = Project.recordByName( os.environ["DRD_JOB"] )
		if project.isRecord():
			index = self.mProjectCombo.findText( project.name() )
			self.mProjectCombo.setCurrentIndex( index )

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
		self.mFrameStartEdit.setText( c.readString( "FrameList" ) )
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
		c.writeString( "FrameList", self.mFrameStartEdit.text() )
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
		hs.setServiceFilter( ServiceList(Service.recordByName( 'Maya' )) )
		hs.setHostList( self.HostList )
		if hs.exec_() == QDialog.Accepted:
			self.HostList = hs.hostStringList()
		del hs
		
	def chooseFileName(self):
		fileName = QFileDialog.getOpenFileName(self,'Choose Scene To Render', QString(), 'Maya Scene (*.ma)' )
		if not fileName.isEmpty():
			self.mFileNameEdit.setText(fileName)
		
	def checkFrameList(self):
		(frames, valid) = expandNumberList( self.mFrameStartEdit.text() )
		return valid
	
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
	
	def buildSubmitArgs(self):
		sl = {}
		sl['renderer'] = 'Maya'
		sl['noCopy'] = 'true'
		sl['packetType'] = 'continuous'
		sl['priority'] = str(self.mPrioritySpin.value())
		sl['user'] = getUserName()
		sl['packetSize'] = str(self.packetSize())
		sl['fileName'] = self.mFileNameEdit.text()
		sl['append'] = self.mAppendEdit.text()
		if os.environ.has_key("DRD_FARM_TEMP"):
			sl['projectPath'] = os.environ["DRD_FARM_TEMP"]
		sl['width'] = str(cmds.getAttr("defaultResolution.width"))
		sl['height'] = str(cmds.getAttr("defaultResolution.height"))
		if self.mAllFramesAsSingleTaskCheck.isChecked():
			sl['allframesassingletask'] = 'true'
			sl['frameList'] = str('1')
		else:
			sl['frameList'] = self.mFrameStartEdit.text() + "-" + self.mFrameEndEdit.text()
		notifyError, notifyComplete = self.buildNotifyStrings()
		sl['notifyOnError'] = notifyError
		sl['notifyOnComplete'] = notifyComplete
		sl['deleteOnComplete'] = str(int(self.mDeleteOnCompleteCheck.isChecked()))
		sl['projectName'] = self.mProjectCombo.currentText()
		if not self.mAllHostsCheck.isChecked() and len(self.HostList):
			sl['hostList'] = str(self.HostList)

		sl['environment'] = QProcess.systemEnvironment().replaceInStrings( QRegExp("^MAYA_LICENSE.*"), "MAYA_LICENSE=render" ).join("\n")
		if os.environ.has_key("DRD_JOB"):
			sl['projectName'] = os.environ["DRD_JOB"]

		sl['maxTaskTime'] = "18000"
		sl['assignmentSlots'] = "4"
		sl['minMemory'] = str(int(sl["assignmentSlots"]) * 1024 * 512)
		sl['maxMemory'] = str(int(sl["assignmentSlots"]) * 1024 * 2048)

		if len(self.Services):
			sl['services'] = ','.join(self.Services)
		if self.mSubmitSuspendedCheck.isChecked():
			sl['submitSuspended'] = str('1')
		Log("Applying Absubmit args: %s" % str(sl))
		return sl

	def accept(self):
		global saveNeeded
		if self.mJobNameEdit.text().isEmpty():
			QMessageBox.critical(self, 'Missing Job Name', 'You must choose a name for this job' )
			return

		if not QFile.exists( self.mFileNameEdit.text() ):
			QMessageBox.critical(self, 'Invalid File', 'You must choose an existing Maya scene' )
			return

		if not self.checkFrameList():
			QMessageBox.critical(self, 'Invalid Frame List', 'Frame Lists are comma separated lists of either "XXX", or "XXX-YYY"' )
			return

		#self.saveSettings()

		melString = str(cmds.getAttr("defaultRenderGlobals.preRenderMel"))
		if melString.find("M2AB") == -1:
			saveNeeded = True
		melString = str(cmds.getAttr("defaultRenderGlobals.postRenderMel"))
		if melString.find("M2AB") == -1:
			saveNeeded = True
		melString = str(cmds.getAttr("defaultRenderGlobals.postMel"))
		if melString.find("M2AB") == -1:
			saveNeeded = True

		cmds.setAttr("defaultRenderGlobals.preRenderMel", 'print(\"M2AB: starting frame\\n\");', type="string")
		cmds.setAttr("defaultRenderGlobals.postRenderMel", 'print(\"M2AB: completed frame\\n\");', type="string")
		cmds.setAttr("defaultRenderGlobals.postMel", 'print(\"M2AB: completed job\\n\");', type="string")

		if saveNeeded:
			cmds.file(save=True)

		renderLayers = cmds.ls(type="renderLayer")
		if len(renderLayers) == 1:
			cmds.setAttr(renderLayers[0] +".renderable", 1)

		for lName in renderLayers:
			if( cmds.getAttr(lName+".renderable") and lName.find(":")==-1 ):
				cmds.editRenderLayerGlobals( currentRenderLayer=lName )

				jobArgs = self.buildSubmitArgs()
				jobArgs.update({})

				if( maya.mel.eval("currentRenderer()") == "mentalRay" ):
					#setCurrentRenderer mentalRay;
					#mentalrayUI "";
					#setAttr mentalrayGlobals.exportVerbosity 5;
					jobArgs['jobType'] = "MentalRay"+version()
					jobArgs['renderer'] = "MentalRay"
				else:
					jobArgs['jobType'] = "Maya"+version()
					jobArgs['renderer'] = "Maya"

				parts = imageNameParts()
				jobArgs["outputPath"] = self.mOutputDirEdit.text() + "/" + parts[0]+".."+parts[2]
				if( lName == "defaultRenderLayer" ):
					jobArgs['job'] = self.mJobNameEdit.text()
				else:
					jobArgs['job'] = self.mJobNameEdit.text() +"_"+lName
				jobArgs["append"] += " -rl "+lName

				submitter = Submitter()
				submitter.applyArgs( jobArgs )
				submitter.submit()

	def submitSuccess(self):
		Log( 'Submission Finished Successfully' )
		QDialog.accept(self)

	def submitError(self,errorMsg):
		QMessageBox.critical(self, 'Submission Failed', 'Submission Failed With Error: ' + errorMsg)
		Log( 'Submission Failed With Error: ' + errorMsg )
		QDialog.reject(self)

def maya2ABSubmit():
	global app
	global dialog
	global saveNeeded
	pt.initializePumpThread()
	app=qApp
    if "ABSUBMIT" in os.environ:
        try:
        	initConfig(os.environ["ABSUBMIT"]+"/burner.ini","/tmp/mayasubmit.log")
        except:
            initConfig(os.environ["ABSUBMIT"]+"/ab.ini","/tmp/mayasubmit.log")
    else:
        if not os.name = 'nt':
            initConfig("/etc/ab/burner.ini")
        else:
            print "Could not init"
            sys.exit(-1)

	classes_loader()
	dialog = MayaRenderDialog()
	dialog.initFields()
	dialog.show()
	#shutdown()

"""
sys.path.append("/drd/users/barry.robison/blur/trunk/cpp/apps/absubmit/maya")
import maya2AB
maya2AB.maya2ABSubmit()
"""
