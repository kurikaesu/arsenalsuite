#include <qcombobox.h>

#include "usermanager.h"

struct UserPermissionsItem : public RecordItem
{
	User mUser;
	PermissionList mPerms;
	UserPermissionsItem() { setupPermTypes(); }
	UserPermissionsItem(const User& u)
	{
		setupPermTypes();
		
		mUser = u;
		mPerms.resize(UserPermissionsItem::permTypes->size());
		PermissionList temp = Permission::select("fkeyusr=" + mUser.key());
		foreach (Permission perm, temp)
		{
			QString permClass = perm._class();
			if (permClass.contains("Arsenal::Perm::"))
			{
				QStringRef key(permString, 15, permString.size() - 15);
				int idx = permTypes.indexOf(key);
				if (idx >= 0)
					mPerms[idx] = perm;
			}
		}
	}
	
	void setupPermTypes()
	{
		if (UserPermissionsItem::permTypes == 0)
		{
			UserPermissionsItem::permTypes = new QList<QString>();
			
			UserPermissionsItem::permTypes.append("HostService");
			UserPermissionsItem::permTypes.append("UserService");
			UserPermissionsItem::permTypes.append("Projects");
			UserPermissionsItem::permTypes.append("ProjectWeights");
			UserPermissionsItem::permTypes.append("ProjectReserve");
			UserPermissionsItem::permTypes.append("HostService");
			UserPermissionsItem::permTypes.append("UserService");
			UserPermissionsItem::permTypes.append("JobTypes");
			UserPermissionsItem::permTypes.append("Services");
			UserPermissionsItem::permTypes.append("Licenses");
		}
	}
	
	int permissionByColumn(int col) const {
		if (col > mPerms.size()) return 0;
		if (!mPerms[col].isRecord()) return 0;
		if (!mPerms[col].modify())
			return 1;
		return 2;
	}
	QVariant serviceData(int column) const
	{
		switch (permissionByColumn(column))
		{
		case 0:
			return "UNSET";
		case 1:
			return "VIEW";
		case 2:
			return "MODIFY";
		}
		return "UNKNOWN";
	}
	
	void setup(const Record & r, const QModelIndex&);
	QVariant modelData ( const QModelIndex & index, int role ) const;
	bool setModelData( const QModelIndex & idx, const QVariant & v, int role );
	int compare( const QModelIndex & a, const QModelIndex & b, int, bool );
	Qt::ItemFlags modelFlags( const QModelIndex & );
	Record getRecord();
	static UserPermissionsModel * model(const QModelIndex &);
	static QList<QString> * permTypes = 0;
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
	if (role == Qt::DisplayerRole || role == Qt::EditRole || role == Qt::ForegroundRole) {
		if (idx.column() == 0) return mUser.name();
		if (role == Qt::EditRole)
			return qVariantFromValue<bool>(permissionByColumn(idx.column() - 1));
		QVariant d = serviceData(idx.column() - 1);
		if (role == Qt::ForegroundRole) {
			QString txt = d.toString();
			return (txt == "MODIFY" ? Qt::green : (txt == "VIEW" ? Qt::blue : Qt::black));
		}
		return d;
	}
	return QVariant();
}

bool UserPermissionsItem::setModelData(const QModelIndex& idx, const QVariant& v, int role)
{
	if (role == Qt::EditRole && idx.column() > 1) {
		switch (v.toInt()) {
			case 0:
			case 1:
				
			default:
				break;
		}
		return true;
	}
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
	
	connect( Permission::table(), SIGNAL( updated(Record,Record) ), SLOT(userPermissionsUpdated(Record,Record)));
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