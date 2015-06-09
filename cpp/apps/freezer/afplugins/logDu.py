
from blur.Stone import *
from blur.Classes import *
from blur.Freezer import *
from PyQt4.QtCore import *
from PyQt4.QtSql import *
import traceback, os
import commands

class LogDuViewerPlugin(JobViewerPlugin):
    def __init__(self):
        JobViewerPlugin.__init__(self)

    def name(self):
        return QString("logDu")

    def icon(self):
        return QString("images/rv2.png")

    def view(self, jobList):
        for job in jobList:
            path = "/farm/logs/%s/%i" % (QString.number(job.key()).right(3), job.key())
            du = "du %s | cut -f1" % (path)
            try:
                size = int(commands.getoutput(du))
                if size > 100000:
                    print "%i    %s" % (size, path)
            except: pass

JobViewerFactory.registerPlugin( LogDuViewerPlugin() )
