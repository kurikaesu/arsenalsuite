
class MainWindow : QMainWindow
{
%TypeHeaderCode
#include <mainwindow.h>
#include <qmetaobject.h>
%End

%ConvertToSubClassCode
	if( sipCpp->inherits( "MainWindow" ) )
		sipType = sipType_MainWindow;
	else
		sipType = 0;
%End

public:
	MainWindow(QWidget * parent /TransferThis/ =0);
	~MainWindow();

	bool autoRefreshEnabled() const;

	bool autoRefreshOnWindowActivation() const;

//	JobListWidget * jobPage() const { return mJobPage; }
//	HostListWidget * hostPage() const { return mHostPage; }
public slots:

	void setAutoRefreshEnabled( bool );
	void setAutoRefreshOnWindowActivation( bool );

	void openHostServiceMatrixWindow();
	
//	void enableAdmin();
	void applyOptions();
	void showDisplayPrefs();
	void showSettings();
	void showAbout();

	void setCounterState(bool);
	void updateCounter();

	void autoRefresh();

	void showProjectWeightDialog();
	void showManageHostListsDialog();

	void showHostView();
	void showJobView();

	void createJobView();
	void createHostView();
	void createGraphiteView();
	
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

	FreezerView * currentView() const;

	void insertView( FreezerView * view, bool checkViewModeCheckCurrent=true );
	void removeView( FreezerView * view );

	void setCurrentView( FreezerView * );
	void showNextView();

signals:
	void currentViewChanged( FreezerView * );

};