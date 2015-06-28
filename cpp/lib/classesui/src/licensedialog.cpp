#include <qdialog.h>

#include "recordtreeview.h"
#include "licensedialog.h"
#include "license.h"

class LicenseItem : public RecordItemBase
{
public:
	License lic;
	LicenseItem( const License & l ) { setup(l); }
	LicenseItem(){}
	void setup( const Record & r, const QModelIndex & = QModelIndex() ) {
		lic = r;
	}
	QVariant modelData( const QModelIndex & i, int role ) {
		int col = i.column();
		switch( col ) {
			case 0:
			{
				if( role == Qt::DisplayRole )
					return lic.key();
				break;
			}	
			case 1:
			{
				if( role == Qt::DisplayRole || role == Qt::EditRole )
					return lic.license();
				else if( role == RecordDelegate::FieldNameRole )
					return "license";
				break;
			}
			case 2:
			{
				if( role == Qt::DisplayRole || role == Qt::EditRole )
					return lic.total();
				else if( role == RecordDelegate::FieldNameRole )
					return "total";
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
					lic.setLicense(v.toString());
					break;
				}
				case 2:
				{
					lic.setTotal(v.toInt());
					break;
				}
			}
			lic.commit();
			return true;
		}
		return false;
	}
	Qt::ItemFlags modelFlags( const QModelIndex & index ) {
		if( index.column() > 1 )
			return Qt::ItemFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
		return Qt::ItemFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
	}
	Record getRecord() { return lic; }
};

typedef TemplateRecordDataTranslator<LicenseItem> PermissionTranslator;

LicenseDialog::LicenseDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	mModel = new RecordSuperModel(mTreeView);
	new PermissionTranslator(mModel->treeBuilder());
	mTreeView->setModel(mModel);
	mModel->setHeaderLabels( QStringList() << "Key" << "License" << "Count" );
	mModel->listen( License::table() );
	connect( mTreeView, SIGNAL( currentChanged( const Record & ) ), SLOT( slotCurrentChanged( const Record & ) ) );
	connect( mNewMethodButton, SIGNAL( clicked() ), SLOT( slotNewMethod() ) );
	connect( mRemoveMethodButton, SIGNAL( clicked() ), SLOT( slotRemoveMethod() ) );
	refresh();
}

void LicenseDialog::refresh()
{
	mModel->setRootList( License::select() );
	slotCurrentChanged( mTreeView->current() );
}

void LicenseDialog::slotNewMethod()
{
	License l;
	mModel->append( l );
}

void LicenseDialog::slotRemoveMethod()
{
	mTreeView->current().remove();
}

void LicenseDialog::slotCurrentChanged( const Record & r )
{
	mRemoveMethodButton->setEnabled( r.isValid() );
}

void LicenseDialog::accept()
{
	QDialog::accept();
}

