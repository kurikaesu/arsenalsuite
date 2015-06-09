#!/usr/bin/env python2.5

import sys
import os
from ui import Ui_DirectoryImport
from DirectoryImport import DirectoryImport
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtSql import *

from bachutil import *
from blur.Stone import *
from Bach import *

#-----------------------------------------------------------------------------
class DirectoryImportWindow( Ui_DirectoryImport, QMainWindow ):
#-----------------------------------------------------------------------------
    def __init__( self, parent = None ):
        QMainWindow.__init__( self, parent )
        self.setupUi( self )

        self.connect( self.mDirectoryChoose, SIGNAL('clicked()'), self.onChoosePathBtn )
        self.connect( self.mGatherFilesBtn, SIGNAL('clicked()'), self.onGatherFilesBtn )
        self.connect( self.mImportBtn, SIGNAL('clicked()'), self.onImportBtn )
        self.connect( self.mSelectAllButton, SIGNAL('clicked()'), self.selectAll )
        self.connect( self.mSelectNoneButton, SIGNAL('clicked()'), self.selectNone )
        self.connect( self.mToggleButton, SIGNAL('clicked()'), self.toggleSelected )
        if len( sys.argv ) > 1 and os.path.isdir( sys.argv[ 1 ] ):
            self.mDirectory.setText( sys.argv[ 1 ] )
        self.di = DirectoryImport( self )

#-----------------------------------------------------------------------------
    def onChoosePathBtn(self):
        path = self.mDirectory.text()
        path = QFileDialog.getExistingDirectory( self, "Open Directory DB", path )
        if path is not None and path != "":
            self.mDirectory.setText( path )

#-----------------------------------------------------------------------------
    def getErrorInfo( self ):
        try:
            import traceback
            return traceback.format_exc()
        except:
            return "Error getting info out of the error.."

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
    def onGatherFilesBtn(self):
        QApplication.setOverrideCursor(QCursor(Qt.WaitCursor));
        res = False
        try:
            path = str( self.mDirectory.text() )
            doSubdirs = self.mDoSubdirsCB.isChecked()
            self.printIt( "Searching for files", 1000000 )
            res = self.di.doGather( path, doSubdirs )
            files = self.di.files
            self.printIt( "Gathering %s files" % len(files), 1000000 )
            files.sort()
            self.mFileTree.clear()
            self.addFiles( files )
        except:
            self.printIt( self.getErrorInfo() )
        QApplication.restoreOverrideCursor();
        if res:
            self.printIt( "Gather completed successfully", 1000000 )
        else:
            self.printIt( "The Gather was a complete and utter failre!!!!", 100000 )

#-----------------------------------------------------------------------------
    def onImportBtn(self):
        QApplication.setOverrideCursor(QCursor(Qt.WaitCursor));
        tags = []
        for tag in self.mTagEdit.text().split(","):
            tag = tag.simplified()
            if not len(tag) > 1: continue
            bachKeyword = BachKeyword.recordByName(tag)
            if bachKeyword.isRecord():
                tags.append(bachKeyword)
            else:
                msgBox = QMessageBox()
                msgBox.setText("Keyword does not exist")
                msgBox.setInformativeText("Do you want to add keyword: %s" % tag)
                msgBox.setStandardButtons( QMessageBox.Yes | QMessageBox.No )
                ret = msgBox.exec_()
                if ret == QMessageBox.Yes:
                    keyword = BachKeyword()
                    keyword.setName(tag)
                    keyword.commit()
                    tags.append(keyword)
        res = False
        fileList = []
        for item in self.mFileTree.findItems("*", Qt.MatchWildcard, 0):
            if item.checkState(0) == Qt.Checked:
                fileList.append( item.text(0) )

        try:
            res = self.di.doImport(fileList, tags)
        except:
            self.printIt( self.getErrorInfo() )

        if res:
            self.printIt( "Import completed successfully", 1000000 )
        else:
            self.printIt( "The Import was a complete and utter failre!!!!", 100000 )
        QApplication.restoreOverrideCursor();

#-----------------------------------------------------------------------------
    def addFiles(self, fileList):
        for file in fileList:
            if not len(file) > 1: continue
            oldPath = str()
            excluded = Qt.Unchecked
            exists = Qt.Unchecked

            # see if it exists in Bach already
            asset = BachAsset.recordByPath(file)
            if asset.isRecord():
                exists = Qt.Checked
                if asset.exclude(): excluded = Qt.Checked
            else:
            # see if the file has same checksum as something else
                identical = BachAsset.recordByHash(assetHash(file))
                if identical.isRecord():
                    oldPath = identical.path()
                    excluded = asset.exclude()

            item = QTreeWidgetItem(self.mFileTree)
            item.setCheckState(0, Qt.Checked)
            item.setText(0, file)
            item.setCheckState(1, exists)
            item.setCheckState(2, excluded)
            item.setText(3, oldPath)
            self.mFileTree.resizeColumnToContents(0)
            QApplication.instance().processEvents()

    def selectAll(self):
        for item in self.mFileTree.findItems("*", Qt.MatchWildcard, 0):
            item.setSelected(True)

    def selectNone(self):
        for item in self.mFileTree.findItems("*", Qt.MatchWildcard, 0):
            item.setSelected(False)

    def toggleSelected(self):
        for item in self.mFileTree.findItems("*", Qt.MatchWildcard, 0):
            if item.isSelected():
                if item.checkState(0) == Qt.Checked: item.setCheckState(0, Qt.Unchecked)
                else: item.setCheckState(0, Qt.Checked)

##############################################################################
if __name__=='__main__':
    app = QApplication( sys.argv )

    config = "bach_%s.ini" % os.environ["DRD_JOB"]
    initConfig(config, "/tmp/pybach.log")
    initStone(sys.argv)
    bach_loader()

    window = DirectoryImportWindow()
    window.show()
    sys.exit( app.exec_() )

