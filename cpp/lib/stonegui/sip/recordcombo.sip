

class RecordCombo : QComboBox
{
%TypeHeaderCode
#include <recordcombo.h>
%End
public:
	RecordCombo( QWidget * parent /TransferThis/ );
	
	void setItems( RecordList rl );

	void setTable( Table * table );
	void setColumn( const QString & column );
	
	void setModel( RecordSuperModel * model );

	RecordSuperModel * model() const;

	Record current();

public slots:
	void setCurrent( const Record & r );
	void refresh( bool checkIndex = false );

signals:
	void currentChanged( const Record & );
	void highlighted( const Record & );
};


