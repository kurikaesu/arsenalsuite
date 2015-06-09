

#ifndef EXT_TREE_WIDGET_ITEM_H
#define EXT_TREE_WIDGET_ITEM_H

#include <qtreewidget.h>

#include "stonegui.h"

/// QTreeWidgetItem with addition of virtual QString toolTip(int column) const;
/// So that tooltips can be generated on demand without overriding data in python
/// which makes the tree widget items slow
class STONEGUI_EXPORT ExtTreeWidgetItem : public QTreeWidgetItem
{
public:
	ExtTreeWidgetItem( int type = Type );
	ExtTreeWidgetItem( const QStringList & strings, int type = Type );
	ExtTreeWidgetItem( QTreeWidget * parent, int type = Type );
	ExtTreeWidgetItem( QTreeWidget * parent, const QStringList & strings, int type = Type );
	ExtTreeWidgetItem( QTreeWidget * parent, QTreeWidgetItem * preceding, int type = Type );
	ExtTreeWidgetItem( QTreeWidgetItem * parent, int type = Type );
	ExtTreeWidgetItem( QTreeWidgetItem * parent, const QStringList & strings, int type = Type );
	ExtTreeWidgetItem( QTreeWidgetItem * parent, QTreeWidgetItem * preceding, int type = Type );
	ExtTreeWidgetItem( const QTreeWidgetItem & other );

	virtual QString toolTip(int column) const;

	virtual QVariant data(int column, int role) const;
};

#endif //  EXT_TREE_WIDGET_ITEM_H