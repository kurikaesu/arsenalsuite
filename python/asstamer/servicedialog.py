
from PyQt4.QtCore import Qt, QVariant
from PyQt4.QtGui import QDialog
from PyQt4 import uic
from blur.Classes import Service, ServiceList

def boolToCheckState(bool):
	if bool is True:
		return Qt.Checked
	return Qt.Unchecked

def checkStateToBool(cs):
	return cs == Qt.Checked

def optionalApply(serviceList, prop, cb):
	cs = cb.checkState()
	if cs != Qt.PartiallyChecked:
		serviceList.setValue(prop,QVariant(checkStateToBool(cs)))

class ServiceDialog(QDialog):
	def __init__(self,parent):
		QDialog.__init__(self,parent)
		uic.loadUi("ui/servicedialog.ui",self)
		
	def accept(self):
		if self.Services.size() == 1:
			self.Services.setServices(self.mServiceNameEdit.text())
			self.Services.setForbiddenProcesses(self.mForbiddenProcessesEdit.text())
			self.Services.setDescriptions(self.mDescriptionEdit.text())
		for cb, prop in {self.mEnabledCheck : 'enabled', self.mActiveCheck : 'active', self.mUniqueCheck : 'unique'}.iteritems():
			optionalApply( self.Services, prop, cb )
		self.Services.commit()
		QDialog.accept(self)
	
	def setServices(self,serviceList):
		self.Services = ServiceList(serviceList)
		multi = serviceList.size() > 1
		self.mServiceNameEdit.setEnabled(not multi)
		self.mForbiddenProcessesEdit.setEnabled(not multi)
		if not multi:
			self.mServiceNameEdit.setText( serviceList[0].service() )
			self.mForbiddenProcessesEdit.setText( serviceList[0].forbiddenProcesses() )
			self.mDescriptionEdit.setText( serviceList[0].description() )
		for cb in [self.mEnabledCheck, self.mActiveCheck, self.mUniqueCheck]:
			cb.setTristate(multi)
		for cb, prop in {self.mEnabledCheck : 'enabled', self.mActiveCheck : 'active', self.mUniqueCheck : 'unique'}.iteritems():
			vals = serviceList.getValue(prop)
			cs = None
			for val in vals:
				if cs is None or cs == boolToCheckState(val.toBool()):
					cs = boolToCheckState(val.toBool())
				else:
					cs = Qt.PartiallyChecked
			cb.setCheckState(cs)
			
