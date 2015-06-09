__version__	= 0.1

import sys

from PyQt4.QtGui		import QApplication
from PyQt4.QtGui		import QMessageBox

#----------------------------------------------------------------------------

if ( __name__ == '__main__' ):
	app = QApplication( sys.argv )
	app.setStyle( "Plastique" )

	msg = "argv = " + str(sys.argv)
	QMessageBox.information( None, "Test Pop-Up", msg )

	#app.exec_()
