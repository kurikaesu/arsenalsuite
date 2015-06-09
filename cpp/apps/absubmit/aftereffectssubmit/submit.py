#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.uic import *
from blur.Stone import *
from blur.Classes import *
import sys, os, traceback

def toWinPath(aePath):
	if len(aePath) >= 3:
		return aePath[1] + ":" + aePath[2:]
	return aePath

class AfterEffectsSubmitDialog(QDialog):
	def __init__(self,parent=None):
		QDialog.__init__(self,parent)
		loadUi("aftereffectssubmitdialogui.ui",self)
		self.connect( self.mAutoPacketSizeCheck, SIGNAL('toggled(bool)'), self.autoPacketSizeToggled )
		self.mProjectCombo.setSpecialItemText( 'None' )
		self.mProjectCombo.setStatusFilters( ProjectStatusList(ProjectStatus.recordByName( 'Production' )) )
		self.OutputPath = None
		self.readOptionsFile()
		
	def readOptionsFile(self):
		try:
			file = open("current_options.txt","r")
			lines = file.read().split('\n')
			
			# Get Version in format 7.0x244, chop the x244 part
			self.Version = lines[0]
			self.Version = self.Version[0:self.Version.index('x')]
			
			# Get filename and convert from AE style /c/path/ to c:/path/
			fileName = toWinPath(lines[1])
			if not QFile(fileName).exists():
				QMessageBox.warning( self, 'File not found', 'The file supplied by the After Effects submission script was not found.' )
				Log( "AE File not found: " + fileName )
				self.close()
				return
			
			self.mFileNameEdit.setText( fileName )
			
			# Parse the passes
			for line in lines[2:]:
				if line.count('; ') == 3:
					(passName, frameStart, frameEnd, outputPath) = line.split('; ')
					twi = QTreeWidgetItem( self.mPassTree, QStringList() << passName << str(int(float(frameStart))) << str(int(float(frameEnd))) << toWinPath(outputPath) )
					twi.setFlags( Qt.ItemIsUserCheckable | Qt.ItemIsEditable | Qt.ItemIsEnabled )
					twi.setCheckState( 0, Qt.Checked )
		except:
			traceback.print_exc()
			QMessageBox.critical( self, "Missing Required Information", "There was an error gathering required information for job submission.  Please contact your System Admin." )
			QTimer.singleShot( 0, self, SLOT('close()') )

	def getJobType(self):
		version = { '7.0':'AfterEffects7', '6.5':'AfterEffects', '8.0':'AfterEffects8' }
		try:
			return version[self.Version]
		except: pass
	
	def autoPacketSizeToggled(self,autoPacketSize):
		self.mPacketSizeSpin.setEnabled(not autoPacketSize)
	
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
	
	def absubmitPath(self):
		return 'c:/blur/absubmit/absubmit.exe'
	
	def buildAbsubmitArgList(self, passItem):
		sl = QStringList()
		sl <<  'jobType' << self.getJobType()
		sl << 'packetType' << 'continuous'
		sl << 'priority' << str(self.mPrioritySpin.value())
		sl << 'user' << getUserName()
		sl << 'packetSize' << str(self.packetSize())
		sl << 'frameList' << (passItem.text(1) + "-" + passItem.text(2))
		sl << 'fileName' << self.mFileNameEdit.text()
		notifyError, notifyComplete = self.buildNotifyStrings()
		sl << 'notifyOnError' << notifyError << 'notifyOnComplete' << notifyComplete
		sl << 'job' << (self.mJobNameEdit.text() + "_" + passItem.text(0))
		sl << 'deleteOnComplete' << str(int(self.mDeleteOnCompleteCheck.isChecked()))
		sl << 'comp' << passItem.text(0)
		if self.mProjectCombo.project().isRecord():
			sl << 'projectName' << self.mProjectCombo.project().name()
		sl << 'outputPath' << passItem.text(3)
		return sl
	
	def readSubmissionError(self, errorId):
		return open( 'C:/blur/absubmit/errors/%i.txt' % errorId, 'r' ).read()
	
	def accept(self):
		if self.mJobNameEdit.text().isEmpty():
			QMessageBox.critical(self, 'Missing Job Name', 'You must choose a name for this job' )
			return
		
		cmd = self.absubmitPath()
		for i in range(self.mPassTree.topLevelItemCount()):
			item = self.mPassTree.topLevelItem(i)
			if( item.checkState(0) == Qt.Checked ):
				args = self.buildAbsubmitArgList(item)
				
				Log("Starting Command: %s %s" % (cmd,str(args.join(','))))
		
				p = QProcess()
				p.setWorkingDirectory( 'c:/blur/absubmit/' )
				p.start(cmd,args)
				if not p.waitForStarted():
					QMessageBox.critical(self, 'Unable to start absubmit.exe', 'Unable to start absubmit.exe Please notify IT.')
					self.close()
					return
		
				p.waitForFinished(-1)
				if p.exitCode() != 0:
					QMessageBox.critical(self, 'Submission Failed', 'Submission Failed With Error: ' + self.readSubmissionError(p.exitCode()))
					self.close()
					return
		
		QDialog.accept(self)
	
if __name__ == "__main__":
	os.chdir("c:\\blur\\absubmit\\aftereffectssubmit\\")
	app = QApplication(sys.argv)
	initConfig("../absubmit.ini","aftereffects.log")
	blurqt_loader()
	dialog = AfterEffectsSubmitDialog()
	dialog.show()
	app.exec_()
