
from PyQt4 import QtGui
from blur.Stonegui import RecordListModel

class ServiceModel(RecordListModel):
	def __init__(self,parent = None):
		RecordListModel.__init__(self,parent)
		
		self.setRecordList( Service.select() )
		self.setAssumeChildren( True )
		self.setColumns( ['Service','Enabled','Pulse'] )
		
	def children(self,record):
		if Service(record).isRecord():
			return Service(record).hostServices()
		return RecordList()
	
	def data(self,record,role,column):
		if Service(record).isRecord() and column=='Pulse':
			return Service(record).forbiddenProcesses()
		return RecordListModel.data(self,record,role,column)

