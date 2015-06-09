from blur.Stone import *
from blur.Classes import *
from blur.Freezer import *
from PyQt4.QtCore import *
from PyQt4.QtGui import *
import re, traceback

class DelightSettingsWidget(CustomJobSettingsWidget):
	def __init__(self,parent):
		CustomJobSettingsWidget.__init__(self,parent)
		layout = QVBoxLayout(self)
		self.Label = QLabel( 'No additional settings for Delight Jobs', self )
		layout.addWidget( self.Label )
		layout.addStretch()
	
class DelightSettingsWidgetPlugin(CustomJobSettingsWidgetPlugin):
	def __init__(self):
		CustomJobSettingsWidgetPlugin.__init__(self)

	def jobTypes(self):
		return QStringList('3Delight')

	def createCustomJobSettingsWidget(self,jobType,parent,foo):
		return DelightSettingsWidget(parent)

CustomJobSettingsWidgetsFactory.registerPlugin( DelightSettingsWidgetPlugin() )
