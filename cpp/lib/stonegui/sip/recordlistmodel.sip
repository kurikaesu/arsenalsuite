

class RecordListModel : RecordSuperModel
{
%TypeHeaderCode
#include <recordlistmodel.h>
%End
public:
	RecordListModel( QObject * parent /TransferThis/ );

	void setRecordList( RecordList );
	RecordList recordList();
	
	void setColumn( const QString & column );
	QString column();

	void setColumns( QStringList columns );
	QStringList columns();

	virtual QVariant data ( const QModelIndex & index, int role ) const;

	virtual RecordList children( const Record & record );

	virtual QVariant recordData( const Record & record, int role, const QString & column ) const;
	virtual Qt::ItemFlags recordFlags( const Record & record, const QString & column ) const;
	virtual int compare( const Record & r1, const QString & column1, const Record & r2, const QString & column2 ) const;

	virtual bool setData( const QModelIndex &, const QVariant &, int );
	virtual bool setRecordData( const Record & record, const QString &, const QVariant &, int );

	virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;
	virtual int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
	virtual bool hasChildren ( const QModelIndex & parent = QModelIndex() ) const;
	QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
	QModelIndex parent ( const QModelIndex & index ) const;
	QModelIndex sibling( int row, int col, const QModelIndex & sib ) const;
	bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );

	QList<QModelIndex> insert( const QModelIndex & par, int rowStart, int cnt );

	void remove( const QModelIndex & i );
	bool removeRows ( int row, int count, const QModelIndex & parent = QModelIndex() );

	void sort ( int column, Qt::SortOrder order, bool recursive, int startRow=0, int endRow=-1 );
	void sort ( int column, Qt::SortOrder order = Qt::AscendingOrder );
	virtual void sort ( int column, Qt::SortOrder order, bool recursive, const QModelIndex & parent, int startRow=0, int endRow=-1 );

	virtual Record getRecord( const QModelIndex & i ) const;
	virtual void updateIndex( const QModelIndex & );
	virtual void setupIndex( const QModelIndex &, const Record & );

	void setupChildren( const QModelIndex & parent, const RecordList & rl );	
	void setRootList( const RecordList & rl );

};

