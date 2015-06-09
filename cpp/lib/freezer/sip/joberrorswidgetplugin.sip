class JobErrorsWidgetPlugin
{
%TypeHeaderCode
#include <joberrorswidgetplugin.h>
%End
public:
    virtual ~JobErrorsWidgetPlugin();
    virtual QString name();
    virtual QWidget* interface();
    virtual void initialize(QWidget* parent);
    virtual void setJobErrors(const JobErrorList &);
    virtual bool enabled( const JobList & );
};

class JobErrorsWidgetFactory
{
%TypeHeaderCode
#include <joberrorswidgetfactory.h>
%End
public:
    static void registerPlugin( JobErrorsWidgetPlugin * jewp /Transfer/ );
};

