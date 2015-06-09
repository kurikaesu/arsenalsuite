
/* $Author$
 * $LastChangedDate: 2010-02-02 16:19:01 +1100 (Tue, 02 Feb 2010) $
 * $Rev: 9295 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/include/jobsettingswidgetplugin.h $
 */

#ifndef JOB_VIEWER_FACTORY_H
#define JOB_VIEWER_FACTORY_H

#include <qmap.h>
#include <qstring.h>

#include "afcommon.h"

class QWidget;
class JobViewerPlugin;

class FREEZER_EXPORT JobViewerFactory
{
public:
	static void registerPlugin( JobViewerPlugin * );
	static QMap<QString,JobViewerPlugin*>  mJobViewerPlugins;
	static bool mPluginsLoaded;
};

#endif // JOB_VIEWER_FACTORY_H
