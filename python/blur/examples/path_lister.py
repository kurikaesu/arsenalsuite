#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtSql import *
from PyQt4.QtGui import *
from blur.Stone import *
from blur.Classes import *
from blur.Classesui import *
import blur.email, blur.jabber
import sys, time, re, os
from math import ceil
import traceback

try:
	import popen2
except: pass

app = QApplication(sys.argv)

initConfig( "/etc/db.ini", "/var/log/path_lister.log" )

blur.RedirectOutputToLog()

blurqt_loader()

VERBOSE_DEBUG = False

if VERBOSE_DEBUG:
	Database.instance().setEchoMode( Database.EchoUpdate | Database.EchoDelete )# | Database.EchoSelect )

FreezerCore.instance().reconnect()

class ProjectChooserDialog( QDialog ):
	def __init__(self):
		QDialog.__init__(self)
		
		# Project Chooser Widget
		self.projectCombo = ProjectCombo(self)
		self.projectCombo.setShowSpecialItem(False)
		
		# Ok | Cancel buttons
		dbb = QDialogButtonBox( QDialogButtonBox.Ok | QDialogButtonBox.Cancel, Qt.Horizontal, self )
		self.connect( dbb, SIGNAL( 'accepted()' ), self.accept )
		self.connect( dbb, SIGNAL( 'rejected()' ), self.reject )
		
		# Layout
		l = QVBoxLayout(self)
		l.addWidget( self.projectCombo )
		l.addWidget( dbb )

	def project(self):
		return self.projectCombo.project()

project = None
regenPaths = len(sys.argv) > 2 and sys.argv[2]

if len(sys.argv) > 1:
	project = Project.recordByName( sys.argv[1] )


if not project or not project.isRecord():
	
	d = ProjectChooserDialog()
	if d.exec_() == QDialog.Accepted:
		project = d.project()

if project.isRecord():
	storageLocations = project.projectStorages()
		
	def print_paths( asset, tabs = 0 ):
		for c in asset.children():
			print ('  ' * tabs), c.name(), c.assetTemplate().name(), c.pathTemplate().name()
			for storage in storageLocations:
				path = c.path(storage)
				pt = c.pathTracker(storage)
				if not path.isEmpty() or pt.isRecord():
					print ('  ' * tabs) + ' ', path
					gen_path = pt.generatePathFromTemplate(storage)
					if path != gen_path:
						if regenPaths:
							print "Changing path to match template: ", gen_path
							pt.setPath(gen_path)
							pt.commit()
						else:
							print "Path doesnt match template: ", gen_path
						
			print_paths( c, tabs + 1 )
	
	print_paths( project )
