
%MappedType MappedBachBucket
{
%TypeHeaderCode
#include <record.h>
#include <pyembed.h>
#include "bachbucketlist.h"
#include "bachbuckettable.h"
#define MappedBachBucket BachBucket
%End

%ConvertToTypeCode
	if (sipIsErr == NULL)
		return isPythonRecordInstance(sipPy) ? 1 : 0;

	*sipCppPtr = new BachBucket(sipUnwrapRecord(sipPy));

	return sipGetState(sipTransferObj);
%End

%ConvertFromTypeCode
	return sipWrapRecord( sipCpp, true, BachBucketSchema::instance() );
%End
};

class BachBucket : Record
{
%TypeHeaderCode
#include <bachbucket.h>
#include "bachnamespace.h"
#include "bachbucketmap.h"

%End
public:
	BachBucket();

	BachBucket( uint key );

	BachBucket( const BachBucket & other );

	BachBucket( const Record & r );

	BachBucket( RecordImp * imp, bool checkType = true );
	
	MappedBachBucket copy() const;

	bool operator==( const Record & other ) const;

	bool operator!=( const Record & other ) const;
	
	bool operator <( const Record & other ) const;

	long __hash__() const;
%MethodCode
		sipRes = sipCpp->key();
%End

	QString name() const;
	MappedBachBucket & setName( const QString & );
	MappedBachNamespace bachNamespace() const;
	MappedBachBucket & setBachNamespace( const BachNamespace & );
	MappedBachBucketMapList bachBucketMaps() const;


	static MappedBachBucketList select( const QString & where = QString::null, const QList<QVariant> & args = QList<QVariant>() );

	MappedBachBucket reload( bool lockForUpdate = false );

	static MappedBachBucketList recordsByBachNamespace( const BachNamespace &fkeyBachNamespace );
	static MappedBachBucketList recordsByName( const QString &name );






	static Table * table();
	//static BachBucketSchema * schema();
};


