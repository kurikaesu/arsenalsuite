#!/usr/bin/python2.5

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.uic import *
from blur.Stone import *
from blur.Classes import *
from blur.Classesui import HostSelector
from blur.absubmit import Submitter
from blur import RedirectOutputToLog
import os
import sys
import time
import subprocess

class NukeRenderDialog(QDialog):
    def __init__(self,parent=None):
        QDialog.__init__(self,parent)
        loadUi(os.environ["ABSUBMIT"]+"/nukesubmit/nukerenderdialogui.ui",self)
        self.connect( self.mAutoPacketSizeCheck, SIGNAL('toggled(bool)'), self.autoPacketSizeToggled )
        self.connect( self.mChooseFileNameButton, SIGNAL('clicked()'), self.chooseFileName )
        self.connect( self.mAllHostsCheck, SIGNAL('toggled(bool)'), self.allHostsToggled )
        self.connect( self.mHostListButton, SIGNAL('clicked()'), self.showHostSelector )
        self.layout().setSizeConstraint(QLayout.SetFixedSize);
        self.mProjectCombo.setSpecialItemText( 'None' )
        #self.mProjectCombo.setStatusFilters( ProjectStatusList(ProjectStatus.recordByName( 'Production' )) )
        self.OutputPath = None
        self.HostList = ''
        self.Services = []
        #self.loadSettings()

    def initFields(self, nukeScript, startFrame, endFrame, writers):
        self.treeWidget.clear()
        for key in writers.keys():
            item = QTreeWidgetItem()
            item.setText(0, str(key))
            item.setText(1, writers[key])
            #item.setFlags()
            self.treeWidget.addTopLevelItem(item)

        self.mFileNameEdit.setText(nukeScript)
        self.mJobNameEdit.setText(os.path.basename(nukeScript))
        self.mFrameStartEdit.setText(startFrame)
        self.mFrameEndEdit.setText(endFrame)

        oculaparts=['O_Solver', 'O_DisparityGenerator', 'O_InteraxialShifter', 'O_VerticalAligner', 'O_ColourMatcher', 'O_NewView', 'O_DepthToDisparity', 'O_DisparityToDepth']
        nukefile = open(str(self.mFileNameEdit.text()))
        nkfiletext= nukefile.read()
        if any(oculafunction in nkfiletext for oculafunction in oculaparts):
            self.mOculaCheck.setChecked(True)

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
        hs.setServiceFilter( ServiceList(Service.recordByName( 'Nuke' )) )
        hs.setHostList( self.HostList )
        if hs.exec_() == QDialog.Accepted:
            self.HostList = hs.hostStringList()
        del hs
        
    def chooseFileName(self):
        fileName = QFileDialog.getOpenFileName(self,'Choose Scene To Render', QString(), 'Nuke Scene (*.nk)' )
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
        sl['jobType'] = "Nuke52"
        sl['noCopy'] = 'true'
        sl['packetType'] = 'continuous'
        sl['priority'] = str(self.mPrioritySpin.value())
        sl['user'] = str(getUserName())
        sl['packetSize'] = str(self.packetSize())
        sl['fileName'] = str(self.mFileNameEdit.text())
        sl['append'] = str(self.mAppendEdit.text())
        launcherPreset = "ext/nuke/nuke"
        if self.mOculaCheck.isChecked():
            launcherPreset = "ext/nuke/nuke_ocula"
            self.Services.append("Nuke")
            self.Services.append("ocula")
        sl['environment'] = subprocess.Popen(["/drd/software/int/bin/launcher.sh","-p", os.environ["DRD_JOB"], "-d", os.environ["DRD_DEPT"], "-e", launcherPreset], stdout=subprocess.PIPE).communicate()[0]

        sl['minMemory'] = "1388608"
        sl['maxMemory'] = "8388608"

        if self.mAllFramesAsSingleTaskCheck.isChecked():
            sl['allframesassingletask'] = 'true'
            sl['frameList'] = str('1')
        else:
            sl['frameList'] = str(self.mFrameStartEdit.text() + "-" + self.mFrameEndEdit.text())
        notifyError, notifyComplete = self.buildNotifyStrings()
        sl['notifyOnError'] = notifyError
        sl['notifyOnComplete'] = notifyComplete
        sl['deleteOnComplete'] = str(int(self.mDeleteOnCompleteCheck.isChecked()))
        if self.mProjectCombo.project().isRecord():
            sl['projectName'] = self.mProjectCombo.project().name()
        if not self.mAllHostsCheck.isChecked() and len(self.HostList):
            sl['hostList'] = str(self.HostList)

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
            QMessageBox.critical(self, 'Invalid File', 'You must choose an existing Nuke scene' )
            return

        if not self.checkFrameList():
            QMessageBox.critical(self, 'Invalid Frame List', 'Frame Lists are comma separated lists of either "XXX", or "XXX-YYY"' )
            return

        self.saveSettings()

        if self.mWriteAllCheck.isChecked():
            jobArgs = self.buildSubmitArgs()
            jobArgs['job'] = self.mJobNameEdit.text()
            allOutputs = self.treeWidget.findItems("*", Qt.MatchWildcard)
            index = 1
            for i in allOutputs:
                jobArgs["outputPath"+str(index)] = i.text(1)
                jobArgs["outputPath"] = i.text(1)
                if self.mAllFramesAsSingleTaskCheck.isChecked():
                    jobArgs['frameList'+str(index)] = str('1')
                else:
                    jobArgs['frameList'+str(index)] = str(self.mFrameStartEdit.text() + "-" + self.mFrameEndEdit.text())
                index = index + 1
                outputDir = QFileInfo( jobArgs["outputPath"] ).absolutePath()
                if not os.path.exists( outputDir ):
                    QDir().mkpath( outputDir )

            submitter = Submitter(self)
            self.connect( submitter, SIGNAL( 'submitSuccess()' ), self.submitSuccess )
            self.connect( submitter, SIGNAL( 'submitError( const QString & )' ), self.submitError )
            submitter.applyArgs( jobArgs )
            submitter.submit()
        else:
            selectedItems = self.treeWidget.selectedItems()
            index = 1
            for i in selectedItems:
                jobArgs = self.buildSubmitArgs()
                jobArgs["outputPath"+str(index)] = i.text(1)
                if self.mAllFramesAsSingleTaskCheck.isChecked():
                    jobArgs['frameList'+str(index)] = str('1')
                else:
                    jobArgs['frameList'+str(index)] = str(self.mFrameStartEdit.text() + "-" + self.mFrameEndEdit.text())
                outputDir = QFileInfo( jobArgs["outputPath"+str(index)] ).absolutePath()
                if not os.path.exists( outputDir ):
                    QDir().mkpath( outputDir )
                jobArgs['job'] = self.mJobNameEdit.text() +"_"+ i.text(0)
                jobArgs['append'] = " -X %s" % i.text(0)

                submitter = Submitter(self)
                self.connect( submitter, SIGNAL( 'submitSuccess()' ), self.submitSuccess )
                self.connect( submitter, SIGNAL( 'submitError( const QString & )' ), self.submitError )
                submitter.applyArgs( jobArgs )
                submitter.submit()

    def submitSuccess(self):
        Log( 'Submission Finished Successfully' )
        QDialog.accept(self)

    def submitError(self,errorMsg):
        QMessageBox.critical(self, 'Submission Failed', 'Submission Failed With Error: ' + errorMsg)
        Log( 'Submission Failed With Error: ' + errorMsg )
        QDialog.reject(self)

print("nuke2ABSubmit()")
app = QApplication(sys.argv)
print("nuke2ABSubmit(): initialize db connection")
initConfig(os.environ['ABSUBMIT']+"/ab.ini", os.environ['TEMP']+"nukesubmit.log")
classes_loader()
args = sys.argv
thisScript = args.pop(0)
nukeScript = args.pop(0)
firstFrame = args.pop(0)
lastFrame = args.pop(0)
writers = {}
index = 0
for i,k in enumerate(args[0::2]):
    print "%s %s" % (index,k)
    writers[k] = args[index+1].replace('%04d','')
    index = index + 2

dialog = NukeRenderDialog()
dialog.initFields(nukeScript, firstFrame, lastFrame, writers)
dialog.show()
app.exec_()

