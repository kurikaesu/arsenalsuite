
#include "projectweightdialog.h"

ProjectWeightDialog::ProjectWeightDialog(QWidget * parent)
: QDialog(parent)
{
	setupUi(this);
	connect( mRefreshButton, SIGNAL( clicked() ), mView, SLOT( refresh() ) );
}
