
%MappedType MappedBachAsset
{
%TypeHeaderCode
#include <record.h>
#include <pyembed.h>
#include "bachassetlist.h"
#include "bachassettable.h"
#define MappedBachAsset BachAsset
%End

%ConvertToTypeCode
	if (sipIsErr == NULL)
		return isPythonRecordInstance(sipPy) ? 1 : 0;

	*sipCppPtr = new BachAsset(sipUnwrapRecord(sipPy));

	return sipGetState(sipTransferObj);
%End

%ConvertFromTypeCode
	return sipWrapRecord( sipCpp, true, BachAssetSchema::instance() );
%End
};

class BachAsset : Record
{
%TypeHeaderCode
#include <bachasset.h>
#include "bachbucketmap.h"
#include "bachkeywordmap.h"

%End
public:
	BachAsset();

	BachAsset( uint key );

	BachAsset( const BachAsset & other );

	BachAsset( const Record & r );

	BachAsset( RecordImp * imp, bool checkType = true );
	
	MappedBachAsset copy() const;

	bool operator==( const Record & other ) const;

	bool operator!=( const Record & other ) const;
	
	bool operator <( const Record & other ) const;

	long __hash__() const;
%MethodCode
		sipRes = sipCpp->key();
%End

	QString path() const;
	MappedBachAsset & setPath( const QString & );
	QString tags() const;
	MappedBachAsset & setTags( const QString & );
	QDateTime importedDatetime() const;
	MappedBachAsset & setImportedDatetime( const QDateTime & );
	QString exif() const;
	MappedBachAsset & setExif( const QString & );
	uint width() const;
	MappedBachAsset & setWidth( const uint & );
	uint height() const;
	MappedBachAsset & setHeight( const uint & );
	QString cachePrefix() const;
	MappedBachAsset & setCachePrefix( const QString & );
	uint filetype() const;
	MappedBachAsset & setFiletype( const uint & );
	uint filesize() const;
	MappedBachAsset & setFilesize( const uint & );
	uint cliplength() const;
	MappedBachAsset & setCliplength( const uint & );
	uint frameStart() const;
	MappedBachAsset & setFrameStart( const uint & );
	uint frameEnd() const;
	MappedBachAsset & setFrameEnd( const uint & );
	uint padLength() const;
	MappedBachAsset & setPadLength( const uint & );
	QString directory() const;
	MappedBachAsset & setDirectory( const QString & );
	float aperture() const;
	MappedBachAsset & setAperture( const float & );
	float shutterSpeed() const;
	MappedBachAsset & setShutterSpeed( const float & );
	uint isoSpeedRating() const;
	MappedBachAsset & setIsoSpeedRating( const uint & );
	QDateTime creationDatetime() const;
	MappedBachAsset & setCreationDatetime( const QDateTime & );
	float focalLength() const;
	MappedBachAsset & setFocalLength( const float & );
	uint camera() const;
	MappedBachAsset & setCamera( const uint & );
	uint cameraSN() const;
	MappedBachAsset & setCameraSN( const uint & );
	uint lens() const;
	MappedBachAsset & setLens( const uint & );
	QString cachedKeywords() const;
	MappedBachAsset & setCachedKeywords( const QString & );
	int tnRotate() const;
	MappedBachAsset & setTnRotate( const int & );
	bool exclude() const;
	MappedBachAsset & setExclude( const bool & );
	QString hash() const;
	MappedBachAsset & setHash( const QString & );
	int position() const;
	MappedBachAsset & setPosition( const int & );
	MappedBachBucketMapList bachBucketMaps() const;
	MappedBachKeywordMapList bachKeywordMaps() const;


	static MappedBachAssetList select( const QString & where = QString::null, const QList<QVariant> & args = QList<QVariant>() );

	MappedBachAsset reload( bool lockForUpdate = false );

	static MappedBachAsset recordByPath( const QString &path );
	static MappedBachAsset recordByHash( const QString &hash );






	static Table * table();
	//static BachAssetSchema * schema();
};


