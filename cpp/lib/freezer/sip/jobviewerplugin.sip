
class JobViewerPlugin
{
%TypeHeaderCode
#include <jobviewerplugin.h>
%End
public:
    virtual ~JobViewerPlugin();
    virtual QString name();
    virtual QString icon();
    virtual void view( const JobList & );
    virtual bool enabled( const JobList & );
};

class JobViewerFactory
{
%TypeHeaderCode
#include <jobviewerfactory.h>
%End
public:
    static void registerPlugin( JobViewerPlugin * jvp /Transfer/ );
};

%PostInitialisationCode
registerBuiltinCustomJobSettingsWidgets();
%End
