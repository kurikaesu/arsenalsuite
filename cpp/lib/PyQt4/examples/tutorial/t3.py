#!/usr/bin/env python

# PyQt tutorial 3


import sys

from PyQt4 import QtGui


app = QtGui.QApplication(sys.argv)

window = QtGui.QWidget()
window.resize(200, 120)

quit = QtGui.QPushButton("Quit", window)
quit.setFont(QtGui.QFont("Times", 18, QtGui.QFont.Bold))
quit.setGeometry(10, 40, 180, 40)
quit.clicked.connect(app.quit)

window.show()
sys.exit(app.exec_())
