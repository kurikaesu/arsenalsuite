/*
 * Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
 * SVN_META_HEADURL = "$HeadURL: http://svn/drd/apps/bach/trunk/view/BachKeywordView.cpp $"
 * SVN_META_ID = "$Id: BachKeywordView.cpp 9408 2010-03-03 22:35:49Z brobison $"
 */

#include "BachKeywordView.h"
#include <assert.h>

#include <qfile.h>
#include <qurl.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qdatetime.h>
#include <qheaderview.h>
#include <qtextdocument.h>
#include <qlineedit.h>
#include <qapplication.h>
#include <qscrollbar.h>
#include <recorddrag.h>
#include <database.h>


#include "iniconfig.h"
#include "bachasset.h"
#include "bachassetlist.h"
#include "bachkeyword.h"
#include "bachkeywordmap.h"
#include "bachkeywordlist.h"
#include "utils.h"

extern bool bach_editor_mode;

//-------------------------------------------------------------------------------------------------
namespace
{
	static const ColumnStruct KeywordColumns [] =
	{
		{ "Has", 	"Has",      35,		BachKeywordView::Column_Has,	false },
		{ "Name", 	"Name",    	185,	BachKeywordView::Column_Name,	false },
		{ "Count", 	"Count",    50,		BachKeywordView::Column_Count,	false },
		{ 0, 0, 0, 0, false }
	};

	enum HasFlags
	{
		eNone			= 0x0000,
		eHas			= 0x0001,
		eGrey			= 0x0010,
		eAdd			= 0x0100,
		eDelete			= 0x0200
	};

	static QPixmap * s_GoToPixmap = NULL;
	static QPixmap * s_HasPixmap = NULL;
	static QPixmap * s_DeletePixmap = NULL;
	static QPixmap * s_AcceptPixmap = NULL;
	static QPixmap * s_AddPixmap = NULL;
	static QPixmap * s_AsteriskPixmap = NULL;
}

//-------------------------------------------------------------------------------------------------
void
BachKeywordView::LoadPixmaps()
{
	s_GoToPixmap = new QPixmap( ":/icons/res/go-next.png" );
	s_HasPixmap = new QPixmap( ":/icons/res/tick.png" );
	s_DeletePixmap = new QPixmap( ":/icons/res/delete.png" );
	s_AcceptPixmap = new QPixmap( ":/icons/res/accept.png" );
	s_AddPixmap = new QPixmap( ":/icons/res/add.png" );
	s_AsteriskPixmap = new QPixmap( ":/icons/res/asterisk_orange.png" );
}

//-------------------------------------------------------------------------------------------------
BachKeywordView::BachKeywordView( QWidget * parent )
:	RecordTreeView( parent )
,	m_MouseIn( false )
,	m_ShowAll( true )
,	m_CallbacksEnabled( false )
{
	mKeywordsModel = new RecordSuperModel( this );
	new BachKeywordTranslator( mKeywordsModel->treeBuilder() );
	mKeywordsModel->setHeaderLabels(QStringList() << "name");
	setModel( mKeywordsModel );
	setItemDelegateForColumn( Column_Has, new KeywordDelegate( this ) );
	setItemDelegateForColumn( Column_Name, new KeywordDelegate( this ) );
	// TODO: why is it "name" aswell?
	// setItemDelegateForColumn( Column_Name, new KeywordDelegate( this ) );

	connect( this, SIGNAL( clicked( const QModelIndex & ) ), SLOT( onClicked( const QModelIndex & ) ) );
	enableCallbacks();

    setSelectionMode( QAbstractItemView::SingleSelection );
    mKeywordsModel->sort(Column_Name, Qt::AscendingOrder);
    header()->setSortIndicator(Column_Name, Qt::AscendingOrder);
}

//-------------------------------------------------------------------------------------------------
void BachKeywordView::enableCallbacks()
{
	if ( m_CallbacksEnabled )
		return;
    connect( BachKeyword::table(), SIGNAL( updated( Record, Record ) ), mKeywordsModel, SLOT( updated( Record ) ) );
    connect( BachKeywordMap::table(), SIGNAL( added( RecordList ) ), SLOT( refresh() ) );
    connect( BachKeywordMap::table(), SIGNAL( removed( RecordList ) ), SLOT( refresh() ) );
    m_CallbacksEnabled = true;
}

//-------------------------------------------------------------------------------------------------
void BachKeywordView::disableCallbacks()
{
	if ( !m_CallbacksEnabled )
		return;

    disconnect( BachKeyword::table(), SIGNAL( updated( Record, Record ) ), mKeywordsModel, SLOT( updated( Record ) ) );
    disconnect( BachKeywordMap::table(), SIGNAL( added( RecordList ) ), NULL, SLOT( refresh() ) );
    disconnect( BachKeywordMap::table(), SIGNAL( removed( RecordList ) ), NULL, SLOT( refresh() ) );
    m_CallbacksEnabled = false;
}

//-------------------------------------------------------------------------------------------------
void BachKeywordView::loadState()
{
	IniConfig & cfg = userConfig();
	cfg.pushSection( "BachKeywordView" );
	setupTreeView( cfg, KeywordColumns );
	cfg.popSection();
}

//-------------------------------------------------------------------------------------------------
void BachKeywordView::saveState()
{
	IniConfig & cfg = userConfig();
	cfg.pushSection( "BachKeywordView" );
	saveTreeView( cfg,KeywordColumns );
	cfg.popSection();
}

//-------------------------------------------------------------------------------------------------
void
BachKeywordView::setSelectedAssets( const BachAssetList & a_BachAssets )
{
	mCurrentSelection = a_BachAssets.reloaded();

	RecordSuperModel * m = model();
	for ( int j = 0 ; j < m->rowCount() ; ++j )
	{
		QModelIndex idxHas = m->index( j, Column_Has );

		if ( mCurrentSelection.size() == 0 )
		{
			setFlags( idxHas, eNone );
			continue;
		}

		QModelIndex idxName = m->index( j, Column_Name );
		QString keyword = m->data( idxName, Qt::DisplayRole).toString();

		int flags = eNone;
		unsigned int containCount = 0;
		foreach ( BachAsset ba, mCurrentSelection )
		{
			if ( ba.cachedKeywords().split( "," ).contains( keyword ) )
				++containCount;
		}
		if ( containCount == mCurrentSelection.size() )
			flags = eHas;
		else if ( containCount == 0 )
			flags = eNone;
		else
			flags = eGrey | eHas;
		setFlags( idxHas, flags );

		// DBG( "Setting "+keyword+" to: "+QString::number( flags )+":"+QString::number( containCount )+":"+QString::number( mCurrentSelection.size() ) );
	}
}


//-------------------------------------------------------------------------------------------------
void BachKeywordView::keywordFilter( const QString & a_Filter )
{
	BachKeywordList obkl = BachKeyword::select();
	mCurrentFilter = a_Filter;
	if ( !mCurrentFilter.isEmpty() )
	{
		QStringList list = mCurrentFilter.split( " ", QString::SkipEmptyParts );
		BachKeywordList nbkl;
		foreach ( BachKeyword bk, obkl )
		{
			QString name = bk.name();

			int partCount = 0;
			foreach( QString s, list )
			{
				if ( name.contains( s, Qt::CaseInsensitive ) )
				{
					++partCount;
				}
			}

			if ( partCount == list.length() )
				nbkl.append( bk );
		}
		model()->setRootList( nbkl );
	}
	else
	{
		if ( mCurrentSelection.isEmpty() || m_ShowAll )
		{
			model()->setRootList( obkl );
		}
		else
		{
			BachKeywordList nbkl;
			foreach ( BachKeyword bk, obkl )
			{
				bool add = false;
				foreach ( BachAsset ba, mCurrentSelection )
				{
					if ( ba.cachedKeywords().split( "," ).contains( bk.name() ) )
					{
						add = true;
						break;
					}
				}
				if ( add )
					nbkl.append( bk );
			}
			model()->setRootList( nbkl );
		}
	}
	model()->sort( Column_Name, Qt::DescendingOrder );
	header()->setSortIndicator( Column_Name, Qt::DescendingOrder );
}


//-------------------------------------------------------------------------------------------------
void BachKeywordView::setShowAll( bool a_Checked )
{
	if ( m_ShowAll == a_Checked )
		return;

	m_ShowAll = a_Checked;
	refresh();
}

//-------------------------------------------------------------------------------------------------
void BachKeywordView::refresh()
{
    int offset = verticalScrollBar()->value();
	RecordList currentSelected = selection();
	keywordFilter( mCurrentFilter );
	setSelection( currentSelected );
	// TODO: remove this copy after stone has been fixed, the operator=() does not check for
	// self-assignment, clearing its content.
	BachAssetList copy = mCurrentSelection;
	setSelectedAssets( copy );
    update();
    repaint();
    verticalScrollBar()->setValue( offset );
}

//-------------------------------------------------------------------------------------------------
void BachKeywordView::dragEnterEvent( QDragEnterEvent * event )
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
void BachKeywordView::dragLeaveEvent( QDragEnterEvent * /*event*/ )
{
	mCurrentDropTarget = BachKeyword();
}

//-------------------------------------------------------------------------------------------------
void BachKeywordView::dragMoveEvent( QDragMoveEvent * event )
{
    if (event->source() == this)
    {
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}
	else
	{
		QModelIndex midx = indexAt( event->pos() );
		mCurrentDropTarget = mKeywordsModel->getRecord( midx );
		setSelection( RecordList( mCurrentDropTarget ) );
		event->acceptProposedAction();
	}
 }

//-------------------------------------------------------------------------------------------------
void BachKeywordView::dropEvent( QDropEvent * event )
{
    if (event->source() == this)
    {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    }
    else if ( mCurrentDropTarget.isValid() )
    {
    	if ( !RecordDrag::canDecode( event->mimeData() ) )
    		return;

    	BachKeywordMapList bkml = mCurrentDropTarget.bachKeywordMaps();

		BachKeywordMapList toAdd;

    	RecordList dropped;
    	RecordDrag::decode( event->mimeData(), &dropped );
    	for ( RecordIter it = dropped.begin() ; it != dropped.end() ; ++it )
    	{
    		BachAsset ba = *it;
    		BachKeywordMap bkm;
    		bkm.setBachKeyword( mCurrentDropTarget );
    		bkm.setBachAsset( ba );

    		BachKeywordMap fbkm = BachKeywordMap::recordByKeywordAndAsset( mCurrentDropTarget, ba );
    		if ( !fbkm.isRecord() )
    			toAdd.append( bkm );
    	}

    	toAdd.commit();
        event->acceptProposedAction();
        refresh();
    }
}

//-------------------------------------------------------------------------------------------------
void BachKeywordView::enterEvent( QEvent * /*event*/ )
{
	m_MouseIn = true;
	m_ItemUnderMouse = QModelIndex();
}

//-------------------------------------------------------------------------------------------------
void BachKeywordView::leaveEvent( QEvent * /*event*/ )
{
	m_MouseIn = false;
	if ( m_ItemUnderMouse.isValid() )
	{
		hoverLeave( m_ItemUnderMouse );
		m_ItemUnderMouse = QModelIndex();
	}
}

//-------------------------------------------------------------------------------------------------
void BachKeywordView::mouseMoveEvent( QMouseEvent * event )
{
	if ( !m_MouseIn )
	{
		return;
	}
	const QPoint & pos = event->pos();

	QModelIndex idx = indexAt( pos );

	if ( idx.column() != Column_Has )
	{
		hoverLeave( m_ItemUnderMouse );
		m_ItemUnderMouse = QModelIndex();
		return;
	}

	if ( m_ItemUnderMouse != idx )
	{
		hoverLeave( m_ItemUnderMouse );
		hoverEnter( idx );
		m_ItemUnderMouse = idx;
	}
}

//-------------------------------------------------------------------------------------------------
void BachKeywordView::onClicked( const QModelIndex & a_Idx )
{
	if ( mCurrentSelection.size() == 0 || !bach_editor_mode )
		return;

	int col = a_Idx.column();
	DEBG( "Clicked:"+QString::number( col ) );
	switch( col )
	{
		case Column_Has:
		{
			RecordSuperModel * m = model();
			BachKeyword bk = m->getRecord( a_Idx );
			int flags = getFlags( a_Idx );
			if ( ( flags & eHas ) && !( flags & eGrey ) )
			{
				// so remove it already!
				deleteHas( a_Idx );
				int removeCount = 0;
                foreach( BachAsset ba, mCurrentSelection )
				{
					BachKeywordMapList bkml = BachKeywordMap::recordsByKeywordAndAssets( bk, ba );
					foreach( BachKeywordMap bkm, bkml )
					{
						DEBG( "Removing: " + bkm.bachAsset().path() + ":" + bkm.bachKeyword().name() );
						bkm.remove();
						bkm.commit();
					}
					removeCount += bkml.size();
				}

				bk.setCount( bk.count() - removeCount );
				QModelIndex countIdx = m->index( a_Idx.row(), Column_Count );
				m->setData( countIdx, bk.count(), Qt::DisplayRole );
				DEBG( "del newcount: " + QString::number( bk.count() ) );
				m_ItemUnderMouse = QModelIndex();
			}
			else
			{
				addHas( a_Idx );
				int addCount = 0;
                foreach( BachAsset ba, mCurrentSelection )
				{
					DEBG( "Adding map: " + ba.path() + ":" + bk.name() );
					BachKeywordMap bkm;
					bkm.setBachAsset( ba );
					bkm.setBachKeyword( bk );
					bkm.commit();
					++addCount;
				}

				bk.setCount( bk.count() + addCount );
				QModelIndex countIdx = m->index( a_Idx.row(), Column_Count );
				m->setData( countIdx, bk.count(), Qt::DisplayRole );
				DEBG( "add newcount: " + QString::number( bk.count() ) );
				m_ItemUnderMouse = QModelIndex();
			}

			// BachKeywordMap::table()->clearIndexes();
		}
		break;
	}
	// setSelection( mCurrentSelection );
	// refresh();
}

//-------------------------------------------------------------------------------------------------
void BachKeywordView::setFlags( const QModelIndex & a_Idx, int a_Flags )
{
	if ( !a_Idx.isValid() )
		return;

	model()->setData( a_Idx, QVariant( a_Flags ), Qt::DisplayRole );
}

//-------------------------------------------------------------------------------------------------
int BachKeywordView::getFlags( const QModelIndex & a_Idx )
{
	if ( !a_Idx.isValid() )
		return eNone;

	return model()->data( a_Idx, Qt::DisplayRole ).toInt();
}

//-------------------------------------------------------------------------------------------------
void BachKeywordView::hoverLeave( const QModelIndex & a_Idx )
{
	if ( mCurrentSelection.isEmpty() || !bach_editor_mode )
		return;
	int flags = getFlags( a_Idx );
	if ( flags & eHas )
		if ( flags & eGrey )
			flags = eGrey | eHas;
		else
			flags = eHas;
	else
		flags = eNone;
	setFlags( a_Idx, flags );
}

//-------------------------------------------------------------------------------------------------
void BachKeywordView::hoverEnter( const QModelIndex & a_Idx )
{
	if ( mCurrentSelection.isEmpty() || !bach_editor_mode )
		return;
	int flags = getFlags( a_Idx );
	if ( flags == eHas )
		flags |= eDelete;
	else if ( flags & eHas && ( !flags & eGrey ) )
		flags |= eDelete;
	else
		flags |= eAdd;
	setFlags( a_Idx, flags );
}

//-------------------------------------------------------------------------------------------------
void BachKeywordView::addHas( const QModelIndex & a_Idx )
{
	setFlags( a_Idx, eHas );
}

//-------------------------------------------------------------------------------------------------
void BachKeywordView::deleteHas( const QModelIndex & a_Idx )
{
	setFlags( a_Idx, eNone );
}













//-------------------------------------------------------------------------------------------------
void BachKeywordItem::setup( const Record & r, const QModelIndex & /*i*/ )
{
	mBachKeyword = r;
    mBachKeyword.setCount( mBachKeyword.bachKeywordMaps().size() );
}

//-------------------------------------------------------------------------------------------------
QVariant BachKeywordItem::modelData( const QModelIndex & a_Idx, int role ) const
{
	int col = a_Idx.column();
	switch ( role )
	{
		case Qt::DisplayRole:
		{
			switch( col ) {
				case BachKeywordView::Column_Has:
					DEBG( "Getting hasKeyword: " + QString::number( mBachKeyword.hasKeyword() ) + " : " + mBachKeyword.name() );
					return QVariant( mBachKeyword.hasKeyword() );
				case BachKeywordView::Column_Name: return mBachKeyword.name();
				case BachKeywordView::Column_Count: return QVariant( mBachKeyword.count() );
			}
		} break;

		default: return QVariant();
	}
	return QVariant();
}

//-------------------------------------------------------------------------------------------------
bool BachKeywordItem::setModelData( const QModelIndex & a_Idx, const QVariant & a_Value, int a_Role )
{
	int col = a_Idx.column();
	switch ( a_Role )
	{
		case Qt::DisplayRole:
		{
			switch( col ) {
				case BachKeywordView::Column_Has:
				{
					DEBG( "Setting hasKeyword: " + QString::number( a_Value.toInt() ) + " : " + mBachKeyword.name() );
					mBachKeyword.setHasKeyword( a_Value.toInt() );
					return true;
				}
				case BachKeywordView::Column_Count:
				{
					mBachKeyword.setCount( a_Value.toInt() );
					return true;
				}
			} break;
		} break;
	}
	return false;
}

//-------------------------------------------------------------------------------------------------
QString BachKeywordItem::sortKey( const QModelIndex & i ) const
{
	return modelData(i,Qt::DisplayRole).toString().toUpper();
}

//-------------------------------------------------------------------------------------------------
int BachKeywordItem::compare( const QModelIndex & a, const QModelIndex & b, int a_Column,  bool )
{
	switch( a_Column )
	{
		case BachKeywordView::Column_Count:
		{
			QVariant qa = BachKeywordTranslator::data( a ).modelData( a, Qt::DisplayRole );
			QVariant qb = BachKeywordTranslator::data( b ).modelData( b, Qt::DisplayRole );
			int ca = qa.toInt();
			int cb = qb.toInt();
			return ca == cb ? 0 : ( ca > cb ? 1 : -1 );
		} break;
		case BachKeywordView::Column_Name:
		{
			QString ska = sortKey( a ), skb = BachKeywordTranslator::data(b).sortKey( b );
			return ska == skb ? 0 : ( ska > skb ? 1 : -1 );
		} break;
		case BachKeywordView::Column_Has:
		{
			QVariant qa = BachKeywordTranslator::data( a ).modelData( a, Qt::DisplayRole );
			QVariant qb = BachKeywordTranslator::data( b ).modelData( b, Qt::DisplayRole );
			int ca = qa.toInt();
			int cb = qb.toInt();
			return ca == cb ? 0 : ( ca > cb ? 1 : -1 );
		} break;
	}
	return 0;
}

//-------------------------------------------------------------------------------------------------
Qt::ItemFlags BachKeywordItem::modelFlags( const QModelIndex & a_Idx )
{
    if( a_Idx.column() == BachKeywordView::Column_Name )
        return Qt::ItemFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable );

    return Qt::ItemFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
}

//-------------------------------------------------------------------------------------------------
Record BachKeywordItem::getRecord()
{
	return mBachKeyword;
}

//-------------------------------------------------------------------------------------------------
void
KeywordDelegate::paint( QPainter * a_Painter, const QStyleOptionViewItem & a_Option, const QModelIndex & a_Idx ) const
{
	if( !a_Idx.isValid() )
		return;

	int col = a_Idx.column();
	int t, l, b, r; //(x1, y1) is the upper-left corner, (x2, y2) the lower-right
	a_Option.rect.getCoords( &l, &t, &r, &b );

	BachKeyword bk = ( ( RecordSuperModel * )( a_Idx.model() ) )->getRecord( a_Idx );

	switch( col )
	{
	case BachKeywordView::Column_Name:
	{
		QItemDelegate::paint( a_Painter, a_Option, a_Idx );
	} break;
	case BachKeywordView::Column_Count:
	{
		QItemDelegate::paint( a_Painter, a_Option, a_Idx );
	} break;

	case BachKeywordView::Column_Has:
	{
		QItemDelegate::drawBackground( a_Painter, a_Option, a_Idx );

		int flags = bk.hasKeyword();

		if ( flags & eAdd )
			a_Painter->drawPixmap( a_Option.rect.topLeft(), *s_AddPixmap );
		else if ( flags & eDelete )
			a_Painter->drawPixmap( a_Option.rect.topLeft(), *s_DeletePixmap );
		else if ( flags & eGrey )
			a_Painter->drawPixmap( a_Option.rect.topLeft(), *s_AsteriskPixmap );
		else if ( flags & eHas )
			a_Painter->drawPixmap( a_Option.rect.topLeft(), *s_HasPixmap );

	} break;
	default:
		assert( false && "Delegation not enabled for this field" );
	}
}

//-------------------------------------------------------------------------------------------------
QWidget *
KeywordDelegate::createEditor( QWidget * a_Parent, const QStyleOptionViewItem & /*a_Option*/, const QModelIndex & a_Idx ) const
{
	if( !a_Idx.isValid() || !bach_editor_mode )
		return NULL;
	QLineEdit * editor = new QLineEdit( a_Idx.data().toString(), a_Parent );
	return editor;
}

//-------------------------------------------------------------------------------------------------
void
KeywordDelegate::setEditorData( QWidget * a_Editor, const QModelIndex & a_Idx ) const
{
	QLineEdit * textEdit = static_cast< QLineEdit* >( a_Editor );
	QVariant displayData = a_Idx.model()->data( a_Idx, Qt::DisplayRole );
	textEdit->setText( displayData.toString() );
}

//-------------------------------------------------------------------------------------------------
void
KeywordDelegate::updateEditorGeometry( QWidget * a_Editor, const QStyleOptionViewItem & a_Option, const QModelIndex & /*a_Idx*/ ) const
{
	a_Editor->setGeometry( a_Option.rect );
}

//-------------------------------------------------------------------------------------------------
void
KeywordDelegate::setModelData( QWidget * a_Editor, QAbstractItemModel * a_Model, const QModelIndex & a_Idx ) const
{
	if ( !bach_editor_mode )
		return;
	QLineEdit * textEdit = static_cast<QLineEdit*>( a_Editor );
	BachKeyword bk = ((RecordSuperModel*)a_Model)->getRecord( a_Idx );
	bk.setName( textEdit->text() );
	bk.commit();
	a_Model->setData( a_Idx, QVariant( textEdit->text() ), Qt::EditRole );
}

