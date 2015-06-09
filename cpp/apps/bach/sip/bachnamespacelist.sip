


%MappedType MappedBachNamespaceList
{
%TypeHeaderCode
#include <record.h>
#include <pyembed.h>
#include "bachnamespacelist.h"
#include "bachnamespacetable.h"
#define MappedBachNamespaceList BachNamespaceList
%End

%ConvertToTypeCode
	return 0;
%End

%ConvertFromTypeCode
	return sipWrapRecordList( sipCpp, true, BachNamespaceSchema::instance() );
%End
};

class BachNamespaceList : RecordList
{
%TypeHeaderCode
#include <bachnamespacelist.h>
#include "bachbucket.h"
#include "bachkeywordmap.h"

%End
public:
	BachNamespaceList();
	BachNamespaceList( const RecordList &, Table * t = 0 );
	BachNamespaceList( const BachNamespaceList & );
	BachNamespaceList( const Record &, Table * t = 0 );
	
	~BachNamespaceList();

//	void insert( BachNamespaceIter, const BachNamespace & );

//	BachNamespaceIter at( uint );

//	BachNamespaceIter find( const Record & );

//	BachNamespaceIter remove( const BachNamespaceIter & );

	int remove( const Record & );
	
	int remove();
	
	//BachNamespaceIter begin();

	//BachNamespaceIter end();

	Table * table() const;
	TableSchema * schema() const;

	MappedBachNamespaceList filter( const QString & column, const QRegExp & re, bool keepMatches = true ) const;
	MappedBachNamespaceList filter( const QString & column, const QVariant & value, bool keepMatches = true ) const;
	MappedBachNamespaceList sorted( const QString & c, bool a = true ) const;

	MappedBachNamespaceList unique() const;
	MappedBachNamespaceList reversed() const;
	MappedBachNamespaceList reloaded() const;

	QStringList names() const;
	MappedBachNamespaceList & setNames( const QString & );
	MappedBachBucketList bachBuckets();
	MappedBachKeywordMapList bachKeywordMaps();


};


