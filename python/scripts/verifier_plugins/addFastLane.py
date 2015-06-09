#!/usr/bin/python2.5

from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *

from verifier_plugin_factory import *

def addFastLanePlugin(job):
	if job.name().toLower().contains("fastlane"):

		job.setPriority(22)
		job.setMaxTaskTime(600)
		wantedService = Service.select("WHERE service='fastlane'")[0]
		serv = JobService()
		serv.setService(wantedService)
		serv.setJob(job)
		serv.commit()
		job.commit()

		print "Verifier - Placed job %d %s %s %s on the fast lane" % (job.key(), job.user().name(), job.jobType().name(), job.name())

	return True

VerifierPluginFactory().registerPlugin("addFastLane", addFastLanePlugin)
