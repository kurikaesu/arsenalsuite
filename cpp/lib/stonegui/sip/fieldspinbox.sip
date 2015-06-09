
class MultipleValueSpinBox : QSpinBox
{
%TypeHeaderCode
#include <fieldspinbox.h>
%End
public:
	MultipleValueSpinBox( QWidget * parent /TransferThis/ );

	void setValues( QList<int> values );
	QList<int> values() const;

	QList<int> originalValues() const;

	bool changed() const;
	void setChanged( bool changed );

signals:
	void valueChanged( int value, bool changed );

protected slots:
	void slotValueChanged();

};

class FieldSpinBox : MultipleValueSpinBox
{
%TypeHeaderCode
#include <fieldspinbox.h>
%End
public:
	FieldSpinBox( QWidget * parent /TransferThis/ );
	FieldSpinBox( RecordProxy *, const QString & field, QWidget * /TransferThis/ );

	void setField( const QString & field );
	QString field() const;

	void setProxy( RecordProxy * proxy );
	
public slots:
	void slotRecordListChanged();
	void slotUpdateRecordList();
};
