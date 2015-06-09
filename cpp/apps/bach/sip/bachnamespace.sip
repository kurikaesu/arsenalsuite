
%MappedType MappedBachNamespace
{
%TypeHeaderCode
#include <record.h>
#include <pyembed.h>
#include "bachnamespacelist.h"
#include "bachnamespacetable.h"
#define MappedBachNamespace BachNamespace
%End

%ConvertToTypeCode
	if (sipIsErr == NULL)
		return isPythonRecordInstance(sipPy) ? 1 : 0;

	*sipCppPtr = new BachNamespace(sipUnwrapRecord(sipPy));

	return sipGetState(sipTransferObj);
%End

%ConvertFromTypeCode
	return sipWrapRecord( sipCpp, true, BachNamespaceSchema::instance() );
%End
};

class BachNamespace : Record
{
%TypeHeaderCode
#include <bachnamespace.h>
#include "bachbucket.h"
#include "bachkeywordmap.h"

%End
public:
	BachNamespace();

	BachNamespace( uint key );

	BachNamespace( const BachNamespace & other );

	BachNamespace( const Record & r );

	BachNamespace( RecordImp * imp, bool checkType = true );
	
	MappedBachNamespace copy() const;

	bool operator==( const Record & other ) const;

	bool operator!=( const Record & other ) const;
	
	bool operator <( const Record & other ) const;

	long __hash__() const;
%MethodCode
		sipRes = sipCpp->key();
%End

	QString name() const;
	MappedBachNamespace & setName( const QString & );
	MappedBachBucketList bachBuckets() const;
	MappedBachKeywordMapList bachKeywordMaps() const;


	static MappedBachNamespaceList select( const QString & where = QString::null, const QList<QVariant> & args = QList<QVariant>() );

	MappedBachNamespace reload( bool lockForUpdate = false );







	static Table * table();
	//static BachNamespaceSchema * schema();
};


