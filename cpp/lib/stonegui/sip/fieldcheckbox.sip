
class FieldCheckBox : QCheckBox
{
%TypeHeaderCode
#include <fieldcheckbox.h>
%End
public:
	FieldCheckBox( QWidget * parent /TransferThis/ );

	void setField( const QString & field );
	QString field() const;

	void setProxy( RecordProxy * proxy );
	
public slots:
	void slotRecordListChanged();
	void slotUpdateRecordList();

};
