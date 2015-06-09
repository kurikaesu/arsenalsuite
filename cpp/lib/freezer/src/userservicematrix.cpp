
#include <qlineedit.h>
#include <qheaderview.h>
#include <qinputdialog.h>
#include <qmessagebox.h>
#include <qmenu.h>

#include "userservicematrix.h"
#include "syslog.h"
#include "syslogseverity.h"
#include "syslogrealm.h"
#include "qvariantcmp.h"

struct UserServiceItem : public RecordItem
{
	User mUser;
	void setup( const Record & r, const QModelIndex & );
	QVariant modelData ( const QModelIndex & index, int role ) const;
	bool setModelData( const QModelIndex & idx, const QVariant & v, int role );
	//int compare( const QModelIndex & a, const QModelIndex & b, int, bool );
	Qt::ItemFlags modelFlags( const QModelIndex & );
	Record getRecord();
	static UserServiceModel * model(const QModelIndex &);
};

typedef TemplateRecordDataTranslator<UserServiceItem> UserServiceTranslator;

UserServiceModel * UserServiceItem::model(const QModelIndex & idx)
{
	return const_cast<UserServiceModel*>(qobject_cast<const UserServiceModel*>(idx.model()));
}

void UserServiceItem::setup( const Record & r, const QModelIndex & )
{
	mUser = r;
	LOG_5( "Setting up user " + mUser.name() );
}

/*
int HostServiceItem::compare( const QModelIndex & a, const QModelIndex & b, int column, bool asc )
{
	if( column == 0 ) return ItemBase::compare( a, b, column, asc );
	HostServiceModel * m = model(a);
	HostService hs = m->findHostService( mHost, column );
	HostService ohs = m->findHostService( HostServiceTranslator::data(b).mHost, column );
	return compareRetI( hsSortVal( hs ), hsSortVal( ohs ) );
}
*/

QVariant UserServiceItem::modelData( const QModelIndex & idx, int role ) const
{
	if( role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::ForegroundRole ) {
		if( idx.column() == 0 ) return mUser.name();
		UserServiceModel * m = model(idx);
		if( role == Qt::EditRole )
			return qVariantFromValue<Record>(m->findUserService( mUser, idx.column() ));
		QVariant d = m->serviceData( mUser, idx.column(), role );

		if( role == Qt::ForegroundRole ) {
			return (d.toInt() > 0 ? Qt::darkGreen : Qt::darkRed);
		}
		return d;
	}
	return QVariant();
}

bool UserServiceItem::setModelData( const QModelIndex & idx, const QVariant & v, int role )
{
	if( role == Qt::EditRole && idx.column() > 0 ) {
		UserServiceModel * m = model(idx);
		UserService us = m->findUserService( mUser, idx.column() );
		switch( v.toInt() ) {
			case 0:
			case 1:
				us.setLimit( v.toInt() );
				us.commit();
				break;
			case 2:
				us.remove();
		}
		return true;
	}
	return false;
}

Qt::ItemFlags UserServiceItem::modelFlags( const QModelIndex & idx )
{
	Qt::ItemFlags ret = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
	return idx.column() == 0 ? ret : Qt::ItemFlags(ret | Qt::ItemIsEditable);
}

Record UserServiceItem::getRecord()
{
	return mUser;
}

UserServiceModel::UserServiceModel( QObject * parent )
: RecordSuperModel( parent )
{
	new UserServiceTranslator(treeBuilder());

	// Set UserService index to cache
	updateServices();
}

void UserServiceModel::updateServices()
{
	Index * i = UserService::table()->index( "UserAndService" );
	if( i ) i->setCacheEnabled( true );
	UserService::schema()->setPreloadEnabled( true );
	mServices = Service::select().sorted( "service" );
	LOG_5( mServices.services().join(",") );
	setHeaderLabels( QStringList() << "User" << mServices.services() );
	UserList users = User::select();
	LOG_5( "setRootList: Adding " + QString::number(users.size()) + " users" );
	setRootList( users );
}

UserService UserServiceModel::findUserService( const User & user, int column ) const
{
	column -= 1;
	if( column < 0 || column >= (int)mServices.size() ) return UserService();
	Service s = mServices[column];
	UserService ret = UserService::recordByUserAndService( user, s );
	if( !ret.isRecord() ) {
		ret.setUser( user );
		ret.setService( s );
	}
	return ret;
}

UserService UserServiceModel::findUserService( const QModelIndex & idx ) const
{
	return findUserService( getRecord(idx), idx.column() );
}

QVariant UserServiceModel::serviceData ( const User & user, int column, int ) const
{
	UserService us = findUserService( user, column );
	if( us.isRecord() )
		return us.limit();

	return "unltd";
}

void UserServiceModel::setUserFilter( const QString & filter )
{
	setRootList( User::select().filter( "name", QRegExp( filter ) ) );
}

class UserServiceDelegate : public RecordDelegate
{
public:
	UserServiceDelegate ( QObject * parent = 0 )
	: RecordDelegate( parent )
	{}

	QWidget * createEditor ( QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index ) const
	{
		if( !index.isValid() ) return 0;
		QVariant v = index.model()->data(index, Qt::EditRole);
		uint t = v.userType();
		if( t == static_cast<uint>(qMetaTypeId<Record>()) ) {
            QLineEdit * ret = new QLineEdit(parent);
			ret->installEventFilter(const_cast<UserServiceDelegate*>(this));
			return ret;
		}
		return RecordDelegate::createEditor(parent,option,index);
	}

	void setModelData ( QWidget * editor, QAbstractItemModel * model, const QModelIndex & index ) const
	{
		if( editor->inherits( "QLineEdit" ) ) {
            QLineEdit * lineEdit = (QLineEdit*)editor;
			model->setData(index, lineEdit->text().toUInt(), Qt::EditRole);
			return;
		}
		RecordDelegate::setModelData(editor,model,index);
	}

	void setEditorData ( QWidget * editor, const QModelIndex & index ) const
	{
		if( editor->inherits( "QLineEdit" ) ) {
            QLineEdit * lineEdit = (QLineEdit*)editor;
			QVariant v = index.model()->data(index, Qt::EditRole);
            lineEdit->setText( v.toString() );
			return;
		}
		RecordDelegate::setEditorData(editor,index);
	}
};

UserServiceMatrix::UserServiceMatrix( QWidget * parent )
: RecordTreeView( parent )
{
	mModel = new UserServiceModel(this);
	setModel( mModel );
	setItemDelegate( new UserServiceDelegate(this) );
	connect( this, SIGNAL( showMenu( const QPoint &, const QModelIndex & ) ), SLOT( slotShowMenu( const QPoint &, const QModelIndex & ) ) );
	setSelectionBehavior( QAbstractItemView::SelectItems );

	// Turn of double-click editing
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	header()->setStretchLastSection( false );

	connect( Service::table(), SIGNAL( added(RecordList) ), SLOT( updateServices() ) );
	connect( Service::table(), SIGNAL( removed(RecordList) ), SLOT( updateServices() ) );
	connect( Service::table(), SIGNAL( updated(Record,Record) ), SLOT( updateServices() ) );
}

void UserServiceMatrix::setUserFilter( const QString & filter )
{
	mUserFilter = filter;
	mModel->setUserFilter( filter );
}

void UserServiceMatrix::setServiceFilter( const QString & filter )
{
	mServiceFilter = filter;
	QHeaderView * h = header();
	QRegExp re(filter);
	for( int i=1; i < h->count(); i++ )
		h->setSectionHidden( i, !model()->headerData(i, Qt::Horizontal, Qt::DisplayRole ).toString().contains( re ) );
}

void UserServiceMatrix::updateServices()
{
	mModel->updateServices();
	setServiceFilter( mServiceFilter );
}

void UserServiceMatrix::slotShowMenu( const QPoint & pos, const QModelIndex & /*underMouse*/ )
{
	QItemSelection sel = selectionModel()->selection();
	if( !sel.isEmpty() ) {
		QMenu * menu = new QMenu( this );
		QAction * limit   = menu->addAction( "Set Limit" );
		QAction * disable = menu->addAction( "Set Unlimited" );
		QAction * result  = menu->exec(pos);

		UserServiceList toUpdate;
		if( result ) {
            uint limitValue = 0;
			bool ok;
            if( result == limit )
                limitValue = QInputDialog::getInt(this, tr("Set Slot Limit"),
                                             tr("Slots:"), 100, 0, 99999, 1, &ok);

			foreach( QModelIndex idx, sel.indexes() ) {
				UserService us = mModel->findUserService( idx );

				if (ok)
					us.setLimit( limitValue );

				toUpdate += us;
			}

			if ( result == limit && ok ) {
				toUpdate.commit();
                SysLog log;
                log.setSysLogRealm( SysLogRealm::recordByName("Farm") );
                log.setSysLogSeverity( SysLogSeverity::recordByName("Warning") );
                log.setMessage( QString("%1 services set to %2").arg(toUpdate.size()).arg(limitValue) );
                log.set_class("UserServiceMatrix");
                log.setMethod("slotShowMenu");
                log.setUserName( User::currentUser().name() );
                log.setHostName( Host::currentHost().name() );
                log.commit();
            }
			else if ( result == disable ) {
				toUpdate.remove();
                SysLog log;
                log.setSysLogRealm( SysLogRealm::recordByName("Farm") );
                log.setSysLogSeverity( SysLogSeverity::recordByName("Warning") );
                log.setMessage( QString("%1 services set to unlimited").arg(toUpdate.size()) );
                log.set_class("UserServiceMatrix");
                log.setMethod("slotShowMenu");
                log.setUserName( User::currentUser().name() );
                log.setHostName( Host::currentHost().name() );
                log.commit();
            }
		}
		delete menu;
	}
}

UserServiceMatrixWindow::UserServiceMatrixWindow( QWidget * parent )
: QMainWindow( parent )
{
	setupUi(this);

	setWindowTitle( "Edit User Service Limits" );

	mView = new UserServiceMatrix( mCentralWidget );
	mCentralWidget->layout()->addWidget(mView);
	mView->show();
	connect( mUserFilterEdit, SIGNAL( textChanged( const QString & ) ), mView, SLOT( setUserFilter( const QString & ) ) );
	connect( mServiceFilterEdit, SIGNAL( textChanged( const QString & ) ), mView, SLOT( setServiceFilter( const QString & ) ) );
	QMenu * fileMenu = menuBar()->addMenu( "&File" );
	fileMenu->addSeparator();
	fileMenu->addAction( "&Close", this, SLOT( close() ) );
}

