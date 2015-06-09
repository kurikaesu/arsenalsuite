
%Module blur.Stone 0

%Import QtCore/QtCoremod.sip
%Import QtGui/QtGuimod.sip
%Import QtSql/QtSqlmod.sip
%Import QtXml/QtXmlmod.sip

%DefaultSupertype sip.wrapper

%ModuleHeaderCode
#include "blurqt.h"
#include "path.h"
#include "process.h"
#include "pyembed.h"
#include "graphite.h"
#include "expression.h"
%End

%Exception PythonException(SIP_Exception)
{
%TypeHeaderCode
#include <pyembed.h>
%End
%RaiseCode
	SIP_BLOCK_THREADS
	sipExceptionRef.restore();
	SIP_UNBLOCK_THREADS
%End
};

void initStone( const QStringList & );

QString stoneOptionsHelp();

bool initConfig( const QString & configName, const QString & logfile = QString() );

void initUserConfig( const QString & userConfigFile );

void shutdown();

IniConfig & config();
IniConfig & userConfig();

Multilog * log();

QString getUserName();

QStringList getLoggedInUsers();

void Log( const QString & message, int severity = 1, const QString & file = QString() );

bool sendEmail( QStringList recipients, const QString & subject, const QString & body, const QString & sender, QStringList attachments = QStringList() );

%Include connection.sip
%Include changeset.sip
%Include database.sip
%Include dateutil.sip
%Include expression.sip
%Include field.sip
%Include freezercore.sip
%Include graphite.sip
%Include index.sip
%Include indexschema.sip
%Include iniconfig.sip
%Include interval.sip
%Include joinedselect.sip
%Include multilog.sip
%Include path.sip
%Include process.sip
%Include pgconnection.sip
%Include pyembed.sip
%Include record.sip
%Include recordimp.sip
%Include recordlist.sip
%Include recordproxy.sip
%Include recordxml.sip
%Include resultset.sip
%Include schema.sip
%Include sqlerrorhandler.sip
%Include table.sip
%Include tableschema.sip
%Include trigger.sip
%Include updatemanager.sip
