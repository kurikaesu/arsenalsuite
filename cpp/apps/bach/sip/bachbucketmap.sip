
%MappedType MappedBachBucketMap
{
%TypeHeaderCode
#include <record.h>
#include <pyembed.h>
#include "bachbucketmaplist.h"
#include "bachbucketmaptable.h"
#define MappedBachBucketMap BachBucketMap
%End

%ConvertToTypeCode
	if (sipIsErr == NULL)
		return isPythonRecordInstance(sipPy) ? 1 : 0;

	*sipCppPtr = new BachBucketMap(sipUnwrapRecord(sipPy));

	return sipGetState(sipTransferObj);
%End

%ConvertFromTypeCode
	return sipWrapRecord( sipCpp, true, BachBucketMapSchema::instance() );
%End
};

class BachBucketMap : Record
{
%TypeHeaderCode
#include <bachbucketmap.h>
#include "bachbucket.h"
#include "bachasset.h"

%End
public:
	BachBucketMap();

	BachBucketMap( uint key );

	BachBucketMap( const BachBucketMap & other );

	BachBucketMap( const Record & r );

	BachBucketMap( RecordImp * imp, bool checkType = true );
	
	MappedBachBucketMap copy() const;

	bool operator==( const Record & other ) const;

	bool operator!=( const Record & other ) const;
	
	bool operator <( const Record & other ) const;

	long __hash__() const;
%MethodCode
		sipRes = sipCpp->key();
%End

	MappedBachBucket bachBucket() const;
	MappedBachBucketMap & setBachBucket( const BachBucket & );
	MappedBachAsset bachAsset() const;
	MappedBachBucketMap & setBachAsset( const BachAsset & );
	int position() const;
	MappedBachBucketMap & setPosition( const int & );


	static MappedBachBucketMapList select( const QString & where = QString::null, const QList<QVariant> & args = QList<QVariant>() );

	MappedBachBucketMap reload( bool lockForUpdate = false );

	static MappedBachBucketMapList recordsByBachBucket( const BachBucket &fkeyBachBucket );
	static MappedBachBucketMapList recordsByBachAsset( const BachAsset &fkeyBachAsset );
	static MappedBachBucketMap recordByBucketAndAsset( const BachBucket &fkeyBachBucket, const BachAsset &fkeyBachAsset );
	static MappedBachBucketMapList recordsByBucketAndAssets( const BachBucket &fkeyBachBucket, const BachAsset &fkeyBachAsset );






	static Table * table();
	//static BachBucketMapSchema * schema();
};


