
/* $Author$
 * $LastChangedDate: 2010-02-02 16:49:52 +1100 (Tue, 02 Feb 2010) $
 * $Rev: 9298 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/src/jobsettingswidgetplugin.cpp $
 */

#include "hostviewerfactory.h"
#include "hostviewerplugin.h"
#include <QAction>

bool HostViewerFactory::mPluginsLoaded = false;

QMap<QString,HostViewerPlugin*>  HostViewerFactory::mHostViewerPlugins;

void HostViewerFactory::registerPlugin( HostViewerPlugin * jvp )
{
    QString name = jvp->name();
    if( !mHostViewerPlugins.contains(name) ) {
        mHostViewerPlugins[name] = jvp;
        //LOG_3( "Registering HostViewerPlugin: " + name );
    }
}

