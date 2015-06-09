#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *
import blur.email, blur.jabber
import sys, time, re, os
from math import ceil
import traceback

try:
	import popen2
except: pass

if sys.argv.count('-daemonize'):
	from blur.daemonize import createDaemon
	createDaemon()

app = QCoreApplication(sys.argv)

initConfig( "/etc/rrd_stats_collector.ini", "/var/log/ab/rrd_stats_collector.log" )
# Read values from db.ini, but dont overwrite values from rrd_stats_collecter.ini
# This allows db.ini defaults to work even if rrd_stats_collecter.ini is non-existent
config().readFromFile( "/etc/db.ini", False )

blur.RedirectOutputToLog()

classes_loader()

VERBOSE_DEBUG = False

if VERBOSE_DEBUG:
	Database.instance().setEchoMode( Database.EchoUpdate | Database.EchoDelete )# | Database.EchoSelect )

Database.current().connection().reconnect()

db = Database.current()

def write_stats( val_dict, filename ):
	rrd = open( filename, "w" )
	for k in val_dict:
		rrd.write( "%s.value %0.3g\n" % (k, val_dict[k] * 100.0) )
	del rrd
	
while True:
	q = db.exec_( "SELECT * FROM hosthistory_status_percentages( 'hosthistory_timespan_duration_adjusted( (now() - ''30 minutes''::interval)::timestamp, now()::timestamp, ''(status!=''''busy'''' or duration is not null)'', '''' )' );" )
	
	total = {}
	non_idle = {}
	non_error = {}
	while q.next():
		status = q.value(0).toString()
		loading = q.value(1).toBool()
		errored = q.value(2).toBool()
		if loading:
			status += 'Loading'
		if errored:
			status += 'Errored'
		if not q.value(4).isNull():
			total[status] = q.value(4).toDouble()[0]
		if not q.value(5).isNull():
			non_idle[status] = q.value(5).toDouble()[0]
		if not q.value(6).isNull():
			non_error[status] = q.value(6).toDouble()[0]
	
	write_stats( total, "/tmp/efficiency_total_lrrd" )
	write_stats( non_idle, "/tmp/efficiency_non_idle_lrrd" )
	write_stats( non_error, "/tmp/efficiency_non_error_lrrd" )

	time.sleep( 60 * 5 )

	
