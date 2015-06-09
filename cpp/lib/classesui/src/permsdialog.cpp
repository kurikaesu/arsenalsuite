
#include <qdialog.h>

#include "employee.h"
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
				if( role == Qt::DisplayRole || role == Qt::EditRole )
					return perm._class();
				break;
			case 1:
			{
				if( role == Qt::DisplayRole )
					return perm.user().displayName();
				else if( role == Qt::EditRole ) {
					Employee none;
					none.setName( "None" );
					EmployeeList el = Employee::select().filter("disabled",0).sorted("name");
					el.insert(el.begin(),none);
					return qVariantFromValue<RecordList>(el);
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
					return "name";
				break;
			}
			case 3:
				if( role == Qt::DisplayRole || role == Qt::EditRole )
					return perm.permission();
				break;
			case 4:
				if( role == Qt::DisplayRole )
					return perm.key();
				break;
		}
		return QVariant();
	}
	bool setModelData( const QModelIndex & i, const QVariant & v, int role ) {
		int col = i.column();
		if( role == Qt::EditRole ) {
			switch( col ) {
				case 0:
					perm.set_class(v.toString());
					break;
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
					QString p = v.toString();
					if( !QRegExp( "^[0-7]{4}$" ).exactMatch( p ) ) {
						// TODO: Warning dialog
						return false;
					}
					perm.setPermission( v.toString() );
					break;
				}
				case 4:
					return false;
			}
			perm.commit();
			return true;
		}
		return false;
	}
	Qt::ItemFlags modelFlags( const QModelIndex & index ) {
		if( index.column() < 4 )
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
	mModel->setHeaderLabels( QStringList() << "Class" << "User" << "Group" << "Permission" << "Key" );
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
	p.setPermission( "0700" );
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

