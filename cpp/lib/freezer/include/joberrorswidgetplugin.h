
#ifndef JOB_ERRORS_WIDGET_PLUGIN_H
#define JOB_ERRORS_WIDGET_PLUGIN_H

#include "afcommon.h"

class QAction;
class JobErrorList;

#ifdef interface
#undef interface
#endif

class FREEZER_EXPORT JobErrorsWidgetPlugin
{
public:
    JobErrorsWidgetPlugin(){}
    virtual ~JobErrorsWidgetPlugin(){}
    virtual QString name(){return QString();}
    virtual QWidget* interface(){return NULL;}
    virtual void initialize(QWidget* parent){}
    virtual void setJobErrors(const JobErrorList &){}
    virtual bool enabled(const JobErrorList &){return true;};
};

#endif 
