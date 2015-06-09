#!/usr/bin/python

from blur.Stone import *
from blur.Classes import *
from PyQt4.QtCore import QCoreApplication
import os, time, sys

# First Create a Qt Application
app = QCoreApplication(sys.argv)

# Load database config
if sys.platform=='win32':
	initConfig("c:\\blur\\resin\\resin.ini")
else:
	initConfig("/etc/db.ini")

initStone(sys.argv)

classes_loader()

VERBOSE = True

# blur=# insert into serverfileactionstatus (status) values ('New'), ('Success'), ('Error');
# INSERT 0 3
# blur=# insert into serverfileactiontype (type) values ('Move'), ('Delete');
# INSERT 0 2
# blur=# alter table serverfileaction alter column fkeyserverfileactionstatus set default 1;
# ALTER TABLE
# blur=# alter table serverfileaction alter column fkeyserverfileactiontype set default 2;
# ALTER TABLE


def recordError( action, msg ):
	print "Recording error:",msg
	action.setErrorMessage( msg )
	action.setStatus( action.Error )
	action.commit()

hardCodedDriveMapping = \
	{ 	'T' : '/tmp/',# Testing
		'G' : '/mnt/thor-animation/animation/',
		'S' : '/mnt/cheetah-renderout/renderOutput/',
		'Q' : '/mnt/cougar-compout/compOutput/',
		'V' : '/mnt/design/broadcast/',
		'I' : '/mnt/netftp/ftpRoot/',
		'U' : '/mnt/design/renderOutput/',
		'K' : '/mnt/snake-library/production/',
		'H' : '/mnt/snake-user/user' }

def translatePath( path ):
	driveLetter = str(path[0]).upper()
	if not driveLetter in hardCodedDriveMapping:
		return ('',False,'Drive Letter not found: ' + driveLetter)
	ret = hardCodedDriveMapping[driveLetter] + path[2:].replace('\\','/')
	if VERBOSE: print ("Path %s translated to %s" % (path, ret))
	return ( ret, True, '' )
	
def executeFileAction( action ):
	(path,success,errorMsg) = translatePath( action.sourcePath() )
	if not success:
		recordError( action, 'Unable to translate source path: ' + errorMsg )
		return
	print action
	actionType = action.serverFileActionType().name()
	print "Executing action", actionType, action.sourcePath(), action.destPath()
	if actionType == 'Move':
		(destPath,success,errorMsg) = translatePath( action.destPath() )
		if not success:
			recordError( action, 'Unable to translate dest path: ' + errorMsg )
			return
		try:
			os.rename( path, destPath )
		except:
			recordError( action, 'Unable to rename file from %s to %s' % (path, destPath) )
			return
	elif actionType == 'Delete':
		if os.path.exists( path ):
			if os.path.isdir( path ):
				recordError( action, 'Cannot remote %s, points to a directory' % path )
				return
			try:
				os.unlink( path )
			except:
				recordError( action, 'Unable to remove file: %s' % path )
				return
	action.setStatus( action.Success )
	action.commit()
	
def performFileActions():
	print "Checking for pending actions"
	actionList = ServerFileAction.recordsByHostAndStatus( Host.currentHost(), ServerFileActionStatus.recordByName( 'New' ) )
	for action in actionList:
		executeFileAction(action)

if __name__ == "__main__":
	while True:
		performFileActions()
		time.sleep(1)


	
	
