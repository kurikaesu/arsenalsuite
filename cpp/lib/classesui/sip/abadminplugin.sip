
class ABAdminPlugin
{
%TypeHeaderCode
#include "abadminplugin.h"
%End
public:
	virtual QList<ABAdminPage> pages() = 0;
	virtual QWidget * createWidget( const QString & pageName, QWidget * parent ) = 0 /Factory/;
};

class ABAdminPage
{
%TypeHeaderCode
#include "abadminplugin.h"
%End
public:
	enum PageType { Group, Widget };
	ABAdminPage(PageType type, QString pageName, QString parentPageName, QPixmap pageIcon);

	PageType type() const;
	QString pageName() const;
	QString parentPageName() const;
	QPixmap icon() const;
};

class ABAdminFactory
{
%TypeHeaderCode
#include "abadminplugin.h"
%End
public:
	void loadPlugins( const QString & path );

	QList<ABAdminPage> pages();
	QWidget * createWidget( const QString & pageName, QWidget * parent );

	void registerPlugin( ABAdminPlugin * plugin /Transfer/ );

	static ABAdminFactory * instance();
};
