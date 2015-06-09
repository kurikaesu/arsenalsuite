

class ExtTreeWidgetItem : QTreeWidgetItem
{
%TypeHeaderCode
#include <exttreewidgetitem.h>
%End

public:
	ExtTreeWidgetItem( int type = Type );
	ExtTreeWidgetItem( const QStringList & strings, int type = Type );
	ExtTreeWidgetItem( QTreeWidget * parent /TransferThis/, int type = Type );
	ExtTreeWidgetItem( QTreeWidget * parent /TransferThis/, const QStringList & strings, int type = Type );
	ExtTreeWidgetItem( QTreeWidget * parent /TransferThis/, QTreeWidgetItem * preceding, int type = Type );
	ExtTreeWidgetItem( QTreeWidgetItem * parent /TransferThis/, int type = Type );
	ExtTreeWidgetItem( QTreeWidgetItem * parent /TransferThis/, const QStringList & strings, int type = Type );
	ExtTreeWidgetItem( QTreeWidgetItem * parent /TransferThis/, QTreeWidgetItem * preceding, int type = Type );
	ExtTreeWidgetItem( const QTreeWidgetItem & other );

	virtual QString toolTip(int column) const;
	virtual QVariant data(int column, int role) const;
};
