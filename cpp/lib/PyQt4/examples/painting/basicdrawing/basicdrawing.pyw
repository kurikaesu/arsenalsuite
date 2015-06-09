#!/usr/bin/env python

############################################################################
##
## Copyright (C) 2005-2005 Trolltech AS. All rights reserved.
##
## This file is part of the example classes of the Qt Toolkit.
##
## This file may be used under the terms of the GNU General Public
## License version 2.0 as published by the Free Software Foundation
## and appearing in the file LICENSE.GPL included in the packaging of
## this file.  Please review the following information to ensure GNU
## General Public Licensing requirements will be met:
## http://www.trolltech.com/products/qt/opensource.html
##
## If you are unsure which license is appropriate for your use, please
## review the following information:
## http://www.trolltech.com/products/qt/licensing.html or contact the
## sales department at sales@trolltech.com.
##
## This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
## WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
##
############################################################################

# This is only needed for Python v2 but is harmless for Python v3.
import sip
sip.setapi('QVariant', 2)

from PyQt4 import QtCore, QtGui

import basicdrawing_rc


class RenderArea(QtGui.QWidget):
    points = QtGui.QPolygon([
        QtCore.QPoint(10, 80),
        QtCore.QPoint(20, 10),
        QtCore.QPoint(80, 30),
        QtCore.QPoint(90, 70)
    ])

    Line, Points, Polyline, Polygon, Rect, RoundedRect, Ellipse, Arc, Chord, \
            Pie, Path, Text, Pixmap = range(13)

    def __init__(self, parent=None):
        super(RenderArea, self).__init__(parent)

        self.pen = QtGui.QPen()
        self.brush = QtGui.QBrush()
        self.pixmap = QtGui.QPixmap()

        self.shape = RenderArea.Polygon
        self.antialiased = False
        self.transformed = False
        self.pixmap.load(':/images/qt-logo.png')

        self.setBackgroundRole(QtGui.QPalette.Base)
        self.setAutoFillBackground(True)

    def minimumSizeHint(self):
        return QtCore.QSize(100, 100)

    def sizeHint(self):
        return QtCore.QSize(400, 200)

    def setShape(self, shape):
        self.shape = shape
        self.update()

    def setPen(self, pen):
        self.pen = pen
        self.update()

    def setBrush(self, brush):
        self.brush = brush
        self.update()

    def setAntialiased(self, antialiased):
        self.antialiased = antialiased
        self.update()

    def setTransformed(self, transformed):
        self.transformed = transformed
        self.update()

    def paintEvent(self, event):
        rect = QtCore.QRect(10, 20, 80, 60)

        path = QtGui.QPainterPath()
        path.moveTo(20, 80)
        path.lineTo(20, 30)
        path.cubicTo(80, 0, 50, 50, 80, 80)

        startAngle = 30 * 16
        arcLength = 120 * 16

        painter = QtGui.QPainter(self)
        painter.setPen(self.pen)
        painter.setBrush(self.brush)
        if self.antialiased:
            painter.setRenderHint(QtGui.QPainter.Antialiasing)

        for x in range(0, self.width(), 100):
            for y in range(0, self.height(), 100):
                painter.save()
                painter.translate(x, y)
                if self.transformed:
                    painter.translate(50, 50)
                    painter.rotate(60.0)
                    painter.scale(0.6, 0.9)
                    painter.translate(-50, -50)

                if self.shape == RenderArea.Line:
                    painter.drawLine(rect.bottomLeft(), rect.topRight())
                elif self.shape == RenderArea.Points:
                    painter.drawPoints(RenderArea.points)
                elif self.shape == RenderArea.Polyline:
                    painter.drawPolyline(RenderArea.points)
                elif self.shape == RenderArea.Polygon:
                    painter.drawPolygon(RenderArea.points)
                elif self.shape == RenderArea.Rect:
                    painter.drawRect(rect)
                elif self.shape == RenderArea.RoundedRect:
                    painter.drawRoundedRect(rect, 25, 25,
                            QtCore.Qt.RelativeSize)
                elif self.shape == RenderArea.Ellipse:
                    painter.drawEllipse(rect)
                elif self.shape == RenderArea.Arc:
                    painter.drawArc(rect, startAngle, arcLength)
                elif self.shape == RenderArea.Chord:
                    painter.drawChord(rect, startAngle, arcLength)
                elif self.shape == RenderArea.Pie:
                    painter.drawPie(rect, startAngle, arcLength)
                elif self.shape == RenderArea.Path:
                    painter.drawPath(path)
                elif self.shape == RenderArea.Text:
                    painter.drawText(rect, QtCore.Qt.AlignCenter,
                            "Qt by\nQt Software")
                elif self.shape == RenderArea.Pixmap:
                    painter.drawPixmap(10, 10, self.pixmap)

                painter.restore()

        painter.setPen(self.palette().dark().color())
        painter.setBrush(QtCore.Qt.NoBrush)
        painter.drawRect(QtCore.QRect(0, 0, self.width() - 1,
                self.height() - 1))


IdRole = QtCore.Qt.UserRole

class Window(QtGui.QWidget):
    def __init__(self):
        super(Window, self).__init__()

        self.renderArea = RenderArea()

        self.shapeComboBox = QtGui.QComboBox()
        self.shapeComboBox.addItem("Polygon", RenderArea.Polygon)
        self.shapeComboBox.addItem("Rectangle", RenderArea.Rect)
        self.shapeComboBox.addItem("Rounded Rectangle", RenderArea.RoundedRect)
        self.shapeComboBox.addItem("Ellipse", RenderArea.Ellipse)
        self.shapeComboBox.addItem("Pie", RenderArea.Pie)
        self.shapeComboBox.addItem("Chord", RenderArea.Chord)
        self.shapeComboBox.addItem("Path", RenderArea.Path)
        self.shapeComboBox.addItem("Line", RenderArea.Line)
        self.shapeComboBox.addItem("Polyline", RenderArea.Polyline)
        self.shapeComboBox.addItem("Arc", RenderArea.Arc)
        self.shapeComboBox.addItem("Points", RenderArea.Points)
        self.shapeComboBox.addItem("Text", RenderArea.Text)
        self.shapeComboBox.addItem("Pixmap", RenderArea.Pixmap)

        shapeLabel = QtGui.QLabel("&Shape:")
        shapeLabel.setBuddy(self.shapeComboBox)

        self.penWidthSpinBox = QtGui.QSpinBox()
        self.penWidthSpinBox.setRange(0, 20)
        self.penWidthSpinBox.setSpecialValueText("0 (cosmetic pen)")

        penWidthLabel = QtGui.QLabel("Pen &Width:")
        penWidthLabel.setBuddy(self.penWidthSpinBox)

        self.penStyleComboBox = QtGui.QComboBox()
        self.penStyleComboBox.addItem("Solid", QtCore.Qt.SolidLine)
        self.penStyleComboBox.addItem("Dash", QtCore.Qt.DashLine)
        self.penStyleComboBox.addItem("Dot", QtCore.Qt.DotLine)
        self.penStyleComboBox.addItem("Dash Dot", QtCore.Qt.DashDotLine)
        self.penStyleComboBox.addItem("Dash Dot Dot", QtCore.Qt.DashDotDotLine)
        self.penStyleComboBox.addItem("None", QtCore.Qt.NoPen)

        penStyleLabel = QtGui.QLabel("&Pen Style:")
        penStyleLabel.setBuddy(self.penStyleComboBox)

        self.penCapComboBox = QtGui.QComboBox()
        self.penCapComboBox.addItem("Flat", QtCore.Qt.FlatCap)
        self.penCapComboBox.addItem("Square", QtCore.Qt.SquareCap)
        self.penCapComboBox.addItem("Round", QtCore.Qt.RoundCap)

        penCapLabel = QtGui.QLabel("Pen &Cap:")
        penCapLabel.setBuddy(self.penCapComboBox)

        self.penJoinComboBox = QtGui.QComboBox()
        self.penJoinComboBox.addItem("Miter", QtCore.Qt.MiterJoin)
        self.penJoinComboBox.addItem("Bevel", QtCore.Qt.BevelJoin)
        self.penJoinComboBox.addItem("Round", QtCore.Qt.RoundJoin)

        penJoinLabel = QtGui.QLabel("Pen &Join:")
        penJoinLabel.setBuddy(self.penJoinComboBox)

        self.brushStyleComboBox = QtGui.QComboBox()
        self.brushStyleComboBox.addItem("Linear Gradient",
                QtCore.Qt.LinearGradientPattern)
        self.brushStyleComboBox.addItem("Radial Gradient",
                QtCore.Qt.RadialGradientPattern)
        self.brushStyleComboBox.addItem("Conical Gradient",
                QtCore.Qt.ConicalGradientPattern)
        self.brushStyleComboBox.addItem("Texture", QtCore.Qt.TexturePattern)
        self.brushStyleComboBox.addItem("Solid", QtCore.Qt.SolidPattern)
        self.brushStyleComboBox.addItem("Horizontal", QtCore.Qt.HorPattern)
        self.brushStyleComboBox.addItem("Vertical", QtCore.Qt.VerPattern)
        self.brushStyleComboBox.addItem("Cross", QtCore.Qt.CrossPattern)
        self.brushStyleComboBox.addItem("Backward Diagonal",
                QtCore.Qt.BDiagPattern)
        self.brushStyleComboBox.addItem("Forward Diagonal",
                QtCore.Qt.FDiagPattern)
        self.brushStyleComboBox.addItem("Diagonal Cross",
                QtCore.Qt.DiagCrossPattern)
        self.brushStyleComboBox.addItem("Dense 1", QtCore.Qt.Dense1Pattern)
        self.brushStyleComboBox.addItem("Dense 2", QtCore.Qt.Dense2Pattern)
        self.brushStyleComboBox.addItem("Dense 3", QtCore.Qt.Dense3Pattern)
        self.brushStyleComboBox.addItem("Dense 4", QtCore.Qt.Dense4Pattern)
        self.brushStyleComboBox.addItem("Dense 5", QtCore.Qt.Dense5Pattern)
        self.brushStyleComboBox.addItem("Dense 6", QtCore.Qt.Dense6Pattern)
        self.brushStyleComboBox.addItem("Dense 7", QtCore.Qt.Dense7Pattern)
        self.brushStyleComboBox.addItem("None", QtCore.Qt.NoBrush)

        brushStyleLabel = QtGui.QLabel("&Brush Style:")
        brushStyleLabel.setBuddy(self.brushStyleComboBox)

        otherOptionsLabel = QtGui.QLabel("Other Options:")
        self.antialiasingCheckBox = QtGui.QCheckBox("&Antialiasing")
        self.transformationsCheckBox = QtGui.QCheckBox("&Transformations")

        self.shapeComboBox.activated.connect(self.shapeChanged)
        self.penWidthSpinBox.valueChanged.connect(self.penChanged)
        self.penStyleComboBox.activated.connect(self.penChanged)
        self.penCapComboBox.activated.connect(self.penChanged)
        self.penJoinComboBox.activated.connect(self.penChanged)
        self.brushStyleComboBox.activated.connect(self.brushChanged)
        self.antialiasingCheckBox.toggled.connect(self.renderArea.setAntialiased)
        self.transformationsCheckBox.toggled.connect(self.renderArea.setTransformed)

        mainLayout = QtGui.QGridLayout()
        mainLayout.setColumnStretch(0, 1)
        mainLayout.setColumnStretch(3, 1)
        mainLayout.addWidget(self.renderArea, 0, 0, 1, 4)
        mainLayout.setRowMinimumHeight(1, 6)
        mainLayout.addWidget(shapeLabel, 2, 1, QtCore.Qt.AlignRight)
        mainLayout.addWidget(self.shapeComboBox, 2, 2)
        mainLayout.addWidget(penWidthLabel, 3, 1, QtCore.Qt.AlignRight)
        mainLayout.addWidget(self.penWidthSpinBox, 3, 2)
        mainLayout.addWidget(penStyleLabel, 4, 1, QtCore.Qt.AlignRight)
        mainLayout.addWidget(self.penStyleComboBox, 4, 2)
        mainLayout.addWidget(penCapLabel, 5, 1, QtCore.Qt.AlignRight)
        mainLayout.addWidget(self.penCapComboBox, 5, 2)
        mainLayout.addWidget(penJoinLabel, 6, 1, QtCore.Qt.AlignRight)
        mainLayout.addWidget(self.penJoinComboBox, 6, 2)
        mainLayout.addWidget(brushStyleLabel, 7, 1, QtCore.Qt.AlignRight)
        mainLayout.addWidget(self.brushStyleComboBox, 7, 2)
        mainLayout.setRowMinimumHeight(8, 6)
        mainLayout.addWidget(otherOptionsLabel, 9, 1, QtCore.Qt.AlignRight)
        mainLayout.addWidget(self.antialiasingCheckBox, 9, 2)
        mainLayout.addWidget(self.transformationsCheckBox, 10, 2)
        self.setLayout(mainLayout)

        self.shapeChanged()
        self.penChanged()
        self.brushChanged()
        self.antialiasingCheckBox.setChecked(True)

        self.setWindowTitle("Basic Drawing")

    def shapeChanged(self):
        shape = self.shapeComboBox.itemData(self.shapeComboBox.currentIndex(),
                IdRole)
        self.renderArea.setShape(shape)

    def penChanged(self):
        width = self.penWidthSpinBox.value()
        style = QtCore.Qt.PenStyle(self.penStyleComboBox.itemData(
                self.penStyleComboBox.currentIndex(), IdRole))
        cap = QtCore.Qt.PenCapStyle(self.penCapComboBox.itemData(
                self.penCapComboBox.currentIndex(), IdRole))
        join = QtCore.Qt.PenJoinStyle(self.penJoinComboBox.itemData(
                self.penJoinComboBox.currentIndex(), IdRole))

        self.renderArea.setPen(QtGui.QPen(QtCore.Qt.blue, width, style, cap, join))

    def brushChanged(self):
        style = QtCore.Qt.BrushStyle(self.brushStyleComboBox.itemData(
                self.brushStyleComboBox.currentIndex(), IdRole))

        if style == QtCore.Qt.LinearGradientPattern:
            linearGradient = QtGui.QLinearGradient(0, 0, 100, 100)
            linearGradient.setColorAt(0.0, QtCore.Qt.white)
            linearGradient.setColorAt(0.2, QtCore.Qt.green)
            linearGradient.setColorAt(1.0, QtCore.Qt.black)
            self.renderArea.setBrush(QtGui.QBrush(linearGradient))
        elif style == QtCore.Qt.RadialGradientPattern:
            radialGradient = QtGui.QRadialGradient(50, 50, 50, 70, 70)
            radialGradient.setColorAt(0.0, QtCore.Qt.white)
            radialGradient.setColorAt(0.2, QtCore.Qt.green)
            radialGradient.setColorAt(1.0, QtCore.Qt.black)
            self.renderArea.setBrush(QtGui.QBrush(radialGradient))
        elif style == QtCore.Qt.ConicalGradientPattern:
            conicalGradient = QtGui.QConicalGradient(50, 50, 150)
            conicalGradient.setColorAt(0.0, QtCore.Qt.white)
            conicalGradient.setColorAt(0.2, QtCore.Qt.green)
            conicalGradient.setColorAt(1.0, QtCore.Qt.black)
            self.renderArea.setBrush(QtGui.QBrush(conicalGradient))
        elif style == QtCore.Qt.TexturePattern:
            self.renderArea.setBrush(QtGui.QBrush(QtGui.QPixmap(':/images/brick.png')))
        else:
            self.renderArea.setBrush(QtGui.QBrush(QtCore.Qt.green, style))


if __name__ == '__main__':

    import sys

    app = QtGui.QApplication(sys.argv)
    window = Window()
    window.show()
    sys.exit(app.exec_())
