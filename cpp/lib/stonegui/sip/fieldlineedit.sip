
class FieldLineEdit : QLineEdit
{
%TypeHeaderCode
#include <fieldlineedit.h>
%End

public:
	FieldLineEdit( RecordProxy *, const QString & field, QWidget */TransferThis/ );
	FieldLineEdit( QWidget * parent /TransferThis/ );

	void setField( const QString & field );
	QString field() const;

	void setProxy( RecordProxy * proxy );

	bool changed() const;
	void reset();
	
public slots:
	void slotRecordListChanged();
	void slotUpdateRecordList();

	void popupItemActivated( int id );

	void slotTextChanged( const QString & text );
};

