#!/usr/bin/env python2.5

#
# Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
# SVN_META_HEADURL = "$HeadURL: $"
# SVN_META_ID = "$Id: BaseImport.py 9408 2010-03-03 22:35:49Z brobison $"
#

import sys
import os
from PyQt4.QtSql import *

#-----------------------------------------------------------------------------
class BaseImport:
#-----------------------------------------------------------------------------
    def __init__( self, parent ):
        self.parent = parent

        self.pgAssetName2Id = {}
        self.pgAssetId2Name = {}

        self.pgKeywordName2Id = {}
        self.pgKeywordId2Name = {}

        self.pgCollectionName2Id = {}
        self.pgCollectionId2Name = {}

        self.pgCollectionMapping = {}
        self.pgKeywordMapping = {}

        self._pgdb = QSqlDatabase.addDatabase( "QPSQL", "pgDB" )
        self._pgdb.setDatabaseName( "bach" )
        self._pgdb.setHostName( "sql01" )
        self._pgdb.setUserName( "bach" )
        self._pgdb.setPassword( "escher" )
        if not self._pgdb.open():
            self.p( "Couldn't open Bach DB" )
            return False
        self.p( "Opened Bach DB" )

#-----------------------------------------------------------------------------
    def p( self, p ):
        self.parent.printIt( p )

#-----------------------------------------------------------------------------
    def pS( self, p ):
        self.parent.stat( p )

#-----------------------------------------------------------------------------
    def _doPGSql( self, query ):
        self.p( '>>> Executing: [Bach] [%s]' % query )
        q = QSqlQuery( query, self._pgdb )
        self.p( '<<< Done' )
        return q

#-----------------------------------------------------------------------------
    def _doPGSqlMod( self, query ):
        self.fout.write( query )
        self.fout.write( '\n' )

        if self.dryRun:
            return
        #self.p( '>>> Executing: [Bach] [%s]' % query )
        q = QSqlQuery( query, self._pgdb )
        #self.p( '<<< Done' )
        return q

#-----------------------------------------------------------------------------
    def collectPGData_Asset(self):
        q = self._doPGSql("""SELECT path, keybachasset FROM bachasset""")
        while(q.next()):
            name, id = extractPGAsset( q )
            self.pgAssetName2Id[ name ] = id
            self.pgAssetId2Name[ id ] = name

#-----------------------------------------------------------------------------
    def collectPGData_Keyword(self):
        q = self._doPGSql("""SELECT name, keybachkeyword FROM bachkeyword""")
        while(q.next()):
            name, id = extractPGKeyword( q )
            self.pgKeywordName2Id[ name ] = id
            self.pgKeywordId2Name[ id ] = name

        q = self._doPGSql("""SELECT path, name FROM
            bachkeywordmap, bachasset, bachkeyword
        WHERE
            fkeybachasset=keybachasset AND
            fkeybachkeyword=keybachkeyword""")

        while(q.next()):
            d = extractPGKeywordMapping( q )
            path = d[ 0 ]
            name = d[ 1 ]
            if not path in self.pgKeywordMapping:
                self.pgKeywordMapping[ path ] = {}
            self.pgKeywordMapping[ path ][ name ] = 1

#-----------------------------------------------------------------------------
    def collectPGData_Collection(self):
        q = self._doPGSql("""SELECT name, keybachbucket FROM bachbucket""")
        while(q.next()):
            name, id = extractPGCollection( q )
            self.pgCollectionName2Id[ name ] = id
            self.pgCollectionId2Name[ id ] = name

        q = self._doPGSql("""SELECT path, name FROM
            bachbucketmap, bachasset, bachbucket
        WHERE
            fkeybachasset=keybachasset AND
            fkeybachbucket=keybachbucket""")

        while(q.next()):
            d = extractPGCollectionMapping( q )
            path = d[ 0 ]
            name = d[ 1 ]
            if not path in self.pgCollectionMapping:
                self.pgCollectionMapping[ path ] = {}
            self.pgCollectionMapping[ path ][ name ] = 1

#-----------------------------------------------------------------------------
    def collectPGData(self):
        self.p( "Preloading Bach data..." )

        #----------------
        self.collectPGData_Asset()
        self.collectPGData_Keyword()
        self.collectPGData_Collection()
        #----------------
        self.p( "... finished" )

#-----------------------------------------------------------------------------
    def assetExists(self, path):
        if not path in self.pgAssetName2Id:
            return 0
        return self.pgAssetName2Id[ path ]

#-----------------------------------------------------------------------------
    def getAssetId( self, path ):
        return self.assetExists( path )


#-----------------------------------------------------------------------------
    def keywordExists(self, name):
        if not name in self.pgKeywordName2Id:
            return 0
        return self.pgKeywordName2Id[ name ]

#-----------------------------------------------------------------------------
    def getKeywordId( self, name ):
        return self.keywordExists( name )

#-----------------------------------------------------------------------------
    def keywordMapExists(self, imgPath, keywordName):
        if not imgPath in self.pgKeywordMapping:
            return False
        if not keywordName in self.pgKeywordMapping[ imgPath ]:
            return False
        return True

#-----------------------------------------------------------------------------
    def collectionExists(self, name):
        if not name in self.pgCollectionName2Id:
            return 0
        return self.pgCollectionName2Id[ name ]

#-----------------------------------------------------------------------------
    def getCollectionId( self, name ):
        return self.collectionExists( name )


#-----------------------------------------------------------------------------
    def collectionMapExists(self, imgPath, collectionName):
        if not imgPath in self.pgCollectionMapping:
            return False
        if not collectionName in self.pgCollectionMapping[ imgPath ]:
            return False
        return True

#-----------------------------------------------------------------------------
def esc( s ):
    s = s.replace( '\'', '\'\'' )
    return s

#-----------------------------------------------------------------------------
def toS( variant ):
    v = variant.toString()
    return str( v.toAscii() )

#-----------------------------------------------------------------------------
def toI( variant ):
    v, ok = variant.toInt()
    return int( v )

#-----------------------------------------------------------------------------
def extractPGAsset( query ):
    name = toS( query.value( 0 ) )
    id = toI( query.value( 1 ) )
    return name, id

#-----------------------------------------------------------------------------
def extractPGKeyword( query ):
    name = toS( query.value( 0 ) )
    id = toI( query.value( 1 ) )
    return name, id

#-----------------------------------------------------------------------------
def extractPGCollection( query ):
    name = toS( query.value( 0 ) )
    id = toI( query.value( 1 ) )
    return name, id

#-----------------------------------------------------------------------------
def extractPGCollectionMapping( query ):
    d = []
    d.append( toS( query.value(0) ) )
    d.append( toS( query.value(1) ) )
    return d

#-----------------------------------------------------------------------------
def extractPGKeywordMapping( query ):
    d = []
    d.append( toS( query.value(0) ) )
    d.append( toS( query.value(1) ) )
    return d
