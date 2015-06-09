from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *

from verifier_plugin_factory import *

import ConfigParser
import os, sys, re

sys.path.append("/drd/software/int/sys/renderops")

from wrapi import wrasql_client_api
#from wrapi import wenv

def jobMatchesRule(job, rule, hosts):
	if job.name().toLower().contains(rule[0]) or re.search(rule[0], str(job.name().toLower())) or rule[0] == "%":
                        if job.user().name().toLower().contains(rule[1]) or rule[1] == "%":
                                if job.jobType().name().toLower().contains(rule[2]) or rule[2] == "%":
					if job.project().shortName().toLower().contains(rule[4]) or rule[4] == "%":
						if not hosts or len(hosts) == 0:
							return True

	return False
'''
					hostCount = len(job.hostList().names())
					reqCount = 0

					for host in job.hostList().names():
						for rhosts in hosts:
							if host.contains(rhosts):
								reqCount += 1
								#return True
						
					if reqCount >= hostCount:
						return True

	return False
'''

def adjustPriority(job):
	if job.name().contains("_fastlane"):
		return True

	curShotPrios = {}
	extra_args = {}
	exceptions = {}

	curShotPrios = []
	exceptions = []

	wmap = {}
	wclient = wrasql_client_api.wrasql_client("drops01", wmap)
	wclient.query("SELECT shot, user_name, type, priority_level, project FROM priority_rules")
	curShotPrios = wclient.poll_for_query_results()

	wclient.query("SELECT override, val FROM priority_overrides")
	overrides = wclient.poll_for_query_results()

	for over in overrides:
		extra_args[over[0]] = over[1]

	wclient.query("SELECT shot, user_name, type, override FROM priority_exceptions")
	exceptions = wclient.poll_for_query_results()

	for exc in exceptions:
		if jobMatchesRule(job, exc, None):
			if exc[3] == "ignore":
				return True

			print "Verifier - Set overriden priority %s to %s's %s job %d - %s" % (exc[3], job.user().name(), job.jobType().name(), job.key(), job.name())
			job.setPriority(int(exc[3]))
			job.commit()
			return True

	foundRule = False
	lowestPrio = 999
	for prios in curShotPrios:
		if jobMatchesRule(job, prios, None):
			foundRule = True
			if int(prios[3]) < lowestPrio:
				lowestPrio = int(prios[3])
	
	if foundRule:
		if len(job.jobTasks().statuses()) == 1:
			if "single_priority_frame_priority" in extra_args:
				print "Verfifier - Set priority %s to %s's single frame priority %s job %d - %s" % (extra_args["single_priority_frame_priority"], job.user().name(), job.jobType().name(), job.key(), job.name())
				job.setPriority(int(extra_args["single_priority_frame_priority"]))
				job.commit()
				return True

		print "Verfifier - Set priority %s to %s's %s job %d - %s" % (lowestPrio, job.user().name(), job.jobType().name(), job.key(), job.name())
		job.setPriority(lowestPrio)
		job.commit()
		return True

	if len(job.jobTasks().statuses()) == 1:
		if "single_frame_priority" in extra_args:
			print "Verfifier - Set priority %s to %s's single frame %s job %d - %s" % (extra_args["single_frame_priority"], job.user().name(), job.jobType().name(), job.key(), job.name())
			job.setPriority(int(extra_args["single_frame_priority"]))
			job.commit()
	
	return True

VerifierPluginFactory().registerPlugin("adjustPriority", adjustPriority)
