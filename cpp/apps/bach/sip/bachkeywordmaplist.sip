


%MappedType MappedBachKeywordMapList
{
%TypeHeaderCode
#include <record.h>
#include <pyembed.h>
#include "bachkeywordmaplist.h"
#include "bachkeywordmaptable.h"
#define MappedBachKeywordMapList BachKeywordMapList
%End

%ConvertToTypeCode
	return 0;
%End

%ConvertFromTypeCode
	return sipWrapRecordList( sipCpp, true, BachKeywordMapSchema::instance() );
%End
};

class BachKeywordMapList : RecordList
{
%TypeHeaderCode
#include <bachkeywordmaplist.h>
#include "bachasset.h"
#include "bachkeyword.h"
#include "bachnamespace.h"

%End
public:
	BachKeywordMapList();
	BachKeywordMapList( const RecordList &, Table * t = 0 );
	BachKeywordMapList( const BachKeywordMapList & );
	BachKeywordMapList( const Record &, Table * t = 0 );
	
	~BachKeywordMapList();

//	void insert( BachKeywordMapIter, const BachKeywordMap & );

//	BachKeywordMapIter at( uint );

//	BachKeywordMapIter find( const Record & );

//	BachKeywordMapIter remove( const BachKeywordMapIter & );

	int remove( const Record & );
	
	int remove();
	
	//BachKeywordMapIter begin();

	//BachKeywordMapIter end();

	Table * table() const;
	TableSchema * schema() const;

	MappedBachKeywordMapList filter( const QString & column, const QRegExp & re, bool keepMatches = true ) const;
	MappedBachKeywordMapList filter( const QString & column, const QVariant & value, bool keepMatches = true ) const;
	MappedBachKeywordMapList sorted( const QString & c, bool a = true ) const;

	MappedBachKeywordMapList unique() const;
	MappedBachKeywordMapList reversed() const;
	MappedBachKeywordMapList reloaded() const;

	MappedBachAssetList bachAssets() const;
	MappedBachKeywordMapList & setBachAssets( const BachAsset & );
	MappedBachKeywordList bachKeywords() const;
	MappedBachKeywordMapList & setBachKeywords( const BachKeyword & );
	MappedBachNamespaceList bachNamespaces() const;
	MappedBachKeywordMapList & setBachNamespaces( const BachNamespace & );


};


