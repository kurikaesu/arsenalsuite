
class UserDialog : QDialog
{
%TypeHeaderCode
#include <userdialog.h>
%End
public:
	UserDialog( QWidget * parent /TransferThis/ =0, ChangeSet changeSet = ChangeSet() );

	ChangeSet changeSet() const;
	/**
	 * Returns a User object set to the
	 * current information in the dialog
	 **/
	User user() const;
	
	/**
	 * Sets the information in the dialog
	 * to the information stored in user
	 **/
	void setUser( const User & );
	
	virtual void accept();
public slots:
	void slotEditGroups();	
};

