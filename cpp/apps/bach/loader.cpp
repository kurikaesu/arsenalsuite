
#include "database.h"
#include "connection.h"
#include "bach.h"
#include "bachassettable.h"
#include "bachbuckettable.h"
#include "bachbucketmaptable.h"
#include "bachhistorytable.h"
#include "bachkeywordtable.h"
#include "bachkeywordmaptable.h"
#include "bachnamespacetable.h"


BACH_EXPORT void bach_loader() {
	static bool classesLoaded = false;
	if( classesLoaded )
		return;
	classesLoaded = true;
	BachAssetSchema::instance();
	BachBucketSchema::instance();
	BachBucketMapSchema::instance();
	BachHistorySchema::instance();
	BachKeywordSchema::instance();
	BachKeywordMapSchema::instance();
	BachNamespaceSchema::instance();
	Database::setCurrent( new Database( bachSchema(), Connection::createFromIni( config(), "Database" ) ) );
}
