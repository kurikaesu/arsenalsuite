
class UpdateManager : QObject
{
%TypeHeaderCode
#include <updatemanager.h>
%End
public:
	UpdateManager();

	static UpdateManager * instance();

signals:
	void statusChanged( bool connected );
};