class JobFramesTabWidgetPlugin
{
%TypeHeaderCode
#include <jobframestabwidgetplugin.h>
%End
public:
    virtual ~JobFramesTabWidgetPlugin();
    virtual QString name();
    virtual QWidget* interface();
    virtual void initialize(QWidget* parent);
    virtual void setJobTaskList(const JobTaskList &, int currentIndex);
    virtual void setJobList(const JobList &, int currentIndex);
    virtual bool enabled( const JobList & );
    virtual bool enabled( const JobTaskList & );
};

class JobFramesTabWidgetFactory
{
%TypeHeaderCode
#include <jobframestabwidgetfactory.h>
%End
public:
    static void registerPlugin( JobFramesTabWidgetPlugin * jftwp /Transfer/ );
};

