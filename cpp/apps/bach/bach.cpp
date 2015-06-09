
#include "bach.h"
#include "database.h"
#include "schema.h"

Schema * bachSchema()
{
	static Schema * _bachSchema = 0;
	if( !_bachSchema ) {
		_bachSchema = new Schema();
		//bach_loader();
	}
	return _bachSchema;
}

Database * bachDb()
{
	return Database::current( bachSchema() );
}
