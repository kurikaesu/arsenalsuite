
class IndexSchema
{
%TypeHeaderCode
#include <indexschema.h>
%End
public:
	IndexSchema( const QString & name, TableSchema * parent /TransferThis/, bool isList=false );
	~IndexSchema();

	QString name() const;
	void setName( const QString & name );

	TableSchema * table() const;
	bool holdsList() const;
	void setHoldsList( bool holdsList );

	Field * field() const;
	void setField( Field * f );
	
	QList<Field*> columns() const;
	
	void addColumn( const QString & name );
	
	bool databaseIndex() const;
	void setDatabaseIndex( bool di );

	QString databaseWhere() const;
	void setDatabaseWhere( const QString & dw );
	
//	bool createDatabaseIndex();
	void setUseCache( bool uc );
	bool useCache() const;
};

