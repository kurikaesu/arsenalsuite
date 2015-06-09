#!/usr/bin/env python

# PyQt tutorial 6


import sys

from PyQt4 import QtCore, QtGui


class LCDRange(QtGui.QWidget):

    def __init__(self, parent=None):

        super(LCDRange, self).__init__(parent)

        lcd = QtGui.QLCDNumber(2)
        lcd.setSegmentStyle(QtGui.QLCDNumber.Filled)

        slider = QtGui.QSlider(QtCore.Qt.Horizontal)
        slider.setRange(0, 99)
        slider.setValue(0)
        slider.valueChanged.connect(lcd.display)

        layout = QtGui.QVBoxLayout()
        layout.addWidget(lcd)
        layout.addWidget(slider)
        self.setLayout(layout)


class MyWidget(QtGui.QWidget):

    def __init__(self, parent=None):

        super(MyWidget, self).__init__(parent)

        quit = QtGui.QPushButton("Quit")
        quit.setFont(QtGui.QFont("Times", 18, QtGui.QFont.Bold))
        quit.clicked.connect(QtGui.qApp.quit)

        grid = QtGui.QGridLayout()
        for row in range(3):
            for column in range(3):
                lcdRange = LCDRange()
                grid.addWidget(lcdRange, row, column)

        layout = QtGui.QVBoxLayout()
        layout.addWidget(quit)
        layout.addLayout(grid)
        self.setLayout(layout)


app = QtGui.QApplication(sys.argv)
widget = MyWidget()
widget.show()
sys.exit(app.exec_())
