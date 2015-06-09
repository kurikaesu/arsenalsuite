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
 * $Id: snafuwidget.h 5408 2007-12-18 00:13:49Z brobison $
 */

#ifndef SNAFU_WIDGET_H
#define SNAFU_WIDGET_H

#include <QByteArray>
#include <QComboBox>
#include <QFile>
#include <QHttp>
#include <QList>
#include <QMap>
#include <QMutex>
#include <QRegExp>
#include <QTabWidget>
#include <QToolBar>
#include <QTreeWidget>
#include <QWidget>

#include "recordproxy.h"

#include "blurqt.h"
#include "ui_snafuwidgetui.h"

#include "syslog.h"
//#include "arkeventalert.h"
#include "host.h"
#include "hostgroup.h"
#include "hoststatus.h"
//#include "tracker.h"
#include "graph.h"
#include "graphpage.h"

#include "snafuhttpthread.h"
#include "snafurrdthread.h"
#include "licensewidget.h"

class QAction;
class QMenu;
class QTextEdit;

class Host;
class HostStatus;

class SnafuWidget : public QWidget, public Ui::SnafuWidgetUI
{
Q_OBJECT

public:
    SnafuWidget(QWidget * parent=0);
		~SnafuWidget();

		/*!
		 * Used by SnafuMapWidget classes to get mCurrentMap;
		 */
    //int getMap() const;

		/*!
		 * Lots of methods need a list of host to act upon
		 */
    HostList selectedHosts(bool recursive=true) const;

		/*!
		 * Methods used to populate and empty queues for thread pools
		 */
		void addHttpUrl( const QString & );
		QString getHttpUrl();

		void addCmd( const QString & );
		QString getCmd();

		/*!
		 * Static Methods
		 */
		static QString timeCode(int);
		static QString getUserName();
		static QPixmap statusPixmap(int);

		QAction *refreshAct;
		QAction *groupAddAct;
		QAction *findHostAct;
		QAction *vncAct;

signals:
		void showStatusBarMessage(const QString &);
		void clearStatusBar();

public slots:
    void initViewCombo();
    void refreshHostList();
    void buildHostTree();

    void buildGroupTree();
    void refreshGroupTree();
		void hostsAddedToGroup(RecordList);

    void buildSoftwareTree();
    void saveSoftwareTree();
    void buildServiceTree();
    void saveServiceTree();

    void findHostSlot();
    void findHosts();
		void selectAll();
		void selectNone();
		void selectInverse();

    void hostSelected();
    void showEvents(HostList &);
//    void showJobs(HostList);
    void showAlerts(HostList);
    void showTickets(HostList);
		void showGraphs(HostList);
		void showDetails(HostList);
		void showLicenses(HostList);

    void addAlert();
    void alertContextMenu(const QPoint &);
		void createAlertMenu();

		void addGraph();

    void launchTicket(QTreeWidgetItem *);
//    void launchJob(QTreeWidgetItem *);
    void launchRemoteControl();

    void hostContextMenu(const QPoint &);
    void eventContextMenu(const QPoint &);
		void resetECM();
    void ticketContextMenu(const QPoint &);
    void vncSlot();
    void msrdpSlot();
    void sshSlot();
    void telnetSlot();

    void addToGroupSlot(const QString &);
    void removeFromGroupSlot();
    void groupAddSlot();

    void changeCommentSlot();

    void farmSetSlotsSlot();
    void farmSetSlotsOffSlot();
    void farmAutoLoginEnableSlot();
    void farmAutoLoginDisableSlot();
    void farmRestartSlot();
    void farmRebootSlot();
    void xcatRebootSlot();
    void xcatImageSlot();

    void eventAckSlot();
		void eventShowAckSlot();

		//void rotateMaps();
		//void mapRotateToggle(int);

		void getEscalationPassword();

		// http thread pool
		void checkHttpThreadSlot();
		void httpThreadFinishedSlot();
		
		// http thread pool
		void checkCmdThreadSlot();
		void cmdThreadFinishedSlot();

protected:

private:
	/*!
	 * hostTimer fires to refresh the host list from the database
	 */
	QTimer * hostTimer;
	/*!
	 * mapTimer fires to rotate the SnafuMapWidget
	 */
	QTimer * mapTimer;

	/*!
	 * build a hostList based on user criteria and populate the tree
	 */
	void initHostTree();
	void buildHostTreeGroup();
	void buildHostTreeText();
	void buildHostTreeSoftware();
	bool mHostTaskRunning;

	/*!
	 * maintain maps of possible values and filtered ones
	 */
	QMap< QString, HostGroup > mGroupByName;
	void initGroupTree();

	/*!
	 * maintain maps of possible values and filtered ones
	 */
	QMap< QString, int > mFarmStatus;
	QMap< QString, int > mFarmStatusFilter;

	QMap< QString, int > mOS;
	QMap< QString, int > mOSFilter;

	QMap< QString, int > mSeverityFilter;

	QMap< QString, int > mClass;
	QMap< QString, int > mClassFilter;

	QMap< QString, int > mTicketStatus;
	QMap< QString, int > mTicketStatusFilter;
	QMap< QString, int > mTicketPriority;
	QMap< QString, int > mTicketPriorityFilter;

	/*!
	 * installed software and a tree to select it
	 */
	QList<QString> mSoftware;
	void initSoftwareTree();
	void initSoftwareSelectTree();
	int softwareTimeLimit();

	/*!
	 * toggled when a find is running to prevent updates to some widgets
	 */
	bool mFindHostRunning;

	/*!
	 * init tree on tabs
	 */
	void initEventTable();
	void initJobTree();
	void initTicketTree();
	void initAlertTab();
	void initLicenseTab();

	/*!
	 * application setup stuff
	 */
	void createActions();
	void createStatusBar();
	void createToolBar();

	/*!
	 * context menus
	 */
	void createHostMenu();
	void createFarmStatusMenu();
	void createOSMenu();

	void createEventMenu();
	void createClassMenu();

	void createTicketMenu();
	void createTicketPriorityMenu();
	void createTicketStatusMenu();

	/*!
	 * actions
	 */

	QAction *selectAllAct;
	QAction *selectNoneAct;
	QAction *selectInvAct;

	QMenu *hostMenu;
	QMenu *farmStatusMenu;
	QMenu *osMenu;

	QMenu *farmActionMenu;
	QAction *farmRestartAct;
	QAction *farmRebootAct;
	QAction *farmSetSlotsAct;
	QAction *farmSetSlotsOffAct;
	QAction *farmAutoLoginEnableAct;
	QAction *farmAutoLoginDisableAct;

	QAction *remoteControlAct;
	QAction *sshAct;
	QAction *telnetAct;
	QAction *msrdpAct;
	QAction *muninAct;
	QAction *graphsAct;
	QAction *changeCommentAct;
	void vncHost(const QString &);
	void msrdpHost(const QString &);
	void sshHost(const QString &);
	void telnetHost(const QString &);

	// xcat actions
	QMenu *xcatMenu;
	QAction *xcatRebootAct;
	QAction *xcatImageAct;
	
	// event actions
	QMenu *eventMenu;
	int mECM;
	QMenu *mClassMenu;
	QAction *eventAckAct;
	bool mShowAckd;

	QAction *eventDeleteAct;
	QAction *eventRaisePriorityAct;
	QAction *eventLowerPriorityAct;
	QList<QString> mEventClasses;
	QList<QString> mEventMethods;
	
	// alert actions
	QMenu *alertMenu;
	//QMap<QTreeWidgetItem*, ArkEventAlert> mAlertMap;
	
	// ticket actions
	QMenu *ticketMenu;
	QMenu *ticketPriorityMenu;
	QMenu *ticketStatusMenu;

	QMenu *groupMenu;
	QToolBar *toolBar;
	QToolBar *eventToolBar;
	QToolBar *alertToolBar;
	QToolBar *ticketToolBar;

	QMap<int, QString> mMaps;
	//void initMapCombo();

	QString mEscalationPassword;

	// http thread pool
	QMutex mUrlMutex;
	QMutex mHttpThreadMutex;
	QList<QString> mUrlList;
	int mHttpThreadCount;
	QTimer * mHttpTimer;

	// cmd pool
	QMutex mCmdThreadMutex;
	int mCmdThreadCount;
	QList<QString> mCmdList;
	QTimer * mCmdTimer;

	// graph tab stuff
	bool mGraphTabsInit;
	QMap<int,GraphPage> mGraphTabMap;

	HostList mHostList;
	HostList mSelected;

	int mCurrentTab;

	void initBootOptions();
	LicenseWidget * mLicenseWidget;
};

#endif

