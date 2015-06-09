#ifndef JOB_FRAMES_TAB_WIDGET_PLUGIN_H
#define JOB_FRAMES_TAB_WIDGET_PLUGIN_H

#include "afcommon.h"
class QAction;
class JobTaskList;
class JobList;

#ifdef interface
#undef interface
#endif

class FREEZER_EXPORT JobFramesTabWidgetPlugin
{
public:
    JobFramesTabWidgetPlugin(){}
	virtual ~JobFramesTabWidgetPlugin(){}
    virtual QString name(){return QString();}
    virtual QWidget* interface(){return NULL;}
    virtual void initialize(QWidget* parent){}
    virtual void setJobTaskList(const JobTaskList &, int currentTab){}
    virtual void setJobList(const JobList &, int currentTab){}
    virtual bool enabled(const JobTaskList &){return true;};
    virtual bool enabled(const JobList &){return true;};
};

#endif 
