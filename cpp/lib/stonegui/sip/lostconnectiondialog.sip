

class LostConnectionDialog : QDialog
{
%TypeHeaderCode
#include <lostconnectiondialog.h>
%End
public:
	LostConnectionDialog( Connection * c, const QString & errorMsg );

protected slots:

	// Override to prevent the user clicking the x close button
	virtual void reject();
	
	void retryConnection();
	void retryNow();
	void quit();
	void showSettings();
	
};
