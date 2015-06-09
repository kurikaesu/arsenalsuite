
#include <qdir.h>
#include <qfileinfo.h>
#include <qlibrary.h>

#include "abadminplugin.h"

ABAdminPage::ABAdminPage(PageType type, QString pageName, QString parentPageName, QPixmap pageIcon)
: mType( type )
, mPageName( pageName )
, mParentPageName( parentPageName )
, mPageIcon( pageIcon )
{}

typedef ABAdminPlugin * (* abadmin_plugin_instance_fn)();

static ABAdminPlugin * tryLoadPlugin( const QString & path )
{
	QLibrary library(path);
	abadmin_plugin_instance_fn func = (abadmin_plugin_instance_fn)library.resolve( "abadmin_plugin_instance" );
	if( func )
		return func();
}

void ABAdminFactory::loadPlugins( const QString & path )
{
	QFileInfo fi(path);
	if( fi.isFile() ) {
		ABAdminPlugin * plugin = tryLoadPlugin(path);
		if( plugin ) registerPlugin(plugin);
	} else if( fi.isDir() ) {
		QDir dir = fi.dir();
		QStringList entries = dir.entryList();
		foreach( QString entry, entries ) {
			QString entryPath = path + QDir::separator() + entry;
			if( QFileInfo(entryPath).isFile() )
				loadPlugins( entryPath );
		}
	}
}

QList<ABAdminPage> ABAdminFactory::pages()
{
	return mPages;
}

QWidget * ABAdminFactory::createWidget( const QString & pageName, QWidget * parent )
{
	if( !mPluginsByPageName.contains( pageName ) ) return 0;
	return mPluginsByPageName[pageName]->createWidget( pageName, parent );
}

void ABAdminFactory::registerPlugin( ABAdminPlugin * plugin )
{
	foreach( ABAdminPage page, plugin->pages() )
	{
		mPages += page;
		mPluginsByPageName[page.mPageName] = plugin;
	}
}

ABAdminFactory * ABAdminFactory::instance()
{
	static ABAdminFactory * factory = 0;
	if( !factory ) factory = new ABAdminFactory();
	return factory;
}
