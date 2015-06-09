
#include <QtDesigner/QtDesigner>
#include <QtCore/qplugin.h>
#include <QtCore/qdebug.h>

#include "kdatepicker.h"

class DatePickerPlugin : public QObject, public QDesignerCustomWidgetInterface
{
Q_OBJECT
Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    DatePickerPlugin(QObject *parent = 0);
    virtual ~DatePickerPlugin();

    virtual QString name() const;
    virtual QString group() const;
    virtual QString toolTip() const;
    virtual QString whatsThis() const;
    virtual QString includeFile() const;
    virtual QIcon icon() const;

    virtual bool isContainer() const;

    virtual QWidget *createWidget(QWidget *parent);

    virtual bool isInitialized() const;
    virtual void initialize(QDesignerFormEditorInterface *core);

    virtual QString domXml() const;

private:
    bool m_initialized;
};

