
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.uic import *
from blur.Stonegui import *
from farmreport import Types
from reportwindow import *

ReportWindows = []

class ReportSelectorDialog(QDialog):
	def __init__(self,parent=None):
		QDialog.__init__(self,parent)
		loadUi("reportselectordialogui.ui",self)
		dt = QDateTime.currentDateTime()
		self.mStart.setDateTime( dt.addDays(-7) )
		self.mEnd.setDateTime( dt )
		
		for reportType in Types:
			self.mReportList.addItem( reportType.Name )
			
		self.connect( self.mExitButton, SIGNAL( 'clicked()' ), self.close )
		self.connect( self.mGenerateButton, SIGNAL( 'clicked()' ), self.generateReport )
		self.mGenerateButton.setEnabled( False )
		self.connect( self.mReportList, SIGNAL( 'currentRowChanged(int)' ), self.currentChanged )
		
	def currentChanged( self, row ):
		self.mGenerateButton.setEnabled( row >= 0 )
	
	def generateReport( self ):
		reportName = self.mReportList.currentItem().text()
		for reportType in Types:
			if reportType.Name == reportName:
				report = reportType.Class()
				report.generate( self.mStart.dateTime(), self.mEnd.dateTime() )
				rw = ReportWindow()
				rw.loadReport( report )
				rw.show()
				ReportWindows.append(rw)
				return
	
	