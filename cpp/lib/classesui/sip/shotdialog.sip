
class ShotDialog : QDialog
{
%TypeHeaderCode
#include <shotdialog.h>
%End
public:

   ShotDialog( const Element & element, QWidget * parent /TransferThis/ = 0 );

	void setShotNumber( float );

	void setShotName( const QString & name );
	
	Shot shotSetup();

	ShotList createdShots() const;
    
	virtual void accept();
	
public slots:
	void updateResult();
	void shotStartChange( const QString & value );
	void shotEndChange( const QString & value );
};



