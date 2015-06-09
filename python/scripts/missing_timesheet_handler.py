#!/usr/bin/python

from PyQt4.QtCore import *
from blur.Stone import *
from blur.Classes import *
import sys, re, blur.email, blur.jabber

app = None
dryRun = False

def init():
	global app
	global dryRun
	# First Create a Qt Application
	app = QCoreApplication(sys.argv)
	
	# Load database config
	if sys.platform=='win32':
		initConfig("c:\\blur\\resin\\resin.ini")
	else:
		initConfig("/etc/missing_timesheet_handler.ini", "/var/log/ab/missing_timesheet_handler.log")
		config().readFromFile( "/etc/db.ini", False ) #Backup read, in case of no missing_timesheet_handler.ini
	
	classes_loader()
	
	Database.current().connection().reconnect()

	if '-dry-run' in sys.argv:
		dryRun = True

def checkTimesheetOnDate( user, date ):
	tsl = TimeSheet.select( "fkeyemployee=? AND datetime=?", [QVariant(user.key()),QVariant(date)] )
	return tsl.size()

vacationCat = None
# Returns true if vacation timesheets were created for this user/date
def checkVacationMigration( user, date ):
	global vacationCat
	if vacationCat is None:
		vacationCat = AssetType.recordByName( 'Vacation' )
		virtualProject = Project.recordByName( "_virtual_project" );
	vacSchedules = Schedule.select( "fkeyassettype=? AND date=? AND fkeyuser=?", [QVariant(vacationCat.key()), QVariant(date), QVariant(user.key())] )
	if vacSchedules.isEmpty():
		return False
	ts = TimeSheet()
	ts.setUser(user)
	ts.setDateTime(QDateTime(date))
	ts.setScheduledHour( 8 )
	ts.setElement( virtualProject )
	ts.setProject( virtualProject )
	ts.commit()
	print "Created Vacation Timesheet from vacation schedule for ", user.fullName(), date.toString()
	return True

def checkTimesheetsForUser( user, firstDate, lastDate ):
	date = firstDate
	missingDates = []
	while date <= lastDate:
		if isWorkday(date) and user.dateOfHire() < date and checkTimesheetOnDate( user, date )==0 and not checkVacationMigration( user, date ):
			missingDates.append(date)
		date = date.addDays(1)
	return missingDates

def getUsersToCheck():
	return Employee.select( "WHERE coalesce(disabled,0)=0 and dateoftermination is null and dateofhire is not null and keyelement not in (SELECT fkeyusr from usrgrp WHERE fkeygrp IN(SELECT keygrp FROM grp WHERE grp IN ('IT','Design','HR','Production','No_Timesheets')))" )
	#ret = EmployeeList()
	#for name in ['newellm','duane']:
	#	ret += Employee.recordByUserName(name)
	#return ret

def main():
	# Check the last 30 days
	currentDate = QDate.currentDate()
	lastDate = currentDate.addDays(-3)
	firstDate = lastDate.addDays(-31)
	msgLog = ''
	for user in getUsersToCheck():
		if not user.isRecord(): continue
		missingDates = checkTimesheetsForUser( user, firstDate, lastDate )
		if missingDates:
			msg = "missing timesheets on the following dates\n" + '\n'.join([str(date.toString()) for date in missingDates])
			print "User", user.displayName(), "is", msg
			msg = "You have " + msg + "\nPlease open Resin and post your time." #+ "\nresin://viewName=Calendar"
			jid = str(user.name() + '@jabber.blur.com')
			if not dryRun:
				blur.jabber.send('thepipe@jabber.blur.com/Timesheets','thePIpe', jid, msg )
			msgLog += 'Jabber sent to ' + jid + '\n' + msg + '\n\n'
	print msgLog
	if not dryRun:
		blur.email.send( 'thePipe@blur.com', ['newellm@blur.com','duane@blur.com','pat@blur.com'], 'Missing Timesheet Jabber Log', msgLog )

if __name__=="__main__":
	init()
	main()

