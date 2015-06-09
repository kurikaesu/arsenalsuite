#!/usr/bin/env python

# PyQt tutorial 4


import sys

from PyQt4 import QtGui


class MyWidget(QtGui.QWidget):

    def __init__(self, parent=None):

        super(MyWidget, self).__init__(parent)

        self.setFixedSize(200, 120)

        quit = QtGui.QPushButton("Quit", self)
        quit.setGeometry(62, 40, 75, 30)
        quit.setFont(QtGui.QFont("Times", 18, QtGui.QFont.Bold))

        quit.clicked.connect(QtGui.qApp.quit)


app = QtGui.QApplication(sys.argv)
widget = MyWidget()
widget.show()
sys.exit(app.exec_())
