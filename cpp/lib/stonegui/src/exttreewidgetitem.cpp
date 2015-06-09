
#include "exttreewidgetitem.h"

ExtTreeWidgetItem::ExtTreeWidgetItem( int type )
: QTreeWidgetItem(type)
{}

ExtTreeWidgetItem::ExtTreeWidgetItem( const QStringList & strings, int type )
: QTreeWidgetItem(strings, type)
{}

ExtTreeWidgetItem::ExtTreeWidgetItem( QTreeWidget * parent, int type )
: QTreeWidgetItem(parent, type)
{}

ExtTreeWidgetItem::ExtTreeWidgetItem( QTreeWidget * parent, const QStringList & strings, int type )
: QTreeWidgetItem(parent, strings, type)
{}

ExtTreeWidgetItem::ExtTreeWidgetItem( QTreeWidget * parent, QTreeWidgetItem * preceding, int type )
: QTreeWidgetItem(parent, preceding, type)
{}

ExtTreeWidgetItem::ExtTreeWidgetItem( QTreeWidgetItem * parent, int type )
: QTreeWidgetItem(parent, type)
{}

ExtTreeWidgetItem::ExtTreeWidgetItem( QTreeWidgetItem * parent, const QStringList & strings, int type )
: QTreeWidgetItem(parent, strings, type)
{}

ExtTreeWidgetItem::ExtTreeWidgetItem( QTreeWidgetItem * parent, QTreeWidgetItem * preceding, int type )
: QTreeWidgetItem(parent, preceding, type)
{}

ExtTreeWidgetItem::ExtTreeWidgetItem( const QTreeWidgetItem & other )
: QTreeWidgetItem(other)
{}

QString ExtTreeWidgetItem::toolTip(int column) const
{
	return QString();
}

QVariant ExtTreeWidgetItem::data(int column, int role) const
{
	if( role == Qt::ToolTipRole )
		return toolTip(column);
	return QTreeWidgetItem::data(column, role);
}
