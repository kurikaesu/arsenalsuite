
class RecordItemBase : ItemBase
{
%TypeHeaderCode
#include <recordsupermodel.h>
%End
public:
	RecordList children( const QModelIndex & );
	Qt::ItemFlags modelFlags( const QModelIndex & );
};

class SipRecordItemBase : RecordItemBase
{
%TypeHeaderCode
#include <recordsupermodel.h>
%End
public:
	SipRecordItemBase(ModelTreeBuilder * builder);
};

class RecordDataTranslatorInterface : ModelDataTranslator /Abstract/
{
%TypeHeaderCode
#include <recordsupermodel.h>
%End
public:
	RecordDataTranslatorInterface(ModelTreeBuilder * builder /TransferThis/ );

	virtual QVariant recordData(const MappedRecord &, const QModelIndex &, int role) const = 0;
	virtual bool setRecordData(MappedRecord, const QModelIndex &, const QVariant &, int role) = 0;
	virtual Qt::ItemFlags recordFlags(const MappedRecord &, const QModelIndex &) const = 0;
	virtual int recordCompare( const MappedRecord &, const MappedRecord &, const QModelIndex &, const QModelIndex &, int, bool ) const = 0;
	virtual RecordList recordChildren( const MappedRecord &, const QModelIndex & ) const = 0;

	virtual MappedRecord getRecord(const QModelIndex &) = 0;
	virtual void setup(const QModelIndex & idx, const Record & record ) = 0;
	virtual QModelIndexList insertRecordList(int row, RecordList & rl, const QModelIndex & parent = QModelIndex() ) = 0;
	virtual QModelIndexList appendRecordList(RecordList rl, const QModelIndex & parent = QModelIndex() );
};

class RecordDataTranslatorBase : RecordDataTranslatorInterface /Abstract/
{
%TypeHeaderCode
#include <recordsupermodel.h>
%End
public:
	RecordDataTranslatorBase(ModelTreeBuilder * builder /TransferThis/ );
	~RecordDataTranslatorBase();

	virtual QVariant recordData(const MappedRecord &, const QModelIndex &, int role) const;
	virtual bool setRecordData(MappedRecord, const QModelIndex &, const QVariant &, int role);
	virtual Qt::ItemFlags recordFlags(const MappedRecord &, const QModelIndex &) const;
	virtual int recordCompare( const MappedRecord &, const MappedRecord &, const QModelIndex &, const QModelIndex &, int, bool ) const;
	virtual RecordList recordChildren( const MappedRecord &, const QModelIndex & ) const;

	void setRecordColumnList( QStringList columns, bool defaultEditable = false );
	void setColumnEditable( int column, bool editable );
	QString recordColumnName( int column ) const;
	int recordColumnPos( int column, const Record & ) const;
	bool columnIsEditable( int column ) const;
	TableSchema * columnForeignKeyTable( int column ) const;
};

class SipRecordDataTranslatorBase : RecordDataTranslatorBase
{
%TypeHeaderCode
#include <recordsupermodel.h>
%End
public:
	SipRecordDataTranslatorBase(ModelTreeBuilder * builder /TransferThis/ );
	~SipRecordDataTranslatorBase();

	virtual Record getRecord(const QModelIndex &) const;

	virtual QVariant modelData( void * dataPtr, const QModelIndex &, int role ) const;
	virtual bool setModelData( void * dataPtr, const QModelIndex &, const QVariant &, int);
	virtual void setup(const QModelIndex & idx, const Record & record );
	virtual Qt::ItemFlags modelFlags( void * dataPtr, const QModelIndex & ) const;
	virtual int compare( void * dataPtr, void * dataPtr2, const QModelIndex & idx1, const QModelIndex & idx2, int column, bool asc ) const;

	virtual QModelIndexList insertRecordList(int row, RecordList & rl /TransferThis/, const QModelIndex & parent = QModelIndex() );
};

class RecordDataTranslator : RecordDataTranslatorBase
{
%TypeHeaderCode
#include <recordsupermodel.h>
%End
public:
	RecordDataTranslator(ModelTreeBuilder * builder /TransferThis/ );
	static MappedRecord getRecordStatic(const QModelIndex &idx);
	virtual MappedRecord getRecord(const QModelIndex &idx);
	virtual void setup(const QModelIndex & idx, const Record & record );
	virtual QModelIndexList insertRecordList( int row, RecordList & rl, const QModelIndex & parent );
};

class RecordTreeBuilder : ModelTreeBuilder
{
%TypeHeaderCode
#include <recordsupermodel.h>
%End
public:
	RecordTreeBuilder( SuperModel * model );

	virtual bool hasChildren( const QModelIndex & parentIndex, SuperModel * model );
	virtual void loadChildren( const QModelIndex & parentIndex, SuperModel * model );
};

class RecordSuperModel : SuperModel
{
%TypeHeaderCode
#include <recordsupermodel.h>
%End
public:
	
	RecordSuperModel( QObject * parent );

	MappedRecordList rootList() const;

	MappedRecord getRecord(const QModelIndex & i) const;
	MappedRecordList getRecords( const QModelIndexList & list ) const;

	void updateIndex( const QModelIndex & i );

	bool setupIndex( const QModelIndex & i, const Record & r );

	QModelIndex findIndex( const Record & r, bool recursive = true, const QModelIndex & parent = QModelIndex(), bool loadChildren = true );
	QModelIndexList findIndexes( RecordList, bool recursive = true, const QModelIndex & parent = QModelIndex(), bool loadChildren = true );
	QModelIndex findFirstIndex( RecordList, bool recursive = true, const QModelIndex & parent = QModelIndex(), bool loadChildren = true );

	MappedRecordList listFromIS( const QItemSelection & is );

	QMimeData * mimeData(const QModelIndexList &indexes) const;
	QStringList mimeTypes() const;
	virtual bool dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );
	
	virtual void setupChildren( const QModelIndex & parent, const RecordList & rl );

	void listen( Table * t );

	QModelIndex append( const QModelIndex & par = QModelIndex(), ModelDataTranslator * trans = 0 );
	QList<QModelIndex> append( const QModelIndex & par, int cnt, ModelDataTranslator * trans = 0 );

	QModelIndex insert( const QModelIndex & par, int row, ModelDataTranslator * = 0 );
	QModelIndexList insert( const QModelIndex & par, int rowStart, int cnt, ModelDataTranslator * = 0 );

	void remove( const QModelIndex & i );
	void remove( QModelIndexList );

public slots:

	virtual QModelIndexList insert( RecordList rl, int row = 0, const QModelIndex & parent = QModelIndex(), RecordDataTranslatorInterface * trans = 0 );
	virtual QModelIndex append( const Record &, const QModelIndex & parent = QModelIndex(), RecordDataTranslatorInterface * trans = 0 );
	virtual QModelIndexList append( RecordList rl, const QModelIndex & parent = QModelIndex(), RecordDataTranslatorInterface * trans = 0 );
	virtual void remove( RecordList rl, bool recursive = false, const QModelIndex & parent = QModelIndex() );
	virtual void updated( RecordList rl, bool recursive = false, const QModelIndex & parent = QModelIndex(), bool loadChildren = true );
	virtual void updated( Record r );

	virtual void setRootList( const RecordList & );

	virtual void updateRecords( RecordList, const QModelIndex & parent = QModelIndex(), bool recursive = false, bool loadChildren = true );

protected:
	RecordDataTranslatorInterface * recordDataTranslator( const QModelIndex & i ) const;
	QModelIndexList findIndexesHelper( RecordList rl, bool recursive, const QModelIndex & index, bool retAfterOne = false, bool loadChildren = true );
};
