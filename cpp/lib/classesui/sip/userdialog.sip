
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
	Employee employee() const;
	
	/**
	 * Sets the information in the dialog
	 * to the information stored in user
	 **/
	void setUser( const User & );
	void setEmployee( const Employee & );
	
	virtual void accept();
public slots:

	void slotEmpToggle( bool );

	void slotEditGroups();
	
};

