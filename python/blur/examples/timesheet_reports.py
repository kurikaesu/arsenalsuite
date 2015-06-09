#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from blur.Stone import *
from blur.Classes import *
import sys
import blur.reports

# First Create a Qt Application
app = QApplication(sys.argv)

# Load database config
if sys.platform=='win32':
	initConfig("c:\\blur\\resin\\resin.ini")
else:
	initConfig("/etc/db.ini")

report_dict = {}
for report in blur.reports.ReportList:
	if report:
		report_dict[report.Name] = report

def usage():
	print 'timesheet_reports.py SQL_WHERE REPORT_NAME OUTPUT_FILE'
	print 'REPORT_NAME can be ' + ', '.join(report_dict.keys())
	print 'example:'
	print """timesheet_reports.py "datetime >= now() - '7 days'::interval" dump test.xls"""

if len(sys.argv) != 4:
	usage()
	sys.exit(1)

if sys.argv[2].lower() not in report_dict:
	print "Invalid REPORT_NAME"
	usage()
	sys.exit(1)

tsl = TimeSheet.select( sys.argv[1] )
minDate, maxDate = None, None
for ts in tsl:
	if not minDate or ts.dateTime() < minDate:
		minDate = ts.dateTime()
	if not maxDate or ts.dateTime() > maxDate:
		maxDate = ts.dateTime()

report_dict[sys.argv[2].lower()].Function( minDate.date(), maxDate.date(), tsl, recipients=[], filename = sys.argv[3] )



