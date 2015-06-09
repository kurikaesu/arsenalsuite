
#include "path.h"

#include "host.h"
#include "mapping.h"
#include "serverfileaction.h"
#include "serverfileactiontype.h"
#include "serverfileactionstatus.h"

Host hostFromPath( const QString & path )
{
	// Check actual mapping and see if we can get a host record
	char driveLetter = path[0].toLatin1();
	QString uncMapping = driveMapping(driveLetter);
	if( !uncMapping.isEmpty() ) {
		QStringList parts = uncMapping.mid(2).split("\\\\");
		QString hostName = parts[0];
		Host host = Host::recordByName(hostName);
		if( host.isRecord() ) return host;
	}

	// Check mappings table
	Mapping mapping = Mapping::select( "mount=? and fkeyhost is not null", VarList() << QString(driveLetter).toLower() )[0];
	if( mapping.isRecord() )
		return mapping.host();

	return Host();
}

bool prepareAction( ServerFileAction & action, const QString & path )
{
	Host h = hostFromPath(path);
	if( h.isRecord() ) {
		action.setHost( h );
		return true;
	}
	action.setErrorMessage( "Unable to determine server host for path: " + path );
	return false;
}

ServerFileAction ServerFileAction::remove( const QString & path )
{
	ServerFileAction action;
	if( prepareAction( action, path ) )
		action.setType( ServerFileAction::Delete ).setSourcePath( path ).commit();
	return action;
}

ServerFileAction ServerFileAction::move( const QString & source, const QString & dest )
{
	ServerFileAction action;
	if( prepareAction( action, source ) )
		action.setType( ServerFileAction::Move ).setSourcePath( source ).setDestPath( dest ).commit();
	return action;
}

static const char * statuses [] = {"New","Complete","Error", 0};

ServerFileAction::Status ServerFileAction::status()
{
	QString status = serverFileActionStatus().name();
	for( int i = 0; statuses[i]; i++ )
		if( statuses[i] == status )
			return Status(i);
	return Error;
}

ServerFileAction & ServerFileAction::setStatus( ServerFileAction::Status status )
{
	return setServerFileActionStatus( ServerFileActionStatus::recordByName( statuses[status] ) );
}

static const char * types [] = {"Delete","Move", 0};

ServerFileAction::Type ServerFileAction::type()
{
	QString type = serverFileActionType().name();
	for( int i = 0; types[i]; i++ )
		if( types[i] == type )
			return Type(i);
	return Move;
}

ServerFileAction & ServerFileAction::setType( ServerFileAction::Type type )
{
	return setServerFileActionType( ServerFileActionType::recordByName( types[type] ) );
}
