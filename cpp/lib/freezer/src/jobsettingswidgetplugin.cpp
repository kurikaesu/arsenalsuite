
/* $Author: newellm $
 * $LastChangedDate: 2010-04-01 15:41:58 -0700 (Thu, 01 Apr 2010) $
 * $Rev: 9636 $
 * $HeadURL: svn://newellm@ocelot/blur/trunk/cpp/lib/assfreezer/src/jobsettingswidgetplugin.cpp $
 */

#include "jobsettingswidgetplugin.h"

#include "jobaftereffectssettingswidget.h"
#include "jobbatchsettingswidget.h"
#include "jobfusionsettingswidget.h"
#include "jobmaxsettingswidget.h"
#include "jobmayasettingswidget.h"
#include "jobmaxscriptsettingswidget.h"
#include "jobshakesettingswidget.h"
#include "jobxsisettingswidget.h"

class BuiltinCustomJobSettingsWidgetsPlugin : public CustomJobSettingsWidgetPlugin
{
public:
	QStringList jobTypes();
	CustomJobSettingsWidget * createCustomJobSettingsWidget( const QString & jobType, QWidget * parent, JobSettingsWidget::Mode );
};

QStringList BuiltinCustomJobSettingsWidgetsPlugin::jobTypes()
{
	return JobMaxSettingsWidget::jobTypes()
		 + JobMayaSettingsWidget::jobTypes()
		 + JobAfterEffectsSettingsWidget::jobTypes()
		 + JobShakeSettingsWidget::jobTypes()
		 + JobBatchSettingsWidget::jobTypes()
		 + JobMaxScriptSettingsWidget::jobTypes()
		 + JobXSISettingsWidget::jobTypes()
		 + JobFusionSettingsWidget::jobTypes()
		 + JobFusionVideoMakerSettingsWidget::jobTypes();
}

CustomJobSettingsWidget * BuiltinCustomJobSettingsWidgetsPlugin::createCustomJobSettingsWidget( const QString & jobType, QWidget * parent, JobSettingsWidget::Mode mode )
{
	if( JobMaxSettingsWidget::jobTypes().contains( jobType ) )
		return new JobMaxSettingsWidget( parent, mode );
	else if( JobMayaSettingsWidget::jobTypes().contains( jobType ) )
		return new JobMayaSettingsWidget( parent, mode );
	else if( JobAfterEffectsSettingsWidget::jobTypes().contains( jobType ) )
		return new JobAfterEffectsSettingsWidget( parent, mode );
	else if( JobShakeSettingsWidget::jobTypes().contains( jobType ) )
		return new JobShakeSettingsWidget( parent, mode );
	else if( JobBatchSettingsWidget::jobTypes().contains( jobType ) )
		return new JobBatchSettingsWidget( parent, mode );
	else if( JobMaxScriptSettingsWidget::jobTypes().contains( jobType ) )
		return new JobMaxScriptSettingsWidget( parent, mode );
	else if( JobXSISettingsWidget::jobTypes().contains( jobType ) )
		return new JobXSISettingsWidget( parent, mode );
	else if( JobFusionSettingsWidget::jobTypes().contains( jobType ) )
		return new JobFusionSettingsWidget( parent, mode );
	else if( JobFusionVideoMakerSettingsWidget::jobTypes().contains( jobType ) )
		return new JobFusionVideoMakerSettingsWidget( parent, mode );
	return 0;
}

static BuiltinCustomJobSettingsWidgetsPlugin * sBuiltinPlugin = 0;

void registerBuiltinCustomJobSettingsWidgets()
{
	if( !sBuiltinPlugin ) {
		sBuiltinPlugin = new BuiltinCustomJobSettingsWidgetsPlugin();
		CustomJobSettingsWidgetsFactory::registerPlugin( sBuiltinPlugin );
	}
}

bool CustomJobSettingsWidgetsFactory::mPluginsLoaded = false;

QMap<QString,CustomJobSettingsWidgetPlugin*>  CustomJobSettingsWidgetsFactory::mCustomJobSettingsWidgetsPlugins;

bool CustomJobSettingsWidgetsFactory::supportsJobType( const QString & jobType )
{
	return !jobType.isEmpty() && mCustomJobSettingsWidgetsPlugins.contains( jobType );
}

void CustomJobSettingsWidgetsFactory::registerPlugin( CustomJobSettingsWidgetPlugin * wp )
{
	QStringList types = wp->jobTypes();
	foreach( QString t, types )
		if( !mCustomJobSettingsWidgetsPlugins.contains(t) ) {
			mCustomJobSettingsWidgetsPlugins[t] = wp;
			LOG_3( "Registering CustomJobSettingsWidget for jobtype: " + t );
		}
}

CustomJobSettingsWidget * CustomJobSettingsWidgetsFactory::createCustomJobSettingsWidget( const QString & jobTypeName, QWidget * parent, JobSettingsWidget::Mode mode  )
{
	if( jobTypeName.isEmpty() )
		return 0;

	JobType jobType = JobType::recordByName( jobTypeName );
	while( jobType.isRecord() && !mCustomJobSettingsWidgetsPlugins.contains( jobType.name() ) )
		jobType = jobType.parentJobType();

	if( jobType.isRecord() && mCustomJobSettingsWidgetsPlugins.contains( jobType.name() ) ) {
		CustomJobSettingsWidget * jsw = mCustomJobSettingsWidgetsPlugins[jobType.name()]->createCustomJobSettingsWidget(jobType.name(),parent,mode);
		return jsw;
	}
	return 0;
}


