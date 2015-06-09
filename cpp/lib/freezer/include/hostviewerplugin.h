
/* $Author$
 * $LastChangedDate: 2010-02-04 10:24:32 +1100 (Thu, 04 Feb 2010) $
 * $Rev: 9301 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/assfreezer/include/jobsettingswidgetplugin.h $
 */

#ifndef HOST_VIEWER_PLUGIN_H
#define HOST_VIEWER_PLUGIN_H

class QAction;
class HostList;

class HostViewerPlugin
{
public:
    HostViewerPlugin(){}
	virtual ~HostViewerPlugin(){}
    virtual QString name(){return QString();};
    virtual QString icon(){return QString();};
    virtual void view(const HostList &){};
    virtual bool enabled(const HostList &){return true;};
};

#endif // HOST_VIEWER_PLUGIN_H
