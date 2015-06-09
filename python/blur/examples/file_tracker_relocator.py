#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtSql import *
from PyQt4.QtGui import *
from blur.Stone import *
from blur.Classes import *
from blur.Classesui import *
import sys, time, re, os, blur

app = QApplication(sys.argv)
initStone( sys.argv )
initConfig( "/etc/db.ini", "/var/log/path_lister.log" )
blur.RedirectOutputToLog()
blurqt_loader()

doIt = len(sys.argv) >= 3 and sys.argv[2]

def usage():
	print "python file_tracker_relocator.py PROJECT_NAME [DOIT]"
	print "This program moves each filetracker to the correct asset based on it's path"
	print "It also removes duplicate filetrackers(and updates the filetrackerdeps)."
	print "It just prints what it would do unless the DOIT arg is passed(must eval to True)"

def remove_duplicates( filetracker ):
	dups = FileTracker.select( "path=? AND filename=? AND keyfiletracker!=?", [QVariant(filetracker.path()),QVariant(filetracker.fileName()),QVariant(filetracker.key())] )
	to_commit = RecordList()
	for ft in dups:
		print "Removing duplicate filetracker: ", ft.key(), ft.path()
		deps = FileTrackerDep.select("fkeyInput=? OR fkeyOutput=?",[QVariant(ft.key()),QVariant(ft.key())])
		for dep in deps:
#			print "Updating dependency"
			if dep.input() == ft:
				dep.setInput( filetracker )
			if dep.output() == ft:
				dep.setOutput( filetracker )
		to_commit += deps
	if doIt:
		to_commit.commit()
		dups.remove()

def adjust_asset( asset, tabs = 0 ):
	print asset.displayName(True)
	trackers = asset.trackers( False ) # Recursive = false
	for t in trackers:
		print "Checking filetracker: ", t.key(), t.filePath()
		if t.isRecord():
			proper_owner = Element.fromPath( t.path(), True )
			if proper_owner.isRecord() and proper_owner != asset:
				print "Filetrackers proper owner is: " + proper_owner.displayName(True)
				if doIt:
					t.setElement( proper_owner )
					t.commit()
			remove_duplicates( t )
	children = asset.children()
	for child in children:
		adjust_asset( child, tabs + 1 )

def main():
	if len(sys.argv) < 2:
		usage()
		return
	
	project = Project.recordByName( sys.argv[1] )
	
	if not project.isRecord():
		print "Unable to find project: " + sys.argv[1]
		return
		
	adjust_asset(project)

if __name__ == "__main__":
	main()
