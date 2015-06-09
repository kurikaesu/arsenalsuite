


class ModelIter
{
%TypeHeaderCode
#include <modeliter.h>
%End
public:

	enum Filter {
		None,
		Checkable,
		NotCheckable,
		Tristate,
		NotTristate,
		Checked,
		Unchecked,
		PartiallyChecked,
		Selectable,
		NotSelectable,
		Selected,
		Unselected,
		Editable,
		NotEditable,
		Enabled,
		Disabled,
		DragEnabled,
		DragDisabled,
		DropEnabled,
		DropDisabled,
		Recursive,
		DescendOpenOnly,
		Hidden,
		NotHidden,
		StartAtEnd
	};

	static Filter fromCheckState( Qt::CheckState cs );

	ModelIter( QAbstractItemModel *, Filter filter = ModelIter::None, QItemSelectionModel * = 0, QTreeView * = 0 );
	
	ModelIter( const QModelIndex & index, Filter filter = ModelIter::None, QItemSelectionModel * = 0, QTreeView * = 0 );

	ModelIter( SuperModel * fm, Filter filter = ModelIter::None, QItemSelectionModel * sm = 0, QTreeView * tv = 0 );

	int depth();
	
	//QModelIndex operator*();
	QModelIndex current();

	//ModelIter & operator++()'
	ModelIter & next();

	//ModelIter & operator--();
	ModelIter & prev();

	ModelIter & last();
	ModelIter & first();

	bool isValid();

	QModelIndex findFirst( int column, const QVariant & value, int role = Qt::DisplayRole );

	static QModelIndexList collect( QAbstractItemModel * m, ModelIter::Filter f = ModelIter::None, QItemSelectionModel * sm = 0, QTreeView * tv = 0 );
	static QModelIndexList collect( QModelIndex & i, ModelIter::Filter f = ModelIter::None, QItemSelectionModel * sm = 0, QTreeView * tv = 0 );
};

