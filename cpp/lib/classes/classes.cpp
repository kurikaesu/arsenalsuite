
#include "classes.h"
#include "database.h"
#include "schema.h"

Schema * classesSchema()
{
	static Schema * _classesSchema = 0;
	if( !_classesSchema ) {
		_classesSchema = new Schema();
	}
	return _classesSchema;
}

Database * classesDb()
{
	return Database::current( classesSchema() );
}
