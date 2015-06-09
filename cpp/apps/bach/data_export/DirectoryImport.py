#!/usr/bin/env python2.5

#
# Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
# SVN_META_HEADURL = "$HeadURL: $"
# SVN_META_ID = "$Id: DirectoryImport.py 9408 2010-03-03 22:35:49Z brobison $"
#

import sys
import os
import subprocess

from Bach import *
from bachutil import *
from PyQt4.QtCore import *

#-----------------------------------------------------------------------------
class DirectoryImport():
#-----------------------------------------------------------------------------
    def __init__( self, parent=None ):
        self.files = []
        self.dryRun = False
        self.verbose = False

    def setVerbose( self, verbose ):
        self.verbose = verbose

#-----------------------------------------------------------------------------
    def doGather(self, path, doSubdirs):
        self.path = path
        self.doSubdirs = doSubdirs
        self.files = []

        listCmd = ["./lsq", "-type", "image", "-fullpath"]
        if doSubdirs: listCmd.append("-R")
        listCmd.append(path)
        fileList = subprocess.Popen(listCmd, stdout=subprocess.PIPE).communicate()[0]
        self.files = fileList.split("\n")

        return True

#-----------------------------------------------------------------------------
    def doImport(self, fileList, tagList):
        if self.verbose: print "importing %s assets" % len(fileList)
        uuid = getUuid()
        fh = open("/drd/software/int/apps/bach/tn_requests/%s"%uuid,"w")

        for path in fileList:
            if not len(path) > 1: continue
            if self.verbose: print "thinking about importing %s" % path

            # see if it exists in Bach already
            asset = BachAsset.recordByPath(path)
            if asset.isRecord():
                if self.verbose: print "asset already exists %s" % path
                continue
            if self.verbose: print "adding asset %s" % path

            # see if the file has same checksum as something else
            identical = BachAsset.recordByHash(assetHash(path))
            if identical.isRecord():
                if self.verbose: print "found identical asset %s" % identical.path()
                asset = identical

            asset.setPath(path)
            asset.setDirectory(os.path.dirname(str(path)))
            if not asset.directory().endsWith( '/' ):
                asset.setDirectory( asset.directory() + '/' )
            asset.setFiletype(2)
            asset.setHash( assetHash(path) )
            asset.setExif( exifData(path) )
            asset.setExclude(False)
            asset.commit()

            if self.verbose: print "added asset %s" % path

            # add path to a file for tn generation
            fh.write(thumbCommand(path,256)+"\n")
            fh.write(thumbCommand(path,512)+"\n")

            for tag in tagList:
                kwm = BachKeywordMap()
                kwm.setBachAsset(asset)
                kwm.setBachKeyword(tag)
                kwm.commit()

        fh.close()
        return True

