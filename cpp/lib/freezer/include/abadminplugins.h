

#ifndef ABADMIN_PLUGINS_H
#define ABADMIN_PLUGINS_H

#include "abadminplugin.h"

class AssfreezerABAdminPlugin : public ABAdminPlugin
{
public:
	AssfreezerABAdminPlugin();
	virtual QList<ABAdminPage> pages();
	virtual QWidget * createWidget( const QString & pageName, QWidget * parent );
protected:
	QList<ABAdminPage> mPages;
};

ABADMIN_EXPORT_PLUGIN(AssfreezerABAdminPlugin)

#endif // ABADMIN_PLUGINS_H

