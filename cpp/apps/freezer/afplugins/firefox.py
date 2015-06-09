
from blur.Stone import *
from blur.Classes import *
from blur.Freezer import *
from PyQt4.QtCore import *
from PyQt4.QtSql import *
import traceback, sys, os

class FireFoxViewerPlugin(FrameViewerPlugin):
    def __init__(self):
        FrameViewerPlugin.__init__(self)

    def name(self):
        return QString("Firefox")

    def icon(self):
        return QString("images/firefox.png")

    def view(self, jobAssignment):
        cmdArgs = QStringList()
        if sys.platform=="darwin":
            cmdString = QString("/Applications/Firfox.app/Contents/MacOS/firefox-bin")
        else:
            cmdString = QString("/usr/bin/firefox")

        cmdArgs << jobAssignment.stdOut()
        QProcess.startDetached( cmdString, cmdArgs )

FrameViewerFactory.registerPlugin( FireFoxViewerPlugin() )
