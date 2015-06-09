
#include <qtimer.h>

#include "projectreservedialog.h"

ProjectReserveDialog::ProjectReserveDialog(QWidget * parent)
: QDialog(0)
{
	Q_UNUSED(parent);

	setupUi(this);
	connect( mRefreshButton, SIGNAL( clicked() ), mView, SLOT( refresh() ) );
	QTimer::singleShot( 0, mView, SLOT( refresh() ) );
}
