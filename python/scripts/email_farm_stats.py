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
	initConfig("/etc/email_farm_stats.ini", "/var/log/ab/email_farm_stats.log")
	# Read values from db.ini, but dont overwrite values from email_calendar_data.ini
	# This allows db.ini defaults to work even if email_calendar_data.ini is non-existent
	config().readFromFile( "/etc/db.ini", False )

classes_loader()

Database.current().connection().reconnect()

tabSize = 8

def column_format( column_widths, data_list ):
	ret = ''
	next_data_prepend = ''
	width_iter = iter(column_widths)
	for data in data_list:
		data = next_data_prepend + data
		ret += data
		width = width_iter.next()
		if len(data) > width:
			next_data_prepend = ' '
		else:
			next_data_prepend = ''
			extra = width - len(data)
			tabs = (extra + tabSize - 1) / tabSize
			ret += '\t' * tabs
	return ret

def padString(s,pl):
	if( len(s) < pl ):
		s = (' ' * (pl - len(s))) + s
	return s

def send_stats_email():
	# Get calendars for the date
	stats = Database.current().exec_( "SELECT * FROM hosthistory_user_slave_summary( (now() - '1 week'::interval)::timestamp, now()::timestamp );" );
	
	subject = 'Farm Stats for ' + QDateTime.currentDateTime().toString()
	body = subject + '\n\n\n'
	
	empty = True
	
	format = [24, 24, 1]
	body += 'User Slave Summary\n\n'
	body += column_format( format, ['User','Host','Slaved Hours'] ) + '\n'
	while stats.next():
		line = column_format( format, [stats.value(0).toString(), stats.value(1).toString(), padString(stats.value(2).toString(),6)] ) + '\n'
		body += line
	
	
	stats = Database.current().exec_( "select usr.name, sum(coalesce(totaltasktime,'0'::interval)) as totalrendertime, sum(coalesce(totalerrortime,'0'::interval)) as totalerrortime, sum(coalesce(totalerrortime,'0'::interval))/sum(coalesce(totaltasktime,'0'::interval)+coalesce(totalerrortime,'0'::interval)) as errortimeperc from jobstat, usr where started > 'today'::timestamp - '7 days'::interval and ended is not null and fkeyusr=keyelement group by usr.name order by errortimeperc desc;" )
	
	format = [24, 24, 24, 1]
	body += '\n\nUser Render Time Summary\n\n'
	body += column_format( format, ['User','Total Render Time','Total Error Time','Percent Error Time'] ) + '\n'
	
	while stats.next():
		data = []
		for i in range(4):
			s = stats.value(i).toString()
			if i in [1,2]:
				i = Interval.fromString(s)[0]
				data.append(padString(i.toString(Interval.Hours,Interval.Seconds), 10))
			elif i==3:
				data.append('%1.2f' % float(s))
			else:
				data.append(s)
		body += column_format( format, data ) + '\n'
	
	#print body
	blur.email.send( 'thePipe@blur.com', ['newellm@blur.com','duane@blur.com', 'dwilson@blur.com'], subject, body )


def check_date( name, date ):
	if date.isValid():
		return True
	print name, "is an invalid date"
	usage()
	return False

if __name__ == "__main__":
	send_stats_email()
	sys.exit(0)
	
	
