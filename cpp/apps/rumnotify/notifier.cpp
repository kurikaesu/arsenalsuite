

#include <qapplication.h>

#include "notifier.h"
#include "database.h"
#include "updatemanager.h"

Notifier::Notifier( const QString & command, const QString & table, const Q3ValueList<uint> & keys )
: QObject( 0 )
, mCommand( command )
, mTable( table )
, mKeys( keys )
{
	// Wait for the udpate manager to connect
	connect( UpdateManager::instance(), SIGNAL( statusChanged( bool ) ), SLOT( slotUpdateManagerStatusChange( bool ) ) );
}


void Notifier::slotUpdateManagerStatusChange( bool status )
{
	if( status ) {
		Table * table = Database::instance()->tableByName( mTable );
		if( !table ) {
			qWarning( "Unknown table name: " + mTable );
			
		} else {
			if( mCommand == "DELETE" ) {
			
				UpdateManager::instance()->recordsDeleted( table->tableName(), mKeys );
			} else {
			
				RecordList records = table->records( mKeys );
				
				if( mCommand == "INSERT" )
					UpdateManager::instance()->recordsAdded( table, records );
				else if( mCommand == "UPDATE" )
					foreach( Record r, records )
						UpdateManager::instance()->recordUpdated( table, r, r, true /* Send all fields */ );
			}
			UpdateManager::instance()->sendBuffer();
		}
	}
	qApp->quit();
}


