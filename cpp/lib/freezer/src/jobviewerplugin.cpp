
/* $Author$
 * $LastChangedDate: 2010-02-02 16:19:01 +1100 (Tue, 02 Feb 2010) $
 * $Rev: 9295 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/src/jobsettingswidgetplugin.cpp $
 */

#include "jobviewerfactory.h"
#include "jobviewerplugin.h"
#include <QAction>

bool JobViewerFactory::mPluginsLoaded = false;

QMap<QString,JobViewerPlugin*>  JobViewerFactory::mJobViewerPlugins;

void JobViewerFactory::registerPlugin( JobViewerPlugin * jvp )
{
    QString name = jvp->name();
    if( !mJobViewerPlugins.contains(name) ) {
        mJobViewerPlugins[name] = jvp;
        //LOG_3( "Registering JobViewerPlugin: " + name );
    }
}

