
#include <qcombobox.h>
#include <qevent.h>
#include <qheaderview.h>
#include <qinputdialog.h>
#include <qmenubar.h>
#include <qmessagebox.h>
#include <qmenu.h>
#include <qtimer.h>

#include "iniconfig.h"
#include "qvariantcmp.h"

#include "syslog.h"
#include "syslogrealm.h"
#include "syslogseverity.h"
#include "user.h"
#include "hostgroup.h"
#include "hostlistsdialog.h"

#include "hostservicematrix.h"

#include "ui_savelistdialogui.h"

struct HostServiceItem : public RecordItem
{
	Host mHost;
	HostStatus mHostStatus;
	HostServiceList mHostServices;
	QVector<HostService> mHostServiceByColumn;
	HostServiceItem() {}
	HostServiceItem( const Host & h, HostServiceList hsl, ServiceList services )
	: mHostServiceByColumn(services.size())
	{
		mHost = h;
		mHostStatus = mHost.hostStatus();
		mHostServices = hsl;
		foreach( HostService hs, hsl ) {
			int col = services.findIndex( hs.service() );
			if( col >= 0 )
				mHostServiceByColumn[col] = hs;
		}
	}
	HostService hostServiceByColumn( int col ) const {
		if( col > 0 && (col-1) < mHostServiceByColumn.size() )
			return mHostServiceByColumn[col-1];
		return HostService();
	}
	QVariant serviceData( int column ) const
	{
		HostService hs = hostServiceByColumn( column );
		if( hs.isRecord() )
			return hs.enabled() ? "Enabled" : "Disabled";
		return "No Service";
	}

	void setup( const Record & r, const QModelIndex & );
	QVariant modelData ( const QModelIndex & index, int role ) const;
	bool setModelData( const QModelIndex & idx, const QVariant & v, int role );
	int compare( const QModelIndex & a, const QModelIndex & b, int, bool );
	Qt::ItemFlags modelFlags( const QModelIndex & );
	Record getRecord();
	static HostServiceModel * model(const QModelIndex &);
};

typedef TemplateRecordDataTranslator<HostServiceItem> HostServiceTranslator;

HostServiceModel * HostServiceItem::model(const QModelIndex & idx)
{
	return const_cast<HostServiceModel*>(qobject_cast<const HostServiceModel*>(idx.model()));
}

void HostServiceItem::setup( const Record & r, const QModelIndex & )
{
	mHost = r;
	mHostStatus = mHost.hostStatus();
}

static int hsSortVal( const HostService & hs )
{
	if( !hs.isRecord() ) return 0;
	if( hs.enabled() ) return 2;
	return 1;
}

int HostServiceItem::compare( const QModelIndex & a, const QModelIndex & b, int column, bool asc )
{
	if( column == 0 ) return ItemBase::compare( a, b, column, asc );
	HostService hs = hostServiceByColumn( column );
	HostService ohs = HostServiceTranslator::data(b).hostServiceByColumn( column );
	return compareRetI( hsSortVal( hs ), hsSortVal( ohs ) );
}

QVariant HostServiceItem::modelData( const QModelIndex & idx, int role ) const
{
	if( role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::ForegroundRole ) {
		if( idx.column() == 0 ) return mHost.name();
		if( idx.column() == 1 ) return mHostStatus.slaveStatus();
		if( idx.column() == 2 ) return mHost.os();
		if( idx.column() == 3 ) return mHost.userIsLoggedIn() ? "Logged On" : "Logged Off";
		if( role == Qt::EditRole )
			return qVariantFromValue<Record>(hostServiceByColumn( idx.column()-3 ));
		QVariant d = serviceData( idx.column()-3 );
		if( role == Qt::ForegroundRole ) {
			QString txt = d.toString();
			return (txt == "Enabled" ? Qt::green : (txt == "Disabled" ? Qt::red : Qt::black));
		}
		return d;
	}
	return QVariant();
}

bool HostServiceItem::setModelData( const QModelIndex & idx, const QVariant & v, int role )
{
	if( role == Qt::EditRole && idx.column() > 3 ) {
		HostServiceModel * m = model(idx);
		HostService hs = hostServiceByColumn( idx.column() - 3 );
		switch( v.toInt() ) {
			case 0:
			case 1:
				if( !hs.isRecord() ) {
					hs.setHost( mHost );
					hs.setService( m->serviceByColumn( idx.column() - 3 ) );
				}
				hs.setEnabled( v.toInt() == 0 );
				hs.commit();
				break;
			case 2:
				hs.remove();
		}
		return true;
	}
	return false;
}

Qt::ItemFlags HostServiceItem::modelFlags( const QModelIndex & idx )
{
	Qt::ItemFlags ret = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
	return idx.column() < 4 ? ret : Qt::ItemFlags(ret | Qt::ItemIsEditable);
}

Record HostServiceItem::getRecord()
{
	return mHost;
}

HostServiceModel::HostServiceModel( QObject * parent )
: RecordSuperModel( parent )
{
	new HostServiceTranslator(treeBuilder());

	connect( HostService::table(), SIGNAL( added(RecordList) ), SLOT( hostServicesAdded(RecordList) ) );
	connect( HostService::table(), SIGNAL( removed(RecordList) ), SLOT( hostServicesRemoved(RecordList) ) );
	connect( HostService::table(), SIGNAL( updated(Record,Record) ), SLOT( hostServiceUpdated(Record,Record) ) );

	// Set HostService index to cache
	updateServices();
	// Initialize the list
	currentFilteredList = Host::select();
	hostFiltering = statusFiltering = osFiltering = userFiltering = false;
}

void HostServiceModel::hostServicesAdded(RecordList rl)
{
	HostServiceList hsl(rl);
	HostList hosts = hsl.hosts();
	mHostServices += hsl;
	mHostServicesByHost = mHostServices.groupedByForeignKey<Host,HostServiceList>("fkeyhost");
	updateHosts(hosts);
}

void HostServiceModel::hostServicesRemoved(RecordList rl)
{
	HostServiceList hsl(rl);
	HostList hosts = hsl.hosts();
	mHostServices -= hsl;
	mHostServicesByHost = mHostServices.groupedByForeignKey<Host,HostServiceList>("fkeyhost");
	updateHosts(hosts);
}

void HostServiceModel::hostServiceUpdated(Record up, Record)
{
	QModelIndex idx = findIndex(up);
	dataChanged( idx.sibling( idx.row(), 0 ), idx.sibling( idx.row(), columnCount() - 1 ) );
}

void HostServiceModel::updateServices()
{
	mHostServices = HostService::select();
	mHostServicesByHost = mHostServices.groupedByForeignKey<Host,HostServiceList>("fkeyhost");

	mServices = Service::select().sorted( "service" );
	setHeaderLabels( QStringList() << "Host" << "Status" << "OS" << "User Logged" << mServices.services() );
	// Keep the list in memory until the items can store each hoststatus record
	mStatuses = HostStatus::select();
	setHostList( Host::select() );
}

Service HostServiceModel::serviceByColumn( int column ) const
{
	column -= 1;
	if( column < 0 || column >= (int)mServices.size() ) return Service();
	return mServices[column];
}

void HostServiceModel::setHostList( HostList hosts )
{
	LOG_5( "Adding " + QString::number(hosts.size()) + " hosts" );
	clear();
	SuperModel::InsertClosure closure(this);
	HostServiceTranslator * hst = (HostServiceTranslator*)treeBuilder()->defaultTranslator();
	QModelIndexList ret = append( QModelIndex(), hosts.size(), hst );
	for( uint i=0; i < hosts.size(); ++i ) {
		Host h = hosts[i];
		LOG_5( "Host " + h.name() + " has services " + mHostServicesByHost[h].services().services().join(",") );
		hst->data(ret[i]) = HostServiceItem( h, mHostServicesByHost[h], mServices );
	}
}

HostService HostServiceModel::findHostService( const QModelIndex & idx )
{
	HostService hs;
	if( idx.isValid() ) {
		HostServiceItem & item = ((HostServiceTranslator*)treeBuilder()->defaultTranslator())->data(idx);
		hs = item.hostServiceByColumn( idx.column() );
		if( !hs.isRecord() ) {
			hs.setHost( item.mHost );
			hs.setService( serviceByColumn( idx.column() ) );
		}
	}
	return hs;
}

void HostServiceModel::setHostFilter( const QString & filter, bool cascading )
{
	currentHostFilter = filter;

	if( filter != "" ) {
		hostFiltering = true;

	if( cascading )
			currentFilteredList = currentFilteredList.filter( "name", QRegExp( filter ) );
		else
			currentFilteredList = Host::select().filter( "name", QRegExp( filter ) );
	} else {
		hostFiltering = false;

		currentFilteredList = Host::select();
	}
	
	if( !cascading ) {
		if( statusFiltering )
			setStatusFilter( currentStatusFilter , true);
		if( osFiltering )
			setOSFilter( currentOsFilter , true);
		if( userFiltering )
			setUserLoggedFilter( userFiltering , true);
	}

	setRootList( currentFilteredList );
}

void HostServiceModel::setStatusFilter( const QString & filter , bool cascading)
{
	currentStatusFilter = filter;

	if( filter != "" ) {
		statusFiltering = true;
		
		if( cascading ) {
			HostStatusList statuses = currentFilteredList.hostStatuses().filter( "slavestatus", QRegExp( filter ));
			currentFilteredList = statuses.hosts();
		} else {
			HostStatusList statuses = HostStatus::select().filter( "slavestatus", QRegExp( filter ));
			currentFilteredList = statuses.hosts();
		}
	} else {
		statusFiltering = false;

		currentFilteredList = Host::select();
	}

	if( !cascading ) {
		if( hostFiltering )
			setHostFilter( currentHostFilter , true);
		if( osFiltering )
			setOSFilter( currentOsFilter , true);
		if( userFiltering )
			setUserLoggedFilter( userFiltering , true);
	}

	setRootList( currentFilteredList );
}

void HostServiceModel::setOSFilter( const QString & filter , bool cascading)
{
	currentOsFilter = filter;

	if( filter != "" ) {
		osFiltering = true;

		if( cascading )
			currentFilteredList = currentFilteredList.filter( "os", QRegExp( filter ) );
		else
			currentFilteredList = Host::select().filter( "os", QRegExp( filter ) );
	} else {
		osFiltering = false;

		currentFilteredList = Host::select();
	}

	if( !cascading ) {
		if( hostFiltering )
			setHostFilter( currentHostFilter , true);
		if( statusFiltering )
			setStatusFilter( currentStatusFilter , true);
		if( userFiltering )
			setUserLoggedFilter( userFiltering , true);
	}

	setRootList( currentFilteredList );
}

void HostServiceModel::setUserLoggedFilter( bool filter, bool useLoggedIn, bool cascading)
{
	if( filter ) {
		userFiltering = true;

		if( cascading )
			currentFilteredList = currentFilteredList.filter( "userisloggedin", useLoggedIn );
		else
			currentFilteredList = Host::select().filter( "userisloggedin", useLoggedIn );
	} else {
		userFiltering = false;

		currentFilteredList = Host::select();
	}

	if( !cascading ) {
		if( hostFiltering )
			setHostFilter( currentHostFilter , true);
		if( statusFiltering )
			setStatusFilter( currentStatusFilter , true);
		if( osFiltering )
			setOSFilter( currentOsFilter , true);
	}

	setRootList( currentFilteredList );
}

void HostServiceModel::updateHosts( HostList hosts )
{
	refreshIndexes( findIndexes( hosts ) );
}

void HostServiceModel::refreshIndexes( QModelIndexList indexes )
{
	HostServiceTranslator * hst = (HostServiceTranslator*)treeBuilder()->defaultTranslator();
	for( int i=0; i < indexes.size(); ++i ) {
		QModelIndex idx = indexes[i];
		HostServiceItem & item = hst->data(idx);
		item = HostServiceItem( item.mHost, mHostServicesByHost[item.mHost], mServices );
		dataChange( idx.sibling( idx.row(), 0 ), idx.sibling( idx.row(), columnCount()-1 ) );
	}
}

class HostServiceDelegate : public RecordDelegate
{
public:
	HostServiceDelegate ( QObject * parent = 0 )
	: RecordDelegate( parent )
	{}

	QWidget * createEditor ( QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index ) const
	{
		if( !index.isValid() ) return 0;
		QVariant v = index.model()->data(index, Qt::EditRole);
		uint t = v.userType();
		if( t == static_cast<uint>(qMetaTypeId<Record>()) ) {
			QComboBox * ret = new QComboBox(parent);
			ret->addItems( QStringList() << "Enabled" << "Disabled" << "No Service" );
			ret->setCurrentIndex( 2 - hsSortVal( qVariantValue<Record>(v) ) );
			ret->installEventFilter(const_cast<HostServiceDelegate*>(this));
			return ret;
		}
		return RecordDelegate::createEditor(parent,option,index);
	}

	void setModelData ( QWidget * editor, QAbstractItemModel * model, const QModelIndex & index ) const
	{
		if( editor->inherits( "QComboBox" ) ) {
			QComboBox * combo = (QComboBox*)editor;
			model->setData(index, combo->currentIndex(),Qt::EditRole);
			return;
		}
		RecordDelegate::setModelData(editor,model,index);
	}

	void setEditorData ( QWidget * editor, const QModelIndex & index ) const
	{
		if( editor->inherits( "QComboBox" ) ) {
			QComboBox * combo = (QComboBox*)editor;
			QVariant v = index.model()->data(index, Qt::EditRole);
			combo->setCurrentIndex( 2 - hsSortVal( qVariantValue<Record>(v) ) );
			return;
		}
		RecordDelegate::setEditorData(editor,index);
	}
};

HostServiceMatrix::HostServiceMatrix( QWidget * parent )
: RecordTreeView( parent )
, mHostFilterCS( false )
, mServiceFilterCS( false )
{
	mModel = new HostServiceModel(this);
	setModel( mModel );
	setItemDelegate( new HostServiceDelegate(this) );
	connect( this, SIGNAL( showMenu( const QPoint &, const QModelIndex & ) ), SLOT( slotShowMenu( const QPoint &, const QModelIndex & ) ) );

	hostFTimer = new QTimer(this);
	statusFTimer = new QTimer(this);
	osFTimer = new QTimer(this);

	connect( hostFTimer, SIGNAL(timeout()), this, SLOT( hostTimerExpired() ) );
	connect( statusFTimer, SIGNAL(timeout()), this, SLOT( statusTimerExpired() ) );
	connect( osFTimer, SIGNAL(timeout()), this, SLOT( osTimerExpired() ) );

	hostFTimer->setSingleShot( true );
	statusFTimer->setSingleShot( true );
	osFTimer->setSingleShot( true );

	setSelectionBehavior( QAbstractItemView::SelectItems );
	header()->setStretchLastSection( false );

	connect( Service::table(), SIGNAL( added(RecordList) ), SLOT( updateServices() ) );
	connect( Service::table(), SIGNAL( removed(RecordList) ), SLOT( updateServices() ) );
	connect( Service::table(), SIGNAL( updated(Record,Record) ), SLOT( updateServices() ) );

	userFiltering = false;
	useLoggedIn = true;
}

HostServiceModel * HostServiceMatrix::getModel() const
{
	return mModel;
}

void HostServiceMatrix::setHostFilter( const QString & filter )
{
	mHostFilter = filter;
	QRegExp re( filter, mHostFilterCS ? Qt::CaseSensitive : Qt::CaseInsensitive );
	for( ModelIter it(mModel); it.isValid(); ++it )
		setRowHidden( (*it).row(), QModelIndex(), !Host(mModel->getRecord(*it)).name().contains( re ) );

	hostFTimer->start(300);
}

void HostServiceMatrix::hostTimerExpired()
{
	mModel->setHostFilter( mHostFilter );
}

void HostServiceMatrix::setStatusFilter( const QString & filter )
{
	mHostStatusFilter = filter;
	statusFTimer->start(300);
}

void HostServiceMatrix::statusTimerExpired()
{
	mModel->setStatusFilter( mHostStatusFilter );
}

void HostServiceMatrix::setOSFilter( const QString & filter )
{
	mHostOsFilter = filter;
	osFTimer->start(300);
}

void HostServiceMatrix::osTimerExpired()
{
	mModel->setOSFilter( mHostOsFilter );
}

void HostServiceMatrix::setUserLoggedFilter( int filter )
{
	userFiltering = filter > 0;
	mModel->setUserLoggedFilter( filter > 0 , useLoggedIn);
}

void HostServiceMatrix::setUserLoggedType( bool type )
{
	useLoggedIn = type;
	if( userFiltering )
		mModel->setUserLoggedFilter( true, useLoggedIn );
}

void HostServiceMatrix::setServiceFilter( const QString & filter )
{
	mServiceFilter = filter;
	QHeaderView * h = header();
	QRegExp re(filter, mServiceFilterCS ? Qt::CaseSensitive : Qt::CaseInsensitive);
	for( int i=1; i < h->count(); i++ )
		h->setSectionHidden( i, !model()->headerData(i, Qt::Horizontal, Qt::DisplayRole ).toString().contains( re ) );
}

void HostServiceMatrix::setHostFilterCS( bool cs )
{
	mHostFilterCS = cs;
	setHostFilter( mHostFilter );
}

void HostServiceMatrix::setServiceFilterCS( bool cs )
{
	mServiceFilterCS = cs;
	setServiceFilter( mServiceFilter );
}

void HostServiceMatrix::updateServices()
{
	mModel->updateServices();
	setServiceFilter( mServiceFilter );
}

void HostServiceMatrix::slotShowMenu( const QPoint & pos, const QModelIndex & /*underMouse*/ )
{
	QItemSelection sel = selectionModel()->selection();
	if( !sel.isEmpty() ) {
		QMenu * menu = new QMenu( this );
		QAction * en = menu->addAction( "Set Enabled" );
		QAction * dis = menu->addAction( "Set Disabled" );
		//QAction * nos = 
		menu->addAction( "Set No Service" );
		QAction * result = menu->exec(pos);
		if( result ) {
			HostServiceList toUpdate;
			foreach( QModelIndex idx, sel.indexes() ) {
				HostService hs = mModel->findHostService( idx );
				// We only want to manipulate what we've filtered out.
				if( mServiceFilter == "" || QRegExp(mServiceFilter).indexIn(hs.service().service()) > -1)
				{
					hs.setEnabled( result == en );
					toUpdate += hs;
				}
			}
			if( result == en || result == dis ) {
				toUpdate.commit();
				SysLog log;
				log.setSysLogRealm( SysLogRealm::recordByName("Farm") );
				log.setSysLogSeverity( SysLogSeverity::recordByName("Warning") );
				log.setMessage( QString("%1 host services modified").arg(toUpdate.size()) );
				log.set_class("UserServiceMatrix");
				log.setMethod("slotShowMenu");
				log.setUserName( User::currentUser().name() );
				log.setHostName( Host::currentHost().name() );
				log.commit();
			} else {
				toUpdate.remove();
				SysLog log;
				log.setSysLogRealm( SysLogRealm::recordByName("Farm") );
				log.setSysLogSeverity( SysLogSeverity::recordByName("Warning") );
				log.setMessage( QString("%1 host services removed").arg(toUpdate.size()) );
				log.set_class("HostServiceMatrix");
				log.setMethod("slotShowMenu");
				log.setUserName( User::currentUser().name() );
				log.setHostName( Host::currentHost().name() );
				log.commit();
			}
		}
		delete menu;
	}
}

HostServiceMatrixWidget::HostServiceMatrixWidget( QWidget * parent )
: QWidget( parent )
, mHostGroupRefreshPending(false)
{
	setupUi(this);

	mView = new HostServiceMatrix( this );
	layout()->addWidget(mView);
	mView->show();
	mHostFilterEdit->installEventFilter(this);
	mServiceFilterEdit->installEventFilter(this);
	connect( mHostFilterEdit, SIGNAL( textChanged( const QString & ) ), mView, SLOT( setHostFilter( const QString & ) ) );
	connect( mServiceFilterEdit, SIGNAL( textChanged( const QString & ) ), mView, SLOT( setServiceFilter( const QString & ) ) );

	connect( mHostStatusFilterEdit, SIGNAL( textChanged( const QString & ) ), mView, SLOT( setStatusFilter( const QString & ) ) );
	connect( mHostOsFilterEdit, SIGNAL( textChanged( const QString & ) ), mView, SLOT( setOSFilter( const QString & ) ) );
	connect( mHostUserLoggedInCheckbox, SIGNAL( stateChanged( int ) ), mView, SLOT( setUserLoggedFilter( int ) ) );
	connect( mUserLoggedInType, SIGNAL( toggled( bool ) ), mView, SLOT( setUserLoggedType( bool ) ) );

	mShowMyGroupsOnlyAction = new QAction( "Show My Lists Only", this );
	mShowMyGroupsOnlyAction->setCheckable( true );
	IniConfig & cfg = userConfig();
	cfg.pushSection( "HostSelector" );
	mShowMyGroupsOnlyAction->setChecked( cfg.readBool( "ShowMyGroupsOnly", false ) );
	cfg.popSection();

	mManageGroupsAction = new QAction( "Manage My Lists", this );
	mSaveGroupAction = new QAction( "Save Current List", this );

	connect( mHostGroupCombo, SIGNAL( currentChanged( const Record & ) ), SLOT( hostGroupChanged( const Record & ) ) );
	connect( mOptionsButton, SIGNAL( clicked() ), SLOT( showOptionsMenu() ) );
	connect( mShowMyGroupsOnlyAction, SIGNAL( toggled(bool) ), SLOT( setShowMyGroupsOnly(bool) ) );
	connect( mSaveGroupAction, SIGNAL( triggered() ), SLOT( saveHostGroup() ) );
	connect( mManageGroupsAction, SIGNAL( triggered() ), SLOT( manageHostLists() ) );

	mHostGroupCombo->setColumn( "name" );

	refreshHostGroups();

}

void HostServiceMatrixWidget::newService()
{
	bool okay;
	while( 1 ) {
		QString serviceName = QInputDialog::getText( this, "New Service", "Enter the name of the service", QLineEdit::Normal, QString(), &okay );
		if( !okay ) break;
		if( serviceName.isEmpty() ) {
			QMessageBox::warning( this, "Invalid Service Name", "Service name cannot be empty" );
			continue;
		}
		if( Service::recordByName( serviceName ).isRecord() ) {
			QMessageBox::warning( this, "Invalid Service Name", "Service '" + serviceName + "' already exists" );
			continue;
		}
		Service s;
		s.setService( serviceName );
		s.setEnabled( true );
		s.commit();
		break;
	}
}

void HostServiceMatrixWidget::saveHostGroup()
{
	QDialog * sld = new QDialog( this );
	Ui::SaveListDialogUI ui;
	ui.setupUi( sld );

	ui.mNameCombo->addItems( HostGroup::recordsByUser( User::currentUser() ).filter( "name", QRegExp( "^.+$" ) ).sorted( "name" ).names() );
	ui.mNameCombo->addItems( HostGroup::recordsByUser( User() ).filter( "name", QRegExp( "^.+$" ) ).sorted( "name" ).names() );

	HostGroup cur_hg(mHostGroupCombo->current());
	if( cur_hg.isRecord() ) {
		int idx = ui.mNameCombo->findText( cur_hg.name() );
		if( idx >= 0 )
			ui.mNameCombo->setCurrentIndex( idx );

		if( !cur_hg.user().isRecord() )
			ui.mGlobalCheck->setChecked( true );
	}

	if( sld->exec() ) {
		HostGroup hg = HostGroup::recordByNameAndUser( ui.mNameCombo->currentText(), ui.mGlobalCheck->isChecked() ? User() : User::currentUser() );
		if( hg.isRecord() &&
			QMessageBox::question( this, "Overwrite Host List?"
				,"This will overwrite the existing list: " + hg.name() + "\nAre you sure you want to continue?"
				, QMessageBox::Yes, QMessageBox::No ) != QMessageBox::Yes
			)
			return;

		//
		// Commit the host group
		hg.setName( ui.mNameCombo->currentText() );
		hg.setUser( User::currentUser() );
		hg.setPrivate_( !ui.mGlobalCheck->isChecked() );
		hg.commit();

		// Gather existing hostgroup items
		QMap<Host,HostGroupItem> exist;
		HostGroupItemList com;

		HostGroupItemList hgl = HostGroupItem::recordsByHostGroup( hg );
		foreach( HostGroupItem hgi, hgl )
			exist[hgi.host()] = hgi;

		// Commit the items
		HostList hl = hostList();
		foreach( Host h, hl ) {
			HostGroupItem hgi;
			if( exist.contains( h ) ) {
				hgi = exist[h];
				exist.remove( h );
			}
			hgi.setHostGroup(hg);
			hgi.setHost(h);
			com += hgi;
		}
		com.commit();

		// Delete the old ones
		HostGroupItemList toRemove;
		for( QMap<Host,HostGroupItem>::Iterator it = exist.begin(); it != exist.end(); ++it )
			toRemove += it.value();
		toRemove.remove();
	}
	refreshHostGroups();
}

void HostServiceMatrixWidget::manageHostLists()
{
	HostListsDialog * hld = new HostListsDialog( this );
	hld->show();
	hld->exec();
	delete hld;
}

HostList HostServiceMatrixWidget::hostList() const
{
	HostList hl = mView->getModel()->getRecords( ModelIter::collect( mView->getModel(), ModelIter::Selected, mView->selectionModel() ) );
	LOG_5("Found " + QString::number( hl.size() ) + " selected hosts" );
	return hl;
}

void HostServiceMatrixWidget::setShowMyGroupsOnly(bool toggle)
{
	IniConfig & cfg = userConfig();
	cfg.pushSection( "HostSelector" );
	cfg.writeBool( "ShowMyGroupsOnly", toggle );
	cfg.popSection();
	mShowMyGroupsOnlyAction->setChecked( toggle );
	refreshHostGroups();
}

void HostServiceMatrixWidget::showOptionsMenu()
{
	QMenu * menu = new QMenu(this);
	menu->addAction( mShowMyGroupsOnlyAction );
	menu->addSeparator();
	menu->addAction( mSaveGroupAction );
	menu->addAction( mManageGroupsAction );
	menu->exec( QCursor::pos() );
	delete menu;
}

void HostServiceMatrixWidget::refreshHostGroups()
{
	if( !mHostGroupRefreshPending ) {
		QTimer::singleShot( 0, this, SLOT( performHostGroupRefresh() ) );
		mHostGroupRefreshPending = true;
	}
}

void HostServiceMatrixWidget::performHostGroupRefresh()
{
	mHostGroupRefreshPending = false;

	// Generate the default list (everything)
	HostGroup hg;
	hg.setName( "Default List" );
	HostGroupList hostGroups = HostGroup::select( "fkeyusr=? or private is null or private=false", VarList() << User::currentUser().key() ).sorted( "name" );
	if( mShowMyGroupsOnlyAction->isChecked() )
		hostGroups = hostGroups.filter( "fkeyusr", User::currentUser().key() );
	hostGroups.insert(hostGroups.begin(), hg);
	mHostGroupCombo->setItems( hostGroups );
}

void HostServiceMatrixWidget::hostGroupChanged( const Record & hgr )
{
	HostServiceModel * temp = mView->getModel();
	if( hgr.isRecord() )
		temp->setRootList(HostGroup(hgr).hosts()); 
	else if (HostGroup(hgr).name() == "Default List")
		temp->setRootList(Host::select());
}

bool HostServiceMatrixWidget::eventFilter( QObject * o, QEvent * e )
{
	if( o == mHostFilterEdit && e->type() == QEvent::ContextMenu ) {
		QMenu * menu = mHostFilterEdit->createStandardContextMenu();
		QAction * first = menu->actions()[0];
		QAction * cs = new QAction( "Case Sensitive Filter", menu );
		cs->setCheckable( true );
		cs->setChecked( mView->hostFilterCS() );
		menu->insertAction( first, cs );
		menu->insertSeparator( first );
		if( menu->exec(((QContextMenuEvent*)e)->globalPos()) == cs )
			mView->setHostFilterCS( cs->isChecked() );
		delete menu;
		return true;
	}
	else if( o == mServiceFilterEdit && e->type() == QEvent::ContextMenu ) {
		QMenu * menu = mServiceFilterEdit->createStandardContextMenu();
		QAction * first = menu->actions()[0];
		QAction * cs = new QAction( "Case Sensitive Filter", menu );
		cs->setCheckable( true );
		cs->setChecked( mView->serviceFilterCS() );
		menu->insertAction( first, cs );
		menu->insertSeparator( first );
		if( menu->exec(((QContextMenuEvent*)e)->globalPos()) == cs )
			mView->setServiceFilterCS( cs->isChecked() );
		delete menu;
		return true;
	}
	return false;
}

HostServiceMatrixWindow::HostServiceMatrixWindow( QWidget * parent )
: QMainWindow( parent )
{
	setCentralWidget( new HostServiceMatrixWidget(this) );
	setWindowTitle( "Edit Host Services" );
	
	QMenu * fileMenu = menuBar()->addMenu( "&File" );
	fileMenu->addAction( "&New Service", centralWidget(), SLOT( newService() ) );
	fileMenu->addSeparator();
	fileMenu->addAction( "&Close", this, SLOT( close() ) );
}
