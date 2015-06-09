
class Schema
{
%TypeHeaderCode
#include <schema.h>
%End
public:
	virtual ~Schema();

	TableSchema * tableByName( const QString & tableName ) const;
	TableSchema * tableByClass( const QString & className ) const;
	
	QList<TableSchema*> tables() const;
	
	void addTable( TableSchema * table /Transfer/);

	void removeTable( TableSchema * table, bool del = true );

	void tableRenamed( TableSchema * table, const QString & oldName );
	void tableClassRenamed( TableSchema * table, const QString & oldName );

	bool mergeXmlSchema( const QString & schemaFile, bool isFile = true, bool ignoreDocs = true, QList<TableSchema*> * tables /Out/ = 0, bool verbose = false );
	
	static Schema * createFromXmlSchema( const QString & schema, bool isFile = true, bool ignoreDocs = true, bool verbose = false );
	
	bool writeXmlSchema( const QString & dest, QString * xml = 0, QList<TableSchema*> tables = QList<TableSchema*>() ) const;

signals:
	void tableAdded( TableSchema * table );
	void tableRemove( TableSchema * table );
};

