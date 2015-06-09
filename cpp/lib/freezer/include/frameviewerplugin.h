#ifndef FRAME_VIEWER_PLUGIN_H
#define FRAME_VIEWER_PLUGIN_H

class QAction;
class JobAssignment;

class FrameViewerPlugin
{
public:
    FrameViewerPlugin(){}
	virtual ~FrameViewerPlugin(){}
    virtual QString name(){return QString();};
    virtual QString icon(){return QString();};
    virtual void view(const JobAssignment &){};
    virtual bool enabled(const JobAssignment &){return true;};
};

#endif // FRAME_VIEWER_PLUGIN_H
