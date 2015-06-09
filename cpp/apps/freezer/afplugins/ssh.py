
from blur.Stone import *
from blur.Classes import *
from blur.Freezer import *
from PyQt4.QtCore import *
from PyQt4.QtSql import *
import traceback, os

class SshViewerPlugin(HostViewerPlugin):
    def __init__(self):
        HostViewerPlugin.__init__(self)

    def name(self):
        return QString("ssh")

    def icon(self):
        return QString("images/Terminal.png")

    def view(self, hostList):
        for host in hostList:
            print "ssh %s" % host.name()
            QProcess.startDetached("exo-open --launch TerminalEmulator ssh %s" % host.name())

HostViewerFactory.registerPlugin( SshViewerPlugin() )

