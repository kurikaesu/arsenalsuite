
class RecordDrag
{
%TypeHeaderCode
#include <recorddrag.h>
%End
public:
	static QByteArray encodeRecordList( RecordList rl );
	static QDrag * create( RecordList, QWidget * parent );
	static QMimeData * toMimeData( RecordList );
	static bool canDecode( const QMimeData * );
	static bool decode( const QMimeData *, RecordList * /Out/ );
	static const char * mimeType;
};
