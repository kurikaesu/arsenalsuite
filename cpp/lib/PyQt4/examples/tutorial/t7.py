#!/usr/bin/env python

# PyQt tutorial 7


import sys

from PyQt4 import QtCore, QtGui


class LCDRange(QtGui.QWidget):

    valueChanged = QtCore.pyqtSignal(int)

    def __init__(self, parent=None):

        super(LCDRange, self).__init__(parent)

        lcd = QtGui.QLCDNumber(2)
        lcd.setSegmentStyle(QtGui.QLCDNumber.Filled)

        self.slider = QtGui.QSlider(QtCore.Qt.Horizontal)
        self.slider.setRange(0, 99)
        self.slider.setValue(0)

        self.slider.valueChanged.connect(lcd.display)
        self.slider.valueChanged.connect(self.valueChanged)

        layout = QtGui.QVBoxLayout()
        layout.addWidget(lcd)
        layout.addWidget(self.slider)
        self.setLayout(layout)

    def value(self):
        return self.slider.value()

    def setValue(self, value):
        self.slider.setValue(value)


class MyWidget(QtGui.QWidget):

    def __init__(self, parent=None):

        super(MyWidget, self).__init__(parent)

        quit = QtGui.QPushButton("Quit")
        quit.setFont(QtGui.QFont("Times", 18, QtGui.QFont.Bold))

        quit.clicked.connect(QtGui.qApp.quit)

        grid = QtGui.QGridLayout()
        previousRange = None

        for row in range(3):
            for column in range(3):
                lcdRange = LCDRange()
                grid.addWidget(lcdRange, row, column)

                if previousRange:
                    lcdRange.valueChanged.connect(previousRange.setValue)

                previousRange = lcdRange

        layout = QtGui.QVBoxLayout()
        layout.addWidget(quit)
        layout.addLayout(grid)
        self.setLayout(layout)


app = QtGui.QApplication(sys.argv)
widget = MyWidget()
widget.show()
sys.exit(app.exec_())
