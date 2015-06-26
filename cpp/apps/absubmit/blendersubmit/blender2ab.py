from blur.Stone import *
from blur.Classes import *
from blur.Classesui import HostSelector
from blur.absubmit import Submitter
from blur import RedirectOutputToLog
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.uic import *

import sys
import os

class BlenderRenderDialog(QDialog):
    def __init__(self,parent=None):
        QDialog.__init__(self,parent)
        arseVer = ""
        if "ARSENALVER" in os.environ:
            arseVer = os.environ["ARSENALVER"] + "/"
            
        loadUi(os.environ["ARSENALDIR"] + "/" + arseVer + "submitterScripts/blendersubmit/blenderrenderdialogui.ui", self)
        self.connect( self.mAutoPacketSizeCheck, SIGNAL('toggled(bool)'), self.autoPacketSizeToggled )
        self.connect( self.mChooseFileNameButton, SIGNAL('clicked()'), self.chooseFileName )
        self.connect( self.mAllHostsCheck, SIGNAL('toggled(bool)'), self.allHostsToggled )
        self.connect( self.mHostListButton, SIGNAL('clicked()'), self.showHostSelector )
        self.mProjectCombo.setSpecialItemText( 'None' )
        self.OutputPath = None
        self.HostList = ''
        self.Services = []
        
    def initFields(self, blendFile, startFrame, endFrame, scenes):
        pass
        
    def autoPacketSizeToggled(self, autoPacketSize):
        self.mPacketSizeSpin.setEnabled(not autoPacketSize)
        
    def allHostsToggled(self, allHosts):
        self.mHostListButton.setEnabled(not allHosts)
        
    def showHostSelector(self):
        hs = HostSelector(self)
        hs.setServiceFilter( ServiceList(Service.recordByName('Blender')))
        hs.setHostList(self.HostList)
        if hs.exec_() == QDialog.Accepted:
            self.HostList = hs.hostStringList()
        del hs
        
    def chooseFileName(self):
        fileName = QFileDialog.getOpenFileName(self,'Choose blend To Render', QString(), 'Blender File (*.blend)')
        if not fileName.isEmpty():
            self.mFileNameEdit.setText(fileName)
            
    def checkFrameList(self):
        (frames, valid) = expandNumberList(self.mFrameStartEdit.text())
        return valid
        
    def packetSize(self):
        if self.mAutoPacketSizeCheck.isChecked():
            return 0
        return self.mPacketSizeSpin.value()
        
    def buildNotifyString(self,xmpp,email):
        ret = ''
        if xmpp or email:
            ret = getUserName() + ':'
            if xmpp:
                ret += 'j'
            if email:
                ret += 'e'
        return ret
        
    def buildNotifyStrings(self):
        return (
            self.buildNotifyString(self.mXMPPErrorsCheck.isChecked(), self.mEmailErrorsCheck.isChecked()),
            self.buildNotifyString(self.mXMPPCompletionCheck.isChecked(), self.mEmailCompletionCheck.isChecked()))
        
    def buildSubmitArgs(self):
        sl = {}
        sl['jobType'] = "Blender"
        sl['noCopy'] = 'true'
        sl['packetType'] = 'continuous'
        sl['priority'] = str(self.mPrioritySpin.value())
        sl['user'] = str(getUserName())
        sl['packetSize'] = str(self.packetSize())
        sl['fileName'] = str(self.mFileNameEdit.text())

        self.Services.append("Blender")
        #sl['environment'] = subprocess.Popen(["/drd/software/int/bin/launcher.sh","-p", os.environ["DRD_JOB"], "-d", os.environ["DRD_DEPT"], "-e", launcherPreset], stdout=subprocess.PIPE).communicate()[0]

        sl['minMemory'] = "1388608"
        sl['maxMemory'] = "8388608"

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
            QMessageBox.critical(self, 'Invalid File', 'You must choose an existing blend file' )
            return

        if not self.checkFrameList():
            QMessageBox.critical(self, 'Invalid Frame List', 'Frame Lists are comma separated lists of either "XXX", or "XXX-YYY"' )
            return
            
        jobArgs = self.buildSubmitArgs()
        jobArgs['job'] = self.mJobNameEdit.text()
        
        submitter = Submitter(self)
        self.connect(submitter,SIGNAL('submitSuccess()'), self.submitSuccess)
        self.connect(submitter,SIGNAL('submitError( const QString & )'), self.submitError)
        submitter.applyArgs(jobArgs)
        submitter.submit()
        
    def submitSuccess(self):
        Log( 'Submission Finished Successfully' )
        QDialog.accept(self)

    def submitError(self,errorMsg):
        QMessageBox.critical(self, 'Submission Failed', 'Submission Failed With Error: ' + errorMsg)
        Log( 'Submission Failed With Error: ' + errorMsg )
        QDialog.reject(self)
        
print("blender2ABSubmit()")
app = QApplication(sys.argv)
print("blender2ABSubmit(): initialize db connection")
initConfig("submitter.ini", os.environ['TEMP']+"blendersubmit.log")
classes_loader()
args = sys.argv

dialog = BlenderRenderDialog()
dialog.show()
app.exec_()
