#!/usr/bin/env python

# PyQt tutorial 2


import sys

from PyQt4 import QtGui


app = QtGui.QApplication(sys.argv)

quit = QtGui.QPushButton("Quit")
quit.resize(75, 30)
quit.setFont(QtGui.QFont("Times", 18, QtGui.QFont.Bold))
quit.clicked.connect(app.quit)

quit.show()
sys.exit(app.exec_())
