#!/usr/bin/python2.5

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from blur.Stone import *
from blur.Classes import *

import os
import sys
import time

# initialisation stuff
app = QCoreApplication(sys.argv)
initConfig(os.getenv('ABSUBMIT','.')+"/ab.ini", os.getenv('TEMP','/tmp')+"/api_example.log")
initStone( sys.argv )
classes_loader()

Database.current().setEchoMode( Database.EchoUpdate | Database.EchoDelete | Database.EchoSelect )


licenses = License.select("WHERE license = ?", [QVariant('3delight')])
for license in licenses:
    print "%s : %s" % ( license.license(), license.total())

