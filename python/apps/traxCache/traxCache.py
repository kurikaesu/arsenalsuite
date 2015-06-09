__version__	= 0.2

import os
import sys

_traxOn = True
if _traxOn:
	sys.path.append("C:\\blur\\code\\it_trunk\\python\\apps")
	import trax.api
	trax.api.init()
	trax.api.setEnvironment("development")
	#basePath = trax.api.path( 'code' )
	basePath = trax.api.path( 'legacyRoot' )
else:
	import blur.globals
	basePath = blur.globals.GetCodePath()

from PyQt4.QtGui		import QApplication
from PyQt4.QtGui		import QMessageBox

_debugFlag = False

#----------------------------------------------------------------------------
_numCmds = 6
menuText = []
menuText.append("TRAX")
menuText.append("TRAX TreeGrunt")
menuText.append("TRAX Publish")
menuText.append("TRAX ReBuilder")
menuText.append("TRAX ShotGenerator")
menuText.append("TRAX MoMoCap")
#----------------------------------------------------------------------------

	
if ( __name__ == '__main__' ):
	app = QApplication( sys.argv )
	app.setStyle( "Plastique" )

	if _debugFlag:
		msg = "argv = " + str(sys.argv)
		QMessageBox.information( None, "traxCache Input Debug Pop-Up", msg )

	inPath = ""
	inCmd = -1
	inCmdVerb = ""
	
	if len(sys.argv) >= 2:
		inPath = sys.argv[1]
	else:
		msg = "argv input too short = " + str(sys.argv)
		QMessageBox.critical( None, "traxCache input error pop-up #1", msg )
		sys.exit(-1)   	
	if len(sys.argv) >= 3:
		inCmd = int(sys.argv[2])
	else:
		msg = "argv input too short = " + str(sys.argv)
		QMessageBox.critical( None, "traxCache input error pop-up #2", msg )
		sys.exit(-1)
		
	if inCmd in range(_numCmds):
		inCmdVerb = menuText[inCmd]
	else:
		msg = "bad command input = " + str(inCmd)
		QMessageBox.critical( None, "traxCache command input error pop-up", msg )
		sys.exit(-1)

	if _debugFlag:
		msg = "inCmd = " + str(inCmd) + "; inCmdVerb = " + inCmdVerb
		QMessageBox.information( None, "traxCache Command Debug Pop-Up", msg )
				
	if inCmdVerb == "TRAX":
		# TRAX
		#cmd = "C:\\blur\\code\\it_trunk\\python\\apps\\trax\\gui\\CoreWindow.pyw -logger -u krash"
		cmd = "C:\\blur\\code\\it_trunk\\python\\apps\\trax\\gui\\CoreWindow.pyw"
		#os.system(cmd)
		os.startfile(cmd)
	elif inCmdVerb == "TRAX TreeGrunt":
		# TreeGrunt
		cmd = basePath + "Main\External_Tools\Production_Tools\Treegrunt_resource\TreegruntDialog.pyw"
		os.startfile(cmd)
	elif inCmdVerb == "TRAX Publish":
		# Publisher
		cmd = basePath + "Main\\External_Tools\\Production_Tools\\BlurEditSuite.py"
		os.startfile(cmd)
	elif inCmdVerb == "TRAX ReBuilder":
		# ReBuilder
		cmd = basePath + "Main\\External_Tools\\Production_Tools\\ReBuilder.py"
		os.startfile(cmd)
	elif inCmdVerb == "TRAX ShotGenerator":
		# Shot Generator
		cmd = basePath + "Main\\External_Tools\\Production_Tools\\ShotGenerator.py " + inPath
		os.system(cmd)
	elif inCmdVerb == "TRAX MoMoCap":
		# MoMoCap
		cmd = basePath + "Main\\External_Tools\\Production_Tools\\MoMoCap.pyw"
		os.startfile(cmd)
	else:
		#msg = "argv = " + str(sys.argv)
		msg = "inCmd = " + str(inCmd) + "; inCmdVerb = " + inCmdVerb
		QMessageBox.critical( None, "traxCache unknown command error pop-up", msg )
		sys.exit(-1)

	#import time
	#time.sleep(10)
	#app.exec_()
	sys.exit(0)
