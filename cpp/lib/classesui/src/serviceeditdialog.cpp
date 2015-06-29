#include <qdialog.h>

#include "recordtreeview.h"
#include "serviceeditdialog.h"
#include "service.h"
#include "license.h"

class ServiceEditItem : public RecordItemBase
{
public:
	Service svc;
	ServiceEditItem( const Service & s ) { setup(s); }
	ServiceEditItem(){}
	void setup( const Record & r, const QModelIndex & = QModelIndex() ) {
		svc = r;
	}
	QVariant modelData( const QModelIndex & i, int role ) {
		int col = i.column();
		switch( col ) {
			case 0:
			{
				if( role == Qt::DisplayRole )
					return svc.key();
				break;
			}	
			case 1:
			{
				if( role == Qt::DisplayRole || role == Qt::EditRole )
					return svc.service();
				else if( role == RecordDelegate::FieldNameRole )
					return "service";
				break;
			}
			case 2:
			{
				if( role == Qt::DisplayRole )
					return svc.license().license();
				else if ( role == Qt::EditRole ) {
					License none;
					none.setLicense( "None" );
					LicenseList ll = License::select();
					ll.insert(ll.begin(), none);
					return qVariantFromValue<RecordList>(ll);
				}
				else if( role == RecordDelegate::FieldNameRole )
					return "license";
				break;
			}
			case 3:
			{
				if( role == Qt::DisplayRole || role == Qt::EditRole )
					return svc.enabled();
				else if( role == RecordDelegate::FieldNameRole )
					return "enabled";
				break;
			}
			case 4:
			{
				if( role == Qt::DisplayRole || role == Qt::EditRole )
					return svc.active();
				else if( role == RecordDelegate::FieldNameRole )
					return "active";
				break;
			}
		}
		return QVariant();
	}
	bool setModelData( const QModelIndex & i, const QVariant & v, int role ) {
		int col = i.column();
		if( role == Qt::EditRole ) {
			switch( col ) {
				case 0:
					return false;
				case 1:
				{
					svc.setService(v.toString());
					break;
				}
				case 2:
				{
					License l = qvariant_cast<Record>(v);
					if (l.isValid())
						svc.setLicense(l);
					break;
				}
				case 3:
				{
					svc.setEnabled(v.toBool());
					break;
				}
				case 4:
				{
					svc.setActive(v.toBool());
					break;
				}
			}
			svc.commit();
			return true;
		}
		return false;
	}
	Qt::ItemFlags modelFlags( const QModelIndex & index ) {
		if( index.column() > 1 )
			return Qt::ItemFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
		return Qt::ItemFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
	}
	Record getRecord() { return svc; }
};

typedef TemplateRecordDataTranslator<ServiceEditItem> PermissionTranslator;

ServiceEditDialog::ServiceEditDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	mModel = new RecordSuperModel(mTreeView);
	new PermissionTranslator(mModel->treeBuilder());
	mTreeView->setModel(mModel);
	mModel->setHeaderLabels( QStringList() << "Key" << "Service" << "License" << "Enabled" << "Active" );
	mModel->listen( Service::table() );
	connect( mTreeView, SIGNAL( currentChanged( const Record & ) ), SLOT( slotCurrentChanged( const Record & ) ) );
	connect( mNewMethodButton, SIGNAL( clicked() ), SLOT( slotNewMethod() ) );
	connect( mRemoveMethodButton, SIGNAL( clicked() ), SLOT( slotRemoveMethod() ) );
	refresh();
}

void ServiceEditDialog::refresh()
{
	mModel->setRootList( Service::select() );
	slotCurrentChanged( mTreeView->current() );
}

void ServiceEditDialog::slotNewMethod()
{
	Service l;
	mModel->append( l );
}

void ServiceEditDialog::slotRemoveMethod()
{
	mTreeView->current().remove();
}

void ServiceEditDialog::slotCurrentChanged( const Record & r )
{
	mRemoveMethodButton->setEnabled( r.isValid() );
}

void ServiceEditDialog::accept()
{
	QDialog::accept();
}

