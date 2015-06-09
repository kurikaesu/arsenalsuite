


%MappedType MappedBachKeywordList
{
%TypeHeaderCode
#include <record.h>
#include <pyembed.h>
#include "bachkeywordlist.h"
#include "bachkeywordtable.h"
#define MappedBachKeywordList BachKeywordList
%End

%ConvertToTypeCode
	return 0;
%End

%ConvertFromTypeCode
	return sipWrapRecordList( sipCpp, true, BachKeywordSchema::instance() );
%End
};

class BachKeywordList : RecordList
{
%TypeHeaderCode
#include <bachkeywordlist.h>
#include "bachkeywordmap.h"

%End
public:
	BachKeywordList();
	BachKeywordList( const RecordList &, Table * t = 0 );
	BachKeywordList( const BachKeywordList & );
	BachKeywordList( const Record &, Table * t = 0 );
	
	~BachKeywordList();

//	void insert( BachKeywordIter, const BachKeyword & );

//	BachKeywordIter at( uint );

//	BachKeywordIter find( const Record & );

//	BachKeywordIter remove( const BachKeywordIter & );

	int remove( const Record & );
	
	int remove();
	
	//BachKeywordIter begin();

	//BachKeywordIter end();

	Table * table() const;
	TableSchema * schema() const;

	MappedBachKeywordList filter( const QString & column, const QRegExp & re, bool keepMatches = true ) const;
	MappedBachKeywordList filter( const QString & column, const QVariant & value, bool keepMatches = true ) const;
	MappedBachKeywordList sorted( const QString & c, bool a = true ) const;

	MappedBachKeywordList unique() const;
	MappedBachKeywordList reversed() const;
	MappedBachKeywordList reloaded() const;

	QStringList names() const;
	MappedBachKeywordList & setNames( const QString & );
	QList<int> hasKeywords() const;
	MappedBachKeywordList & setHasKeywords( const int & );
	QList<uint> counts() const;
	MappedBachKeywordList & setCounts( const uint & );
	MappedBachKeywordMapList bachKeywordMaps();


};


