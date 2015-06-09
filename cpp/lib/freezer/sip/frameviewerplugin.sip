
class FrameViewerPlugin
{
%TypeHeaderCode
#include <frameviewerplugin.h>
%End
public:
    virtual ~FrameViewerPlugin();
    virtual QString name();
    virtual QString icon();
    virtual void view( const JobAssignment & );
    virtual bool enabled( const JobAssignment & );
};

class FrameViewerFactory
{
%TypeHeaderCode
#include <frameviewerfactory.h>
%End
public:
    static void registerPlugin( FrameViewerPlugin * jvp /Transfer/ );
};

