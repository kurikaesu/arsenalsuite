
#include "joberrorswidgetfactory.h"
#include "joberrorswidgetplugin.h"
#include <QAction>

bool JobErrorsWidgetFactory::mPluginsLoaded = false;

QMap<QString,JobErrorsWidgetPlugin*>  JobErrorsWidgetFactory::mJobErrorsWidgetPlugins;

void JobErrorsWidgetFactory::registerPlugin( JobErrorsWidgetPlugin * jewp )
{
    QString name = jewp->name();
    if( !mJobErrorsWidgetPlugins.contains(name) ) {
        mJobErrorsWidgetPlugins[name] = jewp;
    }
}

