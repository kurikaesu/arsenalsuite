
/* $Author$
 * $LastChangedDate: 2010-02-04 10:24:32 +1100 (Thu, 04 Feb 2010) $
 * $Rev: 9301 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/include/jobsettingswidgetplugin.h $
 */

#ifndef JOB_VIEWER_PLUGIN_H
#define JOB_VIEWER_PLUGIN_H

#include "afcommon.h"

class QAction;
class JobList;

class FREEZER_EXPORT JobViewerPlugin
{
public:
    JobViewerPlugin(){}
    virtual ~JobViewerPlugin(){}
    virtual QString name(){return QString();}
    virtual QString icon(){return QString();}
    virtual void view(const JobList &){}
    virtual bool enabled(const JobList &){return true;}
};

#endif // JOB_VIEWER_PLUGIN_H
