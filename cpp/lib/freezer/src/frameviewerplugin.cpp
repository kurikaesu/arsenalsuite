#include "frameviewerfactory.h"
#include "frameviewerplugin.h"
#include <QAction>

bool FrameViewerFactory::mPluginsLoaded = false;

QMap<QString,FrameViewerPlugin*>  FrameViewerFactory::mFrameViewerPlugins;

void FrameViewerFactory::registerPlugin( FrameViewerPlugin * fvp )
{
    QString name = fvp->name();
    if( !mFrameViewerPlugins.contains(name) ) {
        mFrameViewerPlugins[name] = fvp;
    }
}

