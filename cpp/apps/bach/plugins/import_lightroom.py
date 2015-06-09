#!/usr/bin/python2.5

import sys, os, time
from optparse import OptionParser
from Bach import *
from PyQt4.QtCore import *
from PyQt4.QtGui import *

SVN_META_HEADURL = "$HeadURL:$"
SVN_META_ID = "$Id: import_lightroom.py 9408 2010-03-03 22:35:49Z brobison $"
version = "%prog\n"+"SVN_URL: %s\nSVN_ID:  %s" % ( SVN_META_HEADURL, SVN_META_ID )

class bachLightRoomImport():
    def __init__(self):
        self._db = QSqlDatabase("QSQLITE","lrDb")
        self.imgData = {}
        self.exifData = {}
        self.keywordData = {}
        self.keyMapping = []
        self.collectionData = {}
        self.collectionMapping = []

    def setDbFile(self, path):
        self._db.setDatbaseName(path)

    def importImages(self):
        q = QSqlQuery("""
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

    def importExif(self):
        q = QSqlQuery(""" SELECT * FROM AgHarvestedExifMetadata """)
        while(q.next()):
            d = extractExifData( q )
            self.exifData[ d[ 'image' ] ] = d

    def importKeywords(self):
        q = QSqlQuery("""SELECT id_local, name FROM AgLibraryKeyword""")

        while(q.next()):
            d = extractKeywordData( q )
            self.keywordData[ d[ 'keyword' ] ] = d

    def importKeywordMapping(self):
        q = QSqlQuery("""SELECT image, tag
                FROM AgLibraryKeywordImage""")

        while(q.next()):
            d = extractKeywordMappingData( q )
            self.keywordMapping.append(d)

    def importCollections(self):
        q = QSqlQuery("""SELECT id_local, name
                         FROM AgLibraryTag
                         WHERE kindName='AgCollectionTagKind'""")
        while(q.next()):
            d = extractCollectionData( q )
            self.collectionData[ d[ 'collection' ] ] = d

    def importCollectionMapping(self):
        q = QSqlQuery("""SELECT image, tag
                        FROM AgLibraryTagImage
                        WHERE tagKind='AgCollectionTagKind'"""
        while(q.next()):
            d = extractCollectionData( q )
            self.collectionMapping.append(d)

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
def esc( s ):
	s = s.replace( '\'', '\'\'' )
	return s

#-----------------------------------------------------------------------------
# imgdata:
# 0         1                                     2         3                      4
# id_local |id_global                            |id_local |absolutePath          |pathFromRoot
# 883203   |5E613B44-D635-48D0-9989-B4106F52F2CA |         |/drd/jobs/hf1/to_kmm/ |080729_LTO_AL/kref/job_data/lovelace_firedrive/20030126/gouverno (wreck)/wild/wreck  jc 2/
# 5             6           7
# idx_filename |fileHeight |fileWidth
# dsc_5388.jpg |1312.0     |2000.0

def extractImgData( query ):
	d = {}
	d[ 'image_id' ] = int( query.value(0).toInt() )		# int key!
	d[ 'image_id_global' ] = query.value(1).toString()
	d[ 'image_id_other' ] = int( query.value(2).toInt() )		# int key!
	d[ 'rootFolderName' ] = query.value(3).toString()
	d[ 'folderName' ] = query.value(4).toString()
	d[ 'fileName' ] = query.value(5).toString()
	d[ 'width' ] = int( query.value(6).toInt() )
	d[ 'height' ] = int( query.value(7).toInt() )

	d[ 'fullDir' ] = os.path.join( d[ 'rootFolderName' ], d[ 'folderName' ] )
	d[ 'fullPath' ] = os.path.join( d[ 'fullDir' ], d[ 'fileName' ] )
	return d

#-----------------------------------------------------------------------------
def extractExifData( query ):
	parts = line.strip().split( '|' )
	d = {}
	d[ 'image_id' ] = query.value(0).toString()
	d[ 'image' ] = int( query.value(1).toInt() )	# int key!

	# exposure data
	d[ 'aperture' ] = getAperture( query.value(2).toString() )
	d[ 'focalLength' ] = getFocalLength( query.value(9).toString() )
	d[ 'isoSpeedRating' ] = getIsoSpeedRating( query.value(11).toString() )
	d[ 'shutterSpeed' ] = getShutterSpeed( query.value(13).toString() )

	# camera data
	d[ 'cameraModelRef' ] = getInt(query.value(3).toString()) 
	d[ 'cameraSNRef' ] = getInt( query.value(4).toString() )
	d[ 'lensRef' ] = getInt( query.value(12).toString() )

	# other data
	d[ 'dateDay' ] = query.value(5).toString()
	d[ 'dateMonth' ] = query.value(6).toString()
	d[ 'dateYear' ] = query.value(7).toString()
	d[ 'flashFired' ] = query.value(8).toString()
	d[ 'hasGPS' ] = query.value(10).toString()

	return d

#-----------------------------------------------------------------------------
# 0         1
# id_local |name
# 23896    |India New Delhi
def extractKeywordData( query ):
	d = {}
	d[ 'keyword' ] = int( query.value(0).toInt() ) # int key!
	d[ 'name' ] = query.value(1).toString()
	return d

#-----------------------------------------------------------------------------
# 0      1
# image |tag
# 23891 |23896
def extractKeywordMappingData( query ):
	d = {}
	d[ 'image' ] = int( query.value(0).toInt() )
	d[ 'keyword' ] = int( query.value(1).toInt() )
	return d

#-----------------------------------------------------------------------------
# 0         1
# id_local |name
# 10       |Colored Red
def extractCollectionData( query ):
	d = {}
	d[ 'collection' ] = int( query.value(0).toInt() )
	d[ 'name' ] = query.value(1).toString()
	return d

#-----------------------------------------------------------------------------
# 0      1
# image |tag
# 122   |5854382
def extractCollectionMappingData( query ):
	d = {}
	d[ 'image' ] = int( query.value(0).toInt() )
	d[ 'collection' ] = int( query.value(1).toInt() )
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
def formatKeywordMappingInsert( keywordMapping, imgData, keywordData ):
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
    s = "INSERT into bachkeywordmap( fkeybachasset, fkeybachkeyword ) VALUES ( %s, %s );" % \
        ( "(SELECT keybachasset FROM bachasset where path = '%s')" % esc(img[ 'fullPath' ]), \
          "(SELECT keybachkeyword FROM bachkeyword where name = '%s')" % esc(keyword[ 'name' ]) )
    return s

#-----------------------------------------------------------------------------
def formatCollectionInsert( collection ):
    s = "INSERT into bachbucket (name) VALUES ( '%s' );" % esc(collection[ 'name' ])
    return s

#-----------------------------------------------------------------------------
def formatCollectionMappingInsert( collectionMapping, imgData, collectionData ):
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
    s = "INSERT into bachbucketmap( fkeybachasset, fkeybachbucket ) VALUES ( %s, %s );" % \
        ( "(SELECT keybachasset FROM bachasset where path = '%s')" % esc(img[ 'fullPath' ]), \
          "(SELECT keybachbucket FROM bachbucket where name = '%s')" % esc(collection[ 'name' ]) )
    return s


#-----------------------------------------------------------------------------
def process():
	imgData = readImgData()
	exifData = readExifData()
	keywordData = readKeywordData()
	keywordMappingData = readKeywordMappingData()
	collectionData = readCollectionData()
	collectionMappingData = readCollectionMappingData()

	#-----------------------------------------------------------------------------
	# assets
	fout = file( '00_setBachAssets.sql', 'wt' )
	fout.write( 'BEGIN;\n' )

	if options.do_delete:
		fout.write( 'DELETE FROM bachasset;\n' )
		fout.write( 'ALTER TABLE bachasset DROP CONSTRAINT bachasset_path;\n' )

	lines = []
	for img_key in imgData:
		img = imgData[ img_key ]
		exif = exifData[ img_key ]
		lines.append( formatAssetInsert( img, exif ) )
	lines.sort()
	fout.write( '\n'.join( lines ) )

	if options.do_delete:
		fout.write( 'ALTER TABLE bachasset ADD CONSTRAINT bachasset_path UNIQUE (path);\n' );
	fout.write( 'COMMIT;\n' );
	fout.close()

	#-----------------------------------------------------------------------------
	# keywords

	fout = file( '01_setBachKeywords.sql', 'wt' )
	fout.write( 'BEGIN;\n' )

	if options.do_delete:
		fout.write( 'DELETE FROM bachkeyword;\n' )
		fout.write( 'ALTER TABLE bachkeyword DROP CONSTRAINT bachkeyword_name;\n' )

	lines = []
	for key_key in keywordData:
	    keyword = keywordData[ key_key ]
	    lines.append( formatKeywordInsert( keyword ) )
	lines.sort()
	fout.write( '\n'.join( lines ) )

	if options.do_delete:
		fout.write( 'ALTER TABLE bachkeyword ADD CONSTRAINT bachkeyword_name UNIQUE (name);\n' );
	fout.write( 'COMMIT;\n' );
	fout.close()

	#-----------------------------------------------------------------------------
	# collections/buckets
	fout = file( '02_setBachCollections.sql', 'wt' )
	fout.write( 'BEGIN;\n' )

	if options.do_delete:
		fout.write( 'DELETE FROM bachbucket;\n' )
		fout.write( 'ALTER TABLE bachbucket DROP CONSTRAINT bachbucket_name;\n' )

	lines = []
	for collection_key in collectionData:
	    collection = collectionData[ collection_key ]
	    lines.append( formatCollectionInsert( collection ) )
	lines.sort()
	fout.write( '\n'.join( lines ) )

	if options.do_delete:
		fout.write( 'ALTER TABLE bachbucket ADD CONSTRAINT bachbucket_name UNIQUE (name);\n' );
	fout.write( 'COMMIT;\n' );
	fout.close()


	#-----------------------------------------------------------------------------
	# keyword mapping
	fout = file( '03_setBachKeywordMapping.sql', 'wt' )
	fout.write( 'BEGIN;\n' )

	if options.do_delete:
		fout.write( 'DELETE FROM bachkeywordmap;\n' )

	lines = []
	for keywordMapping in keywordMappingData:
	    lines.append( formatKeywordMappingInsert( keywordMapping, imgData, keywordData ) )
	fout.write( '\n'.join( lines ) )

	fout.write( 'COMMIT;\n' );
	fout.close()

	#-----------------------------------------------------------------------------
	# collection/bucket mapping
	fout = file( '04_setBachCollectionMapping.sql', 'wt' )
	fout.write( 'BEGIN;\n' )

	if options.do_delete:
		fout.write( 'DELETE FROM bachkeywordmap;\n' )

	lines = []
	for collectionMapping in collectionMappingData:
	    lines.append( formatCollectionMappingInsert( collectionMapping, imgData, collectionData ) )
	fout.write( '\n'.join( lines ) )

	fout.write( 'COMMIT;\n' );
	fout.close()


process()
