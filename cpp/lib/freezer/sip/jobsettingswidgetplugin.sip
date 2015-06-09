
class CustomJobSettingsWidgetPlugin
{
%TypeHeaderCode
#include <jobsettingswidgetplugin.h>
%End
public:
	virtual ~CustomJobSettingsWidgetPlugin();
	virtual QStringList jobTypes()=0;
	virtual CustomJobSettingsWidget * createCustomJobSettingsWidget( const QString & jobType, QWidget * parent, JobSettingsWidget::Mode ) = 0 /Factory/;
};

class CustomJobSettingsWidgetsFactory
{
%TypeHeaderCode
#include <jobsettingswidgetplugin.h>
%End
public:
	static bool supportsJobType( const QString & );
	static CustomJobSettingsWidget * createCustomJobSettingsWidget( const QString & jobType, QWidget * parent, JobSettingsWidget::Mode ) /Factory/ ;
	static void registerPlugin( CustomJobSettingsWidgetPlugin * bp /Transfer/ );
};
