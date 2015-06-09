#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from blur.Stone import *
from blur.Classes import *
import sys, re, blur.email

# First Create a Qt Application
app = QCoreApplication(sys.argv)

# Load database config
if sys.platform=='win32':
	initConfig("c:\\blur\\resin\\resin.ini")
else:
	initConfig("/etc/email_calendar_data.ini", "/var/log/ab/email_calendar_data.log")
	# Read values from db.ini, but dont overwrite values from email_calendar_data.ini
	# This allows db.ini defaults to work even if email_calendar_data.ini is non-existent
	config().readFromFile( "/etc/db.ini", False )

classes_loader()

Database.current().connection().reconnect()

def send_calendar_email( date ):
	# Get calendars for the date
	cals = Calendar.select( "date::date = ?", [QVariant(date)] )
	
	# Group by type
	grouped = cals.groupedBy( "fkeycalendarcategory" )
	
	# Get a sorted list of types
	cats = cals.categories().unique().sorted( "keycalendarcategory" )
	
	subject = 'Calendar Events for ' + date.toString()
	body = subject + '\n\n\n'
	
	empty = True
	
	for cat in cats:
		cat = CalendarCategory(cat)
		# Dont send personal events
		if cat.name() == 'Personal':
			continue
		
		entries = grouped[QString.number(cat.key())]
		body += cat.name() + "\n\n"
		for entry in entries:
			empty = False
			entry = Calendar(entry)
			body += entry.calendar() + "\n"
			body += "Created By: " + entry.user().displayName()
			body += "\n\n"
		body += "\n"
	
	if empty:
		print "Not sending email(No Events Found) for date: " + date.toString()
	else:
		blur.email.send( 'thePipe@blur.com', ['calendar@blur.com'], subject, body )

def send_emails( startDate, endDate ):
	if startDate > endDate:
		startDate = endDate
		
	print "Sending emails from " + startDate.toString() + " to " + endDate.toString()
	while startDate <= endDate:
		send_calendar_email( startDate )
		startDate = startDate.addDays(1)

def usage():
	print "Usage: python email_calendar_data.py START_DATE END_DATE"
	print " START_DATE and END_DATE are ISO Date format YYYY-MM-DD"
	
def check_date( name, date ):
	if date.isValid():
		return True
	print name, "is an invalid date"
	usage()
	return False

if __name__ == "__main__":
	
	if len( sys.argv ) < 3:
		print "Insufficient arguements"
		usage()
		sys.exit(1)
	
	dateStart = QDate.fromString( sys.argv[1], Qt.ISODate )
	dateEnd = QDate.fromString( sys.argv[2], Qt.ISODate )
	
	if check_date("START_DATE",dateStart) and check_date("END_DATE",dateEnd):
		send_emails( dateStart, dateEnd )
	else:
		sys.exit(1)
	
	
