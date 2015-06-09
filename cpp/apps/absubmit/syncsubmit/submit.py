#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.uic import *
from blur.Stone import *
from blur.Classes import *
from blur.absubmit import *
import sys, os, traceback, re

class SyncSubmitter:
	def __init__(self):
		self.bytes = 0
		self.filePaths = QStringList()
		self.projectName = ""
		self.user = os.environ["USER"]
		self.job = ""
		self.hostDest = ""
		if self.user == "root": self.user = "barry"
		if self.user == "install": self.user = "barry"

	def addFiles(self, path, hostSource, hostDest):
		if self.hostDest == "": self.hostDest = hostDest
		destRsync = "rsync://"+hostDest+"/root"+path
		sourceRsync = "rsync://"+hostSource+"/root"+path

		rsyncCmd = "rsync --stats --dry-run -auv --relative --exclude-from=/mnt/x5/tools/sync.exclude "+path+" "+destRsync+" 2> /dev/null"
		print "running: "+rsyncCmd
		fhout = os.popen(str(rsyncCmd),"r")
		keepReading = True
		filesReading = False
		while keepReading:
			line = ""
			try:
				line = fhout.readline()
			except IOError: keepReading = False

			if line != "":
				if line.endswith("\n"): line = line.rstrip("\n")
				print "read: "+line
				
				moFilesStart = re.match("^building", line)
				if moFilesStart:
					print "starting file list"
					filesReading = True
					continue

				#if line == "building file list ... done": filesReading = True

				if filesReading:
					moFilesDone = re.match("Number of files: \d+", line)
					if moFilesDone:
						filesReading = False
						continue

					if not os.path.isdir('/'+line):
						self.filePaths << line

				mo = re.match("Total transferred file size: (\d+) bytes", line)
				if mo:
					keepReading = False
					self.bytes += int(mo.group(1))
			else:
				continue

		print "DONE WITH RSYNC\n"

	def submit(self):
		tasks = int((self.bytes / 100000000)+0.6)
		if tasks == 0: tasks = 1

		argMap = dict()
		argMap['jobType'] = "Sync"
		argMap['packetType'] = "continuous"
		argMap['packetSize'] = "99999"
		argMap['noCopy'] = "true"
		argMap['append'] = "--exclude-from=/mnt/x5/tools/sync.exclude";
		argMap['user'] = self.user
		argMap['frameStart'] = "1"
		argMap['frameEnd'] = str(tasks)
		argMap['outputPath'] = 'rsync://'+self.hostDest+'/root/'
		argMap['job'] = self.job
		argMap['hostList'] = self.hostDest
		argMap['filesFrom'] = self.filePaths.join("\n")

		if self.projectName == "":
			mo = re.match("/Shows/([^/]+)/", str(destPath))
			if mo:
				self.projectName = mo.group(1)
		argMap['projectName'] = str(self.projectName)

		submitter = Submitter()
		submitter.applyArgs( argMap );
		submitter.setExitAppOnFinish( True );
		submitter.submit()

class SyncSubmitDialog(QDialog):
	def __init__(self,parent=None):
		QDialog.__init__(self,parent)
		loadUi("syncsubmitdialogui.ui",self)
		self.connect( self.mAddButton, SIGNAL('clicked()'), self.addFilesSlot )
		self.connect( self.mPathEdit, SIGNAL('returnPressed()'), self.addFilesSlot )
		self.mProjectCombo.setSpecialItemText( 'None' )
		self.mProjectCombo.setStatusFilters( ProjectStatusList(ProjectStatus.recordByName( 'Production' )) )
		self.mJabberCompletionCheck.setChecked(True)
		self.submitter = SyncSubmitter()

		if QFile("/mnt/x5/_LA_x5.txt").exists():
			self.mSourceSMO.setChecked(True)
			self.mDestVCO.setChecked(True)

		if len(sys.argv) == 2:
			print "got one argument, adding file path"
			self.mPathEdit.setText(sys.argv[1])
			self.addFilesSlot()
		
	def addFilesSlot(self):
		if self.mPathEdit.text().isEmpty(): return

		QApplication.setOverrideCursor(QCursor(Qt.WaitCursor));
		source = str()
		dest = str()
		if self.mSourceSMO.isChecked(): source = "pollux"
		if self.mSourceVCO.isChecked(): source = "arnold"
		if self.mSourceBH.isChecked(): source = "nils"
		if self.mDestSMO.isChecked(): dest = "pollux"
		if self.mDestVCO.isChecked(): dest = "arnold"
		if self.mDestBH.isChecked(): dest = "nils"

		if not self.mProjectCombo.project().isRecord():
			showNameRx = QRegExp("/Shows/([^/]+)/")
			if showNameRx.indexIn(self.mPathEdit.text()) != -1:
				proj = Project.recordByName(showNameRx.cap(1))
				if proj.isRecord():
					self.mProjectCombo.setProject(proj)

		if self.mJobNameEdit.text().isEmpty():
			self.mJobNameEdit.setText(self.mPathEdit.text())

		self.submitter.addFiles( self.mPathEdit.text(), source, dest )

		self.mFilesTree.clear()
		for filePath in self.submitter.filePaths:
			twi = QTreeWidgetItem( self.mFilesTree, QStringList() << filePath )
		self.mFilesTree.resizeColumnToContents(0)
		self.mSizeLabel.setText( str(self.submitter.bytes) )
		QApplication.restoreOverrideCursor();

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
	
	def readSubmissionError(self, errorId):
		return open( 'C:/blur/absubmit/errors/%i.txt' % errorId, 'r' ).read()

	def submitSyncJob(self):
		syncCmd = "/mnt/x5/tools/perl/scripts/sync_AB arnold pollux %s 2>/dev/null" % str(self.mFileNameEdit.text())
		print syncCmd
		fhout = os.popen(str(syncCmd),"r")
		syncId = 0
		alloutput = []
		try:
			allOutput = fhout.readlines()
		except IOError:
			print "IOError but continue"
		for line in allOutput:
			print "read: "+line
			mo = re.match("keyJob\|(\d+)", line)
			if mo:
				syncId = mo.group(1)
				print "syncId: "+str(syncId)
		return syncId
	
	def accept(self):
		if self.mJobNameEdit.text().isEmpty():
			QMessageBox.critical(self, 'Missing Job Name', 'You must choose a name for this job' )
			return
		self.submitter.job = self.mJobNameEdit.text()

		if self.mProjectCombo.project().isRecord():
			self.submitter.projectName = str(self.mProjectCombo.project().name()) 

		self.submitter.submit()
		
		QDialog.accept(self)
	
if __name__ == "__main__":
	os.chdir("/mnt/x5/Global/infrastructure/ab/ppc/absubmit/syncsubmit")

	argList = sys.argv

	if len(argList) == 3:
		hostSource = argList[1]
		hostDest = argList[2]
		destPath = argList[3]
		submitter = SyncSubmitter()
		submitter.addFiles( destPath, hostSource, hostDest )
		submitter.submit()
	else:
		app = QApplication(sys.argv)
		initConfig("/mnt/x5/Global/infrastructure/ab/ppc/absubmit/absubmit.ini","syncsubmit.log")
		blurqt_loader()
		dialog = SyncSubmitDialog()
		dialog.show()
		app.exec_()

