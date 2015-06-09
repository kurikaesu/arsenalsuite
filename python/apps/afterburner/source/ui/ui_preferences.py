# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'preferences.ui'
#
# Created: Thu Jun 16 09:29:42 2011
#      by: PyQt4 UI code generator snapshot-4.7.1-5014f7c72a58
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_Preferences(object):
    def setupUi(self, Preferences):
        Preferences.setObjectName("Preferences")
        Preferences.resize(291, 117)
        Preferences.setMaximumSize(QtCore.QSize(300, 200))
        icon = QtGui.QIcon()
        icon.addPixmap(QtGui.QPixmap("icons/cog.png"), QtGui.QIcon.Normal, QtGui.QIcon.Off)
        Preferences.setWindowIcon(icon)
        self.verticalLayout = QtGui.QVBoxLayout(Preferences)
        self.verticalLayout.setObjectName("verticalLayout")
        self.groupBox = QtGui.QGroupBox(Preferences)
        self.groupBox.setObjectName("groupBox")
        self.gridLayout = QtGui.QGridLayout(self.groupBox)
        self.gridLayout.setObjectName("gridLayout")
        self.slotBox = QtGui.QSpinBox(self.groupBox)
        self.slotBox.setObjectName("slotBox")
        self.gridLayout.addWidget(self.slotBox, 0, 0, 1, 1)
        self.slotLabel = QtGui.QLabel(self.groupBox)
        self.slotLabel.setObjectName("slotLabel")
        self.gridLayout.addWidget(self.slotLabel, 0, 1, 1, 1)
        self.textLabel4 = QtGui.QLabel(self.groupBox)
        self.textLabel4.setAlignment(QtCore.Qt.AlignCenter)
        self.textLabel4.setObjectName("textLabel4")
        self.gridLayout.addWidget(self.textLabel4, 2, 1, 1, 1)
        self.verticalLayout.addWidget(self.groupBox)

        self.retranslateUi(Preferences)
        QtCore.QMetaObject.connectSlotsByName(Preferences)

    def retranslateUi(self, Preferences):
        Preferences.setWindowTitle(QtGui.QApplication.translate("Settings", "Settings", None, QtGui.QApplication.UnicodeUTF8))
        self.groupBox.setTitle(QtGui.QApplication.translate("Settings", "Host Settings", None, QtGui.QApplication.UnicodeUTF8))
        self.slotLabel.setText(QtGui.QApplication.translate("Settings", "Slots", None, QtGui.QApplication.UnicodeUTF8))
        self.textLabel4.setText(QtGui.QApplication.translate("Settings", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:\'Sans Serif\'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<p align=\"justify\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:8pt;\">Set the number of slots (CPU cores)</span></p>\n"
"<p align=\"justify\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-size:8pt;\">you are able to spare</p></body></html>", None, QtGui.QApplication.UnicodeUTF8))

