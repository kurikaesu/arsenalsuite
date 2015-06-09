

#ifndef FREEZER_VIEW_H
#define FREEZER_VIEW_H

#include <qwidget.h>
#include <qmap.h>

#include "afcommon.h"

class QMainWindow;
class QToolBar;
class QMenu;

class FREEZER_EXPORT FreezerView : public QWidget
{
Q_OBJECT
public:
	FreezerView( QWidget * parent );
	~FreezerView();

	virtual QString viewType() const = 0;

	// Unique ID for saving and restoring a tab
	static QString generateViewCode();
	QString viewCode() const;
	void setViewCode( const QString & viewCode );
	
	// User settable view name, displayed as the name of the tab
	QString viewName() const;
	void setViewName( const QString & );

	IniConfig & viewConfig();
	void setViewConfig( IniConfig );

	void restorePopup( QWidget * );

	// Optional toolbar added to the mainwindow when this view is active
	virtual QToolBar * toolBar( QMainWindow * ) { return 0; }

	// Hook to populate main window view menu
	virtual void populateViewMenu( QMenu * ) {}

	// Re-read options that are settable in the options dialog
	virtual void applyOptions() {}

	// Save and restore the view to/from the config file
	virtual void save( IniConfig & ini, bool forceFullSave = false );
	virtual void restore( IniConfig & ini, bool forceFullRestore = false );

	// Ability to show a status bar message at the bottom of the main window when this view is active
	QString statusBarMessage() const;
	void setStatusBarMessage( const QString & );
	void clearStatusBar();

	int refreshCount() const;

public slots:
	void refresh();

signals:
	void statusBarMessageChanged( const QString & );

protected slots:
	virtual void doRefresh();

protected:
	bool eventFilter( QObject *, QEvent * );

	/// Stores pointers to each popup window
	/// to save/restore window geometry
	QMap<QWidget*,int> mPopupList;
	int mNextPopupNumber;

	QString mViewName, mStatusBarMessage;
	mutable QString mViewCode;
	bool mRefreshScheduled;
	int mRefreshCount;
	QDateTime mRefreshLast;
	IniConfig mIniConfig;
};

#endif // FREEZER_VIEW_H
