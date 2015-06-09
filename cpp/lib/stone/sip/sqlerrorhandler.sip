
class SqlErrorHandler
{
%TypeHeaderCode
#include <sqlerrorhandler.h>
%End
public:
	virtual ~SqlErrorHandler();
	virtual void handleError(const QString & error);

	static void setInstance(SqlErrorHandler * /Transfer/);
	static SqlErrorHandler * instance();
};


