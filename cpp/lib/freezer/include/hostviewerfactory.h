
/* $Author$
 * $LastChangedDate: 2010-02-02 16:49:52 +1100 (Tue, 02 Feb 2010) $
 * $Rev: 9298 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/include/jobsettingswidgetplugin.h $
 */

#ifndef HOST_VIEWER_FACTORY_H
#define HOST_VIEWER_FACTORY_H

#include "afcommon.h"

#include <qmap.h>
#include <qstring.h>

class QWidget;
class HostViewerPlugin;

class FREEZER_EXPORT HostViewerFactory
{
public:
	static void registerPlugin( HostViewerPlugin * );
	static QMap<QString,HostViewerPlugin*>  mHostViewerPlugins;
	static bool mPluginsLoaded;
};

#endif // HOST_VIEWER_FACTORY_H
