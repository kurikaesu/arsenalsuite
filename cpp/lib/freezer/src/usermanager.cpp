#include <qcombobox.h>

#include "usermanager.h"

struct UserPermissionsItem : public RecordItem
{
	User mUser;
	UserPermissionsItem() {}
	UserPermissionsItem(const User& u)
	{
		mUser = u;
	}
	
	bool permissionByColumn(int col) const {
		switch(col)
		{
		case 1: // modify hosts
		case 2: // modify users
		case 3: // modify projects
		case 4: // modify project weights
		case 5: // modify host services
		case 6: // modify user services
		case 7: // modify job types
		case 8: // modify services
		case 9: // modify licenses
		default:
			break;
		}
		return false;
	}
	QVariant serviceData(int column) const
	{
		return permissionByColumn(column) ? "YES" : "NO";
	}
	
	void setup(const Record & r, const QModelIndex&);
	QVariant modelData ( const QModelIndex & index, int role ) const;
	bool setModelData( const QModelIndex & idx, const QVariant & v, int role );
	int compare( const QModelIndex & a, const QModelIndex & b, int, bool );
	Qt::ItemFlags modelFlags( const QModelIndex & );
	Record getRecord();
	static UserPermissionsModel * model(const QModelIndex &);
};

typedef TemplateRecordDataTranslator<UserPermissionsItem> UserPermissionsTranslator;

UserPermissionsModel* UserPermissionsItem::model(const QModelIndex& idx)
{
	return const_cast<UserPermissionsModel*>(qobject_cast<const UserPermissionsModel*>(idx.model()));
}

void UserPermissionsItem::setup(const Record& r, const QModelIndex&)
{
	mUser = r;
}

int UserPermissionsItem::compare(const QModelIndex& a, const QModelIndex& b, int column, bool asc)
{
	return 0;
}

QVariant UserPermissionsItem::modelData(const QModelIndex& idx, int role) const
{
	return QVariant();
}

bool UserPermissionsItem::setModelData(const QModelIndex& idx, const QVariant& v, int role)
{
	return false;
}

Qt::ItemFlags UserPermissionsItem::modelFlags( const QModelIndex& idx )
{
	Qt::ItemFlags ret = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
	return idx.column() == 1 ? ret : Qt::ItemFlags(ret | Qt::ItemIsEditable);
}

Record UserPermissionsItem::getRecord()
{
	return mUser;
}

static int upSortVal(const User& u)
{
	if (!u.isRecord()) return 0;
	return 1;
}

UserPermissionsModel::UserPermissionsModel( QObject* parent )
: RecordSuperModel( parent )
{
	new UserPermissionsTranslator(treeBuilder());
	
	//connect( Permission::table(), SIGNAL( updated(Record,Record) ), SLOT(userPermissionsUpdated(Record,Record)));
}

void UserPermissionsModel::userPermissionsUpdated(Record up, Record)
{
	QModelIndex idx = findIndex(up);
	dataChanged( idx.sibling( idx.row(), 0), idx.sibling(idx.row(), columnCount() - 1));
}

class UserPermissionsDelegate : public RecordDelegate
{
public:
	UserPermissionsDelegate(QObject * parent = 0)
	: RecordDelegate(parent)
	{}
	
	QWidget * createEditor (QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		if (!index.isValid()) return 0;
		
		return RecordDelegate::createEditor(parent, option, index);
	}
	
	void setModelData (QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
	{
		if (editor->inherits("QComboBox")) {
			QComboBox* combo = (QComboBox*)editor;
			model->setData(index, combo->currentIndex(),Qt::EditRole);
			return;
		}
		RecordDelegate::setModelData(editor,model,index);
	}
	
	void setEditorData (QWidget* editor, const QModelIndex& index) const
	{
		if (editor->inherits("QComboBox"))
		{
			QComboBox* combo = (QComboBox*)editor;
			QVariant v = index.model()->data(index, Qt::EditRole);
			combo->setCurrentIndex(2 - upSortVal( qVariantValue<Record>(v)));
			return;
		}
		RecordDelegate::setEditorData(editor,index);
	}
};

UserPermissionsView::UserPermissionsView( QWidget * parent )
: RecordTreeView( parent )
{
	mModel = new UserPermissionsModel(this);
	setModel(mModel);
	
	setItemDelegate(new UserPermissionsDelegate(this));
	setSelectionBehavior( QAbstractItemView::SelectItems );
}

UserPermissionsWidget::UserPermissionsWidget( QWidget * parent )
: QWidget( parent )
{
	setupUi(this);
	
	mView = new UserPermissionsView( this );
	layout()->addWidget(mView);
	mView->show();
}

UserPermissionsWindow::UserPermissionsWindow( QWidget * parent )
: QMainWindow( parent )
{
	setCentralWidget( new UserPermissionsWidget(this) );
	setWindowTitle( "Edit User Permissions" );
}