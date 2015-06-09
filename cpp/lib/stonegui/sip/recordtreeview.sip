


class ExtTreeView : QTreeView
{
%TypeHeaderCode
#include <recordtreeview.h>
%End
public:
	ExtTreeView( QWidget * parent /TransferThis/, ExtDelegate * /Transfer/ = 0 );

	void setModel( QAbstractItemModel * model /Transfer/ );

	QModelIndexList selectedRows();
	
	bool columnAutoResize( int col ) const;

	/// If col is -1, then all columns are set to autoResize
	void setColumnAutoResize( int col, bool autoResize );

	bool showBranches() const;
	void setShowBranches( bool showBranches );

	int sizeHintForColumn( int column ) const;

	void setShowGrid( bool showGrid );
	bool showGrid() const;

	void setGridColors( const QColor & gridColor, const QColor & gridHighlightColor );
	void getGridColors( QColor & gridColor, QColor & gridHighlightColor );

	static int State_ShowGrid;
/*
	void setupTreeView( IniConfig & ini, const ColumnStruct columns [] );
	void saveTreeView( IniConfig & ini, const ColumnStruct columns [] );

	void setupTreeView( const QString & group, const QString & key, const ColumnStruct columns [] );
	void saveTreeView( const QString & group, const QString & key, const ColumnStruct columns [] );
*/

	BusyWidget * busyWidget( bool autoCreate = true );
	
	void setPropagateGroupSelection( bool pgs );
	bool propagateGroupSelection() const;

public slots:
	void expandRecursive( const QModelIndex & index = QModelIndex(), int levels = -1 );
	void sortBySelection();

signals:
	void showMenu( const QPoint & pos, const QModelIndex & underMouse );
	void columnVisibilityChanged( int column, bool visible );
};


class RecordTreeView : ExtTreeView
{
%TypeHeaderCode
#include <recordtreeview.h>
%End
public:
	RecordTreeView( QWidget * parent /TransferThis/ );
	void setModel( QAbstractItemModel * model /Transfer/ );

	RecordSuperModel * model() const;

	RecordDelegate * delegate() const;

	QModelIndex findIndex( const Record & r, bool recursive = true, const QModelIndex & parent = QModelIndex(), bool loadChildren = true );
	QModelIndexList findIndexes( RecordList, bool recursive = true, const QModelIndex & parent = QModelIndex(), bool loadChildren = true );
	Record getRecord(const QModelIndex & i);

	Record current();

	RecordList selection();

	bool columnAutoResize( int col ) const;
	void setColumnAutoResize( int col, bool );

	int sizeHintForColumn( int column ) const;

public slots:
	void setSelection( const RecordList & rl );
	void setCurrent( const Record & r );
	void scrollTo( const QModelIndex & );
	void scrollTo( const Record & );
	void scrollTo( RecordList );

signals:
	void currentChanged( const Record & );
	void selectionChanged( RecordList );
	void clicked( const Record & );
	void showMenu( const QPoint &, const Record & underMouse, RecordList selected );
};
