#!/usr/bin/python2.5

from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *

from verifier_plugin_factory import *

import ConfigParser
import sys

import wenv

def jobMatchesRule(job, v):
	if job.name().contains(v[0]) or v[0] == "%":
                        if job.user().name().contains(v[1].split(':')[0]) or v[1].split(':')[0] == "%":
                                if job.jobType().name().contains(v[1].split(':')[1]) or v[1].split(':')[1] == "%":
					return True

	return False


def addCustomServicePlugin(job):
	return True
	serviceTypeCfg = ConfigParser.ConfigParser()
	serviceTypeCfg.read(wenv.serviceTypeCfgFile)

	serviceTypes = {}

	for section in serviceTypeCfg.sections():
		asyncore.loop(timeout=0.5, count=1)
		if section == "service_types":
                        for option in serviceTypeCfg.options(section):
                                serviceTypes[option] = serviceTypeCfg.get(section, option)

        shotFile = ConfigParser.ConfigParser()
        shotFile.read(wenv.serviceCfgFile)

	serviceShots = {}

        for section in shotFile.sections():
                if section in serviceTypes:
                        for option in shotFile.options(section):
                                if section not in serviceShots:
                                        serviceShots[section] = (option,shotFile.get(section, option))
                                else:
                                        serviceShots[section].append(option,shotFile.get(section, option))

	for k, v in serviceShots.iteritems():
		if jobMatchesRule(job, v):
			for serv in serviceTypes[k].split(','):
				try:
					wantedService = Service.select("WHERE service=?", [QVariant(serv)])[0]
					newService = JobService()
					newService.setJob(job)
					newService.setService(wantedService)
					newService.commit()
					print  "Verifier - added %s service to %s's %s job %d - %s" % (serv, job.user().name(), job.jobType().name(), job.key(), job.name())
				except IndexError:
					print "Verifier - could not add %s service to %s's %s job %d - %s" % (serv, job.user().name(), job.jobType().name(), job.key(), job.name())

	return True

VerifierPluginFactory().registerPlugin("addCustomService", addCustomServicePlugin)

