
from blur.Stone import *
from blur.Classes import *
from PyQt4.QtGui import *
from reportwindow import ReportWindow
from farmreport import *
from reportselectordialog import *
import os, sys

if __name__=='__main__':
	app = QApplication(sys.argv)
	if sys.platform == 'win32':
		initConfig('c:/blur/assfreezer/assfreezer.ini')
	else:
		initConfig( '/etc/farm_report.ini', '/var/log/farm_report.log' )
		# Read values from db.ini, but dont overwrite values from energy_saver.ini
		# This allows db.ini defaults to work even if energy_saver.ini is non-existent
		config().readFromFile( "/etc/db.ini", False )
	classes_loader()
	rsd = ReportSelectorDialog()
	rsd.show()
	sys.exit(app.exec_())
	
