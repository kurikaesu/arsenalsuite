

#include <QtDesigner/QDesignerContainerExtension>
#include <QtDesigner/QDesignerCustomWidgetInterface>

#include <QtCore/qplugin.h>
#include <QtGui/QIcon>
#include <QtGui/QLineEdit>

#include "fieldcheckbox.h"
#include "fieldlineedit.h"
#include "fieldtextedit.h"
#include "fieldspinbox.h"

class QDesignerFormEditorInterface;

class StonePlugin : public QObject, public QDesignerCustomWidgetInterface
{
	Q_OBJECT
	Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
	StonePlugin(QObject * parent=0) : QObject(parent), m_initialized(false) { }
	bool isContainer() const { return false; }
	bool isInitialized() const { return m_initialized; }
	QIcon icon() const { return QIcon(); }
	QString codeTemplate() const { return QString(); }
	QString whatsThis() const { return QString(); }
	QString toolTip() const { return QString(); }
	QString group() const { return "StoneUI Widgets"; }
	void initialize(QDesignerFormEditorInterface *)
	{
		if (m_initialized)
			return;
		m_initialized = true;
	}
private:
    bool m_initialized;
};

class FieldLineEditPlugin : public StonePlugin
{
Q_OBJECT
public:
	FieldLineEditPlugin(QObject *parent = 0) : StonePlugin(parent) { }

	QString includeFile() const { return "fieldlineedit.h"; }
	QString name() const { return "FieldLineEdit"; }

	QWidget *createWidget(QWidget *parent)
	{
		FieldLineEdit * fle = new FieldLineEdit( parent );
		return fle;
	}
};

class FieldTextEditPlugin : public StonePlugin
{
Q_OBJECT
public:
	FieldTextEditPlugin(QObject *parent = 0) : StonePlugin(parent) { }
	QString includeFile() const { return "fieldtextedit.h"; }
	QString name() const { return "FieldTextEdit"; }

	QWidget *createWidget(QWidget *parent)
	{
		FieldTextEdit * fte = new FieldTextEdit(parent);
		return fte;
	}
};

class FieldSpinBoxPlugin : public StonePlugin
{
Q_OBJECT
public:
	FieldSpinBoxPlugin(QObject *parent = 0) : StonePlugin(parent) { }
	QString includeFile() const { return "fieldspinbox.h"; }
	QString name() const { return "FieldSpinBox"; }

	QWidget *createWidget(QWidget *parent)
	{
		FieldSpinBox * flb = new FieldSpinBox(parent);
		return flb;
	}
};

class FieldCheckBoxPlugin : public StonePlugin
{
Q_OBJECT
public:
	FieldCheckBoxPlugin(QObject *parent = 0) : StonePlugin(parent) { }
	QString includeFile() const { return "fieldcheckbox.h"; }
	QString name() const { return "FieldCheckBox"; }

	QWidget *createWidget(QWidget *parent)
	{
		FieldCheckBox * fcb = new FieldCheckBox(parent);
		return fcb;
	}
};

class StoneUIPlugins : public QObject, public QDesignerCustomWidgetCollectionInterface
{
	Q_OBJECT
	Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)
public:
	StoneUIPlugins(QObject * parent=0) : QObject(parent) {}
	QList<QDesignerCustomWidgetInterface*> customWidgets() const
	{
		if( mPlugins.isEmpty() ) {
			mPlugins
			<< new FieldLineEditPlugin
			<< new FieldTextEditPlugin
			<< new FieldSpinBoxPlugin
			<< new FieldCheckBoxPlugin;
		}
		return mPlugins;
	}
	mutable QList<QDesignerCustomWidgetInterface*> mPlugins;
};
