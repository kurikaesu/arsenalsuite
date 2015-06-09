#!/usr/bin/env python2.5

import sys
import os
from ui import Ui_LightroomImport
from LightroomImport import LightroomImport
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtSql import *

#-----------------------------------------------------------------------------
class LightroomImportWindow( Ui_LightroomImport, QMainWindow ):
#-----------------------------------------------------------------------------
    def __init__( self, parent = None ):
        QMainWindow.__init__( self, parent )
        self.setupUi( self )

        self.connect( self.mLightroomDBPathChoose, SIGNAL('clicked()'), self.onChoosePathBtn )
        self.connect( self.mOkBtn, SIGNAL('clicked()'), self.onOkBtn )
        self.connect( self.mCancelBtn, SIGNAL('clicked()'), self.onCancelBtn )

#-----------------------------------------------------------------------------
    def onChoosePathBtn(self):
        path = self.mLightroomDBPath.text()
        path = QFileDialog.getOpenFileName( self, "Open Lightroom DB", path, "*.lrcat" )
        if path is not None and path != "":
            self.mLightroomDBPath.setText( path )

#-----------------------------------------------------------------------------
    def printIt(self, p, timeout=5000):
        print p
        p = str( p )
        self.log.appendPlainText( p )
        self.statusBar.showMessage( p, timeout )
        QApplication.processEvents()

#-----------------------------------------------------------------------------
    def stat(self, p, timeout=5000):
        p = str( p )
        self.statusBar.showMessage( p, timeout )
        QApplication.processEvents()

#-----------------------------------------------------------------------------
    def onCancelBtn(self):
        self.printIt( 'So quit!' )

#-----------------------------------------------------------------------------
    def onOkBtn(self):
        path = str( self.mLightroomDBPath.text() )
        emptyTables = self.mEmptyData.isChecked()
        resetExcluded = self.mResetExcluded.isChecked()
        importKeywords = self.mImportKeywords.isChecked()
        importCollections = self.mImportCollections.isChecked()
        dryRun = self.mDryRun.isChecked()
        lri = LightroomImport( self, path, emptyTables, resetExcluded, importKeywords, importCollections, dryRun )


        res = lri.doImport()
        if res:
            self.printIt( "Import completed successfully", 1000000 )
        else:
            self.printIt( "The Import was a complete and utter failre!!!!", 100000 )

##############################################################################
if __name__=='__main__':
    app = QApplication( sys.argv )
    window = LightroomImportWindow()
    window.show()
    sys.exit( app.exec_() )
