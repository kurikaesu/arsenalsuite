
class JobSettingsWidget : QWidget //, public Ui::JobSettingsWidgetUI
{
%TypeHeaderCode
#include <jobsettingswidget.h>
%End
%ConvertToSubClassCode
	if( sipCpp->inherits( "JobSettingsWidget" ) )
		sipType = sipType_JobSettingsWidget;
	else
		sipType = NULL;
%End
public:
	enum Mode {
		SubmitJobs,
		ModifyJobs
	};

	JobSettingsWidget( QWidget * parent, Mode mode = ModifyJobs );
	virtual ~JobSettingsWidget();

	void setSelectedJobs( JobList selected );
	JobList selectedJobs() const;

	CustomJobSettingsWidget * currentCustomWidget();

public slots:
	void resetSettings();
	void applySettings();
	void settingsChange();
	void setAutoPacketSize(int checkState);
	void showHostSelector();

signals:
	void customJobSettingsWidgetCreated( CustomJobSettingsWidget * );
};

class JobServiceBridge
{
%TypeHeaderCode
#include <jobsettingswidget.h>
%End
public:
	virtual ~JobServiceBridge();
	virtual JobServiceList getJobServices( const Job & ) = 0;
	virtual void removeJobServices( const Job &, JobServiceList ) = 0;
	virtual void applyJobServices( const Job &, JobServiceList ) = 0;
};

class CustomJobSettingsWidget : QGroupBox
{
%TypeHeaderCode
#include <jobsettingswidget.h>
%End
%ConvertToSubClassCode
	if( sipCpp->inherits( "CustomJobSettingsWidget" ) )
		sipType = sipType_CustomJobSettingsWidget;
	else
		sipType = NULL;
%End
public:
	CustomJobSettingsWidget( QWidget * parent );
	virtual ~CustomJobSettingsWidget();

	virtual QStringList supportedJobTypes()=0;

	void setSelectedJobs( JobList selected );
	JobList selectedJobs() const;

	JobServiceList getJobServices( const Job & );
	void removeJobServices( const Job &, JobServiceList );
	void applyJobServices( const Job &, JobServiceList );

	void setJobServiceBridge( JobServiceBridge * /Transfer/ );

	JobSettingsWidget * jobSettingsWidget() const;

public slots:
	virtual void resetSettings();
	virtual void applySettings();
	virtual void settingsChange();

};

