
class ModelTreeBuilder : QObject
{
%TypeHeaderCode
#include <supermodel.h>
%End
public:
	ModelTreeBuilder( SuperModel * parent );
	~ModelTreeBuilder();
	virtual bool hasChildren( const QModelIndex & parentIndex, SuperModel * model );
	virtual void loadChildren( const QModelIndex & parentIndex, SuperModel * model );
	//virtual int compare( const QModelIndex & a, const QModelIndex & b, QList<int> columns, bool asc );
	SuperModel * model();

protected:

	void setTranslator( ModelDataTranslator * trans );
	ModelDataTranslator * defaultTranslator();
	void addTranslator(ModelDataTranslator *);
};


class ModelDataTranslator /Abstract/
{
%TypeHeaderCode
#include <supermodel.h>
%End
public:

%ConvertToSubClassCode
	if( sipCpp->iface("RecordDataTranslatorInterface") )
		sipClass = sipClass_RecordDataTranslatorInterface;
	else
		sipClass = NULL;
%End

	// builder takes ownership of this
	ModelDataTranslator( ModelTreeBuilder * builder /TransferThis/ );
/*
	// These methods are used by SuperModel/ModelNode

	/// This function must return the same value throughout the life of the object
	virtual int dataSize() = 0;
	virtual QVariant modelData( void * dataPtr, const QModelIndex &, int role ) const = 0;
	virtual bool setModelData( void * dataPtr, const QModelIndex &, const QVariant & value, int role ) = 0;
	virtual Qt::ItemFlags modelFlags( void * dataPtr, const QModelIndex & ) const = 0;
	virtual int compare( void * dataPtr, void * dataPtr2, const QModelIndex & idx1, const QModelIndex & idx2, int column, bool asc ) const = 0;
	virtual void deleteData( void * dataPtr ) = 0;
	virtual void constructData( void * dataPtr ) = 0;
*/
	QModelIndex insert( int row, const QModelIndex & parent = QModelIndex() );
	QModelIndex append( const QModelIndex & parent = QModelIndex() );

//	static void * dataPtr( const QModelIndex & idx );
	static ModelDataTranslator * translator( const QModelIndex & idx );
	SuperModel * model();
};

struct ItemBase
{
%TypeHeaderCode
#include <supermodel.h>
%End
	bool operator==(const ItemBase & other) const;
	bool setModelData( const QModelIndex &, const QVariant &, int );
	Qt::ItemFlags modelFlags( const QModelIndex & );
	int compare( const QModelIndex & idx, const QModelIndex & idx2, int column, bool  );
};

class StandardTranslator : ModelDataTranslator
{
%TypeHeaderCode
#include <supermodel.h>
%End
public:
	StandardTranslator( ModelTreeBuilder * builder );
};

class SuperModel : QAbstractItemModel
{
%TypeHeaderCode
#include <supermodel.h>
%End
public:
%ConvertToSubClassCode
	if( sipCpp->inherits("RecordSuperModel") )
		sipClass = sipClass_RecordSuperModel;
	else
		sipClass = NULL;
%End

	SuperModel( QObject * parent );
	~SuperModel();

	ModelGrouper * grouper( bool autoCreate = true );
	void setGrouper( ModelGrouper * grouper );

	void setTreeBuilder( ModelTreeBuilder * treeBuilder );
	QModelIndex parent( const QModelIndex & idx ) const;

	QModelIndex sibling( int row, int column, const QModelIndex & idx );
	
	QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;

	int rowCount( const QModelIndex & parent = QModelIndex() ) const;

	bool hasChildren( const QModelIndex & parent ) const;

	int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
	
	QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;

	bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );

	bool dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );

	Qt::ItemFlags flags ( const QModelIndex & index ) const;

	QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

	QModelIndex append( const QModelIndex & par = QModelIndex(), ModelDataTranslator * trans = 0 );

	QList<QModelIndex> append( const QModelIndex & par, int cnt, ModelDataTranslator * trans = 0 );

	QModelIndex insert( const QModelIndex & par, int row, ModelDataTranslator * = 0 );

	QModelIndexList insert( const QModelIndex & par, int rowStart, int cnt, ModelDataTranslator * = 0 );

	void remove( const QModelIndex & i );
	void remove( QModelIndexList );
	bool removeRows( int start, int count, const QModelIndex & parent = QModelIndex() );
	void clear();

	void sort ( int column, Qt::SortOrder order = Qt::DescendingOrder);

	void sort ( int column, Qt::SortOrder order, bool recursive, int startRow=0, int endRow=-1 );

	void sort ( int column, Qt::SortOrder order, bool recursive, const QModelIndex & parent, int startRow=0, int endRow=-1 );

	void moveToTop( QModelIndexList indexes );

	bool childrenLoaded( const QModelIndex & );

	void clearChildren( const QModelIndex & i );

	void dataChange( const QModelIndex &, const QModelIndex & = QModelIndex() );

	bool autoSort() const;
	void setAutoSort( bool as );

	void setHeaderLabels( const QStringList & labels );
	QStringList headerLabels() const;

	// Backward compat, depreciated
	void setSortColumns( const QList<int> & sortColumns, bool forceResort = false );

// TODO: Need to write custom bindings
//	QList<SortColumnPair> sortColumns() const { return mSortColumns; }
//	void setSortColumns( const QList<SortColumnPair> & sortColumns, bool forceResort = false );

	Qt::SortOrder sortOrder( int column = -1 ) const;
	void setSortOrder( Qt::SortOrder order, int column = -1 );

	ModelTreeBuilder * treeBuilder();

	ModelDataTranslator * translator( const QModelIndex & ) const;

	struct InsertClosure {
		InsertClosure(SuperModel *);
		~InsertClosure();
	};
};
