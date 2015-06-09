#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from blur.Stone import *
from blur.Classes import *
import sys
import re
import traceback

# First Create a Qt Application
app = QApplication(sys.argv)

# Load database config
if sys.platform=='win32':
	initConfig("c:\\blur\\resin\\resin.ini")
else:
	initConfig("/etc/db.ini")

blurqt_loader()

# Navigate through every shot in the proj

def showProjectShots( project ):
	print 'Listing Shots in project ', project.name()
	for shot in project.shots().sorted( 'shotNumber' ):
		print shot.sequence().name(), shot.name()

def showProjectSeqShots( project ):
	print 'Listing Sequences in project ', project.name()
	for seq in project.sequences():
		print "\tListing Shots in sequence", seq.name()
		for shot in seq.shots():
			print shot.name()

if __name__ == '__main__':
	if len(sys.argv) < 2:
		print 'Must Pass project name as first arguement to the script'
	project = Project.recordByName( sys.argv[1] )
	if not project.isRecord():
		print 'Unable to find project by name', sys.argv[1]
		
