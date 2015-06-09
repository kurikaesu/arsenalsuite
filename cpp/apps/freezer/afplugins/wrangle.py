
from blur.Stone import *
from blur.Classes import *
from blur.Freezer import *
from PyQt4.QtCore import *
from PyQt4.QtSql import *
import traceback, os
import subprocess

class WranglerViewerPlugin(JobViewerPlugin):
    def __init__(self):
        JobViewerPlugin.__init__(self)

    def name(self):
        return QString("Toggle wrangle status")

    def icon(self):
        return QString("images/wrangled.png")

    def view(self, jobList):
        for job in jobList:
            if job.wrangler() == User.currentUser():
                # toggle wrangle to off
                job.setWrangler(User())
                job.commit()

                jh = JobHistory()
                jh.setHost(Host.currentHost())
                jh.setUser(User.currentUser())
                jh.setJob(job)
                jh.setMessage("Wrangler activity on job finished")
                jh.commit()
            else:
                # toggle on and log history
                job.setWrangler(User.currentUser())
                job.commit()

                jh = JobHistory()
                jh.setHost(Host.currentHost())
                jh.setUser(User.currentUser())
                jh.setJob(job)
                jh.setMessage("Wrangler activity on job began")
                jh.commit()

JobViewerFactory.registerPlugin( WranglerViewerPlugin() )
