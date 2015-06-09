//-------------------------------------------------------------------------------------------------
/*
 * BachCollectionView.cpp
 *
 *  Created on: Jun 18, 2009
 *      Author: david.morris
 */

#include "BachCollectionView.h"
#include <assert.h>

#include <qfile.h>
#include <qurl.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qdatetime.h>
#include <qheaderview.h>
#include <qtextdocument.h>
#include <qtextedit.h>
#include <qapplication.h>
#include <recorddrag.h>
#include <qscrollbar.h>
#include <qlineedit.h>

#include "iniconfig.h"
#include "bachasset.h"
#include "bachbucket.h"
#include "bachbucketmap.h"
#include "bachbucketlist.h"
#include "utils.h"

extern bool bach_editor_mode;

//-------------------------------------------------------------------------------------------------
namespace
{
	static const ColumnStruct CollectionColumns [] =
	{
		{ "Name", 	"Name",    	250,	BachCollectionView::Column_Name,	false },
		{ "Count", 	"Count",    45,		BachCollectionView::Column_Count,	false },
		{ 0, 0, 0, 0, false }
	};
}

//-------------------------------------------------------------------------------------------------
BachCollectionView::BachCollectionView( QWidget * parent )
:	RecordTreeView( parent )
{
	mCollectionsModel = new RecordSuperModel( this );
	new BachBucketTranslator( mCollectionsModel->treeBuilder() );
	setModel( mCollectionsModel );

	setItemDelegateForColumn( Column_Count, new CollectionDelegate( this ) );
	setItemDelegateForColumn( Column_Name, new CollectionDelegate( this ) );

	connect( BachBucket::table(), SIGNAL( updated( Record, Record ) ), mCollectionsModel, SLOT( updated( Record ) ) );
	connect( BachBucketMap::table(), SIGNAL( added( RecordList ) ), SLOT( refresh() ) );
	connect( BachBucketMap::table(), SIGNAL( removed( RecordList ) ), SLOT( refresh() ) );

	setSelectionMode( QAbstractItemView::SingleSelection );
}

//-------------------------------------------------------------------------------------------------
void BachCollectionView::loadState()
{
	IniConfig & cfg = userConfig();
	cfg.pushSection( "BachCollectionView" );
	setupTreeView( cfg, CollectionColumns );
	cfg.popSection();
}

//-------------------------------------------------------------------------------------------------
void BachCollectionView::saveState()
{
	IniConfig & cfg = userConfig();
	cfg.pushSection( "BachCollectionView" );
	saveTreeView( cfg, CollectionColumns );
	cfg.popSection();
}


//-------------------------------------------------------------------------------------------------
void BachCollectionView::refresh()
{
    int offset = verticalScrollBar()->value();
	RecordList currentSelected = selection();
	collectionFilter( mCurrentFilter );
	model()->sort( Column_Name, Qt::DescendingOrder );
	header()->setSortIndicator( Column_Name, Qt::DescendingOrder );
	setSelection( currentSelected );
    verticalScrollBar()->setValue( offset );
}

//-------------------------------------------------------------------------------------------------
void BachCollectionView::dragEnterEvent( QDragEnterEvent * event )
{
    if (event->source() == this)
    {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    }
    else
    {
        event->acceptProposedAction();
    }
}

//-------------------------------------------------------------------------------------------------
void BachCollectionView::dragLeaveEvent( QDragEnterEvent * /*event*/ )
{
	mCurrentDropTarget = BachBucket();
}

//-------------------------------------------------------------------------------------------------
void BachCollectionView::dragMoveEvent( QDragMoveEvent * event )
{
    if (event->source() == this)
    {
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}
	else
	{
		QModelIndex midx = indexAt( event->pos() );
		mCurrentDropTarget = mCollectionsModel->getRecord( midx );
		setSelection( RecordList( mCurrentDropTarget ) );
		event->acceptProposedAction();
	}
 }

//-------------------------------------------------------------------------------------------------
void BachCollectionView::dropEvent( QDropEvent * event )
{
	if ( !bach_editor_mode )
		return;

    if (event->source() == this)
    {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    }
    else if ( mCurrentDropTarget.isValid() )
    {
    	if ( !RecordDrag::canDecode( event->mimeData() ) )
    		return;

    	BachBucketMapList bbml = mCurrentDropTarget.bachBucketMaps();
		DEBG( "Have:"+QString::number(bbml.size()) );

    	BachBucketMapList toAdd;

    	RecordList dropped;
    	RecordDrag::decode( event->mimeData(), &dropped );
    	for ( RecordIter it = dropped.begin() ; it != dropped.end() ; ++it )
    	{
    		BachAsset ba = *it;
    		BachBucketMap bbm;
    		bbm.setBachBucket( mCurrentDropTarget );
    		bbm.setBachAsset( ba );
    		DEBG( "Dropped:"+ba.path() );

    		BachBucketMap fbbm = BachBucketMap::recordByBucketAndAsset( mCurrentDropTarget, ba );
    		if ( !fbbm.isRecord() )
    			toAdd.append( bbm );
    	}

		DEBG( "To Add:"+QString::number(toAdd.size()) );
    	toAdd.commit();
        event->acceptProposedAction();
        refresh();
    }
}

//-------------------------------------------------------------------------------------------------
QString
BachCollectionView::getSelectedCollection()
{
	BachBucket bb = current();
	return bb.name();
}

//-------------------------------------------------------------------------------------------------
void
BachCollectionView::collectionFilter( const QString & a_Filter )
{
	BachBucketList obbl = BachBucket::select();
	mCurrentFilter = a_Filter;
	if ( !mCurrentFilter.isEmpty() )
	{
		QStringList list = mCurrentFilter.split( " ", QString::SkipEmptyParts );
		BachBucketList nbbl;
		foreach ( BachBucket bb, obbl )
		{
			QString name = bb.name();

			int partCount = 0;
			foreach( QString s, list )
			{
				if ( name.contains( s, Qt::CaseInsensitive ) )
				{
					++partCount;
				}
			}

			if ( partCount == list.length() )
				nbbl.append( bb );
		}
		model()->setRootList( nbbl );
	}
	else
	{
		model()->setRootList( obbl );
	}
	model()->sort( Column_Name, Qt::DescendingOrder );
	header()->setSortIndicator( Column_Name, Qt::DescendingOrder );
}


//-------------------------------------------------------------------------------------------------
void BachBucketItem::setup( const Record & r, const QModelIndex & /*i*/ )
{
	mBachBucket = r;
	mMappingsCount = mBachBucket.bachBucketMaps().count();
}

//-------------------------------------------------------------------------------------------------
QVariant BachBucketItem::modelData( const QModelIndex & a_Idx, int role ) const
{
	int col = a_Idx.column();
	switch ( role )
	{
		case Qt::DisplayRole:
		{
			switch( col ) {
				case BachCollectionView::Column_Name: return mBachBucket.name();
				case BachCollectionView::Column_Count: return QVariant( mMappingsCount );
			}
		} break;
	}
	return QVariant();
}

//-------------------------------------------------------------------------------------------------
QString BachBucketItem::sortKey( const QModelIndex & i ) const
{
	return modelData(i,Qt::DisplayRole).toString().toUpper();
}

//-------------------------------------------------------------------------------------------------
int BachBucketItem::compare( const QModelIndex & a, const QModelIndex & b, int a_Column,  bool )
{
	switch( a_Column )
	{
		case BachCollectionView::Column_Count:
		{
			QVariant qa = BachBucketTranslator::data( a ).modelData( a, Qt::DisplayRole );
			QVariant qb = BachBucketTranslator::data( b ).modelData( b, Qt::DisplayRole );
			int ca = qa.toInt();
			int cb = qb.toInt();
			return ca == cb ? 0 : ( ca > cb ? 1 : -1 );
		} break;
		case BachCollectionView::Column_Name:
		{
			QString ska = sortKey( a ), skb = BachBucketTranslator::data(b).sortKey( b );
			return ska == skb ? 0 : ( ska > skb ? 1 : -1 );
		} break;
	}
	return 0;
}

//-------------------------------------------------------------------------------------------------
Qt::ItemFlags BachBucketItem::modelFlags( const QModelIndex & i )
{
	int col = i.column();
	if( col == BachCollectionView::Column_Name )
	{
		return Qt::ItemFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable );
	}
	else
	{
		return Qt::ItemFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
	}
}

//-------------------------------------------------------------------------------------------------
Record BachBucketItem::getRecord()
{
	return mBachBucket;
}


//-------------------------------------------------------------------------------------------------
void
CollectionDelegate::paint( QPainter * a_Painter, const QStyleOptionViewItem & a_Option, const QModelIndex & a_Idx ) const
{
/*
	if( !a_Idx.isValid() )
		return;

	// int col = a_Idx.column();
	int t, l, b, r; //(x1, y1) is the upper-left corner, (x2, y2) the lower-right
	a_Option.rect.getCoords( &l, &t, &r, &b );

	BachBucket bk = ( ( RecordSuperModel * )( a_Idx.model() ) )->getRecord( a_Idx );
*/
	QItemDelegate::paint( a_Painter, a_Option, a_Idx );
}

//-------------------------------------------------------------------------------------------------
QWidget *
CollectionDelegate::createEditor( QWidget * a_Parent, const QStyleOptionViewItem & /*a_Option*/, const QModelIndex & a_Idx ) const
{
	if( !a_Idx.isValid() || !bach_editor_mode )
		return NULL;
	QLineEdit * editor = new QLineEdit( a_Idx.data().toString(), a_Parent );
	return editor;
}

//-------------------------------------------------------------------------------------------------
void
CollectionDelegate::setEditorData( QWidget * a_Editor, const QModelIndex & a_Idx ) const
{
	QLineEdit * textEdit = static_cast< QLineEdit* >( a_Editor );
	QVariant displayData = a_Idx.model()->data( a_Idx, Qt::DisplayRole );
	textEdit->setText( displayData.toString() );
}

//-------------------------------------------------------------------------------------------------
void
CollectionDelegate::updateEditorGeometry( QWidget * a_Editor, const QStyleOptionViewItem & a_Option, const QModelIndex & /*a_Idx*/ ) const
{
	a_Editor->setGeometry( a_Option.rect );
}

//-------------------------------------------------------------------------------------------------
void
CollectionDelegate::setModelData( QWidget * a_Editor, QAbstractItemModel * a_Model, const QModelIndex & a_Idx ) const
{
	if ( !bach_editor_mode )
		return;
	QLineEdit * textEdit = static_cast<QLineEdit*>( a_Editor );
	BachBucket bk = ((RecordSuperModel*)a_Model)->getRecord( a_Idx );
	bk.setName( textEdit->text() );
	bk.commit();
	a_Model->setData( a_Idx, QVariant( textEdit->text() ), Qt::EditRole );
}

