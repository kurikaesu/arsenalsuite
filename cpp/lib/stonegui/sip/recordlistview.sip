

class RecordListView : QListView
{
%TypeHeaderCode
#include <recordlistview.h>
%End
public:
	RecordListView( QWidget * parent /TransferThis/ );

	RecordSuperModel * model() const;
	
	void setModel(RecordSuperModel *);

	Record current();

	RecordList selection();

public slots:
	void setSelection( const RecordList & rl );
	void setCurrent( const Record & r );

signals:
	void currentChanged( const Record & );
	void selectionChanged( RecordList );

};

