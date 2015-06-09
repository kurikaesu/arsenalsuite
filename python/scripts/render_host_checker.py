#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *
import blur.email, blur.jabber
import sys, time, re
from math import ceil
try:
	import popen2
except: pass

if sys.argv.count('-daemonize'):
	from blur.daemonize import createDaemon
	createDaemon()

app = QCoreApplication(sys.argv)

initConfig( "/etc/render_host_checker.ini", "/var/log/ab/render_host_checker.log" )
# Read values from db.ini, but dont overwrite values from render_host_checker.ini
# This allows db.ini defaults to work even if render_host_checker.ini is non-existent
config().readFromFile( "/etc/db.ini", False )

initStone(sys.argv)

blur.RedirectOutputToLog()

classes_loader()

VERBOSE_DEBUG = False

if VERBOSE_DEBUG:
	Database.instance().setEchoMode( Database.EchoUpdate | Database.EchoDelete )# | Database.EchoSelect )

Database.current().connection().reconnect()

class HostStat:
	def __init__(self):
		self.errorTime = 0
		self.successTime = 0
		self.errorCount = 0
		self.errorJobCount = 0
		self.successCount = 0
		self.successJobCount = 0

def get_error_string(jobError):
	dt = QDateTime()
	dt.setTime_t(jobError.errorTime())
	return str(dt.toString() + '\n' + jobError.message())

def find_fucked_hosts( interval ):
	statsByHost = {}
	ret = []
	ret_errors = []
	q = Database.current().exec_('SELECT fkeyhost, count(distinct(fkeyjob)) as jobcount, extract(epoch from sum(duration)), count(*),' +
				" ((fkeyjobtask is not null and success is null) or (fkeyjobtask is null and status='busy' and nextstatus!='busy')) as error" +
				" FROM hosthistory WHERE duration IS NOT NULL AND nextstatus IS NOT NULL AND status='busy' AND datetime > now() - '%s'::interval GROUP BY fkeyhost, error;" % interval.toString() )
	while q.next():
		fkeyhost = q.value(0).toInt()[0]
		jobCount = q.value(1).toInt()[0]
		duration = q.value(2).toInt()[0]
		count = q.value(3).toInt()[0]
		error = q.value(4).toBool()
		if not fkeyhost in statsByHost:
			statsByHost[fkeyhost] = HostStat()
		if error:
			statsByHost[fkeyhost].errorTime = duration
			statsByHost[fkeyhost].errorCount = count
			statsByHost[fkeyhost].errorJobCount = jobCount
		else:
			statsByHost[fkeyhost].successTime = duration
			statsByHost[fkeyhost].successCount = count
			statsByHost[fkeyhost].successJobCount = jobCount
	
	for fkeyhost, stats in statsByHost.iteritems():
		errorTimeRatio = stats.errorTime / float(max(1,stats.successTime + stats.errorTime))
		errorCountRatio = stats.errorCount / float(max(1,stats.successCount + stats.errorCount))
		#keeps a couple large error times(could be exceeded max task minutes) from triggering false positives in most cases
		if (errorTimeRatio > .75 or errorCountRatio > .5) and (stats.errorCount > 1 or stats.errorTime > 60) and (stats.errorJobCount > 1):
			h = Host(fkeyhost)
			msg = "%s\t\t%i\t\t%i\t\t%i\t\t%i\t\t%i%%\t\t%i%%" % (h.name(), stats.errorTime / 60, stats.errorCount, stats.successTime / 60, stats.successCount, int(errorTimeRatio*100), int(errorCountRatio*100))
			print msg
			print errorTimeRatio, errorCountRatio 
			ret.append(msg)
			errors = JobError.select("fkeyhost=%i order by errortime desc limit 10" % fkeyhost)
			ret_errors.append( str(h.name() + '\n' + '\n'.join([get_error_string(e) for e in errors])) )
	return (ret, ret_errors)

def runOnce():
	(fucked_hosts_messages, fucked_hosts_errors) = find_fucked_hosts( Interval.fromString('%i hours' % config.loopPeriod)[0] )
	
	if fucked_hosts_messages:
		msg = """
	Here are the crispy hosts for the last 12 hours
	A host is considered crispy if it's error time % is larger than 75%
	or it's error count % is larger than 50%
	
	Host\t\t\tError Time\tError Count\tSuccess Time\tSuccess Count\tError Time %\tError Count %
	"""
		msg += '\n'.join(fucked_hosts_messages) + "\n\n"
		msg += '\n\n'.join(fucked_hosts_errors) + "\n"
		Log(msg)
		Notification.create('render_host_checker','crispy host report', 'Crispy Hosts Report', msg )
		#blur.email.send( sender = 'thePipe@blur.com', ['it@blur.com'], 'Crispy Hosts Report', msg)
	else:
		Log('No crispy hosts at this time')

class RHCConfig:
	def update(self):
		self.loopPeriod = Config.getInt('renderHostCheckerLoopPeriodHours',12)
		
config = RHCConfig()
lastRun = QDateTime()
while True:
	config.update()
	if lastRun.isNull() or lastRun.secsTo( QDateTime.currentDateTime() ) / (60 * 60) > config.loopPeriod:
		lastRun = QDateTime.currentDateTime()
		runOnce()
	time.sleep(60)
	
