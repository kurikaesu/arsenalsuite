
#include <qdialog.h>

#include "recordtreeview.h"
#include "permsdialog.h"
#include "user.h"
#include "group.h"

class PermissionItem : public RecordItemBase
{
public:
	Permission perm;
	PermissionItem( const Permission & p ) { setup(p); }
	PermissionItem(){}
	void setup( const Record & r, const QModelIndex & = QModelIndex() ) {
		perm = r;
	}
	QVariant modelData( const QModelIndex & i, int role ) {
		int col = i.column();
		switch( col ) {
			case 0:
			{
				if( role == Qt::DisplayRole )
					return perm.key();
				break;
			}	
			case 1:
			{
				if( role == Qt::DisplayRole )
					return perm.user().displayName();
				else if( role == Qt::EditRole ) {
					User none;
					none.setName( "None" );
					UserList ul = User::select().filter("disabled",0).sorted("name");
					ul.insert(ul.begin(),none);
					return qVariantFromValue<RecordList>(ul);
				} else if( role == RecordDelegate::CurrentRecordRole )
					return qVariantFromValue<Record>(perm.user());
				else if( role == RecordDelegate::FieldNameRole )
					return "name";
				break;
			}
			case 2:
			{
				if( role == Qt::DisplayRole )
					return perm.group().name();
				else if( role == Qt::EditRole ) {
					Group none;
					none.setName( "None" );
					GroupList gl = Group::select().sorted("name");
					gl.insert(gl.begin(),none);
					return qVariantFromValue<RecordList>(gl);
				} else if( role == RecordDelegate::CurrentRecordRole )
					return qVariantFromValue<Record>(perm.group());
				else if( role == RecordDelegate::FieldNameRole )
					return "grp";
				break;
			}
			case 3:
			{
				if( role == Qt::DisplayRole || role == Qt::EditRole )
					return perm._class();
				break;
			}
			case 4:
				if( role == Qt::DisplayRole || role == Qt::EditRole )
					return perm.modify();
				break;
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
					User u = qvariant_cast<Record>(v);
					if( u.isValid() )
						perm.setUser( u );
					break;
				}
				case 2:
				{
					Group g = qvariant_cast<Record>(v);
					if( g.isValid() )
						perm.setGroup( g );
					break;
				}
				case 3:
				{
					perm.set_class(v.toString());
					break;
				}
				case 4:
				{
					perm.setModify(v.toBool());
					break;
				}
			}
			perm.commit();
			return true;
		}
		return false;
	}
	Qt::ItemFlags modelFlags( const QModelIndex & index ) {
		if( index.column() > 0 )
			return Qt::ItemFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
		return Qt::ItemFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
	}
	Record getRecord() { return perm; }
};

typedef TemplateRecordDataTranslator<PermissionItem> PermissionTranslator;

PermsDialog::PermsDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	mModel = new RecordSuperModel(mTreeView);
	new PermissionTranslator(mModel->treeBuilder());
	mTreeView->setModel(mModel);
	mModel->setHeaderLabels( QStringList() << "Key" << "User" << "Group" << "Class" << "Modify" );
	mModel->listen( Permission::table() );
	connect( mTreeView, SIGNAL( currentChanged( const Record & ) ), SLOT( slotCurrentChanged( const Record & ) ) );
	connect( mNewMethodButton, SIGNAL( clicked() ), SLOT( slotNewMethod() ) );
	connect( mRemoveMethodButton, SIGNAL( clicked() ), SLOT( slotRemoveMethod() ) );
	refresh();
}

void PermsDialog::refresh()
{
	mModel->setRootList( Permission::select() );
	slotCurrentChanged( mTreeView->current() );
}

void PermsDialog::slotNewMethod()
{
	Permission p;
	p.setEnabled(false);
	p.setModify(false);
	mModel->append( p );
}

void PermsDialog::slotRemoveMethod()
{
	mTreeView->current().remove();
}

void PermsDialog::slotCurrentChanged( const Record & r )
{
	mRemoveMethodButton->setEnabled( r.isValid() );
}

void PermsDialog::accept()
{
	QDialog::accept();
}

