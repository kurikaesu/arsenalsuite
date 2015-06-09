

class DateChooserDialog : QDialog
{
%TypeHeaderCode
#include "datechooserwidget.h"
%End
	DateChooserDialog( QWidget * parent );

public:
	static QDate getDate(QWidget*parent,const QDate & date);
};
