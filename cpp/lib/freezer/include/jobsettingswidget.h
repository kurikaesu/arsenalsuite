
/* $Author$
 * $LastChangedDate: 2010-02-16 17:20:26 +1100 (Tue, 16 Feb 2010) $
 * $Rev: 9358 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/include/jobsettingswidget.h $
 */

#ifndef JOB_SETTINGS_WIDGET_H
#define JOB_SETTINGS_WIDGET_H

#include <qwidget.h>

#include "host.h"
#include "jobtype.h"
#include "job.h"
#include "user.h"
#include "employee.h"

#include "afcommon.h"

#include "ui_jobsettingswidgetui.h"

class CustomJobSettingsWidget;
class Stone::RecordProxy;
using namespace Stone;

class CustomJobSettingsWidget;

/**
 * Widget for settings that are shared by all job types
 * Also handles showing the correct CustomJobSettingsWidget for
 * the selected jobs.
 **/
class FREEZER_EXPORT JobSettingsWidget : public QWidget, public Ui::JobSettingsWidgetUI
{
Q_OBJECT
public:

	enum Mode {
		SubmitJobs,
		ModifyJobs
	};

	JobSettingsWidget( QWidget * parent, Mode mode = ModifyJobs );
	virtual ~JobSettingsWidget();

	Mode mode() const { return mMode; }

	void setSelectedJobs( JobList selected );
	JobList selectedJobs() const { return mSelectedJobs; }

	CustomJobSettingsWidget * currentCustomWidget();

public slots:
	void resetSettings();
	void applySettings();
	void settingsChange();
	void setAutoPacketSize(int checkState);
	void showHostSelector();
	void showEnvironmentWindow();

	void showEmailErrorListWindow();
	void showJabberErrorListWindow();

	void showEmailCompleteListWindow();
	void showJabberCompleteListWindow();

	void buildServiceTree();
	void saveServiceTree();

signals:
	void customJobSettingsWidgetCreated( CustomJobSettingsWidget * );

protected:

	void updateCustomJobSettingsWidget();
	QString buildNotifyString(UserList emailList, UserList jabberList);
	void extractNotifyUsers();

	Mode mMode;
	bool mChanges;
	// Used to ignore changes that are done internally, to avoid infinite recursion
	bool mIgnoreChanges;
	HostList mUpdatedHostList;
	QString mUpdatedHostListString;
	QString mUpdatedEnvironment;

	JobList mSelectedJobs;
	RecordProxy * mSelectedJobsProxy;
	QMap<QString, CustomJobSettingsWidget*> mCustomJobSettingsWidgetMap;

	UserList jabberErrorList;
	UserList emailErrorList;

	UserList jabberCompleteList;
	UserList emailCompleteList;

	bool mNotifyChanged;

	EmployeeList mMainUserList;
};

class FREEZER_EXPORT JobServiceBridge
{
public:
	virtual ~JobServiceBridge(){}
	virtual JobServiceList getJobServices( const Job & ) = 0;
	virtual void removeJobServices( const Job &, JobServiceList ) = 0;
	virtual void applyJobServices( const Job &, JobServiceList ) = 0;
};

/**
 *  To be implemented for each job type's own settings
 *  Sits to the right of the JobSettingsWidget
 */
class FREEZER_EXPORT CustomJobSettingsWidget : public QGroupBox
{
Q_OBJECT
public:
	CustomJobSettingsWidget( QWidget * parent, JobSettingsWidget::Mode mode = JobSettingsWidget::ModifyJobs );
	virtual ~CustomJobSettingsWidget();

	JobSettingsWidget::Mode mode() const { return mMode; }

	virtual QStringList supportedJobTypes()=0;

	void setSelectedJobs( JobList selected );
	JobList selectedJobs() const { return mSelectedJobs; }

	JobServiceList getJobServices( const Job & );
	void removeJobServices( const Job &, JobServiceList );
	void applyJobServices( const Job &, JobServiceList );

	void setJobServiceBridge( JobServiceBridge * );

	JobSettingsWidget * jobSettingsWidget() const;

public slots:
	virtual void resetSettings();
	virtual void applySettings();
	virtual void settingsChange();

protected:
	JobServiceBridge * mJobServiceBridge;
	QHBoxLayout * mApplyResetLayout;
	QPushButton * mApplySettingsButton, * mResetSettingsButton;

	JobSettingsWidget::Mode mMode;

	JobList mSelectedJobs;
	bool mChanges;
};

#endif // JOB_SETTINGS_WIDGET_H

