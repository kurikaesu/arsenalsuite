
from PyQt4 import QtCore,QtGui,uic
from servicemodel import ServiceModel
from servicedialog import ServiceDialog
from blur.Classes import Service, ServiceList

class MainWindow(QtGui.QMainWindow):
	def __init__(self):
		QtGui.QMainWindow.__init__(self)
		uic.loadUi("ui/mainwindow.ui",self)
		
		self.setWindowTitle( 'AssTamer v' )
		self.ServiceModel = ServiceModel(self)
		print (self.ServiceModel.index(0,0).parent() == QtCore.QModelIndex())
		self.mServiceTree.setModel(self.ServiceModel)
		self.connect( self.mServiceTree, QtCore.SIGNAL('showMenu( const QPoint &, const Record &, RecordList )'), self.showServiceTreeMenu )
		
		self.setupActions()
		self.setupToolbar()
		self.setupMenu()
		
		self.RefreshTimer = QtCore.QTimer(self)
		self.connect( self.RefreshTimer, QtCore.SIGNAL('timeout()'), self.refresh )
		self.refresh()
		#self.RefreshTimer.start( 30 * 1000 )
	
	def refresh(self):
		self.ServiceModel.refresh()
		self.mServiceTree.viewport().update()
	
	def setAutoRefreshEnabled(self, enabled):
		if enabled:
			self.RefreshTimer.start( 30 * 1000 )
		else:
			self.RefreshTimer.stop()
	
	def setupActions(self):
		self.RefreshAction = QtGui.QAction('Refresh',self)
		self.connect( self.RefreshAction, QtCore.SIGNAL('triggered()'), self.refresh )
		self.QuitAction = QtGui.QAction('Quit',self)
		self.connect( self.QuitAction, QtCore.SIGNAL('triggered()'), QtGui.QApplication.instance(), QtCore.SLOT('quit()'))
		self.AutoRefreshAction = QtGui.QAction('Auto Refresh',self)
		self.AutoRefreshAction.setCheckable( True )
		self.connect( self.AutoRefreshAction, QtCore.SIGNAL('toggled(bool)'), self.setAutoRefreshEnabled )
	
	def setupToolbar(self):
		self.Toolbar = self.addToolBar('Main Toolbar')
		self.Toolbar.addAction( self.RefreshAction )
		
	def setupMenu(self):
		self.FileMenu = self.menuBar().addMenu( "File" )
		self.FileMenu.addAction( self.QuitAction )
		self.ViewMenu = self.menuBar().addMenu( "View" )
		self.ViewMenu.addAction( self.AutoRefreshAction )
		
	def updateActions(self):
		pass
	
	def showServiceTreeMenu(self, pos, recUnder, recsSel):
		menu = QtGui.QMenu(self)
		editServicesAction = None
		# All services
		if recsSel.size() == ServiceList(recsSel).size():
			editServicesAction = menu.addAction( "Edit Services" )
		
		result = menu.exec_(pos)
		
		if result and result==editServicesAction:
			sd = ServiceDialog(self)
			sd.setServices(recsSel)
			sd.exec_()

		
