
class RecordProxy : QObject
{

%TypeHeaderCode
#include <recordproxy.h>
%End
public:
	RecordProxy( QObject * parent /TransferThis/ );

	MappedRecordList & records();

	void applyChanges( bool commit = false );
	
	void setRecordList( const RecordList &, bool applyChanges = true, bool commitChanges = true );

	void clear();

signals:
	void recordListChange();
	void updateRecordList();

};
