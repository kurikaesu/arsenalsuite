
from blur.Stone import *
from blur.Classes import *
from blur.Freezer import *
from PyQt4.QtCore import *
from PyQt4.QtSql import *
import traceback, os, sys

class ShellViewerPlugin(JobViewerPlugin):
    def __init__(self):
        JobViewerPlugin.__init__(self)

    def name(self):
        return QString("shell")

    def icon(self):
        return QString("images/Terminal.png")

    def view(self, jobList):
        cmdArgs = QStringList()
        if sys.platform=="darwin":
            cmdString = QString("/Applications/Utilities/Terminal.app/Contents/MacOS/Terminal")
        elif sys.platform=="win32":
            cmdString = QString("C:/windows/system32/cmd.exe /C \"start c:\\windows\\system32\\cmd.exe\"")
        else:
            cmdString = QString("exo-open")
            cmdArgs << "--launch"
            cmdArgs << "TerminalEmulator"

        for job in jobList:
            Log( "Launch %s in %s" % (cmdString, QFileInfo(job.outputPath()).path() ))
            QProcess.startDetached( cmdString, cmdArgs, QFileInfo( job.outputPath() ).path() )

JobViewerFactory.registerPlugin( ShellViewerPlugin() )

