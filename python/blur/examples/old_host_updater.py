#!/usr/bin/python

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from blur.Stone import *
from blur.Classes import *
import sys, re, time

# First Create a Qt Application
app = QApplication(sys.argv)

# Load database config
if sys.platform=='win32':
	initConfig("c:\\blur\\resin\\resin.ini")
else:
	initConfig("/etc/db.ini")

blurqt_loader()

#Database.instance().setEchoMode( Database.EchoSelect )

while True:
	hosts = Host.select(" INNER JOIN HostStatus ON HostStatus.fkeyhost=Host.keyHost WHERE Host.abversion != 'v1.3.1 r7267' and HostStatus.slaveStatus IN ('ready','copy','assigned','busy')")
	for host in hosts:
		print host.name(), ' is online with old assburner'
		hs = host.hostStatus()
		hs.setSlaveStatus('client-update')
		hs.returnSlaveFrames(True)
		print host.name(), 'set to client-update'
	time.sleep(10)