
from PyQt4 import QtCore,QtGui
from PyQt4.QtCore import Qt, QRect, QDateTime
from PyQt4.QtGui import QImage, QPainter, QColor

from blur.Stonegui import RecordListModel, ModelIter
from blur.Classes import Service, HostService
from blur.Stone import RecordList, Database

ServiceStateOnline = 1
ServiceStateOffline = 2
ServiceStateError = 3
ServiceStateConflict = 4

class ServiceModel(RecordListModel):
	def __init__(self,parent = None):
		RecordListModel.__init__(self,parent)
		self.ServiceFilter = '^AB_'
		self.setColumns( ['Service','Enabled','Pulse'] )
		self.ServiceStateIcons = {}
		self.ServiceStateIcons[ServiceStateOnline] = self.createOnlineIcon(Qt.green)
		self.ServiceStateIcons[ServiceStateOffline] = self.createOnlineIcon(Qt.black)
		self.ServiceStateIcons[ServiceStateError] = self.createOnlineIcon(Qt.red)
		self.ServiceStateIcons[ServiceStateConflict] = self.createOnlineIcon(QColor(200, 100, 100))
		self.RefreshTime = QDateTime.currentDateTime()
		self.setRootList(self.getServiceList())
		
		self.HostServiceTable = Database.current().tableByName('HostService')
		self.HostServiceIndex = self.HostServiceTable.indexFromSchema( self.HostServiceTable.schema().field('fkeyService').index() )
		self.HostServiceIndex.setCacheEnabled( True )
		
	def getServiceList(self):
		return Service.select("service~'%s'" % self.ServiceFilter)
	
	def refresh(self):
		self.RefreshTime = QDateTime.currentDateTime()
		self.updateRecords(self.getServiceList())
		self.HostServiceIndex.setCacheEnabled( False )
		for idx in ModelIter.collect(self):
			self.updateRecords(Service(self.getRecord(idx)).hostServices(),idx)
		self.HostServiceIndex.setCacheEnabled( True )
	
	def createOnlineIcon(self,color):
		dim = QRect(0,0,32,32)
		img = QImage(dim.size(),QImage.Format_ARGB32)
		p = QPainter(img)
		p.setCompositionMode(QPainter.CompositionMode_Source)
		p.fillRect(dim,Qt.transparent)
		p.setCompositionMode(QPainter.CompositionMode_SourceOver)
		p.setRenderHints(QPainter.Antialiasing)
		p.setPen(Qt.black)
		p.setBrush(color)
		p.drawEllipse(dim.adjusted(1,1,-2,-2))
		return img

	def children(self,record):
		if Service(record).isRecord():
			return Service(record).hostServices()
		return RecordList()
	
	def serviceState(self,record):
		ret = ServiceStateOffline
		s = Service(record)
		hs = HostService(record)
		if s.isRecord() and s.enabled():
			ret = ServiceStateOnline
			if s.unique() and s.active():
				onlineHostCount = 0
				for hs in Service(record).hostServices():
					if self.serviceState(hs)==ServiceStateOnline:
						onlineHostCount += 1
				if onlineHostCount > 1:
					ret = ServiceStateConflict
				elif onlineHostCount < 1:
					ret = ServiceStateError
		elif hs.isRecord() and hs.enabled():
			if hs.pulseDateTime().secsTo(self.RefreshTime) > (60 * 10):
				ret = ServiceStateError
			else: ret = ServiceStateOnline
		return ret
	
	def serviceStateIcon(self,record):
		return self.ServiceStateIcons[self.serviceState(record)]
	
	def recordData(self,record,role,column):
		# Online/Offline Icon
		if role == Qt.DecorationRole and column=='Service':
			return QtCore.QVariant(self.serviceStateIcon(record))
		
		if role in [Qt.DisplayRole,Qt.EditRole]:
			if Service(record).isRecord() and column=='Pulse':
				return QtCore.QVariant(Service(record).forbiddenProcesses())
			if HostService(record).isRecord() and column=='Service':
				return QtCore.QVariant(HostService(record).host().name())
		return RecordListModel.recordData(self,record,role,column)

	def recordFlags(self,record,column):
		if column == 'Enabled':
			return Qt.ItemFlags(Qt.ItemIsEditable | Qt.ItemIsEnabled | Qt.ItemIsSelectable)
		return Qt.ItemFlags(Qt.ItemIsEnabled | Qt.ItemIsSelectable)
	
	def setRecordData(self,record,column,var,role):
		if column == 'Enabled' and role==Qt.EditRole:
			record.setValue('enabled',var)
			record.commit()
			return True
		return RecordListModel.setRecordData(self,record,column,var,role)
