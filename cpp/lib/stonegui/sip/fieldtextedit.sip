
class FieldTextEdit : QTextEdit
{
%TypeHeaderCode
#include <fieldtextedit.h>
%End
public:
	FieldTextEdit( QWidget * parent /TransferThis/ );
	FieldTextEdit( RecordProxy *, const QString & field, QWidget * /TransferThis/ );

	void setField( const QString & field );
	QString field() const;

	void setProxy( RecordProxy * proxy );

public slots:
	void slotRecordListChanged();
	void slotUpdateRecordList();

	void popupItemActivated( int id );

	void slotTextChanged();
};

