
/* $Author: newellm $
 * $LastChangedDate: 2008-05-16 04:36:37 +1000 (Fri, 16 May 2008) $
 * $Rev: 6567 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/classesui/src/hostlistsdialog.cpp $
 */

#include <qmenu.h>

#include "recordsupermodel.h"

#include "dynamichostgroup.h"
#include "hostgroup.h"
#include "user.h"

#include "hostlistsdialog.h"

struct HostGroupListItem : public RecordItemBase
{
	HostGroup hostGroup;
	QString type;
	void setup( const Record & r, const QModelIndex & );
	QVariant modelData( const QModelIndex & i, int role ) const;
	bool setModelData( const QModelIndex & i, const QVariant & data, int role );
	Qt::ItemFlags modelFlags( const QModelIndex & i );
	Record getRecord() { return hostGroup; }
};

typedef TemplateRecordDataTranslator<HostGroupListItem> HostGroupListTranslator;

void HostGroupListItem::setup( const Record & r, const QModelIndex & )
{
	hostGroup = r;
	type = DynamicHostGroup(r).isRecord() ? "Dynamic" : "Static";
}

QVariant HostGroupListItem::modelData( const QModelIndex & i, int role ) const
{
	int col = i.column();
	if( role == Qt::DisplayRole || role == Qt::EditRole ) {
		switch( col ) {
			case 0: return hostGroup.name();
			case 1: return type;
			case 2: return !hostGroup.private_();
		}
	}
	return QVariant();
}

bool HostGroupListItem::setModelData( const QModelIndex & i, const QVariant & data, int role )
{
	if( role == Qt::EditRole ) {
		switch( i.column() ) {
			case 0:
				hostGroup.setName( data.toString() );
				hostGroup.commit();
				return true;
				break;
			case 2:
				hostGroup.setPrivate_(!data.toBool());
				hostGroup.commit();
				return true;
				break;
		}
	}
	return false;
}

Qt::ItemFlags HostGroupListItem::modelFlags( const QModelIndex & i )
{
	Qt::ItemFlags ret = Qt::ItemFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
	if( i.column() != 1 )
		ret |= Qt::ItemIsEditable;
	return ret;
}

HostListsDialog::HostListsDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );

	mModel = new RecordSuperModel( mHostListTree );
	new HostGroupListTranslator(mModel->treeBuilder());

	// Automatically Add/Delete records from the model
	mModel->listen( HostGroup::table() );
	mModel->setHeaderLabels( QStringList() << "Name" << "Type" << "Global" );

	mHostListTree->setModel( mModel );
	connect( mHostListTree, SIGNAL( showMenu( const QPoint&, const Record&, RecordList ) ), SLOT( showMenu( const QPoint&, const Record&, RecordList) ) );
	refresh();
}

void HostListsDialog::refresh()
{
	mModel->setRootList( HostGroup::recordsByUser( User::currentUser() ) );
}

void HostListsDialog::showMenu( const QPoint & pos, const Record &, RecordList list )
{
	QMenu * menu = new QMenu( this );
	QAction * del = menu->addAction( "Delete" );
	QAction * result = menu->exec( pos );
	if( result ) {
		if( result == del ) {
			list.remove();
		}
	}
	delete menu;
}


