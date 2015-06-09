
%MappedType MappedBachKeyword
{
%TypeHeaderCode
#include <record.h>
#include <pyembed.h>
#include "bachkeywordlist.h"
#include "bachkeywordtable.h"
#define MappedBachKeyword BachKeyword
%End

%ConvertToTypeCode
	if (sipIsErr == NULL)
		return isPythonRecordInstance(sipPy) ? 1 : 0;

	*sipCppPtr = new BachKeyword(sipUnwrapRecord(sipPy));

	return sipGetState(sipTransferObj);
%End

%ConvertFromTypeCode
	return sipWrapRecord( sipCpp, true, BachKeywordSchema::instance() );
%End
};

class BachKeyword : Record
{
%TypeHeaderCode
#include <bachkeyword.h>
#include "bachkeywordmap.h"

%End
public:
	BachKeyword();

	BachKeyword( uint key );

	BachKeyword( const BachKeyword & other );

	BachKeyword( const Record & r );

	BachKeyword( RecordImp * imp, bool checkType = true );
	
	MappedBachKeyword copy() const;

	bool operator==( const Record & other ) const;

	bool operator!=( const Record & other ) const;
	
	bool operator <( const Record & other ) const;

	long __hash__() const;
%MethodCode
		sipRes = sipCpp->key();
%End

	QString name() const;
	MappedBachKeyword & setName( const QString & );
	int hasKeyword() const;
	MappedBachKeyword & setHasKeyword( const int & );
	uint count() const;
	MappedBachKeyword & setCount( const uint & );
	MappedBachKeywordMapList bachKeywordMaps() const;


	static MappedBachKeywordList select( const QString & where = QString::null, const QList<QVariant> & args = QList<QVariant>() );

	MappedBachKeyword reload( bool lockForUpdate = false );

	static MappedBachKeyword recordByName( const QString &name );






	static Table * table();
	//static BachKeywordSchema * schema();
};


