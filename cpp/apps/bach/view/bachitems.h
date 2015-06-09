
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
 * $Id: bachitems.h 9408 2010-03-03 22:35:49Z brobison $
 */

#ifndef BACH_ITEMS_H
#define BACH_ITEMS_H

#include <qtreewidget.h>
#include <qdatetime.h>
#include <qitemdelegate.h>
#include <qpainter.h>
#include <qpixmap.h>

#include "recordsupermodel.h"
#include "blurqt.h"
#include "recordtreeview.h"

#include "bachasset.h"
#include "bachthumbnailloader.h"

#include <math.h>

// color if valid
QVariant civ( const QColor & c );

struct BachAssetItem : public RecordItem
{
	enum Columns
	{
		Column_Number = 0,
		Column_Preview = 1,
		Column_Info = 2,
	};
	BachAssetItem();

	BachAsset mBachAsset;
	QPixmap mPreview;
	bool mPreviewLoaded;

	void setup( const Record & r, const QModelIndex & );
	QVariant modelData( const QModelIndex & i, int role ) const;
	int compare( const QModelIndex & a, const QModelIndex & b, int, bool );
	Qt::ItemFlags modelFlags( const QModelIndex & );
	Record getRecord();
};

typedef TemplateRecordDataTranslator<BachAssetItem> BachAssetTranslator;

void setupAssetTree( RecordTreeView * );
void saveAssetTree( RecordTreeView * );

class WrapTextDelegate : public QItemDelegate
{
Q_OBJECT
public:
	WrapTextDelegate( QObject * parent=0 ) : QItemDelegate( parent ) {};
	~WrapTextDelegate() {}

	void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
	QSize sizeHint( const QStyleOptionViewItem & option, const QModelIndex &index ) const;
	QWidget * createEditor( QWidget *, const QStyleOptionViewItem &, const QModelIndex & ) const;
	void setEditorData( QWidget *, const QModelIndex & ) const;
	void updateEditorGeometry(QWidget *editor,
	         const QStyleOptionViewItem &option, const QModelIndex &index) const;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

};

class ThumbDelegate : public QItemDelegate
{
Q_OBJECT
public:
	ThumbDelegate( QObject * parent=0 ) : QItemDelegate( parent ) {};
	~ThumbDelegate() {}

	void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
        QSize sizeHint( const QStyleOptionViewItem & option, const QModelIndex &index ) const;
	QWidget * createEditor( QWidget *, const QStyleOptionViewItem &, const QModelIndex & ) const;
	void setEditorData( QWidget *, const QModelIndex & ) const;
	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;

};

class BachBucketMap;
class BachBucket;
class BachAssetList;

struct BachBucketMapSorter
{
    bool operator()( const BachBucketMap & r1, const BachBucketMap & r2 );
};

bool GetAssets( const BachBucket & a_BachBucket,bool a_ShowExclude, BachAssetList & o_List );


#endif // SHOT_ITEMS_H

