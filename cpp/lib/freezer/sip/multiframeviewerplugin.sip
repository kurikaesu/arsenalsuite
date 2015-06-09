
class MultiFrameViewerPlugin
{
%TypeHeaderCode
#include <multiframeviewerplugin.h>
%End
public:
    virtual ~MultiFrameViewerPlugin();
    virtual QString name();
    virtual QString icon();
    virtual void view( JobTaskList );
    virtual bool enabled( JobTaskList );
};

class MultiFrameViewerFactory
{
%TypeHeaderCode
#include <multiframeviewerfactory.h>
%End
public:
    static void registerPlugin( MultiFrameViewerPlugin * mjvp /Transfer/ );
};

