#include "multiframeviewerfactory.h"
#include "multiframeviewerplugin.h"
#include <QAction>

bool MultiFrameViewerFactory::mPluginsLoaded = false;

QMap<QString,MultiFrameViewerPlugin*>  MultiFrameViewerFactory::mMultiFrameViewerPlugins;

void MultiFrameViewerFactory::registerPlugin( MultiFrameViewerPlugin * mfvp )
{
    QString name = mfvp->name();
    if( !mMultiFrameViewerPlugins.contains(name) ) {
        mMultiFrameViewerPlugins[name] = mfvp;
    }
}

