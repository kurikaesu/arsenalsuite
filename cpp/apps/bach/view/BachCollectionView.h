//-------------------------------------------------------------------------------------------------
/*
 * BachCollectionView.h
 *
 *  Created on: Jun 18, 2009
 *      Author: david.morris
 */

#ifndef BACHCOLLECTIONVIEW_H_
#define BACHCOLLECTIONVIEW_H_

#include <qevent.h>
#include <qwidget.h>
#include <qitemdelegate.h>

#include "recordtreeview.h"
#include "recordsupermodel.h"
#include "bachbucket.h"


//-------------------------------------------------------------------------------------------------
class BachCollectionView : public RecordTreeView
{
Q_OBJECT

public:
	enum Columns
	{
		Column_Name = 0,
		Column_Count = 1,
	};

	BachCollectionView( QWidget * parent );
	virtual ~BachCollectionView() {}

	QString getSelectedCollection();

	void collectionFilter( const QString & a_Filter );

	void loadState();
	void saveState();

public slots:
	void refresh();

protected:
	void dragEnterEvent( QDragEnterEvent * event );
	void dragLeaveEvent( QDragEnterEvent * event );
	void dragMoveEvent( QDragMoveEvent * event );
	void dropEvent( QDropEvent * event );
private:
	RecordSuperModel * mCollectionsModel;
	BachBucket mCurrentDropTarget;
	QString mCurrentFilter;
};

//-------------------------------------------------------------------------------------------------
struct BachBucketItem : public RecordItem
{
	BachBucket mBachBucket;
	int mMappingsCount;

	void setup( const Record & r, const QModelIndex & );
	QVariant modelData( const QModelIndex & i, int role ) const;
	QString sortKey( const QModelIndex & i ) const;
	int compare( const QModelIndex & a, const QModelIndex & b, int, bool );
	Qt::ItemFlags modelFlags( const QModelIndex & );
	Record getRecord();
};

//-------------------------------------------------------------------------------------------------
typedef TemplateRecordDataTranslator<BachBucketItem> BachBucketTranslator;

//-------------------------------------------------------------------------------------------------
class CollectionDelegate : public QItemDelegate
{
Q_OBJECT
public:
	CollectionDelegate( QObject * parent = NULL ) : QItemDelegate( parent ) {};
	~CollectionDelegate() {}

	void paint( QPainter * a_Painter, const QStyleOptionViewItem & a_Option, const QModelIndex & a_Idx ) const;
	QWidget * createEditor( QWidget * a_Editor, const QStyleOptionViewItem & a_Option, const QModelIndex & a_Idx ) const;
	void setEditorData( QWidget * a_Editor, const QModelIndex & a_Idx ) const;
	void updateEditorGeometry( QWidget * a_Editor, const QStyleOptionViewItem &option, const QModelIndex & a_Idx ) const;
	void setModelData( QWidget * a_Editor, QAbstractItemModel * a_Model, const QModelIndex & a_Idx ) const;

};



#endif /* BACHCOLLECTIONVIEW_H_ */
