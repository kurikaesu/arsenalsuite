


%MappedType MappedBachAssetList
{
%TypeHeaderCode
#include <record.h>
#include <pyembed.h>
#include "bachassetlist.h"
#include "bachassettable.h"
#define MappedBachAssetList BachAssetList
%End

%ConvertToTypeCode
	return 0;
%End

%ConvertFromTypeCode
	return sipWrapRecordList( sipCpp, true, BachAssetSchema::instance() );
%End
};

class BachAssetList : RecordList
{
%TypeHeaderCode
#include <bachassetlist.h>
#include "bachbucketmap.h"
#include "bachkeywordmap.h"

%End
public:
	BachAssetList();
	BachAssetList( const RecordList &, Table * t = 0 );
	BachAssetList( const BachAssetList & );
	BachAssetList( const Record &, Table * t = 0 );
	
	~BachAssetList();

//	void insert( BachAssetIter, const BachAsset & );

//	BachAssetIter at( uint );

//	BachAssetIter find( const Record & );

//	BachAssetIter remove( const BachAssetIter & );

	int remove( const Record & );
	
	int remove();
	
	//BachAssetIter begin();

	//BachAssetIter end();

	Table * table() const;
	TableSchema * schema() const;

	MappedBachAssetList filter( const QString & column, const QRegExp & re, bool keepMatches = true ) const;
	MappedBachAssetList filter( const QString & column, const QVariant & value, bool keepMatches = true ) const;
	MappedBachAssetList sorted( const QString & c, bool a = true ) const;

	MappedBachAssetList unique() const;
	MappedBachAssetList reversed() const;
	MappedBachAssetList reloaded() const;

	QStringList paths() const;
	MappedBachAssetList & setPaths( const QString & );
	QStringList tags() const;
	MappedBachAssetList & setTags( const QString & );
	QList<QDateTime> importedDatetimes() const;
	MappedBachAssetList & setImportedDatetimes( const QDateTime & );
	QStringList exif() const;
	MappedBachAssetList & setExif( const QString & );
	QList<uint> widths() const;
	MappedBachAssetList & setWidths( const uint & );
	QList<uint> heights() const;
	MappedBachAssetList & setHeights( const uint & );
	QStringList cachePrefixes() const;
	MappedBachAssetList & setCachePrefixes( const QString & );
	QList<uint> filetypes() const;
	MappedBachAssetList & setFiletypes( const uint & );
	QList<uint> filesizes() const;
	MappedBachAssetList & setFilesizes( const uint & );
	QList<uint> cliplengths() const;
	MappedBachAssetList & setCliplengths( const uint & );
	QList<uint> frameStarts() const;
	MappedBachAssetList & setFrameStarts( const uint & );
	QList<uint> frameEnds() const;
	MappedBachAssetList & setFrameEnds( const uint & );
	QList<uint> padLengths() const;
	MappedBachAssetList & setPadLengths( const uint & );
	QStringList directories() const;
	MappedBachAssetList & setDirectories( const QString & );
	QList<float> apertures() const;
	MappedBachAssetList & setApertures( const float & );
	QList<float> shutterSpeeds() const;
	MappedBachAssetList & setShutterSpeeds( const float & );
	QList<uint> isoSpeedRatings() const;
	MappedBachAssetList & setIsoSpeedRatings( const uint & );
	QList<QDateTime> creationDatetimes() const;
	MappedBachAssetList & setCreationDatetimes( const QDateTime & );
	QList<float> focalLengths() const;
	MappedBachAssetList & setFocalLengths( const float & );
	QList<uint> cameras() const;
	MappedBachAssetList & setCameras( const uint & );
	QList<uint> cameraSNs() const;
	MappedBachAssetList & setCameraSNs( const uint & );
	QList<uint> lenses() const;
	MappedBachAssetList & setLenses( const uint & );
	QStringList cachedKeywords() const;
	MappedBachAssetList & setCachedKeywords( const QString & );
	QList<int> tnRotates() const;
	MappedBachAssetList & setTnRotates( const int & );
	QList<bool> excludes() const;
	MappedBachAssetList & setExcludes( const bool & );
	QStringList hashs() const;
	MappedBachAssetList & setHashs( const QString & );
	QList<int> positions() const;
	MappedBachAssetList & setPositions( const int & );
	MappedBachBucketMapList bachBucketMaps();
	MappedBachKeywordMapList bachKeywordMaps();


};


