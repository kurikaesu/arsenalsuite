
/* $Author$
 * $LastChangedDate: 2010-02-16 17:20:26 +1100 (Tue, 16 Feb 2010) $
 * $Rev: 9358 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/include/jobsettingswidgetplugin.h $
 */

#ifndef JOB_SETTINGS_WIDGET_PLUGIN_H
#define JOB_SETTINGS_WIDGET_PLUGIN_H

#include <qmap.h>
#include <qstring.h>

#include "afcommon.h"
#include "jobsettingswidget.h"

class QWidget;
class CustomJobSettingsWidget;

class FREEZER_EXPORT CustomJobSettingsWidgetPlugin
{
public:
	virtual ~CustomJobSettingsWidgetPlugin(){}
	virtual QStringList jobTypes()=0;
	virtual CustomJobSettingsWidget * createCustomJobSettingsWidget( const QString & jobType, QWidget * parent, JobSettingsWidget::Mode )=0;
};

FREEZER_EXPORT void registerBuiltinCustomJobSettingsWidgets();

class FREEZER_EXPORT CustomJobSettingsWidgetsFactory
{
public:
	static bool supportsJobType( const QString & );
	static CustomJobSettingsWidget * createCustomJobSettingsWidget( const QString & jobType, QWidget * parent, JobSettingsWidget::Mode );
	static void registerPlugin( CustomJobSettingsWidgetPlugin * bp );
	static QMap<QString,CustomJobSettingsWidgetPlugin*>  mCustomJobSettingsWidgetsPlugins;
	static bool mPluginsLoaded;
};

#endif // JOB_SETTINGS_WIDGET_PLUGIN_H
