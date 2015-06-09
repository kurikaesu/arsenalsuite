#include "jobframestabwidgetfactory.h"
#include "jobframestabwidgetplugin.h"
#include <QAction>

bool JobFramesTabWidgetFactory::mPluginsLoaded = false;

QMap<QString,JobFramesTabWidgetPlugin*>  JobFramesTabWidgetFactory::mJobFramesTabWidgetPlugins;

void JobFramesTabWidgetFactory::registerPlugin( JobFramesTabWidgetPlugin * jftwp )
{
    QString name = jftwp->name();
    if( !mJobFramesTabWidgetPlugins.contains(name) ) {
        mJobFramesTabWidgetPlugins[name] = jftwp;
    }
}

