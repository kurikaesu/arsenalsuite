#!/usr/bin/env python

# PyQt tutorial 10


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

        self.setFocusProxy(self.slider)

    def value(self):
        return self.slider.value()

    def setValue(self, value):
        self.slider.setValue(value)

    def setRange(self, minValue, maxValue):
        if minValue < 0 or maxValue > 99 or minValue > maxValue:
            QtCore.qWarning("LCDRange::setRange(%d, %d)\n"
                    "\tRange must be 0..99\n"
                    "\tand minValue must not be greater than maxValue" % (minValue, maxValue))
            return

        self.slider.setRange(minValue, maxValue)


class CannonField(QtGui.QWidget):

    angleChanged = QtCore.pyqtSignal(int)

    forceChanged = QtCore.pyqtSignal(int)

    def __init__(self, parent=None):

        super(CannonField, self).__init__(parent)

        self.currentAngle = 45
        self.currentForce = 0
        self.setPalette(QtGui.QPalette(QtGui.QColor(250, 250, 200)))
        self.setAutoFillBackground(True)

    def angle(self):
        return self.currentAngle

    def setAngle(self, angle):
        if angle < 5:
            angle = 5
        if angle > 70:
            angle = 70;
        if self.currentAngle == angle:
            return

        self.currentAngle = angle
        self.update(self.cannonRect())
        self.angleChanged.emit(self.currentAngle)

    def force(self):
        return self.currentForce

    def setForce(self, force):
        if force < 0:
            force = 0
        if self.currentForce == force:
            return

        self.currentForce = force;
        self.forceChanged.emit(self.currentForce)

    def paintEvent(self, event):
        painter = QtGui.QPainter(self)

        painter.setPen(QtCore.Qt.NoPen)
        painter.setBrush(QtCore.Qt.blue)

        painter.translate(0, self.height())
        painter.drawPie(QtCore.QRect(-35, -35, 70, 70), 0, 90 * 16)
        painter.rotate(-self.currentAngle)
        painter.drawRect(QtCore.QRect(33, -5, 20, 10))

    def cannonRect(self):
        result = QtCore.QRect(0, 0, 50, 50)
        result.moveBottomLeft(self.rect().bottomLeft())
        return result


class MyWidget(QtGui.QWidget):

    def __init__(self, parent=None):

        super(MyWidget, self).__init__(parent)

        quit = QtGui.QPushButton("&Quit")
        quit.setFont(QtGui.QFont("Times", 18, QtGui.QFont.Bold))

        quit.clicked.connect(QtGui.qApp.quit)

        angle = LCDRange()
        angle.setRange(5, 70)

        force = LCDRange()
        force.setRange(10, 50)

        cannonField = CannonField()

        angle.valueChanged.connect(cannonField.setAngle)
        cannonField.angleChanged.connect(angle.setValue)

        force.valueChanged.connect(cannonField.setForce)
        cannonField.forceChanged.connect(force.setValue)

        leftLayout = QtGui.QVBoxLayout()
        leftLayout.addWidget(angle)
        leftLayout.addWidget(force)

        gridLayout = QtGui.QGridLayout()
        gridLayout.addWidget(quit, 0, 0)
        gridLayout.addLayout(leftLayout, 1, 0)
        gridLayout.addWidget(cannonField, 1, 1, 2, 1)
        gridLayout.setColumnStretch(1, 10)
        self.setLayout(gridLayout)

        angle.setValue(60)
        force.setValue(25)
        angle.setFocus()


app = QtGui.QApplication(sys.argv)
widget = MyWidget()
widget.setGeometry(100, 100, 500, 355)
widget.show()
sys.exit(app.exec_())
