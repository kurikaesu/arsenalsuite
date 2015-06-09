#!/usr/bin/env python2.5

#
# Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
# SVN_META_HEADURL = "$HeadURL: $"
# SVN_META_ID = "$Id: LightroomImport.py 9408 2010-03-03 22:35:49Z brobison $"
#

import sys
import os
from PyQt4.QtSql import *
from BaseImport import *

#-----------------------------------------------------------------------------
class LightroomImport( BaseImport ):
#-----------------------------------------------------------------------------
    def __init__( self,
                  parent,
                  path,
                  emptyTables=False,
                  resetExcluded=False,
                  importKeywords=True,
                  importCollections=True,
                  dryRun=True):
        BaseImport.__init__( self, parent )
        self.fout = file( 'lightroomImport.bach.sql', 'wt' )
        self.path = path
        self.doEmptyTables = emptyTables
        self.doResetExcluded = resetExcluded
        self.doImportKeywords = importKeywords
        self.doIimportCollections = importCollections
        self.dryRun = dryRun

        self.imgData = {}
        self.exifData = {}
        self.keywordData = {}
        self.keywordMapping = []
        self.collectionData = {}
        self.collectionMapping = []

        self._lrdb = QSqlDatabase.addDatabase( "QSQLITE", "lrDB" )
        self._lrdb.setDatabaseName( self.path )

#-----------------------------------------------------------------------------
    def doImport(self):
        if not self._lrdb.open():
            self.p( "Couldn't open LightroomDB [%s]" % self.path )
            return False

        self.p( "Opened LightroomDB [%s]" % self.path )

        self.importImages()
        self.importExif()
        self.importKeywords()
        self.importKeywordMapping()
        self.importCollections()
        self.importCollectionMapping()
        self.p( "finished importing data" )

        self.collectPGData()

        self._doPGSqlMod( "BEGIN;" )
        self.p( "Writing Assets" )
        self.writeAssets()
        self.p( "Writing Keywords" )
        self.writeKeywords()
        self.p( "Writing Collections" )
        self.writeCollections()
        self._doPGSqlMod( "COMMIT;" )

        return True

#-----------------------------------------------------------------------------
    def _doLRSql( self, query ):
        self.p( '>>> Executing: [Lightroom] [%s]' % query )
        q = QSqlQuery( query, self._lrdb )
        self.p( '<<< Done' )
        return q

#-----------------------------------------------------------------------------
    def importImages(self):
        q = self._doLRSql("""
SELECT
    i.id_local,
    i.id_global,
    f.id_local,
    rfo.absolutePath,
    fo.pathFromRoot,
    f.idx_filename,
    i.fileHeight,
    i.fileWidth
FROM
    Adobe_images i,
    AgLibraryFile f,
    AgLibraryFolder fo,
    AgLibraryRootFolder rfo
WHERE
    f.id_local = i.rootFile and
    fo.id_local = f.folder and
    fo.rootFolder = rfo.id_local
ORDER BY
    rfo.absolutePath,
    fo.pathFromRoot,
    f.idx_filename;
        """)

        while( q.next() ):
            d = extractImgData( q )
            self.imgData[ d[ 'image_id' ] ] = d

#-----------------------------------------------------------------------------
    def importExif(self):
        q = self._doLRSql(""" SELECT * FROM AgHarvestedExifMetadata """)

        while(q.next()):
            d = extractExifData( q )
            self.exifData[ d[ 'image' ] ] = d

#-----------------------------------------------------------------------------
    def importKeywords(self):
        q = self._doLRSql("""SELECT id_local, name FROM AgLibraryKeyword""")

        while(q.next()):
            d = extractKeywordData( q )
            self.keywordData[ d[ 'keyword' ] ] = d

#-----------------------------------------------------------------------------
    def importKeywordMapping(self):
        q = self._doLRSql("""SELECT image, tag
                FROM AgLibraryKeywordImage""")

        while(q.next()):
            d = extractKeywordMappingData( q )
            self.keywordMapping.append(d)

#-----------------------------------------------------------------------------
    def importCollections(self):
        q = self._doLRSql("""SELECT id_local, name
                         FROM AgLibraryTag
                         WHERE kindName='AgCollectionTagKind'""")
        while(q.next()):
            d = extractCollectionData( q )
            self.collectionData[ d[ 'collection' ] ] = d

#-----------------------------------------------------------------------------
    def importCollectionMapping(self):
        q = self._doLRSql("""SELECT image, tag
                        FROM AgLibraryTagImage
                        WHERE tagKind='AgCollectionTagKind'""")
        while(q.next()):
            d = extractCollectionMappingData( q )
            self.collectionMapping.append(d)

#-----------------------------------------------------------------------------
    def writeAssets(self):

        if self.doEmptyTables:
            self._doPGSqlMod( 'DELETE FROM bachasset;\n' )
            self._doPGSqlMod( 'ALTER TABLE bachasset DROP CONSTRAINT bachasset_path;\n' )
        if self.doResetExcluded:
            self._doPGSqlMod( 'UPDATE bachasset SET exclude=true;\n' )

        cnt = 0;
        for img_key in self.imgData:
            if cnt % 100 == 0:
                self.pS( 'Asset %d/%d' % ( cnt, len( self.imgData ) ) )
            cnt += 1

            img = self.imgData[ img_key ]
            exif = self.exifData[ img_key ]

            assetId = self.assetExists( img[ 'fullPath' ] )
            if assetId != 0:
                if self.doResetExcluded:
                    self._doPGSqlMod( "UPDATE bachasset SET exclude=false WHERE keybachasset=%d;" % assetId )
                # self.p( 'Already have that one: [%s]' % img[ 'fullPath' ] )
                continue

            sql = formatAssetInsert( img, exif )
            self._doPGSqlMod( sql )

        if self.doEmptyTables:
            self._doPGSqlMod( 'ALTER TABLE bachasset ADD CONSTRAINT bachasset_path UNIQUE (path);\n' )

        self.collectPGData_Asset()


#-----------------------------------------------------------------------------
    def writeKeywords(self):
        if self.doEmptyTables:
            self._doPGSqlMod( 'DELETE FROM bachkeyword;\n' )
            self._doPGSqlMod( 'DELETE FROM bachkeywordmap;\n' )
            self._doPGSqlMod( 'ALTER TABLE bachkeyword DROP CONSTRAINT bachkeyword_name;\n' )

        cnt = 0;
        for key_key in self.keywordData:
            if cnt % 100 == 0:
                self.pS( 'Keyword %d/%d' % ( cnt, len( self.keywordData ) ) )
            cnt += 1

            keyword = self.keywordData[ key_key ]
            if self.keywordExists( keyword[ 'name' ] ):
                continue

            sql = formatKeywordInsert( keyword )
            self._doPGSqlMod( sql )

        self.collectPGData_Keyword()

        cnt = 0;
        for keywordMapping in self.keywordMapping:
            if cnt % 100 == 0:
                self.pS( 'KeywordMap %d/%d' % ( cnt, len( self.keywordMapping ) ) )
            cnt += 1

            sql, img, key, imgId, keyId = formatKeywordMappingInsert( keywordMapping, self.imgData, self.keywordData, self )
            if self.keywordMapExists(img, key):
                continue
            self._doPGSqlMod( sql )


        if self.doEmptyTables:
            self._doPGSqlMod( 'ALTER TABLE bachkeyword ADD CONSTRAINT bachkeyword_name UNIQUE (name);\n' )

#-----------------------------------------------------------------------------
    def writeCollections(self):
        if self.doEmptyTables:
            self._doPGSqlMod( 'DELETE FROM bachbucket;\n' )
            self._doPGSqlMod( 'DELETE FROM bachkeywordmap;\n' )
            self._doPGSqlMod( 'ALTER TABLE bachkeyword DROP CONSTRAINT bachbucket_name;\n' )

        cnt = 0;
        for collection_key in self.collectionData:
            if cnt % 100 == 0:
                self.pS( 'Collection %d/%d' % ( cnt, len( self.collectionData ) ) )
            cnt += 1

            collection = self.collectionData[ collection_key ]
            if self.collectionExists( collection[ 'name' ] ):
                continue
            sql = formatCollectionInsert( collection )
            self._doPGSqlMod( sql )

        self.collectPGData_Collection()

        cnt = 0;
        for collectionMapping in self.collectionMapping:
            if cnt % 100 == 0:
                self.pS( 'CollectionMap %d/%d' % ( cnt, len( self.collectionMapping ) ) )
            cnt += 1

            sql, img, col, imgId, colId = formatCollectionMappingInsert( collectionMapping, self.imgData, self.collectionData, self )
            if self.collectionMapExists(img, col):
                continue
            self._doPGSqlMod( sql )

        if self.doEmptyTables:
            self._doPGSqlMod( 'ALTER TABLE bachbucket ADD CONSTRAINT bachbucket_name UNIQUE (name);\n' )

#-----------------------------------------------------------------------------
# helpers
#-----------------------------------------------------------------------------
def getInt( v ):
    if len( v ) == 0:
        return 0
    return float( v )

#-----------------------------------------------------------------------------
def getAperture( v ):
    if len( v ) == 0:
        return 0.0
    return round( 2 ** ( float( v ) / 2.0 ) )

#-----------------------------------------------------------------------------
def getShutterSpeed( v ):
    if len( v ) == 0:
        return 0.0
    return round( 2 ** float( v ) )

#-----------------------------------------------------------------------------
def getFocalLength( v ):
    if len( v ) == 0:
        return 0
    return float( v )

#-----------------------------------------------------------------------------
def getIsoSpeedRating( v ):
    if len( v ) == 0:
        return 0
    return float( v )

#-----------------------------------------------------------------------------
def extractImgData( query ):
    d = {}
    d[ 'image_id' ] = toI( query.value(0) )        # int key!
    d[ 'image_id_global' ] = toS( query.value(1) )
    d[ 'image_id_other' ] = toI( query.value(2) )        # int key!
    d[ 'rootFolderName' ] = toS( query.value(3) )
    d[ 'folderName' ] = toS( query.value(4) )
    d[ 'fileName' ] = toS( query.value(5) )
    d[ 'width' ] = toI( query.value(6) )
    d[ 'height' ] = toI( query.value(7) )

    d[ 'fullDir' ] = os.path.join( d[ 'rootFolderName' ], d[ 'folderName' ] )
    d[ 'fullPath' ] = os.path.join( d[ 'fullDir' ], d[ 'fileName' ] )
    return d

#-----------------------------------------------------------------------------
def extractExifData( query ):
    d = {}
    d[ 'image_id' ] = toS( query.value(0) )
    d[ 'image' ] = toI( query.value(1) )    # int key!

    # exposure data
    d[ 'aperture' ] = getAperture( toS( query.value(2) ) )
    d[ 'focalLength' ] = getFocalLength( toS( query.value(9) ) )
    d[ 'isoSpeedRating' ] = getIsoSpeedRating( toS( query.value(11) ) )
    d[ 'shutterSpeed' ] = getShutterSpeed( toS( query.value(13) ) )

    # camera data
    d[ 'cameraModelRef' ] = getInt( toS( query.value(3) ) )
    d[ 'cameraSNRef' ] = getInt( toS( query.value(4) ) )
    d[ 'lensRef' ] = getInt( toS( query.value(12) ) )

    # other data
    d[ 'dateDay' ] = toS( query.value(5) )
    d[ 'dateMonth' ] = toS( query.value(6) )
    d[ 'dateYear' ] = toS( query.value(7) )
    d[ 'flashFired' ] = toS( query.value(8) )
    d[ 'hasGPS' ] = toS( query.value(10) )

    return d

#-----------------------------------------------------------------------------
def extractKeywordData( query ):
    d = {}
    d[ 'keyword' ] = toI( query.value(0) ) # int key!
    d[ 'name' ] = toS( query.value(1) )
    return d

#-----------------------------------------------------------------------------
def extractKeywordMappingData( query ):
    d = {}
    d[ 'image' ] = toI( query.value(0) )
    d[ 'keyword' ] = toI( query.value(1) )
    return d

#-----------------------------------------------------------------------------
def extractCollectionData( query ):
    d = {}
    d[ 'collection' ] = toI( query.value(0) )
    d[ 'name' ] = toS( query.value(1) )
    return d

#-----------------------------------------------------------------------------
def extractCollectionMappingData( query ):
    d = {}
    d[ 'image' ] = toI( query.value(0) )
    d[ 'collection' ] = toI( query.value(1) )
    return d

#-----------------------------------------------------------------------------
def formatExif( exif ):
    s = 'aperture:f/%.1f shutterSpeed:1/%.1f focalLength:%.1fmm isoSpeedRating:%d' % ( exif[ 'aperture' ], exif[ 'shutterSpeed' ], exif[ 'focalLength' ], exif[ 'isoSpeedRating' ] )
    return s

#-----------------------------------------------------------------------------
def formatAssetInsert( img, exif ):
    exifS = formatExif( exif )
    s = "INSERT into bachasset (path, exif, width, height, filetype, directory, aperture, shutterSpeed, isoSpeedRating, focalLength, camera, cameraSN, lens) VALUES"+\
        " ('%s', '%s', %d, %d, 2, '%s', %f, %f, %d, %f, %d, %d, %d); -- id: %d guid: %s" % \
        ( esc(img[ 'fullPath' ]), esc(exifS), img[ 'width' ], img[ 'height' ], esc(img[ 'fullDir' ]), exif[ 'aperture' ], \
        exif[ 'shutterSpeed' ], exif[ 'isoSpeedRating' ], exif[ 'focalLength' ], exif[ 'cameraModelRef' ], exif[ 'cameraSNRef' ], exif[ 'lensRef' ], img[ 'image_id' ], esc(img[ 'image_id_global' ]) )
    return s

#-----------------------------------------------------------------------------
def formatKeywordInsert( keyword ):
    s = "INSERT into bachkeyword (name) VALUES ( '%s' );" % esc(keyword[ 'name' ])
    return s

#-----------------------------------------------------------------------------
def formatKeywordMappingInsert( keywordMapping, imgData, keywordData, querier ):
    imgKey = keywordMapping[ 'image' ]
    if not imgData.has_key( imgKey ):
        print 'no image', keywordMapping
        return ''

    keywordKey = keywordMapping[ 'keyword' ]
    if not keywordData.has_key( keywordKey ):
        print 'no keyword', keywordMapping
        return ''

    img = imgData[ imgKey ]
    keyword = keywordData[ keywordKey ]

    assetId = querier.getAssetId( img[ 'fullPath' ] )
    keywordId = querier.getKeywordId( keyword[ 'name' ] )

    s = "INSERT into bachkeywordmap( fkeybachasset, fkeybachkeyword ) VALUES ( %d, %d );" % ( assetId, keywordId )
    if assetId == 0 or keywordId == 0:
        print 'dubious', s, esc(img[ 'fullPath' ]), esc(keyword[ 'name' ]), assetId, keywordId
        s = "INSERT into bachkeywordmap( fkeybachasset, fkeybachkeyword ) VALUES ( %s, %s );" % \
            ( "(SELECT keybachasset FROM bachasset where path = '%s')" % esc(img[ 'fullPath' ]), \
              "(SELECT keybachkeyword FROM bachkeyword where name = '%s')" % esc(keyword[ 'name' ]) )
    return s, esc(img[ 'fullPath' ]), esc(keyword[ 'name' ]), assetId, keywordId


#-----------------------------------------------------------------------------
def formatCollectionInsert( collection ):
    s = "INSERT into bachbucket (name) VALUES ( '%s' );" % esc(collection[ 'name' ])
    return s

#-----------------------------------------------------------------------------
def formatCollectionMappingInsert( collectionMapping, imgData, collectionData, querier ):
    imgKey = collectionMapping[ 'image' ]
    if not imgData.has_key( imgKey ):
        print 'no image', keywordMapping
        return ''

    collectionKey = collectionMapping[ 'collection' ]
    if not collectionData.has_key( collectionKey ):
        print 'no collection', collectionMapping
        return ''

    img = imgData[ imgKey ]
    collection = collectionData[ collectionKey ]

    assetId = querier.getAssetId( img[ 'fullPath' ] )
    collectionId = querier.getCollectionId( collection[ 'name' ] )

    s = "INSERT into bachbucketmap( fkeybachasset, fkeybachbucket ) VALUES ( %d, %d );" % ( assetId, collectionId )
    if assetId == 0 or collectionId == 0:
        print 'dubious', s, esc(img[ 'fullPath' ]), esc(collection[ 'name' ]), assetId, collectionId
        s = "INSERT into bachbucketmap( fkeybachasset, fkeybachbucket ) VALUES ( %s, %s );" % \
            ( "(SELECT keybachasset FROM bachasset where path = '%s')" % esc(img[ 'fullPath' ]), \
              "(SELECT keybachbucket FROM bachbucket where name = '%s')" % esc(collection[ 'name' ]) )
    return s, esc(img[ 'fullPath' ]), esc(collection[ 'name' ]), assetId, collectionId

