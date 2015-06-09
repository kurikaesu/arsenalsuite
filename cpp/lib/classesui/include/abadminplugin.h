
#ifndef ABADMIN_PLUGIN_H
#define ABADMIN_PLUGIN_H

#include <qstring.h>
#include <qhash.h>
#include <qpixmap.h>
#include <qplugin.h>

#include "classesui.h"

class QWidget;

class CLASSESUI_EXPORT ABAdminPage
{
public:
	enum PageType { Group, Widget };
	ABAdminPage(PageType type, QString pageName, QString parentPageName, QPixmap pageIcon);

	PageType type() const { return mType; }
	QString pageName() const { return mPageName; }
	QString parentPageName() const { return mParentPageName; }
	QPixmap icon() const { return mPageIcon; }

	PageType mType;
	QString mPageName, mParentPageName;
	QPixmap mPageIcon;
};

class CLASSESUI_EXPORT ABAdminPlugin
{
public:
	virtual QList<ABAdminPage> pages() = 0;
	virtual QWidget * createWidget( const QString & pageName, QWidget * parent ) = 0;
};

class CLASSESUI_EXPORT ABAdminFactory
{
public:
	// Path can be to a shared library(dll) or to a directory and it will try all the shared libraries
	void loadPlugins( const QString & path );
	QList<ABAdminPage> pages();
	QWidget * createWidget( const QString & pageName, QWidget * parent );

	// Takes ownership
	void registerPlugin( ABAdminPlugin * plugin );

	static ABAdminFactory * instance();

protected:
	QList<ABAdminPage> mPages;
	QHash<QString,ABAdminPlugin*> mPluginsByPageName;
};

#define ABADMIN_EXPORT_PLUGIN(PLUGIN_CLASS) \
	Q_EXTERN_C Q_DECL_EXPORT ABAdminPlugin * Q_STANDARD_CALL abadmin_plugin_instance() \
	{ \
		static ABAdminPlugin * _instance = 0; \
		if (!_instance)      \
			_instance = new PLUGIN_CLASS; \
		return _instance; \
	}

#endif // ABADMIN_PLUGIN_H

