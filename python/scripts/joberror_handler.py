#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtSql import *
from blur.Stone import *
from blur.Classes import *
import blur.email, blur.jabber
import sys, time, re, os
from math import ceil
import traceback

if sys.argv.count('-daemonize'):
	from blur.daemonize import createDaemon
	createDaemon()

app = QCoreApplication(sys.argv)

initConfig( "/etc/joberror_handler.ini", "/var/log/ab/joberror_handler.log" )
# Read values from db.ini, but dont overwrite values from reaper.ini
# This allows db.ini defaults to work even if reaper.ini is non-existent
config().readFromFile( "/etc/db.ini", False )

blur.RedirectOutputToLog()

classes_loader()

VERBOSE_DEBUG = False

if VERBOSE_DEBUG:
	Database.current().setEchoMode( Database.EchoUpdate | Database.EchoDelete )# | Database.EchoSelect )

Database.current().connection().reconnect()

service = Service.ensureServiceExists('AB_JobErrorHandler')

def perform_script_action( handler_script, error ):
	func = getCompiledFunction( 'handleError', handler_script, handler_script.script(), handler_script.name() )
	if func and callable(func):
		func( error )

def handle_error( handler, error ):
	# Does the handler match the error?
	if error.message().contains( handler.errorRegEx() ):
		perform_script_action( handler.jobErrorHandlerScript(), error )
	
def joberror_handler():
#	Config: managerDriveLetter, managerSpoolDir, assburnerErrorStep
	print "JobError Handler is starting up"
	
	while True:
		service.pulse()
		
		errorsToCheck = JobError.select( "checked=false" )
		
		handlersByJobType = {}
		jobTypeByJob = {}
		
		for error in errorsToCheck:
			# Get the job key, but don't load the job(if we don't have to)
			jobKey = error.getValue('fkeyJob').toInt()[0]
			
			# Hmm, if we haven't looked at the job yet, then we have to load
			# it to figure out the jobtype.
			if not jobKey in jobTypeByJob:
				job = error.job()
				jobTypeByJob[job.key()] = job.jobType()
			
			# Get the jobtype
			jobType = jobTypeByJob[jobKey]
			if not jobType.isRecord():
				continue
			
			# Load the handlers for this jobtype if not already loaded
			if not jobType in handlersByJobType:
				handlersByJobType[jobType] = jobType.jobErrorHandlers()
			
			# Run each handler
			for handler in handlersByJobType[jobType]:
				handle_error( handler, error )
			
		errorsToCheck.setChecked( True )
		errorsToCheck.commit()
		
		time.sleep(5)
		
if __name__ == "__main__":
	joberror_handler()
