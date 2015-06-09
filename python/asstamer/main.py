
import sys
from mainwindow import MainWindow
from PyQt4 import QtGui
from blur.Stone import initConfig, initStone, Database
from blur.Classes import classes_loader

if __name__=="__main__":
	app = QtGui.QApplication(sys.argv)
	initConfig("asstamer.ini")
	initStone(sys.argv)
	classes_loader()
	mw = MainWindow()
	mw.show()
	sys.exit(app.exec_())
