
class FreezerMenuPlugin
{
%TypeHeaderCode
#include <assfreezermenus.h>
%End
public:
	virtual ~FreezerMenuPlugin();

	virtual void executeMenuPlugin( QMenu * )=0;
};

class FreezerMenuFactory
{
%TypeHeaderCode
#include <assfreezermenus.h>
%End
public:
	static FreezerMenuFactory * instance();
	
	void registerMenuPlugin( FreezerMenuPlugin * plugin /Transfer/, const QString & menuName );
	QList<QAction*> aboutToShow( QMenu * menu, bool addPreSep = false, bool addPostSep = false );
};

class FreezerMenu : QMenu
{
%TypeHeaderCode
#include <assfreezermenus.h>
%End

%ConvertToSubClassCode
	sipType = 0;
	if( sipCpp->inherits("FreezerMenu") )
		sipType = sipType_FreezerMenu;
	//if( sipCpp->inherits("JobListMenu") )
//		sipType = sipType_JobListMenu;
//	if( sipCpp>inherits("HostListMenu") )
//		sipType = sipType_HostListMenu;
%End

public:
	FreezerMenu( QWidget * parent, const QString & title = QString() );

public slots:
	virtual void slotCurrentViewChanged( FreezerView * );
	virtual void slotAboutToShow() = 0;
	virtual void slotActionTriggered( QAction * );
};
/*
class JobListMenu : FreezerMenu
{
%TypeHeaderCode
#include <assfreezermenus.h>
%End
public:
	JobListMenu(JobListWidget *, const QString & title = QString());
	JobListWidget * mJobList;
};
*/
/*class HostListMenu : FreezerMenu
{
%TypeHeaderCode
#include <assfreezermenus.h>
%End
public:
	HostListMenu(HostListWidget *, const QString & title = QString());
	HostListWidget * mHostList;
};*/

