#!/usr/bin/env python

# PyQt tutorial 1


import sys

from PyQt4 import QtGui


app = QtGui.QApplication(sys.argv)

hello = QtGui.QPushButton("Hello world!")

hello.show()
sys.exit(app.exec_())
