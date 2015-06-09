
from PyQt4.QtCore import *
from blur.Stone import *
from blur.Classes import *
import sys

app = QCoreApplication.instance()
if app is None:
	try:
		args = sys.argv
	except:
		args = []
	app = QCoreApplication(args)

if sys.platform == 'win32':
	initConfig('c:/arsenalsuite/burner/burner.ini')
else:
	initConfig('/etc/db.ini')

initStone(sys.argv)
blurqt_loader()
