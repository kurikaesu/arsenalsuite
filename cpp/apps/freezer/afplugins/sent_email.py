
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
        return QString("Indicate that an email has been sent regarding the job")

    def icon(self):
        return QString("images/emailed.png")

    def view(self, jobList):
        for job in jobList:
            if job.toggleFlags() & 0x00000001:
                # toggle wrangle to off
                job.setToggleFlags(job.toggleFlags() ^ 0x00000001)
                job.commit()

                jh = JobHistory()
                jh.setHost(Host.currentHost())
                jh.setUser(User.currentUser())
                jh.setJob(job)
                jh.setMessage("Removed 'email sent' flag")
                jh.commit()
            else:
                # toggle on and log history
                job.setToggleFlags(job.toggleFlags() ^ 0x00000001)
                job.commit()

                jh = JobHistory()
                jh.setHost(Host.currentHost())
                jh.setUser(User.currentUser())
                jh.setJob(job)
                jh.setMessage("Email sent toggled")
                jh.commit()

JobViewerFactory.registerPlugin( ToDeleteViewerPlugin() )
