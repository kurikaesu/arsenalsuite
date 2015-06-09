	


#include <qpushbutton.h>
#include <qinputdialog.h>
#include <qmessagebox.h>

#include "changeset.h"

#include "groupsdialog.h"

struct UserGroupItem : public RecordItemBase
{
	UserGroup ug;
	QString name;
	void setup( const Record & r, const QModelIndex & = QModelIndex() );
	QVariant modelData( const QModelIndex & i, int role );
	bool setModelData( const QModelIndex & i, const QVariant & v, int role );
	Qt::ItemFlags modelFlags( const QModelIndex & i );
	int compare( const QModelIndex &, const QModelIndex &, int column, bool asc );
	Record getRecord() { return ug; }
	ChangeSet changeSet( const QModelIndex & idx );
};

typedef TemplateRecordDataTranslator<UserGroupItem> UserGroupTranslator;

ChangeSet UserGroupItem::changeSet( const QModelIndex & idx )
{
	SuperModel * model = (SuperModel*)idx.model();
	// The tree is the parent of the model, and the dialog the parent of the tree
	GroupsDialog * gd = qobject_cast<GroupsDialog*>(model->parent()->parent());
	return gd ? gd->changeSet() : ChangeSet();
}

void UserGroupItem::setup( const Record & r, const QModelIndex & )
{
	ug = r;
	name = ug.group().name();
}

QVariant UserGroupItem::modelData( const QModelIndex & i, int role )
{
	CS_ENABLE(changeSet(i));
	int col = i.column();
	if( col == 0 && role == Qt::CheckStateRole ) {
		return ug.isRecord() ? Qt::Checked : Qt::Unchecked;
	} if( col == 0 && role == Qt::DisplayRole )
		return ug.group().name();
	return QVariant();
}

bool UserGroupItem::setModelData( const QModelIndex & i, const QVariant & v, int role )
{
	CS_ENABLE(changeSet(i));
	int col = i.column();
	if( col == 0 && role == Qt::CheckStateRole ) {
		Qt::CheckState cs = Qt::CheckState(v.toInt());
		if( cs == Qt::Checked )
			ug.commit();
		else
			ug.remove();
		return true;
	}
	return false;
}

Qt::ItemFlags UserGroupItem::modelFlags( const QModelIndex & i )
{
	if( i.column() == 0 )
		return Qt::ItemFlags( Qt::ItemIsUserCheckable | Qt::ItemIsEnabled );
	return Qt::ItemFlags( Qt::ItemIsEnabled );
}

int UserGroupItem::compare( const QModelIndex & idx, const QModelIndex & idx2, int column, bool asc )
{
	CS_ENABLE(changeSet(idx));
	int diff = int(ug.isRecord()) - int(UserGroupTranslator::data(idx2).ug.isRecord());
	return diff ? diff : ItemBase::compare(idx,idx2,column,asc);
}

GroupsDialog::GroupsDialog( QWidget * parent, ChangeSet changeSet )
: QDialog( parent )
, mChangeSet( changeSet.isValid() ? changeSet : ChangeSet::create() )
{
	setupUi( this );
	
	mGroupNotifier = new ChangeSetNotifier(mChangeSet, Group::table(), this);
	connect( mGroupNotifier, SIGNAL(added(RecordList)), SLOT(groupsAdded(RecordList)) );
	connect( mGroupNotifier, SIGNAL(removed(RecordList)), SLOT(groupsRemoved(RecordList)) );
	connect( mGroupNotifier, SIGNAL(updated(Record,Record)), SLOT(groupUpdated(Record,Record)) );
	
	mUserGroupNotifier = new ChangeSetNotifier(mChangeSet, UserGroup::table(), this);
	connect( mUserGroupNotifier, SIGNAL(added(RecordList )), SLOT(userGroupsAdded(RecordList)) );
	connect( mUserGroupNotifier, SIGNAL(removed(RecordList)), SLOT(userGroupsRemoved(RecordList)) );
	
	connect( mNewGroup, SIGNAL( clicked() ), SLOT( newGroup() ) );
	connect( mDeleteGroup, SIGNAL( clicked() ), SLOT( deleteGroup() ) );
	
	RecordSuperModel * sm = new RecordSuperModel( mGroupTree );
	new UserGroupTranslator(sm->treeBuilder());
	mGroupTree->setModel( sm );
	sm->setHeaderLabels( QStringList() << "Group" );
	sm->setAutoSort(true);
	sm->sort(0,Qt::DescendingOrder);

	if( !User::hasPerms( "Group", true ) ) {
		mNewGroup->setEnabled( false );
		mDeleteGroup->setEnabled( false );
	}
}

void GroupsDialog::reset()
{
}

void GroupsDialog::setUser( const User & u )
{
	CS_ENABLE(mChangeSet);
	reset();
	mUser = u;
	UserGroupList ugl = UserGroup::recordsByUser( u );
	GroupList gl = Group::select();
	gl -= ugl.groups();
	foreach( Group g, gl ) {
		UserGroup ug;
		ug.setUser( u );
		ug.setGroup( g );
		ugl += ug;
	}
	mGroupTree->model()->setRootList( ugl );
}

User GroupsDialog::user()
{
	return mUser;
}

void GroupsDialog::newGroup()
{
	CS_ENABLE(mChangeSet);
	QString groupName = QInputDialog::getText( this, "New Group", "Enter the Group Name:" );
	if( groupName.isEmpty() )
		return;
		
	if( Group::recordByName( groupName ).isRecord() ) {
		QMessageBox::warning( this, "Group Already Exists", "Group " + groupName + " already exists" );
		return;
	}
	
	Group newGrp;
	newGrp.setName( groupName );
	newGrp.commit();
}

void GroupsDialog::deleteGroup()
{
//	GroupItem * gi = (GroupItem*)mGroupList->currentItem();
//	if( gi ) {
//		gi->mGroup.remove();
//		delete gi;
//	}
}

void GroupsDialog::accept()
{
//	if( mUser.isRecord() ) {
//		mRemoved.remove();
//		mAdded.commit();
//	}
	mChangeSet.commit();
	QDialog::accept();
}

GroupList GroupsDialog::checkedGroups()
{
	return UserGroupList(mGroupTree->model()->rootList()).groups();
}
	
void GroupsDialog::setCheckedGroups( GroupList gl )
{
	foreach( Group g, gl ) {
		
	}
	reset();
	mGroupTree->model()->setRootList( gl );
}

void GroupsDialog::groupsAdded(RecordList rl)
{
	UserGroupList toAdd;
	foreach( Group g, rl )
		toAdd += UserGroup().setGroup(g).setUser(mUser);
	mGroupTree->model()->append(toAdd);
}

void GroupsDialog::groupsRemoved(RecordList rl)
{
	QModelIndexList toRemove;
	for( ModelIter it(mGroupTree->model()); it.isValid(); ++it ) {
		if( rl.contains( UserGroupTranslator::data(*it).ug.group() ) )
			toRemove += *it;
	}
	mGroupTree->model()->remove(toRemove);
}

void GroupsDialog::groupUpdated(Record old,Record up)
{
	for( ModelIter it(mGroupTree->model()); it.isValid(); ++it ) {
		if( UserGroupTranslator::data(*it).ug.group() == old )
			mGroupTree->model()->dataChange(*it);
	}
}

void GroupsDialog::userGroupsAdded(RecordList rl)
{
	mGroupTree->model()->updated(rl);
	UserGroupList ugl(rl);
	foreach(UserGroup ug, ugl) {
		if( ug.user() == mUser ) {
			for( ModelIter it(mGroupTree->model()); it.isValid(); ++it ) {
				if( UserGroupTranslator::data(*it).ug.group() == ug.group() ) {
					UserGroupTranslator::data(*it).ug = ug;
					mGroupTree->model()->dataChange(*it);
				}
			}
		}
	}
}

void GroupsDialog::userGroupsRemoved(RecordList rl)
{
	foreach( QModelIndex idx, mGroupTree->findIndexes(rl) ) {
		mGroupTree->model()->dataChange(idx);
	}
}
