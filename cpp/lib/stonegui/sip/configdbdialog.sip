

class ConfigDBDialog : QDialog
{
%TypeHeaderCode
#include <configdbdialog.h>
%End
public:
	ConfigDBDialog( QWidget * parent /TransferThis/ = 0 );

	virtual void accept();
};

