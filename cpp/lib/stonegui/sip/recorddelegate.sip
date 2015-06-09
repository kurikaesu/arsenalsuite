
class ExtDelegate : QItemDelegate
{
%TypeHeaderCode
#include <recorddelegate.h>
%End
public:
	ExtDelegate ( QObject * parent /TransferThis/ );

	QWidget * createEditor ( QWidget *, const QStyleOptionViewItem &, const QModelIndex & ) const;
	void setModelData ( QWidget *, QAbstractItemModel *, const QModelIndex & ) const;
	void setEditorData ( QWidget *, const QModelIndex & ) const;
	bool editorEvent ( QEvent * event, QAbstractItemModel * model, const QStyleOptionViewItem & option, const QModelIndex & index );
	void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
};


class RecordDelegate : ExtDelegate
{
%TypeHeaderCode
#include <recorddelegate.h>
%End
public:
	RecordDelegate ( QObject * parent /TransferThis/ );

	enum { CurrentRecordRole, FieldNameRole };
	
	QWidget * createEditor ( QWidget *, const QStyleOptionViewItem &, const QModelIndex & ) const;
	void setModelData ( QWidget *, QAbstractItemModel *, const QModelIndex & ) const;
	void setEditorData ( QWidget *, const QModelIndex & ) const;
	bool editorEvent ( QEvent * event, QAbstractItemModel * model, const QStyleOptionViewItem & option, const QModelIndex & index );
	void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
};

