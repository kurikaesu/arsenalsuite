# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'afterburner.ui'
#
# Created: Thu Jun 16 09:29:42 2011
#      by: PyQt4 UI code generator snapshot-4.7.1-5014f7c72a58
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_Afterburner(object):
    def setupUi(self, Afterburner):
        Afterburner.setObjectName("Afterburner")
        Afterburner.resize(467, 190)
        Afterburner.setMinimumSize(QtCore.QSize(450, 190))
        icon = QtGui.QIcon()
        icon.addPixmap(QtGui.QPixmap("icons/server.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        Afterburner.setWindowIcon(icon)
        self.centralwidget = QtGui.QWidget(Afterburner)
        self.centralwidget.setObjectName("centralwidget")
        self.gridLayout = QtGui.QGridLayout(self.centralwidget)
        self.gridLayout.setObjectName("gridLayout")
        self.jobBox = QtGui.QGroupBox(self.centralwidget)
        self.jobBox.setMaximumSize(QtCore.QSize(16777215, 16777215))
        self.jobBox.setObjectName("jobBox")
        self.gridLayout_2 = QtGui.QGridLayout(self.jobBox)
        self.gridLayout_2.setObjectName("gridLayout_2")
        self.jobView = QtGui.QTableView(self.jobBox)
        self.jobView.setObjectName("jobView")
        self.gridLayout_2.addWidget(self.jobView, 1, 0, 1, 1)
        self.gridLayout.addWidget(self.jobBox, 0, 0, 1, 1)
        Afterburner.setCentralWidget(self.centralwidget)
        self.statusbar = QtGui.QStatusBar(Afterburner)
        self.statusbar.setObjectName("statusbar")
        Afterburner.setStatusBar(self.statusbar)
        self.actionExit = QtGui.QAction(Afterburner)
        self.actionExit.setObjectName("actionExit")

        self.retranslateUi(Afterburner)
        QtCore.QMetaObject.connectSlotsByName(Afterburner)

    def retranslateUi(self, Afterburner):
        Afterburner.setWindowTitle(QtGui.QApplication.translate("Afterburner", "Afterburner", None, QtGui.QApplication.UnicodeUTF8))
        self.jobBox.setTitle(QtGui.QApplication.translate("Afterburner", "Running Jobs", None, QtGui.QApplication.UnicodeUTF8))
        self.actionExit.setText(QtGui.QApplication.translate("Afterburner", "E&xit", None, QtGui.QApplication.UnicodeUTF8))

