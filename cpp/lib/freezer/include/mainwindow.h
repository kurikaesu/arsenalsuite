/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Arsenal.
 *
 * Arsenal is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Arsenal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Blur; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMenu>
#include <QToolBar>
#include <QMainWindow>

#include "afcommon.h"

const QString VERSION("1.1.15");

class QLabel;
class QTimer;
class QTabWidget;
class QStackedWidget;

class FreezerView;
class JobListWidget;
class HostListWidget;
class Service;

class FREEZER_EXPORT MainWindow : public QMainWindow
{
Q_OBJECT
public:
	MainWindow(QWidget * parent=0);
	~MainWindow();

	virtual void closeEvent( QCloseEvent * );
	virtual bool event( QEvent * );

	bool autoRefreshEnabled() const;

	bool autoRefreshOnWindowActivation() const;

	JobListWidget * jobPage() const { return mJobPage; }
	HostListWidget * hostPage() const { return mHostPage; }

public slots:
	void setAutoRefreshEnabled( bool );
	void setAutoRefreshOnWindowActivation( bool );

	void openHostServiceMatrixWindow();
	void openUserServiceMatrixWindow();

	void enableAdmin();
	void applyOptions();
	void showDisplayPrefs();
	void showSettings();
	void showAbout();

	void setCounterState(bool);
	void updateCounter();

	void autoRefresh();

	void showProjectWeightDialog();
	void showProjectReserveDialog();
	void showManageHostListsDialog();

	void showHostView();
	void showJobView();

	void createJobView();
	void createHostView();
	void createGraphiteView();
	void createWebView();
	void createErrorView();

void closeCurrentView();

	void cloneView( FreezerView * view );
	void cloneCurrentView();
	void saveCurrentViewAs();
	void saveViewAs( FreezerView * );
	void moveViewLeft( FreezerView * );
	void moveViewRight( FreezerView * );
	void moveCurrentViewLeft();
	void moveCurrentViewRight();

	// Pops up dialog to prompt user
	void renameView( FreezerView * view );
	void renameCurrentView();

	FreezerView * currentView() const { return mCurrentView; }

	void insertView( FreezerView * view, bool checkViewModeCheckCurrent=true );
	void removeView( FreezerView * view );

	void setCurrentView( const QString & );
	void setCurrentView( FreezerView * );

	void showNextView();

	void saveCurrentViewToFile();
	void saveViewToFile( FreezerView * );
	void loadViewFromFile(bool notUsed=true);
	void loadViewFromFile(const QString &);

	void saveSettings();

	void showLastVisitedView();
signals:
    void currentViewChanged( FreezerView * );

protected slots:
	void currentTabChanged( int );
	void populateViewMenu();
	void populateRestoreViewMenu();
    void populateToolsMenu();

	void restoreViewActionTriggered(QAction*);
	bool checkViewModeChange();

	void hostViewActionToggled(bool);
	void jobViewActionToggled(bool);
protected:
	virtual bool eventFilter( QObject *, QEvent * );
	virtual void keyPressEvent( QKeyEvent * event );

	void showTabMenu( const QPoint & pos, FreezerView * view );

	void saveViews();
	void saveView( FreezerView * );
	void restoreViews();
	FreezerView * restoreSavedView( const QString & viewName, bool updateWindow = true );
	FreezerView * restoreView( IniConfig &, const QString & viewName, bool updateWindow = true, bool forceFullRestore = false );

	void repopulateToolBar();

	void setupStackedView();
	void setupTabbedView();

	// Periodic check to ensure renderfarm is 'online'
	void updateFarmStatus( const Service & managerService );

	QTabWidget * mTabWidget;
	QStackedWidget * mStackedWidget;

	FreezerView * mCurrentView;
	QList<FreezerView*> mViews, mViewVisitList;

	JobListWidget * mJobPage;
	HostListWidget * mHostPage;
	QLabel * mCounterLabel;
	QLabel * mFarmStatusLabel;

	bool mAdminEnabled, mCounterActive;

	QTimer * mAutoRefreshTimer;

	/* Actions */
	QAction* FileExitAction;
	QAction* FileSaveAction;
	QAction* HelpAboutAction;

	QAction* ViewHostsAction;
	QAction* ViewJobsAction;

	QAction* DisplayPrefsAction;
	QAction* SettingsAction;
	QAction* AdminAction;
	QAction* HostServiceMatrixAction;
	QAction* UserServiceMatrixAction;
	QAction* ProjectWeightingAction;
	QAction* ProjectReserveAction;
	QAction* AutoRefreshAction;

	QAction * mNewJobViewAction;
	QAction * mNewHostViewAction;
	QAction * mNewGraphiteViewAction;
	QAction * mNewWebViewAction;
	QAction * mNewErrorViewAction;
	QAction * mCloneViewAction;
	QAction * mSaveViewAsAction;
	QAction * mCloseViewAction;
	QAction * mMoveViewLeftAction;
	QAction * mMoveViewRightAction;

	QAction * mSaveViewToFileAction;
	QAction * mLoadViewFromFileAction;

	QMenu * mFileMenu, * mToolsMenu, * mOptionsMenu, * mViewMenu, * mRestoreViewMenu, * mHelpMenu;
	QToolBar *Toolbar;
};

#endif // MAIN_WINDOW_H

