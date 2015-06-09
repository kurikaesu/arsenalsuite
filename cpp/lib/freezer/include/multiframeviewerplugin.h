#ifndef MULTI_FRAME_VIEWER_PLUGIN_H
#define MULTI_FRAME_VIEWER_PLUGIN_H

#include "jobtask.h"

#include "afcommon.h"

class QAction;
class JobTaskList;

class FREEZER_EXPORT MultiFrameViewerPlugin
{
public:
    MultiFrameViewerPlugin(){}
	virtual ~MultiFrameViewerPlugin(){}
    virtual QString name(){return QString();}
    virtual QString icon(){return QString();}
    virtual void view(JobTaskList){}
    virtual bool enabled(JobTaskList){return true;}
};

#endif // MULTI_FRAME_VIEWER_PLUGIN_H
