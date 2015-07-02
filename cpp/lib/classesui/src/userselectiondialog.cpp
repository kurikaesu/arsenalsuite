#include "userselectiondialog.h"

class UserSelectionItem : public RecordItemBase
{
public:
	UserSelectionItem( const User& u ) { setup(u); }
	UserSelectionItem(){}

	QVariant modelData( const QModelIndex& i, int role ) {
		int col = i.column();
		switch( col ) {
			case 0:
			{
				if (role == Qt::DisplayRole)
					return user.name();
				break;
			}	
		}
		return QVariant();
	}
	bool setModelData( const QModelIndex& i, const QVariant& v, int role ) {
		int col = i.column();
		if (role == Qt::EditRole) {
			switch(col) {
				case 0:
					return false;
			}
		}
		
		return false;
	}
	Qt::ItemFlags modelFlags( const QModelIndex & index ) {
		return Qt::ItemFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
	}
	
	void setup( const Record& r, const QModelIndex& = QModelIndex() ) {
		user = r;
	}
	Record getRecord() { return user; }
private:
	User user;
};

typedef TemplateRecordDataTranslator<UserSelectionItem> UserSelectionTranslator;

UserSelectionDialog::UserSelectionDialog( QWidget * parent, const UserList& available, const UserList& selected )
: QDialog( parent )
{
	setupUi( this );
	mAvailableModel = new RecordSuperModel(mAvailableUsers);
	mSelectedModel = new RecordSuperModel(mSelectedUsers);
	
	new UserSelectionTranslator(mAvailableModel->treeBuilder());
	new UserSelectionTranslator(mSelectedModel->treeBuilder());
	
	mAvailableUsers->setModel(mAvailableModel);
	mSelectedUsers->setModel(mSelectedModel);
	
	mAvailableModel->setHeaderLabels(QStringList() << "User");
	mSelectedModel->setHeaderLabels(QStringList() << "User");
	
	mAvailableModel->setRootList(available);
	mSelectedModel->setRootList(selected);
	
	connect(mAddButton, SIGNAL( clicked() ), SLOT( slotAddUser() ) );
	connect(mRemoveButton, SIGNAL( clicked() ), SLOT( slotRemoveUser() ) );
}

void UserSelectionDialog::slotRemoveUser()
{
	RecordList current = mSelectedUsers->selection();
	mSelectedModel->remove(current);
	mAvailableModel->append(current);
}

void UserSelectionDialog::slotAddUser()
{
	RecordList current = mAvailableUsers->selection();
	mAvailableModel->remove(current);
	mSelectedModel->append(current);
}

void UserSelectionDialog::setAvailableUsers(const UserList& available)
{
	mAvailableModel->setRootList(available);
}

void UserSelectionDialog::setSelectedUsers(const UserList& selected)
{
	mSelectedModel->setRootList(selected);
}

void UserSelectionDialog::accept()
{
	QDialog::accept();
}

UserList UserSelectionDialog::selectedUsers() const
{
	return mSelectedModel->rootList();
}