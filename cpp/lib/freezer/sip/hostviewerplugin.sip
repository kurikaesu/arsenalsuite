
class HostViewerPlugin
{
%TypeHeaderCode
#include <hostviewerplugin.h>
%End
public:
    virtual ~HostViewerPlugin();
    virtual QString name();
    virtual QString icon();
    virtual void view( const HostList & );
    virtual bool enabled( const HostList & );
};

class HostViewerFactory
{
%TypeHeaderCode
#include <hostviewerfactory.h>
%End
public:
    static void registerPlugin( HostViewerPlugin * jvp /Transfer/ );
};

