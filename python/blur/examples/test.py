#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from blur.Stone import *
from blur.Classes import *
import sys

# First Create a Qt Application
app = QApplication(sys.argv)

# Load database config
initConfig("c:\\blur\\resin\\resin.ini")

# Now we can use the database classes

# This function updates all timesheets that have fkeyproject=0
# it sets fkeyproject to the same project as the nearest timesheet
# for that user
def timesheet_to_nearest_project():
	for ts in TimeSheet.select( "WHERE fkeyproject=0" ):
		# Find closest timesheet
		print( "TimeSheet " + ts.user().displayName() + ":" + ts.dateTime().toString() + " has 0 fkeyproject" )
		close = TimeSheet.select( "WHERE fkeyproject!=0 AND fkeyemployee=? ORDER BY abs(?::date-datetime::date) ASC LIMIT 1",
			[ QVariant(ts.user().key())
			 ,QVariant(ts.dateTime().date())] )

		if close.size() == 1:
			c = close[0]
			print( "Closest timesheet was " + c.dateTimeSubmitted().toString() + " with project " + c.project().name() )
			ts.setProject( c.project() )
			ts.commit()
		else:
			print( "Couldn't find another timesheet to set fkeyproject" )

#timesheet_to_nearest_project()


for hl in HostLoad.select():
	print hl.host().name() + ": " + str(hl.loadAvg) + " " + str(hl.loadAvgAdjust)

