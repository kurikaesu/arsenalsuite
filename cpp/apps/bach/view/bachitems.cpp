
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Bach.
 *
 * Bach is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Bach is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Bach; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: bachitems.cpp 9408 2010-03-03 22:35:49Z brobison $
 */

#include <qrect.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qdatetime.h>
#include <qheaderview.h>
#include <qtextdocument.h>
#include <qtextedit.h>
#include <qapplication.h>

#include "iniconfig.h"
#include "blurqt.h"
#include "imagesequencewidget.h"

#include "bachthumbnailloader.h"
#include "bachitems.h"

#include "bachasset.h"
#include "bachbucketmap.h"
#include "bachmainwindow.h"

#include "utils.h"

//-------------------------------------------------------------------------------------------------
namespace
{
	static const ColumnStruct AssetColumns [] =
	{
		{ "#", 			"Number",       24,		BachAssetItem::Column_Number,	false },
		{ "Preview", 	"Preview",    	250,	BachAssetItem::Column_Preview,	false },
		{ "Info", 		"Info", 		640,	BachAssetItem::Column_Info,		false },
		{ 0, 0, 0, 0, false }
	};
}

void setupAssetTree( RecordTreeView * lv )
{
  IniConfig & cfg = userConfig();
  cfg.pushSection( "BachAssetList" );
  lv->setupTreeView(cfg,AssetColumns);
  cfg.popSection();
}

void saveAssetTree( RecordTreeView * lv )
{
  IniConfig & cfg = userConfig();
  cfg.pushSection( "BachAssetList" );
  lv->saveTreeView(cfg,AssetColumns);
  cfg.popSection();
}



QSize ThumbDelegate::sizeHint(const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const
{
    QSize hint = BachMainWindow::tnSize();
		hint.rheight() += 4;
		hint.rwidth() += 4;
		return hint;
}

void ThumbDelegate::paint( QPainter * p, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    int x1, y1, x2, y2; //(x1, y1) is the upper-left corner, (x2, y2) the lower-right
    option.rect.getCoords(&x1, &y1, &x2, &y2);

    if( index.isValid() && index.column() == BachAssetItem::Column_Preview ) {
        if( option.state & QStyle::State_Selected ) {
            p->fillRect(option.rect, option.palette.brush(QPalette::Normal, QPalette::Highlight));
        } else {
            QItemDelegate::drawBackground( p, option, index );
        }
        BachAsset ba = ((RecordSuperModel*)(index.model()))->getRecord(index);
        if( ba.filetype() == 3 /* movie */ ) {
            p->save();
            p->setPen( QPen( QBrush("#9CC"), 2) );
            p->drawRect( option.rect );
            p->restore();
        }
        QPixmap icon(index.data(Qt::DecorationRole).value<QPixmap>());
        QRect bounds(icon.rect());
        bounds.moveCenter(option.rect.center());
        p->drawPixmap( bounds, icon  );
    }
}

void ThumbDelegate::setEditorData( QWidget * editor, const QModelIndex & index ) const
{
	if( index.column() == BachAssetItem::Column_Info ) {
		QTextEdit * textEdit = static_cast<QTextEdit*>(editor);
		QVariant displayData = index.model()->data(index, Qt::DisplayRole);
		textEdit->setPlainText(displayData.toString());
	}
}

QWidget * ThumbDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex & index ) const
{
	//qWarning( "WrapTextDelegate::createEditor called" );
	if( index.column() == BachAssetItem::Column_Preview ) {
		BachAsset ba = ((RecordSuperModel*)(index.model()))->getRecord(index);
		if( ba.path().toLower().endsWith(".mov") || ba.path().toLower().endsWith(".avi" ) ) {
			ImageSequenceWidget * editor = new ImageSequenceWidget( parent );
			editor->setShowControls(false);
			editor->setLoop(true);
			editor->setInputFile( ba.path() );
			editor->playClicked();
			return editor;
		}
	}
	return new QWidget(parent);
}

void ThumbDelegate::updateEditorGeometry(QWidget *editor,
      const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
	qWarning( "WrapTextDelegate::updateEditorGeometry called" );
	editor->setGeometry(option.rect);
}

QSize WrapTextDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSize newSize = QItemDelegate::sizeHint( option, index );
	newSize.setHeight(190);
        return newSize;
}

void WrapTextDelegate::paint( QPainter * p, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
	int x1, y1, x2, y2; //(x1, y1) is the upper-left corner, (x2, y2) the lower-right
	option.rect.getCoords(&x1, &y1, &x2, &y2);
	p->save();

	int lineWidths = 4;
	// draw a line below each row
	p->setPen( QPen( QBrush("#9CC"), lineWidths) );
	p->drawLine(x1, y2, x2, y2);
	// draw a line between each column
	p->setPen( QPen( QBrush("#CCC"), lineWidths) );
	p->drawLine( x2, y1+((y2-y1)/5), x2, y2-((y2-y1)/5));

	p->restore();

	if( index.isValid() && index.column() == BachAssetItem::Column_Info )
	{
		BachAsset ba = ((RecordSuperModel*)(index.model()))->getRecord(index);

		QRect pathRect(x1+5, y1+5, x2-x1-10, ((y2-y1)/3)-10);
		QRect tagsRect(pathRect.x(), pathRect.y()+pathRect.height()+10, x2-x1-10, y2-y1-10);
		QRect keywordsRect(pathRect.x(), pathRect.y()+pathRect.height()+30, x2-x1-10, y2-y1-10);
		QRect fullSize;

		p->save();
		QItemDelegate::drawBackground( p, option, index );
		if( option.state & QStyle::State_Selected )
			p->setPen( option.palette.highlightedText().color() );
		else
			p->setPen( option.palette.text().color() );

		p->drawText( pathRect, Qt::TextWordWrap, ba.path(), &fullSize );
		p->drawText( tagsRect, Qt::TextWordWrap, QString("Tags: ")+ba.tags(), &fullSize );
		p->drawText( keywordsRect, Qt::TextWordWrap, QString("Keywords: ")+ba.cachedKeywords(), &fullSize );

		p->setPen( QPen( QBrush("#9CC"), 2) );
		p->drawLine(x1+10, y1+((y2-y1)/3), x2-10, y1+((y2-y1)/3));

		//QRect dateRect(tagsRect.bottomRight().x()-40,tagsRect.bottomRight().y()-14, tagsRect.bottomRight().x(), tagsRect.bottomRight.y());

		QFont curFont = option.font;
		p->setFont( QFont(curFont.family(), curFont.pointSize()-2, curFont.weight(), true ) );
		curFont.setPointSize( curFont.pointSize()-1 );
		p->setPen( QColor("#CCC") );

		QRect sizeRect(x1+20,y2-20,x1+100,y2-4);
		p->drawText( sizeRect, Qt::TextWordWrap, QString::number(ba.width())+"x"+QString::number(ba.height()), &fullSize );

		p->setFont(option.font);

		p->restore();
		return;
	}
	else
	{
		QStyleOptionViewItem newOption = QStyleOptionViewItem(option);
		newOption.displayAlignment = Qt::AlignHCenter | Qt::AlignVCenter;
		newOption.decorationAlignment = Qt::AlignHCenter | Qt::AlignVCenter;
		return QItemDelegate::paint( p, newOption, index );
	}
}

void WrapTextDelegate::setEditorData( QWidget * editor, const QModelIndex & index ) const
{
	//qWarning( "WrapTextDelegate::setEditorData called" );
	if( index.column() == BachAssetItem::Column_Info ) {
		QTextEdit * textEdit = static_cast<QTextEdit*>(editor);
		QVariant displayData = index.model()->data(index, Qt::DisplayRole);
		textEdit->setPlainText(displayData.toString());
	}
}

QWidget * WrapTextDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex & index ) const
{
	//qWarning( "WrapTextDelegate::createEditor called" );
	if( index.column() == BachAssetItem::Column_Preview ) {
		BachAsset ba = ((RecordSuperModel*)(index.model()))->getRecord(index);
		if( ba.path().toLower().endsWith(".mov") || ba.path().toLower().endsWith(".avi" ) ) {
			ImageSequenceWidget * editor = new ImageSequenceWidget( parent );
			editor->setShowControls(false);
			editor->setLoop(true);
			editor->setInputFile( ba.path() );
			editor->playClicked();
			return editor;
		}
	} else if ( index.column() == BachAssetItem::Column_Info ) {
		QTextEdit * editor = new QTextEdit( index.data().toString(), parent);
		return editor;
	}
	return new QWidget(parent);
}

void WrapTextDelegate::updateEditorGeometry(QWidget *editor,
      const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
	qWarning( "WrapTextDelegate::updateEditorGeometry called" );
	editor->setGeometry(option.rect);
}

void WrapTextDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
	qWarning( "WrapTextDelegate::setModelData called" );
	QTextEdit * textEdit = static_cast<QTextEdit*>(editor);

	if ( index.column() == BachAssetItem::Column_Info ) {
		BachAsset ba = ((RecordSuperModel*)model)->getRecord(index);
		ba.setTags( textEdit->toPlainText() );
		ba.commit();
		model->setData(index, QVariant(textEdit->toPlainText()), Qt::EditRole );
	}
}

QVariant civ( const QColor & c )
{
	if( c.isValid() )
		return QVariant(c);
	return QVariant();
}

void BachAssetItem::setup( const Record & r, const QModelIndex & ) {
	mBachAsset = r;
	mPreviewLoaded = false;
	//DEBG( "BachAssetItem::setup()"+QString::number( mBachAsset.position() )+":"+mBachAsset.path() );
    //mPreview = ThumbnailLoader::load( BachMainWindow::cacheRoot(), mBachAsset.path(), BachMainWindow::tnSize(), false );
}

QVariant BachAssetItem::modelData( const QModelIndex & i, int role ) const {
	int col = i.column();
	if( role == Qt::DisplayRole ) {
		// DEBG( "modelData: "+QString::number( i.row() ) + ": Tags: " + mBachAsset.tags() );
		switch( col ) {
			case Column_Number: return QString::number(i.row()+1);
			case Column_Preview: return "";
			case Column_Info: return mBachAsset.cachedKeywords();
		}
	}
	else if( role == Qt::DecorationRole ) {
		switch( col ) {
			case Column_Preview:
                                return ThumbnailLoader::load( BachMainWindow::cacheRoot(),
															  mBachAsset.path(),
															  BachMainWindow::tnSize(),
															  mBachAsset.tnRotate(),
															  false );
                                break;
		}
	}
	else if ( role == Qt::UserRole )
	{
		return QString::number( mBachAsset.position() );
	}
	return QVariant();
}

BachAssetItem::BachAssetItem()
{
}

int BachAssetItem::compare( const QModelIndex & a, const QModelIndex & b, int,  bool )
{
	//DEBG( "modelData: "+QString::number( a.row() ) + ":" + QString::number( b.row() ) + ": Tags: " + mBachAsset.tags() );
	unsigned int ka = modelData( a, Qt::UserRole ).toUInt();
	unsigned int kb = modelData( b, Qt::UserRole ).toUInt();
	return ka - kb;
}

Qt::ItemFlags BachAssetItem::modelFlags( const QModelIndex & i ) {
	int col = i.column();
	if( col == Column_Preview || col == Column_Info ) {
		return Qt::ItemFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable );
	} else {
		return Qt::ItemFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
	}
}
Record BachAssetItem::getRecord() { return mBachAsset; }


bool BachBucketMapSorter::operator()( const BachBucketMap & r1, const BachBucketMap & r2 )
{
    QVariant v1 = r1.getValue( "position" );
    QVariant v2 = r2.getValue( "position" );

    QVariant::Type t1 = v1.type();
    QVariant::Type t2 = v2.type();
    if( t1 != t2 ) return false;

    int p1 = v1.toInt();
    int p2 = v2.toInt();
    if ( v1.isNull() )
    	p1 = INT_MAX;
    if ( v2.isNull() )
    	p2 = INT_MAX;
    return p1 < p2;
}

bool GetAssets( const BachBucket & a_BachBucket,bool a_ShowExclude, BachAssetList & o_List )
{
	BachBucketMapList bbml = a_BachBucket.bachBucketMaps();
	std::vector<Record> vec;
    st_foreach( RecordIter, it, bbml )
	{
        vec.push_back( *it );
	}
    std::sort( vec.begin(), vec.end(), BachBucketMapSorter() );
    BachBucketMapList ret;
    for( std::vector<Record>::iterator it = vec.begin(); it != vec.end(); ++it )
        ret += *it;
    bbml = ret;

	foreach( BachBucketMap bbm, bbml )
	{
		QVariant v1 = bbm.getValue( "position" );
		BachAsset ba = bbm.bachAsset();
		if ( ba.exclude() == a_ShowExclude )
			o_List.append( bbm.bachAsset() );
	}
	return true;
}
