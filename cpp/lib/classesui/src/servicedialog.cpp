
#include <qheaderview.h>
#include <qtreeview.h>
#include <qpushbutton.h>
#include <qinputdialog.h>

#include "servicedialog.h"
#include "hostservice.h"

struct ServiceItem : public RecordItemBase
{
	HostService hs;
	QString name;
	bool toggle;
	void setup( const Record & r, const QModelIndex & = QModelIndex() );
	QVariant modelData( const QModelIndex & i, int role );
	bool setModelData( const QModelIndex & i, const QVariant & v, int role );
	Qt::ItemFlags modelFlags( const QModelIndex & i );
	int compare( const QModelIndex &, const QModelIndex &, int, bool );
	Record getRecord() { return hs; }
};

typedef TemplateRecordDataTranslator<ServiceItem> ServiceTranslator;

void ServiceItem::setup( const Record & r, const QModelIndex & )
{
	hs = r;
	name = hs.service().service();
	toggle = hs.isRecord();
}

QVariant ServiceItem::modelData( const QModelIndex & i, int role )
{
	int col = i.column();
	if( col == 0 && role == Qt::CheckStateRole )
		return toggle ? Qt::Checked : Qt::Unchecked;
	if( col == 0 && role == Qt::DisplayRole )
		return name;
	if( col == 1 && role == Qt::CheckStateRole )
		return (hs.isRecord() && hs.enabled()) ? Qt::Checked : Qt::Unchecked;
	return QVariant();
}

bool ServiceItem::setModelData( const QModelIndex & i, const QVariant & v, int role )
{
	int col = i.column();
	if( col == 0 && role == Qt::CheckStateRole ) {
		toggle = v.toBool();
		if( toggle ) {
			hs.setEnabled( true );
			hs.commit();
		} else
			hs.remove();
		return true;
	}
	if( col == 1 && role == Qt::CheckStateRole ) {
		hs.setEnabled( v.toInt() == Qt::Checked );
		hs.commit();
		return true;
	}
	return false;
}

Qt::ItemFlags ServiceItem::modelFlags( const QModelIndex & i )
{
	if( i.column() == 0 )
		return Qt::ItemFlags( Qt::ItemIsUserCheckable | Qt::ItemIsEnabled );
	if( i.column() == 1 ) {
		return hs.isRecord() ? Qt::ItemFlags( Qt::ItemIsUserCheckable | Qt::ItemIsEnabled ) : Qt::ItemFlags( Qt::ItemIsUserCheckable );
	}
	return Qt::ItemFlags( Qt::ItemIsEnabled );
}

int ServiceItem::compare( const QModelIndex & idx, const QModelIndex & idx2, int column, bool asc )
{
	int diff = int(toggle) - int(ServiceTranslator::data(idx2).toggle);
	return diff ? diff : RecordItemBase::compare(idx,idx2,column,asc);
}

ServiceDialog::ServiceDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	connect( mNewServiceButton, SIGNAL( clicked() ), SLOT( slotNewService() ) );
	connect( mDeleteServiceButton, SIGNAL( clicked() ), SLOT( slotRemoveService() ) );
	
	mServiceModel = new RecordSuperModel( mServiceView );
	new ServiceTranslator(mServiceModel->treeBuilder());
	QStringList hl; hl << "Service" << "Enabled";
	mServiceModel->setHeaderLabels(hl);
	mServiceModel->setAutoSort(true);
	mServiceModel->sort(0,Qt::DescendingOrder);
	mServiceView->setModel( mServiceModel );
	mServiceView->setRootIsDecorated(false);
	mServiceView->header()->setClickable(true);
	mServiceView->header()->setSortIndicatorShown(true);
	mServiceView->header()->setSortIndicator(0,Qt::DescendingOrder);
	mServices = Service::select();
}

Host ServiceDialog::host() const
{
	return mHost;
}

void ServiceDialog::setHost( const Host & host )
{
	mHost = host;
	refresh();
}

void ServiceDialog::refresh()
{
	HostServiceList hsl = HostService::recordsByHost( mHost );
	QMap<Service,HostService> hsm;
	foreach( HostService hs, hsl )
		hsm[hs.service()]=hs;
	foreach( Service s, mServices ) {
		if( !hsm.contains(s) ) {
			HostService hs;
			hs.setHost(mHost);
			hs.setService(s);
			hsl += hs;
		}
	}
	mServiceModel->setRootList( hsl );
}

void ServiceDialog::slotNewService()
{
	QString serviceName = QInputDialog::getText( this, "New Service", "Enter Service Name" );
	if( !serviceName.isEmpty() ) {
		Service s;
		s.setService( serviceName );
		s.commit();
		mServices += s;
		refresh();
	}
}

void ServiceDialog::slotRemoveService()
{
}

void ServiceDialog::accept()
{
	QDialog::accept();
}

void ServiceDialog::reject()
{
	QDialog::reject();
}

