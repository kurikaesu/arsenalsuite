#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.uic import *
from blur.Stone import *
from blur.Classes import *
from blur.absubmit import *
import sys, os, traceback, re

if __name__ == "__main__":
	arch = os.popen('arch').read()
	os.chdir("/mnt/x5/Global/infrastructure/ab/"+arch[:-1]+"/absubmit")
	app = QApplication(sys.argv)
	initConfig("./absubmit.ini","aftereffects.log")
	blurqt_loader()

	argList = app.arguments()
	hostSource = argList[1]
	hostDest = argList[2]
	destPath = argList[3]

	destRsync = "rsync://"+hostDest+"/root"+destPath
	sourceRsync = "rsync://"+hostSource+"/root"+destPath

	rsyncCmd = "rsync --stats --out-format='%l %f' --dry-run -auv --relative --exclude-from=/mnt/x5/tools/sync.exclude "+destPath+" "+destRsync+" 2> /dev/null"
	print "running: "+rsyncCmd
	fhout = os.popen(str(rsyncCmd),"r")
	keepReading = True
	filePaths = ""
	bytes = 0
	while keepReading:
		line = fhout.readline()
		if line != "":
			print "read: "+line

			moFileLine = re.match("\d+ (.*)", line)
			if moFileLine:
				filePaths += moFileLine.group(1)+"\n"

			mo = re.match("Total transferred file size: (\d+) bytes", line)
			if mo:
				keepReading = False
				bytes = int(mo.group(1))
		else:
			continue
	
	print "DONE WITH RSYNC\n"

	tasks = int((bytes / 100000000)+0.6)
	if tasks == 0: tasks = 1

	argMap = dict()
	mo = re.match("/Shows/([^/]+)/", str(destPath))
	if mo:
		argMap["projectName"] = mo.group(1)

	user = os.environ["USER"]
	if user == "root": user = "barry"
	if user == "install": user = "barry"

	argMap['jobType'] = "Sync"
	argMap['packetType'] = "continuous"
	argMap['packetSize'] = "99999"
	argMap['noCopy'] = "true"
	argMap['append'] = "--exclude-from=/mnt/x5/tools/sync.exclude";
	argMap['user'] = user
	argMap['frameStart'] = "1"
	argMap['frameEnd'] = str(tasks)
	argMap['outputPath'] = destPath
	argMap['job'] = sourceRsync
	argMap['hostList'] = hostDest
	argMap['filesFrom'] = filePaths

	submitter = Submitter()
	submitter.applyArgs( argMap );
	submitter.setExitAppOnFinish( True );

	QTimer.singleShot( 0, submitter, SLOT( "submit()" ) );

	app.exec_()

