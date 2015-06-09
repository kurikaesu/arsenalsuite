

#include <qitemdelegate.h>

class RecordDelegate : public QItemDelegate
{
Q_OBJECT
public:
	RecordDelegate ( QObject * parent = 0 );

	static const int CurrentRecordRole = Qt::UserRole+1,
					FieldNameRole = Qt::UserRole+2;
	
	QWidget * createEditor ( QWidget *, const QStyleOptionViewItem &, const QModelIndex & ) const;
	void setModelData ( QWidget *, QAbstractItemModel *, const QModelIndex & ) const;
	void setEditorData ( QWidget *, const QModelIndex & ) const;
	bool editorEvent ( QEvent * event, QAbstractItemModel * model, const QStyleOptionViewItem & option, const QModelIndex & index );
	void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
};

