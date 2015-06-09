
%MappedType MappedBachHistory
{
%TypeHeaderCode
#include <record.h>
#include <pyembed.h>
#include "bachhistorylist.h"
#include "bachhistorytable.h"
#define MappedBachHistory BachHistory
%End

%ConvertToTypeCode
	if (sipIsErr == NULL)
		return isPythonRecordInstance(sipPy) ? 1 : 0;

	*sipCppPtr = new BachHistory(sipUnwrapRecord(sipPy));

	return sipGetState(sipTransferObj);
%End

%ConvertFromTypeCode
	return sipWrapRecord( sipCpp, true, BachHistorySchema::instance() );
%End
};

class BachHistory : Record
{
%TypeHeaderCode
#include <bachhistory.h>

%End
public:
	BachHistory();

	BachHistory( uint key );

	BachHistory( const BachHistory & other );

	BachHistory( const Record & r );

	BachHistory( RecordImp * imp, bool checkType = true );
	
	MappedBachHistory copy() const;

	bool operator==( const Record & other ) const;

	bool operator!=( const Record & other ) const;
	
	bool operator <( const Record & other ) const;

	long __hash__() const;
%MethodCode
		sipRes = sipCpp->key();
%End

	QString action() const;
	MappedBachHistory & setAction( const QString & );
	QString user() const;
	MappedBachHistory & setUser( const QString & );
	QString tableName() const;
	MappedBachHistory & setTableName( const QString & );


	static MappedBachHistoryList select( const QString & where = QString::null, const QList<QVariant> & args = QList<QVariant>() );

	MappedBachHistory reload( bool lockForUpdate = false );







	static Table * table();
	//static BachHistorySchema * schema();
};


