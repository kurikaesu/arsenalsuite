
%ModuleHeaderCode
#include "hostselector.h"
%End

HostList hostListFromString( const QString & );

class HostSelector : QDialog
{
%TypeHeaderCode
#include <hostselector.h>
%End
public:
	HostSelector(QWidget * parent /TransferThis/ = 0);

	void setHostList( const RecordList & );
	void setHostList( const QString & hostList );

	QString hostStringList() const;

	HostList hostList() const;

	ServiceList serviceFilter() const;

public slots:

	void selectAll();

	void checkSelected();

	void uncheckSelected();

	void hostGroupChanged( const Record & hg );

	void saveHostGroup();

	void manageHostLists();
	void setServiceFilter( ServiceList );
	void refresh();
};
