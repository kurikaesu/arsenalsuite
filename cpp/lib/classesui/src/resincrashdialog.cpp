
#include <qhostinfo.h>

#include "blurqt.h"

#include "resincrashdialog.h"

ResinCrashDialog::ResinCrashDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi(this);
}

void ResinSqlErrorHandler::handleError( const QString & error )
{
	QString emailText;
	emailText += "Host: " + QHostInfo::localHostName() + "\n";
	emailText += "User: " + getUserName() + "\n\n";
	emailText += error;
	sendEmail( QStringList() << "newellm@blur.com", "Resin Sql Error", emailText, "thePipe@blur.com" );
	ResinCrashDialog * rcd = new ResinCrashDialog();
	rcd->setError( error );
	rcd->exec();
	delete rcd;
	qApp->exit();
	exit(1);
}

void ResinSqlErrorHandler::install()
{
	SqlErrorHandler::setInstance( new ResinSqlErrorHandler() );
}
