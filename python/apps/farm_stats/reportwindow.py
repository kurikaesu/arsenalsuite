
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.uic import *
from blur.Stonegui import *
from blur.reports import *

class ReportWindow(QMainWindow):
	def __init__(self,parent = None):
		QMainWindow.__init__(self,parent)
		loadUi("reportwindowui.ui",self)
		self.ReportView = RecordTreeView(self)
		self.ReportView.header().setStretchLastSection(False)
		self.ReportModel = RecordListModel(self)
		self.ReportView.setModel( self.ReportModel )
		self.setCentralWidget( self.ReportView )
		self.connect( self.mExitAction, SIGNAL( 'triggered()' ), self.close )
		self.connect( self.mExportSpreadsheetAction, SIGNAL( 'triggered()' ), self.exportSpreadsheet )
		
	def loadReport( self, report ):
		self.setWindowTitle( "Report: " + report.name )
		self.Model = report.createModel(self)
		self.ReportView.setModel( self.Model )
		self.Report = report
	
	def getTempFilename( self ):
		dt = QDateTime.currentDateTime().toString()
		od = '/tmp/'
		if sys.platform == 'win32':
			od = 'C:/temp/'
		return od + self.Report.name + '-' + dt
	
	def exportSpreadsheet( self ):
		#new spread sheet
		workbook  = Workbook()
		worksheet = workbook.add_sheet(self.Report.name)

		cc = self.Model.columnCount()
		
		row = 2
		worksheet.write(row, 1, self.Report.name + "%s - %s" % (self.Report.startDate.toString('MM/dd/yy'), self.Report.endDate.toString('MM/dd/yy')) )
		row += 1
		
		# Set a margin
		set_col_width(worksheet, 0, 50)
		for i in range(cc):
			set_col_width( worksheet, i + 1, self.ReportView.header().sectionSize(i) )
			worksheet.write( row, i + 1, str( self.Model.headerData( i, Qt.Horizontal, Qt.DisplayRole ).toString() ) )
		
		mi = ModelIter( self.Model )
		while mi.isValid():
			row += 1
			idx = mi.current()
			for i in range(cc):
				idx = idx.sibling( idx.row(), i )
				worksheet.write( row, i + 1, str( idx.data(Qt.DisplayRole).toString() ) )
			mi = mi.next()
		
		fn = self.getTempFilename()
		workbook.save( fn )
		self.preview( fn )
	
	def preview(self,fileName):
		if sys.platform == "win32":
			win32api.ShellExecute( 0, "open", fileName, None, '.', 0 )
		else:
			QProcess.startDetached( "oocalc", QStringList() << fileName )
	