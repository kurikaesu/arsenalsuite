
class TableSchema : QObject
{
%TypeHeaderCode
#include <table.h>
%End
public:
	TableSchema( Schema * schema /TransferThis/ );
	virtual ~TableSchema();
	
	Schema * schema() const;

	Table * table() const;

	TableSchema * parent() const;

	void setParent( TableSchema * parent );

	QList<TableSchema*> inherits();

	QList<TableSchema*> children();

	QList<TableSchema*> tableTree();

	bool isDescendant( const TableSchema * t ) const;

	Record load( QVariant * v );
	
	Record * createObject( const Record & r ) /Factory/;
	
	Record * newObject() /Factory/;

	void setTableName(const QString &);
	QString tableName();
	
	void setClassName( const QString & );
	QString className();

	QString docs() const;
	void setDocs( const QString & docs );

	QString primaryKey();

	uint primaryKeyIndex();

	uint fieldCount();

	QStringList fieldDisplayNames();
	QStringList fieldNames();

	QList<Field*> fields();

	QList<Field*> ownedFields();
	
	QList<Field*> columns();

	QList<Field*> ownedColumns();

	QList<Field*> localVariables();

	QList<Field*> ownedLocalVariables();

	Field * field( const QString & fieldName, bool silent=false );

	Field * field( int pos );
	
	bool addField( Field * field /Transfer/ );

	void removeField( Field * f /TransferBack/ );

	int fieldPos( const QString & field );

	uint columnCount();

	void setPreloadEnabled( bool );
	bool isPreloadEnabled() const;

	QString projectPreloadColumn() const;
	void setProjectPreloadColumn( const QString & );

	bool useCodeGen() const;
	void setUseCodeGen( bool useCodeGen );

	bool expireKeyCache() const;
	void setExpireKeyCache( bool ekc );

	bool baseOnly() const;
	void setBaseOnly( bool baseOnly );

	QList<IndexSchema*> indexes();
	void addIndex( IndexSchema * index /Transfer/ );
	void removeIndex( IndexSchema * index, bool dontDelete = false );

	IndexSchema * index( const QString & name ) const;

	uint firstColumnIndex() const;

	void addTrigger( Trigger * trigger /Transfer/ );
	void removeTrigger( Trigger * trigger /TransferBack/ );
	QList<Trigger*> triggers() const;

signals:
	void triggerAdded( Trigger * trigger );

protected:

	void recalcFieldPositions( int start = 0, bool skipSelf = false );

	void addChild( TableSchema * );
	void removeChild( TableSchema * table );
};
