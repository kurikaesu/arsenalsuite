#ifndef MULTI_FRAME_VIEWER_FACTORY_H
#define MULTI_FRAME_VIEWER_FACTORY_H

#include <qmap.h>
#include <qstring.h>

#include "afcommon.h"

class QWidget;
class MultiFrameViewerPlugin;

class FREEZER_EXPORT MultiFrameViewerFactory
{
public:
	static void registerPlugin( MultiFrameViewerPlugin * );
	static QMap<QString, MultiFrameViewerPlugin*>  mMultiFrameViewerPlugins;
	static bool mPluginsLoaded;
};

#endif // MULTI_FRAME_VIEWER_FACTORY_H
