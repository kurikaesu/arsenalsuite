#!/usr/bin/python2.5

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from blur.Stone import *
from blur.Classes import *
from blur.absubmit import Submitter
#from blur import RedirectOutputToLog
import os
import sys
import time
import threading

global app
global keepRunning
pumpedThread = None
app = None
keepRunning = False

def pumpQt():
    global app
    global keepRunning
    while keepRunning:
        time.sleep(0.01)
        app.processEvents()

def initializePumpThread(useGUI=False):
    global pumpedThread
    global app

    if pumpedThread == None:
        app = QCoreApplication.instance()
        if app is None:
            try:
                args = sys.argv
            except:
                args = []
            app = QApplication(args, useGUI)

        pumpedThread = threading.Thread( target = pumpQt, args = () )
        pumpedThread.start()

def buildSubmitArgs():
	argMap = {}
	# standard batch job stuff
	argMap['jobType'] = "Batch"
	argMap['noCopy'] = 'true'
	argMap['packetType'] = 'continuous'
	argMap['priority'] = str(50)
	argMap['user'] = os.environ["USER"]
	argMap['environment'] = QProcess.systemEnvironment().join("\n")
	argMap['notifyOnError'] = argMap['user']+":j"
	argMap['notifyOnComplete'] = argMap['user']+":j"
	if os.environ.has_key("DRD_JOB"):
		argMap['projectName'] = os.environ["DRD_JOB"]
	if os.environ.has_key("DRD_JOB") and os.environ.has_key("DRD_DEPT"):
		argMap['projectName'] = os.environ["DRD_JOB"] + "-" + os.environ["DRD_DEPT"]
	argMap['packetSize'] = str(9999)
	argMap['runasSubmitter'] = "true"
	argMap['maxTaskTime'] = "18000"
	argMap['assignmentSlots'] = "4"
	# Log("Applying Absubmit args: %s" % str(argMap))
	return argMap

def submitSuccess():
    sys.exit(0)

def submitError(errorString):
    stop()
    raise str(errorString)

def farmSubmit(userArgs):
    jobArgs = buildSubmitArgs()
    jobArgs.update( userArgs )

    # this is overridden as a "security measure"
    jobArgs['runasSubmitter'] = "true"

    if not jobArgs.has_key("minMemory"):
        jobArgs['minMemory'] = str(int(jobArgs["assignmentSlots"]) * 1024 * 512)
    if not jobArgs.has_key("maxMemory"):
        jobArgs['maxMemory'] = str(int(jobArgs["assignmentSlots"]) * 1024 * 2048)

    # edit per job
    # argMap['fileName'] = "/tmp/myFile.sh"
    #argMap["outputPath"] = "/tmp/outputFoo..exr"
    #argMap['frameList'] = "1-1"
    #argMap['job'] = "my_batch_job"

    start()
    submitter = Submitter()
    submitter.setExitAppOnFinish(False)
    #QObject.connect( submitter, SIGNAL( 'submitSuccess()' ), submitSuccess )
    QObject.connect( submitter, SIGNAL( 'submitError( const QString & )' ), submitError )
    submitter.applyArgs( jobArgs )
    submitter.submit()
    stop()
    return submitter.job()

def start():
    global keepRunning
    keepRunning = True

def stop():
    global keepRunning
    keepRunning = False

initializePumpThread()
print("py2ab(): initialize db connection")
if "ABSUBMIT" in os.environ:
    try:
        initConfig(os.environ['ABSUBMIT']+"/ab.ini", os.environ['TEMP']+"/pysubmit.log")
    except:
        initConfig(os.environ['ABSUBMIT']+"/burner.ini", os.environ['TEMP']+"/pysubmit.log")
else:
    if not os.name == "nt":
        initConfig("/etc/ab/burner.ini", os.environ['TEMP']+"/pysubmit.log")

classes_loader()
args = sys.argv

