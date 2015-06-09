
#include "abadminplugins.h"
#include "hostservicematrix.h"
#include "projectweightview.h"
#include "servicestatusview.h"

AssfreezerABAdminPlugin::AssfreezerABAdminPlugin()
{
	mPages += ABAdminPage(ABAdminPage::Widget, "Host Service Matrix", "", QPixmap());
	mPages += ABAdminPage(ABAdminPage::Widget, "Service Usage Stats", "", QPixmap());
	mPages += ABAdminPage(ABAdminPage::Widget, "Project Weighting", "", QPixmap());
}

QList<ABAdminPage> AssfreezerABAdminPlugin::pages()
{
	return mPages;
}

QWidget * AssfreezerABAdminPlugin::createWidget( const QString & pageName, QWidget * parent )
{
	if( pageName == "Host Service Matrix" ) {
		return new HostServiceMatrixWidget(parent);
	} else if( pageName == "Service Usage Stats" ) {
		return new ServiceStatusView(parent,0);
	} else if( pageName == "Project Weighting" ) {
		return new ProjectWeightView(parent);
	}
	return 0;
}
