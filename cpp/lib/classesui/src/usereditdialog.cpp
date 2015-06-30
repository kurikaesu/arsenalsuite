
#include <qdialog.h>

#include "recordtreeview.h"
#include "usereditdialog.h"
#include "userdialog.h"
#include "user.h"

class UserEditItem : public RecordItemBase
{
public:
	User user;
	UserEditItem( const User & u ) { setup(u); }
	UserEditItem(){}
	void setup( const Record & r, const QModelIndex & = QModelIndex() ) {
		user = r;
	}
	QVariant modelData( const QModelIndex & i, int role ) {
		int col = i.column();
		switch( col ) {
			case 0:
			{
				if( role == Qt::DisplayRole )
					return user.key();
				break;
			}	
			case 1:
			{
				if( role == Qt::DisplayRole || role == Qt::EditRole )
					return user.name();
				else if( role == RecordDelegate::FieldNameRole )
					return "name";
				break;
			}
			case 2:
			{
				if( role == Qt::DisplayRole || role == Qt::EditRole )
					return user.disabled();
				else if ( role == RecordDelegate::FieldNameRole )
					return "disabled";
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
					user.setName(v.toString());
					break;
				}
				case 2:
				{
					user.setDisabled(v.toBool());
					break;
				}
			}
			user.commit();
			return true;
		}
		return false;
	}
	Qt::ItemFlags modelFlags( const QModelIndex & index ) {
		if( index.column() > 0 )
			return Qt::ItemFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
		return Qt::ItemFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
	}
	Record getRecord() { return user; }
};

typedef TemplateRecordDataTranslator<UserEditItem> UserEditTranslator;

UserEditDialog::UserEditDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	mModel = new RecordSuperModel(mTreeView);
	new UserEditTranslator(mModel->treeBuilder());
	mTreeView->setModel(mModel);
	mModel->setHeaderLabels( QStringList() << "Key" << "User" << "Disabled" );
	mModel->listen( User::table() );
	connect( mTreeView, SIGNAL( currentChanged( const Record & ) ), SLOT( slotCurrentChanged( const Record & ) ) );
	connect( mNewMethodButton, SIGNAL( clicked() ), SLOT( slotNewMethod() ) );
	connect( mEditMethodButton, SIGNAL( clicked() ), SLOT( slotEditMethod() ) );
	connect( mRemoveMethodButton, SIGNAL( clicked() ), SLOT( slotRemoveMethod() ) );
	refresh();
}

void UserEditDialog::refresh()
{
	mModel->setRootList( User::select() );
	slotCurrentChanged( mTreeView->current() );
}

void UserEditDialog::slotNewMethod()
{
	UserDialog* ud = new UserDialog(this);
	ud->exec();
	delete ud;
}

void UserEditDialog::slotEditMethod()
{
	
}

void UserEditDialog::slotRemoveMethod()
{
	mTreeView->current().remove();
}

void UserEditDialog::slotCurrentChanged( const Record & r )
{
	mRemoveMethodButton->setEnabled( r.isValid() );
}

void UserEditDialog::accept()
{
	QDialog::accept();
}

