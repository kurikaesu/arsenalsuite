#ifndef JOB_ERRORS_WIDGET_FACTORY_H
#define JOB_ERRORS_WIDGET_FACTORY_H

#include <qmap.h>
#include <qstring.h>

#include "afcommon.h"

class QWidget;
class JobErrorsWidgetPlugin;

class FREEZER_EXPORT JobErrorsWidgetFactory
{
public:
	static void registerPlugin( JobErrorsWidgetPlugin * );
	static QMap<QString,JobErrorsWidgetPlugin*>  mJobErrorsWidgetPlugins;
	static bool mPluginsLoaded;
};

#endif 
