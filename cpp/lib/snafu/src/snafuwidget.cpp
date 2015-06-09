/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Snafu.
 *
 * Snafu is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Snafu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Snafu; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: snafuwidget.cpp 9407 2010-03-03 22:17:59Z brobison $
 */

#include <QtGui>
#include <QItemSelectionModel>
#include <QPixmapCache>
class QTreeWidgetItem;
class QLineEdit;
class QCursor;
class QContextMenuEvent;
class QPoint;
class QWidget;
class QTreeWidget;
class QFile;
class QProcess;
class QComboBox;
class QTimer;
class QItemSelectionModel;

#include "snafuwidget.h"
#include "syslogtablewidgetitem.h"
#include "snafuhttpthread.h"
#include "snafucmdthread.h"
#include "graphpagelist.h"
#include "freezercore.h"
#include "items.h"
#include "job.h"
#include "host.h"
#include "hoststatus.h"
#include "software.h"
#include "hostsoftware.h"
#include "service.h"
#include "hostservice.h"
#include "recordpropvaltree.h"
#include "permission.h"
#include "diskimage.h"

SnafuWidget::SnafuWidget(QWidget * parent)
: QWidget( parent )
,mGraphTabsInit(false)
,mShowAckd(false)
,mHostTaskRunning(false)
,mSelected()
,mCurrentTab(0)
{
	SnafuWidgetUI::setupUi(this);
	qApp->setWindowIcon(QIcon("images/icon.png"));
	setWindowIcon(QIcon("images/icon.png"));
	mFarmStatus.clear();
	mFarmStatusFilter.clear();
	mOS.clear();
	mOSFilter.clear();

/*
	Permission::table()->setCacheIncoming(true);
	Permission::select();
	Permission::table()->setCacheIncoming(false);
	HostSoftware::table()->setCacheIncoming(true);
	HostSoftware::select();
	HostSoftware::table()->setCacheIncoming(false);
	SysLog::table()->setCacheIncoming(true);
	SysLog::select("ack=0");
	SysLog::table()->setCacheIncoming(false);
	*/

	tabWidget->setCurrentIndex(0);
	mHostSelectWidget->setCurrentIndex(0);

	createActions();
  {
	  RecordSuperModel * hm = new RecordSuperModel(mHostTree);
	  new HostModel( hm->treeBuilder() );
	  hm->setAutoSort( true );
	  mHostTree->setModel( hm );

		setupHostView(mHostTree);
	}
	//refreshHostList();

	initHostTree();
	initEventTable();

	initGroupTree();
	buildGroupTree();

	initSoftwareTree();
	initAlertTab();
	initTicketTree();
	initLicenseTab();

	initBootOptions();

	mHttpTimer = new QTimer(this);
	connect(mHttpTimer, SIGNAL(timeout()), this, SLOT(checkHttpThreadSlot()));
	mHttpTimer->start(3000);

	mCmdTimer = new QTimer(this);
	connect(mCmdTimer, SIGNAL(timeout()), this, SLOT(checkCmdThreadSlot()));
	mCmdTimer->start(3000);

	mapTimer = new QTimer(this);
	//connect(mapTimer, SIGNAL(timeout()), this, SLOT(rotateMaps()));
	//connect(mMapRotateCheckBox, SIGNAL(stateChanged(int)), this, SLOT(mapRotateToggle(int)));
	
	//statusBar()->showMessage("Situation Normal...",10000);

  connect(mGroupTree, SIGNAL(itemSelectionChanged()), this, SLOT(buildHostTree()));
  connect(mFindEdit, SIGNAL(returnPressed()), this, SLOT(findHosts()));
	connect(mTextSelectButton, SIGNAL(clicked()), this, SLOT(buildHostTree()));
	connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(hostSelected()));
	connect(mAllButton, SIGNAL(clicked()), this, SLOT(selectAll()));
	connect(mNoneButton, SIGNAL(clicked()), this, SLOT(selectNone()));
	connect(mInverseButton, SIGNAL(clicked()), this, SLOT(selectInverse()));

	mGroupTree->setItemSelected( mGroupTree->topLevelItem(0), true );

	createAlertMenu();
	//initMapCombo();
	//mMapFrame->initSlots();

	mECM = 0;
	mHttpThreadCount = 0;
	mCmdThreadCount = 0;
	mFindHostRunning = false;

	connect( HostGroupItem::table(), SIGNAL( added( RecordList ) ), SLOT(hostsAddedToGroup( RecordList ) ) );

  QPixmap toCache = QPixmap(":/images/clear.png");
	QPixmapCache::insert("host-status-clear", toCache);
  toCache = QPixmap(":/images/warning.png");
	QPixmapCache::insert("host-status-warning", toCache);
  toCache = QPixmap(":/images/minor.png");
	QPixmapCache::insert("host-status-minor", toCache);
  toCache = QPixmap(":/images/major.png");
	QPixmapCache::insert("host-status-major", toCache);
  toCache = QPixmap(":/images/critical.png");
	QPixmapCache::insert("host-status-critical", toCache);
}

SnafuWidget::~SnafuWidget()
{
	saveHostView(mHostTree);
}

void SnafuWidget::refreshHostList()
{
	qWarning("refreshing HostListTask\n");
	if( mHostTaskRunning ) return;
	mHostTaskRunning = true;
	emit showStatusBarMessage( "Refreshing Host List..." );

	QString sql = "WHERE online=1";

	HostGroupList groupFilter;
	foreach( QTreeWidgetItem * item, mGroupTree->selectedItems() )
		groupFilter += mGroupByName[item->text(0)];

	if(groupFilter.size() > 0)
		sql += " AND keyhost IN ( SELECT hgi.fkeyhost FROM HostGroupItem hgi WHERE fkeyHostGroup IN (" + groupFilter.keyString() + ") )";

		//"WHERE keyhost IN ( SELECT hostservice.fkeyhost FROM HostService WHERE fkeyservice IN (" + mServiceFilter + ") ) AND online=1" );
		//

	mHostList = Host::select( sql );
	HostStatusList hostStatuses = HostStatus::select( "fkeyhost in (" + mHostList.keyString() + ")" );

	mHostTaskRunning = false;

	//SysLog::select("ack=0");

	//qWarning("running HostListTask\n");
	//FreezerCore::addTask( new HostListTask( this, "" ) );
	//FreezerCore::wakeup();
}

/*
void SnafuWidget::initMapCombo()
{
	mMaps = SnafuMapWidget::getMaps();
	QList<int> keys = mMaps.keys();
	foreach (int key, keys)
		mMapComboBox->addItem(mMaps[key]);

	//connect(mMapComboBox,SIGNAL(activated(int)), SnafuApp::get(), SLOT(setCurrentMap(int)));
	connect(mMapComboBox,SIGNAL(activated(const QString &)), mMapFrame, SLOT(setMap(const QString &)));
}

int SnafuWidget::getMap() const
{
	return mMapComboBox->currentIndex() + 1;
}
*/

void SnafuWidget::initHostTree()
{
	mHostTree->setContextMenuPolicy(Qt::CustomContextMenu);
	mHostTree->setDragEnabled(true);
  connect(mHostTree, SIGNAL(selectionChanged(RecordList)), this, SLOT(hostSelected()));
  connect(mHostTree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(hostContextMenu(const QPoint &)));
}

void SnafuWidget::hostContextMenu( const QPoint & point )
{
	Q_UNUSED(point);
	QAction * result = hostMenu->exec( QCursor::pos() );
	if (result)
	{
		//qWarning( result->text().toAscii() );
		if (result->data().toString() == "add_to_group")
			addToGroupSlot(result->text());
		else if (result->data().toString() == "farm_status_filter")
		{
			if (result->isChecked())
				mFarmStatusFilter[result->text()] = 1;
			else
				mFarmStatusFilter.remove(result->text());
			buildHostTree();
		}
		else if (result->data().toString() == "os_filter")
		{
			if (result->isChecked())
				mOSFilter[result->text()] = 1;
			else
				mOSFilter.remove(result->text());
			buildHostTree();
		}
		else if (result->text() == "Remove from Group")
		{
			removeFromGroupSlot();
			buildHostTree();
		}
	}
}

void SnafuWidget::initEventTable()
{
	QList<QString> labels;
	labels += "Hostname";
	labels += "Event Message";
	labels += "Count";
	labels += "Last Occurence";
	labels += "Class";
	labels += "Method";
	mEventTable->setSortingEnabled(true);
	mEventTable->setHeaderLabels(labels);
	mEventTable->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(mEventTable, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(eventContextMenu(const QPoint &)));
}

void SnafuWidget::eventContextMenu( const QPoint & point )
{
	Q_UNUSED(point);
	qWarning("event context requested " +QString::number(mECM).toAscii());
	mECM++;
	if (mECM > 1)
		return;

	createEventMenu();

	QAction * result = eventMenu->exec( QCursor::pos() );
	if (result)
	{
		//qWarning( result->text().toAscii() );
		if (result->data().toString() == "ack")
		{
			eventAckSlot();
			hostSelected();
		}
		else if (result->data().toString() == "class_filter")
		{
			if (result->isChecked())
				mClassFilter[result->text()] = 1;
			else
				mClassFilter.remove(result->text());

			hostSelected();
		}
	}
	eventMenu->hide();
	delete eventMenu;
	QTimer::singleShot(1000, this, SLOT(resetECM()));
}

void SnafuWidget::resetECM()
{
	mECM = 0;
}

void SnafuWidget::addToGroupSlot( const QString & groupName )
{
	HostList selected = selectedHosts();

	foreach ( Host host, selected )
	{
		mGroupByName[groupName].addHost(host);
		emit showStatusBarMessage("Added "+ host.name() +" to group "+groupName);
	}
}

void SnafuWidget::removeFromGroupSlot()
{
	HostList hosts = selectedHosts(false);
	foreach ( Host host, hosts )
	{
		foreach( QTreeWidgetItem * item, mGroupTree->selectedItems() )
		{
			QString groupName = item->text(0);
			mGroupByName[groupName].removeHost(host);
			emit showStatusBarMessage("Removed "+ host.name() +" from group "+groupName);
		}
	}
}

void SnafuWidget::farmSetSlotsSlot()
{
	HostList selected = selectedHosts();

	foreach (Host host, selected)
		addCmd("echo "+host.name()+" -1");
}

void SnafuWidget::farmSetSlotsOffSlot()
{
	HostList selected = selectedHosts();

	foreach (Host host, selected)
		addCmd("echo "+host.name()+" 0");
}

void SnafuWidget::farmAutoLoginEnableSlot()
{
	if(mEscalationPassword.isEmpty())
		getEscalationPassword();
	if(mEscalationPassword.isEmpty())
	{
		QMessageBox::information(this, "Snafu Error",
    "Unable to proceed without priv escalation password.");
		return;
	}

	HostList selected = selectedHosts();
	QStringList hostNames;

	foreach (Host host, selected)
	{
		hostNames += host.name();
	}
	addCmd("perl set_autologin.pl barryr "+mEscalationPassword+" enable "+hostNames.join(" "));
}

void SnafuWidget::farmAutoLoginDisableSlot()
{
	if(mEscalationPassword.isEmpty())
		getEscalationPassword();
	if(mEscalationPassword.isEmpty())
	{
		QMessageBox::information(this, "Snafu Error",
    "Unable to proceed without priv escalation password.");
		return;
	}

	HostList selected = selectedHosts();
	QStringList hostNames;

	foreach (Host host, selected)
		hostNames += host.name();

	addCmd("perl set_autologin.pl barryr "+mEscalationPassword+" disable "+hostNames.join(" "));
}

void SnafuWidget::getEscalationPassword()
{
	bool ok;
	QString title = "Enter password for " + getUserName();
  QString text = QInputDialog::getText(this, title,
                                             tr("Password:"), QLineEdit::Password,
                                             "", &ok);
  if (ok && !text.isEmpty())
		mEscalationPassword = text;
}

void SnafuWidget::farmRestartSlot()
{
	HostList selected = selectedHosts();
	if (selected.size() > 14 && QMessageBox::question(
				this,
				tr("Farm Restart Blades?"),
				tr("Are you sure you want to restart %1 blades?")
				.arg(QString::number(selected.size())),
				tr("&Yes"), tr("&No"),
				QString(), 0, 1))
		return;

	QString url("/mason/event_record?event=Slave%20Restart%20Requested&eventSeverity=2&eventClass=farm&eventMethod=restart&");
	foreach (Host host, selected)
	{
		addCmd("echo "+host.name());
		addHttpUrl(url+"host="+ host.name());
	}
}

void SnafuWidget::farmRebootSlot()
{
	HostList selected = selectedHosts();
	if (selected.size() > 14 && QMessageBox::question(
				this,
				tr("Farm Reboot Blades?"),
				tr("Are you sure you want to reboot %1 blades?")
				.arg(QString::number(selected.size())),
				tr("&Yes"), tr("&No"),
				QString(), 0, 1))
		return;

	QString url("/mason/event_record?event=Slave%20Reboot%20Requested&eventSeverity=3&eventClass=farm&eventMethod=reboot&");
	foreach (Host host, selected)
	{
		addCmd("echo "+host.name());
		QString tempUrl(url+"host="+ host.name());
		addHttpUrl(tempUrl);
	}
}

void SnafuWidget::xcatRebootSlot()
{
	HostList selected = selectedHosts();
	if (selected.size() > 14 && QMessageBox::question(
				this,
				tr("XCat Reboot Blades?"),
				tr("Are you sure you want to reboot %1 blades?")
				.arg(QString::number(selected.size())),
				tr("&Yes"), tr("&No"),
				QString(), 0, 1))
		return;

	QString url("/cgi-bin/rpower.cgi?hosts=");
	foreach (Host host, selected)
	{
		QString tempUrl(url + host.name());
		addHttpUrl(tempUrl);
	}
}

void SnafuWidget::xcatImageSlot()
{
	HostList selected = selectedHosts();

	if (QMessageBox::question(
				this,
				tr("XCat Image Blades?"),
				tr("Are you sure you want to image %1 blades?")
				.arg(QString::number(selected.size())),
				tr("&Yes"), tr("&No"),
				QString(), 0, 1))
		return;

	QString url("/cgi-bin/rinstall.cgi?hosts=");
	foreach (Host host, selected)
	{
		QString tempUrl(url + host.name());
		addHttpUrl(tempUrl);
	}
	farmRebootSlot();
}

void SnafuWidget::groupAddSlot()
{
	bool ok;
  QString text = QInputDialog::getText(this, tr("New Group name"),
                                             tr("Group name:"), QLineEdit::Normal,
                                             "", &ok);
  if (ok && !text.isEmpty())
	{
		HostGroup newGroup = HostGroup();
		newGroup.setName(text);
		newGroup.commit();
		buildGroupTree();
	}
}

void SnafuWidget::eventAckSlot()
{
	QList<QTreeWidgetItem*> selected = mEventTable->selectedItems();
	SysLogList sll;
	foreach (QTreeWidgetItem * item, selected)
		sll += ((SysLogTableWidgetItem*)item)->syslog();
	sll.setAcks(1);
	sll.commit();
}

void SnafuWidget::eventShowAckSlot()
{
	if (mShowAckd)
	{
		mShowAckd = false;
		if (mShowAckCheckBox->checkState() == Qt::Checked)
			mShowAckCheckBox->setCheckState(Qt::Unchecked);
	} else {
		mShowAckd = true;
		if (mShowAckCheckBox->checkState() == Qt::Unchecked)
			mShowAckCheckBox->setCheckState(Qt::Checked);
	}
	hostSelected();
}

void SnafuWidget::changeCommentSlot()
{
	bool ok;
  QString text = QInputDialog::getText(this, tr("Change Comment"),
                                             tr("Comment:"), QLineEdit::Normal,
                                             "", &ok);

  if (ok)
	{
		HostList selected = selectedHosts(true);
		selected.setDescriptions(text);
		selected.commit();

		buildHostTree();
	}
}

void SnafuWidget::findHostSlot()
{
	mFindEdit->setCursorPosition(mFindEdit->text().size());
	mFindEdit->selectAll();
	mFindEdit->setFocus(Qt::OtherFocusReason);
}

void SnafuWidget::findHosts()
{
	mFindHostRunning = true;
	QString text = mFindEdit->text();
	if( text.isEmpty() )
		return;

	emit showStatusBarMessage("Searching for hosts: "+text);
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	mHostTree->clearSelection();

	if(!text.endsWith("*"))
		text += "*";

	HostList found;
	foreach( Host h, mHostList )
		if( h.name().contains(QRegExp(text, Qt::CaseInsensitive, QRegExp::Wildcard)) )
			found += h;

	mHostTree->setSelection( found );

	if( found.size() > 0 )
	{
		QModelIndex mi = (mHostTree->selectionModel()->selectedIndexes())[0];
		mHostTree->scrollTo(mi);
	}
	
	mFindHostRunning = false;
	hostSelected();
	QApplication::restoreOverrideCursor();
	emit showStatusBarMessage("Found "+QString::number(found.size())+" matching hosts");
}

void SnafuWidget::vncSlot()
{
	HostList hl = selectedHosts();
	foreach( Host h, hl)
		vncHost(h.name());
}

void SnafuWidget::sshSlot()
{
	HostList hl = selectedHosts();
	foreach( Host h, hl)
			sshHost(h.name());
}

void SnafuWidget::telnetSlot()
{
	HostList hl = selectedHosts();
	foreach( Host h, hl)
		telnetHost(h.name());
}

void SnafuWidget::msrdpSlot()
{
	HostList hl = selectedHosts();
	foreach( Host h, hl)
		msrdpHost(h.name());
}

HostList SnafuWidget::selectedHosts(bool recursive) const
{
	return mHostTree->selection();
}

void SnafuWidget::hostSelected()
{
	if (mFindHostRunning)
		return;

	if( mSelected.size() && mCurrentTab == 1 ) {
		// save old stuff, only if already on details page
		saveSoftwareTree();
		saveServiceTree();
		mSelected.setDiskImages(
			DiskImage( 
				mDiskImageCombo->itemData( mDiskImageCombo->currentIndex() ).toUInt()
			)
		);
		mSelected.setBootActions(
			mBootActionCombo->itemText( mBootActionCombo->currentIndex() ) 
		);
		mSelected.commit();
	}
	mSelected = selectedHosts(true);

	if( mSelected.size() == 1 ) {
		emit showStatusBarMessage( mSelected[0].name() + " Selected" );
	} else if( mSelected.size() )
		emit showStatusBarMessage(QString::number( mSelected.size() ) + " Hosts Selected");
	else
		emit clearStatusBar();

	mCurrentTab = tabWidget->currentIndex();
	//LOG_3("showing tab "+QString::number(mCurrentTab));
	switch (mCurrentTab)
	{
		case 0:
			showEvents(mSelected);
			break;
		case 1:
			showDetails(mSelected);
			break;
		case 2:
			showAlerts(mSelected);
			break;
			/*
		case 4:
			showTickets(mSelected);
			break;
			*/
		case 6:
			showGraphs(mSelected);
			break;
		case 7:
			showLicenses(mSelected);
			break;
	}
}

void SnafuWidget::showEvents(HostList & hosts )
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	SysLogList events;
	foreach( Host host, hosts )
		events += SysLog::recordsByHostAndAck( host, mShowAckd ? 1 : 0);

	mEventTable->clear();
	initEventTable();
	foreach ( SysLog event, events ) 
	{
		if (!mClassFilter.isEmpty())
			if (!mClassFilter.contains(event._class()))
				continue;

		SysLogTableWidgetItem * newItem = new SysLogTableWidgetItem(mEventTable);
		newItem->setSysLog(event);

		newItem->setText(0, event.host().name());
		newItem->setText(1, event.message());
		newItem->setText(2, QString::number(event.count()));
		newItem->setText(3, event.lastOccurrence().toString("yyyy.MM.dd hh:mm:ss"));
		newItem->setText(4, event._class());
		mClass[event._class()]++;
		newItem->setText(5, event.method());
		/* TODO
		newItem->setBackgroundColor( 0, event.statusColor() );
		newItem->setBackgroundColor( 1, event.statusColor() );
		newItem->setBackgroundColor( 2, event.statusColor() );
		newItem->setBackgroundColor( 3, event.statusColor() );
		newItem->setBackgroundColor( 4, event.statusColor() );
		newItem->setBackgroundColor( 5, event.statusColor() );
		*/
	}
	mEventTable->sortItems(3, Qt::DescendingOrder);
	mEventTable->resizeColumnToContents(0);
	mEventTable->resizeColumnToContents(1);
	mEventTable->resizeColumnToContents(2);
	mEventTable->resizeColumnToContents(3);
	mEventTable->resizeColumnToContents(4);
	mEventTable->resizeColumnToContents(5);
	//	createEventMenu();
	QApplication::restoreOverrideCursor();
}

void SnafuWidget::showDetails( HostList hosts )
{
	mHostPropTree->setRecords(hosts);
	mHostPropTree->setEditable( true ); //User::hasPerms( "Host", true ) );
	mHostPropTree->model()->setMultiEdit( true );

	if( hosts.size() > 0) {
		//mHostInterface->setHost(hosts[0]);

		int baIndex = mBootActionCombo->findText( hosts[0].bootAction() );
		mBootActionCombo->setCurrentIndex(baIndex);

		int diIndex = mDiskImageCombo->findData( hosts[0].diskImage().key() );
		mDiskImageCombo->setCurrentIndex(diIndex);
	}
	buildSoftwareTree();
	buildServiceTree();
}

void SnafuWidget::showGraphs(HostList hosts)
{
	mGraphTabWidget->removeTab(0);

	if( hosts.size() > 0)
	{
		// for the first Host selected show it's graphs
		GraphPage hostPage = GraphPage::recordByname( hosts[0].name() );
		// if no page for this host exists yet, create it
		if( !hostPage.isRecord() )
		{
			GraphPage newPage = GraphPage();
			newPage.setName( hosts[0].name() );
			newPage.commit();
			hostPage = newPage;
		}
		QWidget * newWidget = new QWidget();
		int newIndex = mGraphTabWidget->insertTab(0, newWidget, hostPage.name());
		mGraphTabMap[newIndex] = hostPage;
		LOG_3(" adding graph page "+hostPage.name());
	}

	if(!mGraphTabsInit)
	{
		GraphPageList pages = GraphPage::select("fkeygraphpage = 0");
		foreach( GraphPage page, pages ) 
		{
			QWidget * newWidget = new QWidget();
			int newIndex = mGraphTabWidget->insertTab(9999, newWidget, page.name());
			mGraphTabMap[newIndex] = page;
			LOG_3(" adding graph page "+page.name());
		}
		mGraphTabsInit = true;
	}
}

void SnafuWidget::addGraph()
{
}

void SnafuWidget::initLicenseTab()
{
  QBoxLayout * hlayout = new QHBoxLayout( licenseTab );
	hlayout->setMargin( 0 );
	hlayout->setSpacing( 0 );

	mLicenseWidget = new LicenseWidget();
	hlayout->addWidget( mLicenseWidget );
}

void SnafuWidget::showLicenses(HostList hosts)
{
}

void SnafuWidget::initViewCombo()
{
}

void SnafuWidget::createEventMenu()
{
	eventMenu = new QMenu("Event");

	QAction * ackAction  = new QAction("Ack", this);
	ackAction->setData(QVariant("ack"));
	eventMenu->addAction(ackAction);

	eventMenu->addSeparator();
	eventMenu->addAction(eventAckAct);
	createClassMenu();
	eventMenu->addMenu(mClassMenu);
}

void SnafuWidget::createHostMenu()
{
	hostMenu = new QMenu("Host");
	hostMenu->addAction(vncAct);
	hostMenu->addAction(msrdpAct);
	hostMenu->addAction(remoteControlAct);
	hostMenu->addAction(sshAct);
	hostMenu->addAction(telnetAct);
	hostMenu->addAction(muninAct);
	hostMenu->addAction(changeCommentAct);

	hostMenu->addSeparator();
	farmActionMenu = new QMenu("Farm");
	farmActionMenu->addAction( farmSetSlotsAct );
	farmActionMenu->addAction( farmSetSlotsOffAct );
	farmActionMenu->addAction( farmRebootAct );
	farmActionMenu->addAction( farmRestartAct );
	hostMenu->addMenu(farmActionMenu);

	xcatMenu = new QMenu("XCat");
	xcatMenu->addAction( xcatRebootAct );
	xcatMenu->addAction( xcatImageAct );
	xcatMenu->addAction( farmAutoLoginEnableAct );
	xcatMenu->addAction( farmAutoLoginDisableAct );
	hostMenu->addMenu(xcatMenu);

	hostMenu->addSeparator();
	createFarmStatusMenu();
	hostMenu->addMenu(farmStatusMenu);
	createOSMenu();
	hostMenu->addMenu(osMenu);

	groupMenu = new QMenu("Add to Group");
	QList<QString> groups = mGroupByName.keys();
	qSort(groups);
	foreach ( QString group, groups )
	{
		QAction * groupAction  = new QAction(group, this);
		groupAction->setData(QVariant("add_to_group"));
		groupMenu->addAction(groupAction);
	}
	hostMenu->addSeparator();
	hostMenu->addAction("Remove from Group");
	hostMenu->addMenu(groupMenu);
}

void SnafuWidget::createFarmStatusMenu()
{
	farmStatusMenu = new QMenu("Farm Status");
	QList<QString> keys = mFarmStatus.keys();
	foreach (const QString & key, keys)
	{
		QAction * filterAction  = new QAction(key, this);
		filterAction->setData(QVariant("farm_status_filter"));
		filterAction->setCheckable(true);
		if (mFarmStatusFilter.contains(key))
			filterAction->setChecked(true);
		farmStatusMenu->addAction(filterAction);
	}
}

void SnafuWidget::createOSMenu()
{
	osMenu = new QMenu("OS");
	QList<QString> keys = mOS.keys();
	foreach (const QString & key, keys)
	{
		QAction * filterAction  = new QAction(key, this);
		filterAction->setData(QVariant("os_filter"));
		filterAction->setCheckable(true);
		if (mOSFilter.contains(key))
			filterAction->setChecked(true);
		osMenu->addAction(filterAction);
	}
}

void SnafuWidget::createClassMenu()
{
	mClassMenu = new QMenu("Event Class");
	QList<QString> keys = mClass.keys();
	foreach (const QString & key, keys)
	{
		QAction * filterAction  = new QAction(key, this);
		filterAction->setData(QVariant("class_filter"));
		filterAction->setCheckable(true);
		if (mClassFilter.contains(key))
			filterAction->setChecked(true);
		mClassMenu->addAction(filterAction);
	}
}

void SnafuWidget::createActions()
{
	refreshAct = new QAction(QIcon("images/refresh_all.png"), tr("&Refresh"), this);
  refreshAct->setShortcut(tr("Ctrl+R"));
  refreshAct->setStatusTip(tr("Refresh view"));
  connect(refreshAct, SIGNAL(triggered()), this, SLOT(refreshGroupTree()));
  connect(refreshAct, SIGNAL(triggered()), this, SLOT(buildHostTree()));

  findHostAct = new QAction(QIcon("images/find.bmp"), tr("&Find"), this);
  findHostAct->setShortcut(tr("Ctrl+F"));
  findHostAct->setStatusTip(tr("Find Host"));
  connect(findHostAct, SIGNAL(triggered()), this, SLOT(findHostSlot()));

  groupAddAct = new QAction(QIcon("images/groupadd.png"), tr("&New Group"), this);
  groupAddAct->setShortcut(tr("Ctrl+N"));
  groupAddAct->setStatusTip(tr("Add New Group"));
  connect(groupAddAct, SIGNAL(triggered()), this, SLOT(groupAddSlot()));

  vncAct = new QAction(QIcon("images/vnc.bmp"), tr("&VNC"), this);
  vncAct->setStatusTip(tr("VNC to Host"));
	vncAct->setData(QVariant("connect_to_host"));
  connect(vncAct, SIGNAL(triggered()), this, SLOT(vncSlot()));

  sshAct = new QAction(QIcon("images/ssh.bmp"), tr("&SSH"), this);
  sshAct->setStatusTip(tr("SSH to Host"));
	sshAct->setData(QVariant("connect_to_host"));
  connect(sshAct, SIGNAL(triggered()), this, SLOT(sshSlot()));

  telnetAct = new QAction(QIcon("images/telnet.bmp"), tr("&Telnet"), this);
  telnetAct->setStatusTip(tr("Telnet to Host"));
	telnetAct->setData(QVariant("connect_to_host"));
  connect(telnetAct, SIGNAL(triggered()), this, SLOT(telnetSlot()));

  msrdpAct = new QAction(QIcon("images/msrdp.bmp"), tr("&MS RDP"), this);
  msrdpAct->setStatusTip(tr("MS Terminal Service"));
	msrdpAct->setData(QVariant("connect_to_host"));
  connect(msrdpAct, SIGNAL(triggered()), this, SLOT(msrdpSlot()));

  remoteControlAct = new QAction(QIcon("images/msrdp.bmp"), tr("&Remote Control"), this);
  remoteControlAct->setStatusTip(tr("IBM Remote Control"));
	remoteControlAct->setData(QVariant("connect_to_host"));
  connect(remoteControlAct, SIGNAL(triggered()), this, SLOT(launchRemoteControl()));

  muninAct = new QAction(QIcon("images/find.png"), tr("&Graphs"), this);
  muninAct->setStatusTip(tr("VNC to Host"));
  connect(muninAct, SIGNAL(triggered()), mFindEdit, SLOT(selectAll()));

  farmSetSlotsAct = new QAction(QIcon("images/.bmp"), tr("Set Slots On"), this);
  farmSetSlotsAct->setStatusTip(tr("Set Slots On"));
	farmSetSlotsAct->setData(QVariant("farmAction"));
  connect(farmSetSlotsAct, SIGNAL(triggered()), this, SLOT(farmSetSlotsSlot()));

  farmSetSlotsOffAct = new QAction(QIcon("images/.bmp"), tr("Set Slots Off"), this);
  farmSetSlotsOffAct->setStatusTip(tr("Set Slots Off"));
	farmSetSlotsOffAct->setData(QVariant("farmAction"));
  connect(farmSetSlotsOffAct, SIGNAL(triggered()), this, SLOT(farmSetSlotsOffSlot()));

  farmRestartAct = new QAction(QIcon("images/host_restart.png"), tr("Restart"), this);
  farmRestartAct->setStatusTip(tr("Restart Slave"));
	farmRestartAct->setData(QVariant("farmAction"));
  connect(farmRestartAct, SIGNAL(triggered()), this, SLOT(farmRestartSlot()));

  farmRebootAct = new QAction(QIcon("images/host_restart.png"), tr("Reboot"), this);
  farmRebootAct->setStatusTip(tr("Reboot Slave"));
	farmRebootAct->setData(QVariant("farmAction"));
  connect(farmRebootAct, SIGNAL(triggered()), this, SLOT(farmRebootSlot()));

  farmAutoLoginEnableAct = new QAction(QIcon("images/xcat.bmp"), tr("Enable Autologin"), this);
  farmAutoLoginEnableAct->setStatusTip(tr("Enable Autologin"));
	farmAutoLoginEnableAct->setData(QVariant("xcat"));
  connect(farmAutoLoginEnableAct, SIGNAL(triggered()), this, SLOT(farmAutoLoginEnableSlot()));

  farmAutoLoginDisableAct = new QAction(QIcon("images/xcat.bmp"), tr("Disable Autologin"), this);
  farmAutoLoginDisableAct->setStatusTip(tr("Disable Autologin"));
	farmAutoLoginDisableAct->setData(QVariant("xcat"));
  connect(farmAutoLoginDisableAct, SIGNAL(triggered()), this, SLOT(farmAutoLoginDisableSlot()));

  xcatRebootAct = new QAction(QIcon("images/xcat.bmp"), tr("Reboot"), this);
  xcatRebootAct->setStatusTip(tr("Reboot Host"));
	xcatRebootAct->setData(QVariant("xcat"));
  connect(xcatRebootAct, SIGNAL(triggered()), this, SLOT(xcatRebootSlot()));

  xcatImageAct = new QAction(QIcon("images/xcat.bmp"), tr("Image"), this);
  xcatImageAct->setStatusTip(tr("Image Host"));
	xcatImageAct->setData(QVariant("xcat"));
  connect(xcatImageAct, SIGNAL(triggered()), this, SLOT(xcatImageSlot()));

  changeCommentAct = new QAction(QIcon("images/foo.bmp"), tr("&Comment"), this);
  changeCommentAct->setStatusTip(tr("Change Comment"));
	changeCommentAct->setData(QVariant("comment"));
  connect(changeCommentAct, SIGNAL(triggered()), this, SLOT(changeCommentSlot()));

	eventAckAct = new QAction(QIcon("images/foo.bmp"),tr("&Show Ack'd"), this);
	eventAckAct->setData(QVariant("show-ack"));
	eventAckAct->setCheckable(true);
  connect(eventAckAct, SIGNAL(triggered()), this, SLOT(eventShowAckSlot()));
  connect(mShowAckCheckBox, SIGNAL(stateChanged(int)), eventAckAct, SLOT(toggle()));
}

void SnafuWidget::selectAll()
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	mFindHostRunning = true;

	mFindHostRunning = false;
	hostSelected();
	QApplication::restoreOverrideCursor();
}

void SnafuWidget::selectNone()
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	mFindHostRunning = true;

	mFindHostRunning = false;
	hostSelected();
	QApplication::restoreOverrideCursor();
}

void SnafuWidget::selectInverse()
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	mFindHostRunning = true;

	mFindHostRunning = false;
	hostSelected();
	QApplication::restoreOverrideCursor();
}

void SnafuWidget::buildHostTree()
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	refreshHostList();
	// if something is already selected, we want to go back to that
	RecordList sel = mHostTree->selection();
	mHostTree->model()->setRootList( mHostList );
	mHostTree->setSelection( sel );

	switch (mHostSelectWidget->currentIndex()) {
		case 0:
			buildHostTreeGroup();
			break;
		case 1:
			buildHostTreeText();
			break;
		case 2:
			buildHostTreeSoftware();
			break;
	}

	createHostMenu();
/*
	mHostTree->resizeColumnToContents(0);
	mHostTree->resizeColumnToContents(1);
	mHostTree->resizeColumnToContents(2);
	mHostTree->resizeColumnToContents(3);

	// try to keep the user's view consistent
	QList<QTreeWidgetItem *> found = mHostTree->findItems( "", Qt::MatchStartsWith | Qt::MatchWrap | Qt::MatchRecursive, 0 );
	*/

	QApplication::restoreOverrideCursor();
}

void SnafuWidget::buildHostTreeGroup()
{
/*
	int sortCol = mHostTree->sortColumn();
	QList<QString> selectedGroups;
	QList<QTreeWidgetItem*> items = mGroupTree->selectedItems();
	foreach ( QTreeWidgetItem * item, items )
		selectedGroups += item->text(0);

	mHostTree->clear();
	foreach (QString key, selectedGroups)
	{
		HostGroup group = mGroupByName[key];
		SnafuHostItem *groupItem = new SnafuHostItem(mHostTree);
		groupItem->setText(0, group.name());

		HostList hosts = group.hosts();

		foreach (Host host, hosts)
		{
			if (!mFarmStatusFilter.isEmpty())
				if (!mFarmStatusFilter.contains(host.hostStatus().slaveStatus()))
					continue;

			if (!mOSFilter.isEmpty())
				if (!mOSFilter.contains(host.os()))
					continue;

			SnafuHostItem *hostItem = new SnafuHostItem(groupItem);
			hostItem->setHost(host);

			mFarmStatus[host.hostStatus().slaveStatus()] = 1;
			mOS[host.os()] = 1;
		}
		mHostTree->setItemExpanded(groupItem, true);
	}
	mHostTree->sortItems(sortCol, Qt::AscendingOrder);
*/
}

void SnafuWidget::buildHostTreeText()
{
	/*
	QStringList hostNames = mHostTextEdit->toPlainText().split(QRegExp("\\s+"));

	mHostTree->clear();
	SnafuHostItem *rootItem = new SnafuHostItem(mHostTree);
	rootItem->setText(0, "Selected Text");
	foreach (QString key, hostNames)
	{
		Host host = Host::recordByName(key);
		if (host.name().isEmpty())
			continue;

		if (!mFarmStatusFilter.isEmpty())
			if (!mFarmStatusFilter.contains(host.hostStatus().slaveStatus()))
				continue;

		if (!mOSFilter.isEmpty())
			if (!mOSFilter.contains(host.os()))
				continue;

		SnafuHostItem *hostItem = new SnafuHostItem(rootItem);
		hostItem->setHost(host);

		mFarmStatus[host.hostStatus().slaveStatus()] = 1;
		mOS[host.os()] = 1;
	}
	mHostTree->setItemExpanded(rootItem, true);
	*/
}

void SnafuWidget::buildHostTreeSoftware()
{
/*
	QTreeWidgetItem * item = static_cast<QTreeWidgetItem*>(mSoftwareTree->currentItem());
	
	//if (item == 0);
	//	return;

	QString software = item->text(0);
	int timeLimit = softwareTimeLimit();
	QList<int> hostIds; // = SysLog::retrieve_host_software(software, timeLimit);

	mHostTree->clear();
	SnafuHostItem *rootItem = new SnafuHostItem(mHostTree);
	rootItem->setText(0, software);

	HostList hosts;
	if (mSoftwareComboBox->currentIndex() == 0)
	{
		foreach (int key, hostIds)
		{
			Host & host = Host::recordBykey(key);
			if (host.host.isEmpty() > 0)
				continue;

			hosts.append(host);
		}
	}
	else if (mSoftwareComboBox->currentIndex() == 1)
	{
		QList<Host> allHosts = Host::select();
		foreach (Host host, allHosts)
		{
			if (!hostIds.contains(host.id))
				hosts.append(host);
		}
	}

	foreach (Host host, hosts)
	{
		if (!mFarmStatusFilter.isEmpty())
			if (!mFarmStatusFilter.contains(host.farmStatus))
				continue;

		if (!mOSFilter.isEmpty())
			if (!mOSFilter.contains(host.osname))
				continue;

		SnafuHostItem *hostItem = new SnafuHostItem(rootItem);
		hostItem->setHost(host);

		mFarmStatus[host.farmStatus] = 1;
		mOS[host.osname] = 1;
	}
	mHostTree->setItemExpanded(rootItem, true);
	*/
}

void SnafuWidget::hostsAddedToGroup(RecordList added)
{
	foreach( Record rec, added )
	{
	//	HostGroupItem hgi = (HostGroupItem*)rec;
	}
	buildHostTree();
}

void SnafuWidget::initGroupTree()
{
	mGroupTree->setRootIsDecorated(true);
	mGroupTree->setColumnCount(2);
	QList<QString> labels;
	labels += "Group";
	labels += "Hosts";
	mGroupTree->setHeaderLabels(labels);
}

void SnafuWidget::buildGroupTree()
{
	mGroupTree->clear();
	mGroupByName.clear();
	HostGroupList groups = HostGroup::select();

	foreach (HostGroup group, groups) {
		//HostGroup group(*it);
		mGroupByName[group.name()] = group;
		QTreeWidgetItem *groupItem = new QTreeWidgetItem(mGroupTree);
		groupItem->setIcon(0, QIcon(SnafuWidget::statusPixmap(group.status())));
		groupItem->setText(0, group.name());
		groupItem->setText(1, "("+ QString::number(group.hosts().count()) +")");
	}

	mGroupTree->sortItems(0, Qt::AscendingOrder);
	mGroupTree->resizeColumnToContents(0);
	mGroupTree->resizeColumnToContents(1);
}

void SnafuWidget::refreshGroupTree()
{
	QList<QTreeWidgetItem*> all = mGroupTree->findItems( "", Qt::MatchStartsWith | Qt::MatchWrap | Qt::MatchRecursive, 0 );
	foreach (QTreeWidgetItem * groupItem, all) 
	{
		HostGroup & group = mGroupByName[groupItem->text(0)];
		groupItem->setIcon(0, QIcon(SnafuWidget::statusPixmap(group.status())));
		groupItem->setText(1, "("+ QString::number(group.hosts().count()) +")");
	}
}

void SnafuWidget::initSoftwareSelectTree()
{
	mSoftwareComboBox->clear();
	mSoftwareComboBox->addItem("Is Installed");
	mSoftwareComboBox->addItem("Is Not Installed");
	mSoftwareComboBox->addItem("Had Error Installing");

	mSoftwareLimitComboBox->clear();
	mSoftwareLimitComboBox->addItem("Always");
	mSoftwareLimitComboBox->addItem("Last 90 Days");
	mSoftwareLimitComboBox->addItem("Last 30 Days");
	mSoftwareLimitComboBox->addItem("Last 10 Days");
	mSoftwareLimitComboBox->addItem("Last 1 Day");
	connect(mSoftwareTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(buildHostTree()));
	connect(mSoftwareLimitComboBox, SIGNAL(activated(int)), this, SLOT(buildSoftwareTree()));
	connect(mSoftwareComboBox, SIGNAL(activated(int)), this, SLOT(buildHostTree()));
}

void SnafuWidget::initSoftwareTree()
{
/*
	mSoftwareTree->setColumnCount(3);
	QList<QString> labels;
	labels += " ";
	labels += "Name";
	labels += "Version";
	mSoftwareTree->setHeaderLabels(labels);
	*/
}

int SnafuWidget::softwareTimeLimit()
{
	int timeLimit = 0;
	switch (mSoftwareLimitComboBox->currentIndex()) {
		case 0:
			timeLimit = 0;
			break;
		case 1:
			timeLimit = 7776000;
			break;
		case 2:
			timeLimit = 2592000;
			break;
		case 3:
			timeLimit = 864000;
			break;
		case 4:
			timeLimit = 86400;
			break;
	}
	return timeLimit;
}

void SnafuWidget::buildSoftwareTree()
{
	mSoftwareTree->setRootElement();

  QMap<uint, uint> depMap;
  foreach( Host h, mSelected )
  {
		SoftwareList installed = h.hostSoftwares().softwares();
    foreach( Software s, installed )
    {
      uint key = s.key();
      if( !depMap.contains( key ) )
        depMap[key] = 1;
      else
        depMap[key]++;
    }
  }

  SoftwareList checked, tri;
  for( QMap<uint, uint>::Iterator it = depMap.begin(); it != depMap.end(); ++it ){
    if( it.value() == mSelected.size() )
      checked += Software( it.key() );
    else
      tri += Software( it.key() );
  }

  mSoftwareTree->setChecked( checked );
  mSoftwareTree->setNoChange( tri );

	mSoftwareTree->resizeColumnToContents(0);
	mSoftwareTree->resizeColumnToContents(1);
	mSoftwareTree->resizeColumnToContents(2);
}

void SnafuWidget::saveSoftwareTree()
{
	qWarning("saving software tree\n");
	SoftwareList on = mSoftwareTree->checkedElements(); 
	SoftwareList nc = mSoftwareTree->noChangeElements();
	foreach( Host h, mSelected )
	{
		qWarning("checking "+h.name().toAscii());
		HostSoftwareList installed = h.hostSoftwares();
		HostSoftwareList toRemove;
		foreach( HostSoftware hs, installed )
			if( !on.contains( hs.software() ) && !nc.contains( hs.software() ) )
				toRemove += hs;
		toRemove.remove();
				
		foreach( Software s, on ) {
			if( !installed.softwares().contains( s ) ) {
				HostSoftware hs = HostSoftware();
				hs.setHost(h);
				hs.setSoftware(s);
				hs.commit();
			}
		}
	}
}

void SnafuWidget::buildServiceTree()
{
	mServiceTree->setRootElement();

  QMap<uint, uint> depMap;
  foreach( Host h, mSelected )
  {
		ServiceList installed = h.hostServices().services();
    foreach( Service s, installed )
    {
      uint key = s.key();
      if( !depMap.contains( key ) )
        depMap[key] = 1;
      else
        depMap[key]++;
    }
  }

  ServiceList checked, tri;
  for( QMap<uint, uint>::Iterator it = depMap.begin(); it != depMap.end(); ++it ){
    if( it.value() == mSelected.size() )
      checked += Service( it.key() );
    else
      tri += Service( it.key() );
  }

  mServiceTree->setChecked( checked );
  mServiceTree->setNoChange( tri );

	mServiceTree->resizeColumnToContents(0);
	mServiceTree->resizeColumnToContents(1);
	mServiceTree->resizeColumnToContents(2);
}

void SnafuWidget::saveServiceTree()
{
	qWarning("saving service tree\n");
	ServiceList on = mServiceTree->checkedElements(); 
	ServiceList nc = mServiceTree->noChangeElements();
	foreach( Host h, mSelected )
	{
		qWarning("checking "+h.name().toAscii());
		HostServiceList installed = h.hostServices();
		HostServiceList toRemove;
		foreach( HostService hs, installed )
			if( !on.contains( hs.service() ) && !nc.contains( hs.service() ) )
				toRemove += hs;
		toRemove.remove();
				
		foreach( Service s, on ) {
			if( !installed.services().contains( s ) ) {
				HostService hs = HostService();
				hs.setHost(h);
				hs.setService(s);
				hs.commit();
			}
		}
	}
}

void SnafuWidget::vncHost(const QString & hostname)
{
        const char * VNC_LINK =
                "[connection]\nhost=%1\nport=5900\npassword=ecada947e132f211\n[options]\nuse_encoding_0=1\nuse_encoding_1=1\n"
                "use_encoding_2=1\nuse_encoding_3=0\nuse_encoding_4=1\nuse_encoding_5=1\nuse_encoding_6=1\nuse_encoding_7=1\n"
                "use_encoding_8=1\npreferred_encoding=7\nrestricted=0\nviewonly=0\nfullscreen=0\n8bit=0\nshared=1\nswapmouse=0\n"
                "belldeiconify=0\nemulate3=1\nemulate3timeout=100\nemulate3fuzz=4\ndisableclipboard=0\nlocalcursor=1\nscale_den=1\n"
                "scale_num=1\ncursorshape=1\nnoremotecursor=0\ncompresslevel=8\nquality=0\n";

#ifdef Q_WS_WIN
        QFile vncFile("c:\\temp\\"+hostname+".vnc");
        vncFile.open(QIODevice::WriteOnly);
        {
                QTextStream out( &vncFile );
                out << QString( VNC_LINK ).arg( hostname );
        }
        vncFile.close();
	QProcess::startDetached("cmd /c start c:\\temp\\"+hostname+".vnc");
#endif 
}

void SnafuWidget::msrdpHost(const QString & hostname)
{
#ifdef Q_WS_WIN
	QProcess::startDetached("mstsc /v:"+hostname);
#endif 
}

void SnafuWidget::sshHost(const QString & hostname)
{
#ifdef Q_WS_WIN
	QProcess::startDetached("putty.exe -ssh "+hostname);
#endif 
}

void SnafuWidget::telnetHost(const QString & hostname)
{
#ifdef Q_WS_WIN
	QProcess::startDetached("putty.exe -telnet "+hostname);
#endif 
}

void SnafuWidget::launchRemoteControl()
{
	HostList selected = selectedHosts();
	QMap<QString, bool> selectedChassis;
	
	foreach (Host host, selected)
	{
		QString chassis = host.name();
		chassis.remove(4,3);
		selectedChassis[chassis] = true;
	}
	foreach (QString chassis, selectedChassis.keys())
	{
		QString url("cmd /c start http://"+chassis+"m01/");
		qWarning(url.toAscii());
		QProcess::startDetached(url);
	}
}

void SnafuWidget::initAlertTab()
{
	QList<QString> labels;
	labels += "Data Source";
	labels += "Variable";
	labels += "Threshold";
	labels += "Type";
	labels += "Minutes";
	labels += "Direction";
	labels += "Severity";
	mAlertTree->setHeaderLabels(labels);

	QList<QString> labels2;
	labels2 += "Data Source";
	mDataSourceTree->setHeaderLabels(labels2);

	connect(mAlertButton, SIGNAL(clicked()), this, SLOT(addAlert()));
	mAlertTree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(mAlertTree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(alertContextMenu(const QPoint &)));
}

void SnafuWidget::addAlert()
{
	HostList hosts = selectedHosts();
	if (hosts.count() == 0)
		return;

	// first verify all fields
	QTreeWidgetItem * item = mDataSourceTree->currentItem();
	if (!item)
		return;
	if (item->childCount() > 0)
		return;
	
	Host host = hosts[0];
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	// if all is ok then do insert
	
	/* TODO
	ArkEventAlert alert;
	alert.mHost =  host.id;
	alert.mGraphDS = item->parent()->text(0);
	alert.mVarName = item->text(0);
	alert.mSampleType = mAlertSampleCombo->currentText();
	alert.mSamplePeriod = mAlertSampleEdit->text().toInt();
	alert.mSampleDirection = mAlertDirectionCombo->currentText();
	alert.mSampleValue = mAlertValueEdit->text().toDouble();
	alert.mSeverity = mAlertSeverityCombo->currentIndex() + 1;
	ArkEventAlert::addAlert(alert);
	*/

	hostSelected();
	QApplication::restoreOverrideCursor();
}

void SnafuWidget::showAlerts( HostList hosts )
{
	if (hosts.count() == 0)
		return;
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	Host host = hosts[0];
	if (hosts.count() > 1)
		//statusBar()->showMessage("Limiting to first selected host...",1000);

	// TODO show alerts
	mAlertTree->clear();
	//mAlertMap.clear();
	/* TODO
	QList<ArkEventAlert> alerts = ArkEventAlert::retrieve_alerts(host.id);
	foreach (ArkEventAlert alert, alerts)
	{
		QTreeWidgetItem * item = new QTreeWidgetItem(mAlertTree);
		mAlertMap[item] = alert;
		item->setText(0, alert.mGraphDS);
		item->setText(1, alert.mVarName);
		item->setText(2, QString::number(alert.mSampleValue));
		item->setText(3, alert.mSampleType);
		item->setText(4, QString::number(alert.mSamplePeriod));
		item->setText(5, alert.mSampleDirection);
		item->setText(6, QString::number(alert.mSeverity));
	}
	*/

	mDataSourceTree->clear();
	QMap< QString, QList<QString> > sources; // = ArkEvent::retrieve_datasources(host.host);
	QList<QString> keys = sources.keys();
	foreach ( QString key, keys ) 
	{
		QTreeWidgetItem * item = new QTreeWidgetItem(mDataSourceTree);
		item->setText(0, key);

		QList<QString> varnames = sources[key];
		foreach (QString varname, varnames)
		{
			QTreeWidgetItem * subItem = new QTreeWidgetItem(item);
			subItem->setText(0, varname);
		}
	}
	mDataSourceTree->sortItems(0, Qt::AscendingOrder);
	QApplication::restoreOverrideCursor();
}

void SnafuWidget::createAlertMenu()
{
	alertMenu = new QMenu("Alert");
	alertMenu->addAction("Remove");
	alertMenu->addAction("Edit");
}

void SnafuWidget::alertContextMenu( const QPoint & point )
{
	Q_UNUSED(point);
	QAction * result = alertMenu->exec( QCursor::pos() );
	if (result)
	{
		if (result->text() == "Remove")
		{
			QTreeWidgetItem * item = mAlertTree->currentItem();
			//mAlertMap[item].sqlDelete();
		}
		if (result->text() == "Edit")
		{
			QTreeWidgetItem * item = mAlertTree->currentItem();
			bool ok;
			double text = QInputDialog::getDouble(this, tr("Change Threshold"),
																								 tr("Threshold:"),
																								 item->text(2).toDouble(), -2147483647, 2147483647, 2, &ok);

			if (ok)
			{
				//mAlertMap[item].setThreshold(text);
			}
		}
		hostSelected();
	}
}

void SnafuWidget::initTicketTree()
{
	mTicketTree->setRootIsDecorated(false);
	mTicketTree->setColumnCount(6);
	QList<QString> labels;
	labels += "TicketId";
	labels += "Host";
	labels += "Priority";
	labels += "Status";
	labels += "Subject";
	labels += "Updated";
	mTicketTree->setHeaderLabels(labels);

	mTicketStatus["Open"] = 1;
	mTicketStatus["Resolved"] = 1;
	mTicketStatus["Closed"] = 1;
	mTicketStatus["Pending"] = 1;
	mTicketStatus["Cancelled"] = 1;
	mTicketStatus["In Progress"] = 1;

	mTicketStatusFilter["Open"] = 1;
	mTicketStatusFilter["Pending"] = 1;
	mTicketStatusFilter["In Progress"] = 1;

	mTicketPriority["Project"] = 1;
	mTicketPriority["Low"] = 1;
	mTicketPriority["Medium"] = 1;
	mTicketPriority["High"] = 1;
	mTicketPriority["Urgent"] = 1;

	mTicketTree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(mTicketTree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ticketContextMenu(const QPoint &)));
	connect(mTicketTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(launchTicket(QTreeWidgetItem*)));
}

void SnafuWidget::showTickets( HostList hosts )
{
/* TODO
	if (hosts.count() == 0)
		return;
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	TrackerList tickets; 
	foreach( Host tmpHost, hosts )
		tickets += Tracker::recordsByHost(tmpHost);

	mTicketTree->clear();
	foreach ( Tracker ticket, tickets ) 
	{
		if (!mTicketPriorityFilter.isEmpty())
			if (!mTicketPriorityFilter.contains(ticket.mPriority))
				continue;
		if (!mTicketStatusFilter.isEmpty())
			if (!mTicketStatusFilter.contains(ticket.mStatus))
				continue;

		QTreeWidgetItem * item = new QTreeWidgetItem(mTicketTree);
		item->setText(0, QString::number(ticket.mTicketId));
		item->setText(1, Host::recordBykey((ticket.mHost).host ));
		item->setText(2, ticket.mPriority);
		item->setText(3, ticket.mStatus);
		item->setText(4, ticket.mSubject);
		item->setText(5, ticket.mLastUpdated);
	}
	mTicketTree->resizeColumnToContents(0);
	mTicketTree->resizeColumnToContents(1);
	mTicketTree->resizeColumnToContents(2);
	mTicketTree->resizeColumnToContents(3);
	mTicketTree->resizeColumnToContents(4);
	mTicketTree->resizeColumnToContents(5);
	mTicketTree->sortItems(5, Qt::DescendingOrder);
	createTicketMenu();
	QApplication::restoreOverrideCursor();
	*/
}

void SnafuWidget::ticketContextMenu( const QPoint & point )
{
	Q_UNUSED(point);
	QAction * result = ticketMenu->exec( QCursor::pos() );
	if (result)
	{
		if (result->data().toString() == "ticket_priority_filter")
		{
			if (result->isChecked())
				mTicketPriorityFilter[result->text()] = 1;
			else
				mTicketPriorityFilter.remove(result->text());

			buildHostTree();
		}
		if (result->data().toString() == "ticket_status_filter")
		{
			if (result->isChecked())
				mTicketStatusFilter[result->text()] = 1;
			else
				mTicketStatusFilter.remove(result->text());
			buildHostTree();
		}
		if (result->text() == "Open")
		{
			launchTicket(mTicketTree->currentItem());
		}
	}
}

void SnafuWidget::createTicketMenu()
{
	ticketMenu = new QMenu("Ticket");
	ticketMenu->addAction("Open");

	ticketMenu->addSeparator();
	createTicketPriorityMenu();
	ticketMenu->addMenu(ticketPriorityMenu);
	createTicketStatusMenu();
	ticketMenu->addMenu(ticketStatusMenu);
}

void SnafuWidget::createTicketPriorityMenu()
{
	ticketPriorityMenu = new QMenu("Ticket Priority");
	QList<QString> keys = mTicketPriority.keys();
	foreach (const QString & key, keys)
	{
		QAction * filterAction  = new QAction(key, this);
		filterAction->setData(QVariant("ticket_priority_filter"));
		filterAction->setCheckable(true);
		if (mTicketPriorityFilter.contains(key))
			filterAction->setChecked(true);
		ticketPriorityMenu->addAction(filterAction);
	}
}

void SnafuWidget::createTicketStatusMenu()
{
	ticketStatusMenu = new QMenu("Ticket Status");
	QList<QString> keys = mTicketStatus.keys();
	foreach (const QString & key, keys)
	{
		QAction * filterAction  = new QAction(key, this);
		filterAction->setData(QVariant("ticket_status_filter"));
		filterAction->setCheckable(true);
		if (mTicketStatusFilter.contains(key))
			filterAction->setChecked(true);
		ticketStatusMenu->addAction(filterAction);
	}
}

void SnafuWidget::launchTicket(QTreeWidgetItem * item)
{
	QString jobId(item->text(0));
	QString url("cmd /c start http://helpdesk.al.com.au:8081/helpdesk/WebObjects/Helpdesk.woa/wa/ticket?ticketId="+jobId);
	qWarning(url.toAscii());
	QProcess::startDetached(url);
}

/*
void SnafuWidget::rotateMaps()
{
	int current = mMapComboBox->currentIndex();
	if (current == mMapComboBox->count()-1)
		current = 0;
	else
		current++;

	mMapComboBox->setCurrentIndex(current);
	mMapFrame->setMap(mMapComboBox->currentText());
	mapTimer->start(5000);
}

void SnafuWidget::mapRotateToggle(int checked)
{
	if( checked == Qt::Checked )
		mapTimer->start(5000);
	else
		mapTimer->stop();
}
*/

QString SnafuWidget::timeCode(int seconds)
{
		int hr = int(seconds / 3600);
		seconds -= hr * 3600;

		int mn = int(seconds / 60);
		seconds -= mn * 60;

		QString ret;
		return ret.sprintf("%02d:%02d:%02d", hr, mn, seconds);
}

QString SnafuWidget::getUserName()
{
	QString ret;
	QStringList env = QProcess::systemEnvironment();
	foreach( QString pair, env )
	{
		QStringList keyvalue = pair.split("=");
		if( keyvalue[0] == "USERNAME" )
			ret = keyvalue[1];
	}
	return ret;
}

// http thread pool
void SnafuWidget::checkHttpThreadSlot()
{
	mHttpTimer->stop();
	while( mUrlList.size() > 0 && mHttpThreadCount < 20 ) 
	{
		mHttpThreadMutex.lock();
		mHttpThreadCount++;

		qWarning("starting thread");
		SnafuHttpThread * newThread = new SnafuHttpThread(getHttpUrl(), this);
		connect(newThread, SIGNAL(finished()), this, SLOT(httpThreadFinishedSlot()));
		newThread->start();

		mHttpThreadMutex.unlock();
	}
	mHttpTimer->start(3000);
}

void SnafuWidget::httpThreadFinishedSlot() 
{
	mHttpThreadMutex.lock();
	mHttpThreadCount--;
	mHttpThreadMutex.unlock();
}

void SnafuWidget::addHttpUrl( const QString & url )
{
	mUrlMutex.lock();
	mUrlList.append(url);
	mUrlMutex.unlock();
}

QString SnafuWidget::getHttpUrl()
{
	QString ret;
	if( mUrlList.size() > 0 )
	{
		mUrlMutex.lock();
		ret = mUrlList.takeFirst();
		mUrlMutex.unlock();
	}
	return ret;
}

// cmd thread pool
void SnafuWidget::checkCmdThreadSlot()
{
	mCmdTimer->stop();
	while( mCmdList.size() > 0 && mCmdThreadCount < 6 ) 
	{
		mCmdThreadMutex.lock();
		mCmdThreadCount++;

		SnafuCmdThread * newThread = new SnafuCmdThread(getCmd(), this);
		connect(newThread, SIGNAL(finished()), this, SLOT(cmdThreadFinishedSlot()));
		newThread->start();

		mCmdThreadMutex.unlock();
	}
	mCmdTimer->start(3000);
}

void SnafuWidget::cmdThreadFinishedSlot() 
{
	mCmdThreadMutex.lock();
	mCmdThreadCount--;
	mCmdThreadMutex.unlock();
}

void SnafuWidget::addCmd( const QString & command )
{
	mCmdList.append(command);
}

QString SnafuWidget::getCmd()
{
	QString ret;
	if( mCmdList.size() > 0 )
	{
		ret = mCmdList.takeFirst();
	}
	return ret;
}

/*!
 *  * static method to return pixmap icons, used by trees and maps
 *   */
QPixmap SnafuWidget::statusPixmap(int s)
{
  QPixmap result = QPixmap(":/images/clear.png");
  switch (s) {
    case 0:
      result = QPixmap(":/images/clear.png");
      break;
    case 1:
      result = QPixmap(":/images/warning.png");
      break;
    case 2:
      result = QPixmap(":/images/minor.png");
      break;
    case 3:
      result = QPixmap(":/images/major.png");
      break;
    case 4:
      result = QPixmap(":/images/critical.png");
      break;
  }
  return result;
}

void SnafuWidget::initBootOptions()
{
	DiskImageList dil = DiskImage::select();
	for( uint i=0; i<=dil.size(); i++ ) {
		mDiskImageCombo->insertItem( i, dil[i].diskImage() );
		mDiskImageCombo->setItemData( i, dil[i].key() );
	}
}

