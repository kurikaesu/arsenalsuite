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

print Element.fromPath( sys.argv[1], True ).displayName( True )