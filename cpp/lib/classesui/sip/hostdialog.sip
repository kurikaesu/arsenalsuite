
class HostDialog : QDialog
{
%TypeHeaderCode
#include <hostdialog.h>
%End
public:
	HostDialog( QWidget * parent /TransferThis/ =0 );

	void setHost( const Host & h );
	Host host();
	virtual void accept();
	virtual void reject();

public slots:
	void slotEditServices();
	void slotEditInterfaces();
};
