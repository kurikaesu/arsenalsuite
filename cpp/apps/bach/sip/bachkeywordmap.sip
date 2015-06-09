
%MappedType MappedBachKeywordMap
{
%TypeHeaderCode
#include <record.h>
#include <pyembed.h>
#include "bachkeywordmaplist.h"
#include "bachkeywordmaptable.h"
#define MappedBachKeywordMap BachKeywordMap
%End

%ConvertToTypeCode
	if (sipIsErr == NULL)
		return isPythonRecordInstance(sipPy) ? 1 : 0;

	*sipCppPtr = new BachKeywordMap(sipUnwrapRecord(sipPy));

	return sipGetState(sipTransferObj);
%End

%ConvertFromTypeCode
	return sipWrapRecord( sipCpp, true, BachKeywordMapSchema::instance() );
%End
};

class BachKeywordMap : Record
{
%TypeHeaderCode
#include <bachkeywordmap.h>
#include "bachasset.h"
#include "bachkeyword.h"
#include "bachnamespace.h"

%End
public:
	BachKeywordMap();

	BachKeywordMap( uint key );

	BachKeywordMap( const BachKeywordMap & other );

	BachKeywordMap( const Record & r );

	BachKeywordMap( RecordImp * imp, bool checkType = true );
	
	MappedBachKeywordMap copy() const;

	bool operator==( const Record & other ) const;

	bool operator!=( const Record & other ) const;
	
	bool operator <( const Record & other ) const;

	long __hash__() const;
%MethodCode
		sipRes = sipCpp->key();
%End

	MappedBachAsset bachAsset() const;
	MappedBachKeywordMap & setBachAsset( const BachAsset & );
	MappedBachKeyword bachKeyword() const;
	MappedBachKeywordMap & setBachKeyword( const BachKeyword & );
	MappedBachNamespace bachNamespace() const;
	MappedBachKeywordMap & setBachNamespace( const BachNamespace & );


	static MappedBachKeywordMapList select( const QString & where = QString::null, const QList<QVariant> & args = QList<QVariant>() );

	MappedBachKeywordMap reload( bool lockForUpdate = false );

	static MappedBachKeywordMapList recordsByBachAsset( const BachAsset &fkeyBachAsset );
	static MappedBachKeywordMapList recordsByBachKeyword( const BachKeyword &fkeyBachKeyword );
	static MappedBachKeywordMapList recordsByBachNamespace( const BachNamespace &fkeyBachNamespace );
	static MappedBachKeywordMap recordByKeywordAndAsset( const BachKeyword &fkeyBachKeyword, const BachAsset &fkeyBachAsset );
	static MappedBachKeywordMapList recordsByKeywordAndAssets( const BachKeyword &fkeyBachKeyword, const BachAsset &fkeyBachAsset );






	static Table * table();
	//static BachKeywordMapSchema * schema();
};


