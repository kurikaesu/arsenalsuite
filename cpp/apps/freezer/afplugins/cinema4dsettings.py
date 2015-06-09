from blur.Stone import *
from blur.Classes import *
from blur.Freezer import *
from PyQt4.QtCore import *
from PyQt4.QtGui import *
import re, traceback

class Cinema4DSettingsWidget(CustomJobSettingsWidget):
	def __init__(self,parent):
		CustomJobSettingsWidget.__init__(self,parent)
		layout = QVBoxLayout(self)
		self.Label = QLabel( 'No additional settings for Cinema4d Jobs', self )
		layout.addWidget( self.Label )
		layout.addStretch()
	
class Cinema4DSettingsWidgetPlugin(CustomJobSettingsWidgetPlugin):
	def __init__(self):
		CustomJobSettingsWidgetPlugin.__init__(self)

	def jobTypes(self):
		return QStringList('Cinema4D') << 'Cinema4D R11'

	def createCustomJobSettingsWidget(self,jobType,parent):
		Log( "Cinema4DSettingsWidgetPlugin::createCustomJobSettingsWidget() called, Creating Cinema4DSettingsWidget" )
		return Cinema4DSettingsWidget(parent)

CustomJobSettingsWidgetsFactory.registerPlugin( Cinema4DSettingsWidgetPlugin() )
