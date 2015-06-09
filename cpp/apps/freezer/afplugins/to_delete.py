
from blur.Stone import *
from blur.Classes import *
from blur.Freezer import *
from PyQt4.QtCore import *
from PyQt4.QtSql import *
import traceback, os
import subprocess

class ToDeleteViewerPlugin(JobViewerPlugin):
    def __init__(self):
        JobViewerPlugin.__init__(self)

    def name(self):
        return QString("Indicate Job to be Deleted status")

    def icon(self):
        return QString("images/todelete.png")

    def view(self, jobList):
        for job in jobList:
            if job.toggleFlags() & 0x00000010:
                # toggle wrangle to off
                job.setToggleFlags(job.toggleFlags() ^ 0x00000010)
                job.commit()

                jh = JobHistory()
                jh.setHost(Host.currentHost())
                jh.setUser(User.currentUser())
                jh.setJob(job)
                jh.setMessage("Removed 'To be deleted' status")
                jh.commit()
            else:
                # toggle on and log history
                job.setToggleFlags(job.toggleFlags() ^ 0x00000010)
                job.commit()

                jh = JobHistory()
                jh.setHost(Host.currentHost())
                jh.setUser(User.currentUser())
                jh.setJob(job)
                jh.setMessage("Marked job for future deletion")
                jh.commit()

JobViewerFactory.registerPlugin( ToDeleteViewerPlugin() )
