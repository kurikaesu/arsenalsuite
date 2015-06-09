
#include <qheaderview.h>
#include <qmenu.h>

#include "recordcombo.h"

#include "hostinterfacedialog.h"
#include "hostinterface.h"
#include "hostinterfacetype.h"
#include "recorddelegate.h"

struct HostInterfaceItem : public RecordItemBase
{
	HostInterface iface;
	void setup( const Record & r, const QModelIndex & = QModelIndex() ) {
		iface = r;
	}

	QVariant modelData( const QModelIndex & i, int role ) const {
		int col = i.column();
		if( col == 2 ) {
			switch (role) {
				case Qt::DisplayRole:
					return iface.type().name();
				case Qt::EditRole:
					return qVariantFromValue<RecordList>(HostInterfaceType::select());
				case RecordDelegate::CurrentRecordRole:
					return qVariantFromValue<Record>(iface.type());
				case RecordDelegate::FieldNameRole:
					return "name";
			}	
		} else if( role == Qt::DisplayRole || role == Qt::EditRole ) {
			switch( col ) {
				case 0: return iface.ip();
				case 1: return iface.mac();
				case 3: return iface.switchPort();
			}
		}
		return QVariant();
	}
	bool setModelData( const QModelIndex & i, const QVariant & v, int role ) {
		int col = i.column();
		if( role == Qt::EditRole ) {
			switch( col ) {
				case 0:
					iface.setIp( v.toString() );
					break;
				case 1:
					iface.setMac( v.toString() );
					break;
				case 2:
				{
					HostInterfaceType type = qvariant_cast<Record>(v);
					if( type.isRecord() )
						iface.setType( type );
					break;
				}
				case 3:
					iface.setSwitchPort( v.toInt() );
					break;
			}
			iface.commit();
			return true;
		}
		return false;
	}
	Qt::ItemFlags modelFlags( const QModelIndex & ) { return Qt::ItemFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable ); }
	Record getRecord() { return iface; }
};

typedef TemplateRecordDataTranslator<HostInterfaceItem> HostInterfaceTranslator;


HostInterfaceDialog::HostInterfaceDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	mModel = new RecordSuperModel( mTable );
	new HostInterfaceTranslator(mModel->treeBuilder());
	QStringList hl;
	hl << "IP" << "MAC" << "Type" << "Switch Port";
	mModel->setHeaderLabels( hl );
	mTable->setModel( mModel );
	mTable->setContextMenuPolicy( Qt::CustomContextMenu );
	mTable->verticalHeader()->hide();
	connect( mTable, SIGNAL( clicked( const QModelIndex & ) ), mTable, SLOT( edit( const QModelIndex & ) ) );

	RecordDelegate * rd = new RecordDelegate(mTable);
	mTable->setItemDelegate(rd);
	connect( mTable, SIGNAL( customContextMenuRequested( const QPoint & ) ), SLOT( showMenu() ) );
}

void HostInterfaceDialog::setHost( const Host & host )
{
	mHost = host;
	mModel->setRootList( mHost.hostInterfaces() );
}

void HostInterfaceDialog::showMenu()
{
	QPoint pos = QCursor::pos();
	QModelIndex index = mTable->indexAt( mTable->viewport()->mapFromGlobal( pos ) );
	HostInterface iface;
	QMenu * menu = new QMenu( this );
	QAction * newIf = menu->addAction( "New Interface" );
	QAction * delIf = 0;
	if( index.isValid() ) {
		delIf = menu->addAction( "Remove Interface" );
		iface = mModel->getRecord(index);
	}
	QAction * res = menu->exec( pos );
	if( res && res == newIf ) {
		HostInterface hi;
		hi.setHost( mHost );
		hi.setIp( "127.0.0.1" );
		hi.commit();
		mModel->append(hi);
	} else if( res && res == delIf ) {
		iface.remove();
		mModel->remove( index );
	}
	delete menu;
}

