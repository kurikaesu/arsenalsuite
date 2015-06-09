#!/usr/bin/env python

# PyQt tutorial 5


import sys

from PyQt4 import QtCore, QtGui


class MyWidget(QtGui.QWidget):

    def __init__(self, parent=None):

        super(MyWidget, self).__init__(parent)

        quit = QtGui.QPushButton("Quit")
        quit.setFont(QtGui.QFont("Times", 18, QtGui.QFont.Bold))

        lcd = QtGui.QLCDNumber(2)
        lcd.setSegmentStyle(QtGui.QLCDNumber.Filled)

        slider = QtGui.QSlider(QtCore.Qt.Horizontal)
        slider.setRange(0, 99)
        slider.setValue(0)

        quit.clicked.connect(QtGui.qApp.quit)
        slider.valueChanged.connect(lcd.display)

        layout = QtGui.QVBoxLayout()
        layout.addWidget(quit);
        layout.addWidget(lcd);
        layout.addWidget(slider);
        self.setLayout(layout);


app = QtGui.QApplication(sys.argv)
widget = MyWidget()
widget.show()
sys.exit(app.exec_())
