
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of libstone.
 *
 * libstone is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libstone is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libstone; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: recordtreeview.cpp 13596 2012-09-17 23:43:49Z newellm $
 */

#include <qapplication.h>
#include <qevent.h>
#include <qheaderview.h>
#include <qmenu.h>
#include <QPlastiqueStyle>
#include <qpainter.h>
#include <qtimer.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qwidget.h>
#include <qgridlayout.h>
#include <qboxlayout.h>
#include <qscrollarea.h>
#include <qscrollbar.h>
#include <qlineedit.h>

#include "iniconfig.h"

#include "busywidget.h"
#include "recordtreeview.h"
#include "modelgrouper.h"
#include "recordfilterwidget.h"
#include "modeliter.h"
#include "supermodel.h"

int ExtTreeView::State_ShowGrid = 0x10000000;

ExtTreeView::ExtTreeView( QWidget * parent, ExtDelegate * delegate )
: QTreeView( parent )
, mDelegate( delegate )
, mColumnAutoResize( false )
, mShowBranches( true )
, mAutoResizeScheduled( false )
, mShowGrid( false )
, mRecordFilterWidget( 0 )
, mBusyWidget( 0 )
, mHeaderClickIsResize( false )
, mLastGroupColumn(-1)
, mPropagateGroupSelection(true)
{
	header()->setClickable( true );
	header()->setSortIndicatorShown( true );

	setContextMenuPolicy( Qt::CustomContextMenu );
	connect( this, SIGNAL( customContextMenuRequested( const QPoint & ) ), SLOT( slotCustomContextMenuRequested( const QPoint & ) ) );
	connect( header(), SIGNAL( sectionResized( int, int, int ) ), SLOT( slotSectionResized() ) );

	setSelectionMode( QAbstractItemView::ExtendedSelection );

#if QT_VERSION >= 0x040200
	setSortingEnabled( true );
#endif

	// QT4.1 specific stuff, leave out for now.
	//setAutoFillBackground( false );
	//viewport()->setAutoFillBackground( false );
	//setAttribute( Qt::WA_OpaquePaintEvent, true );
	//viewport()->setAttribute( Qt::WA_OpaquePaintEvent, true );

#if QT_VERSION <= 0x040202
//	setAttribute( Qt::WA_NoSystemBackground, true );
//	viewport()->setAttribute( Qt::WA_NoSystemBackground, true );
#endif
	setFrameStyle( QFrame::NoFrame );
	setRootIsDecorated( false );
	setAlternatingRowColors( true );
	
	if( !mDelegate )
		mDelegate = new ExtDelegate(this);
	setItemDelegate(mDelegate);
}

void ExtTreeView::addFilterLayout()
{
	QWidget * parentWidget = qobject_cast<QWidget*>(parent());

	QLayout * parentLayout = parentWidget->layout();
	delete parentLayout;

	QVBoxLayout * fooLay = new QVBoxLayout(parentWidget);
	fooLay->setContentsMargins(0,0,0,0);
	fooLay->setSpacing(0);
	fooLay->setAlignment(Qt::AlignLeft | Qt::AlignTop);

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	mRecordFilterWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

	fooLay->addWidget(mRecordFilterWidget);
	fooLay->addWidget(this);
}

void ExtTreeView::slotSectionResized()
{
	mHeaderClickIsResize = true;
}

bool ExtTreeView::eventFilter( QObject * object, QEvent * event )
{
	bool headerChild = false;
	QObject * ob = object;
	while( ob ) {
		if( ob == header() ) {
			headerChild = true;
			break;
		}
		ob = ob->parent();
	}

	if( headerChild )
	{
		switch ( event->type() ) {
			case QEvent::ChildAdded:
			case QEvent::ChildPolished:
			{
				QChildEvent * ce = (QChildEvent*)event;
				ce->child()->installEventFilter(this);
				if( object == header() )
					connect( header(), SIGNAL( sectionResized( int, int, int ) ), SLOT( slotSectionResized() ) );
				break;
			}
			default: break;
		}
	}

	if( headerChild && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) )
	{
		QMouseEvent * mouseEvent = (QMouseEvent*)event;
		if( mouseEvent->button() == Qt::RightButton && event->type() == QEvent::MouseButtonPress ) {
			int section = header()->logicalIndexAt(mouseEvent->pos());
			showHeaderMenu( ((QMouseEvent*)event)->globalPos(), section );
			return true;
		}
		// Control-click on a header column moves it to the second place in the sort order list
		// Eat the press event to keep the QHeaderView from starting a move operation
		if( mouseEvent->button() == Qt::LeftButton && event->type() == QEvent::MouseButtonPress ) {
			mHeaderClickIsResize = false;
			if(mouseEvent->modifiers() & Qt::ControlModifier)
				return true;
		}
		if( mouseEvent->button() == Qt::LeftButton && event->type() == QEvent::MouseButtonRelease && !mHeaderClickIsResize ) {
			SuperModel * sm = dynamic_cast<SuperModel*>(model());
			if( sm ) {
				int section = header()->logicalIndexAt(mouseEvent->pos());
				QList<SortColumnPair> sortColumns = sm->sortColumns();
				Qt::SortOrder so = Qt::DescendingOrder;
				
				bool secondColumnSort = (mouseEvent->modifiers() & Qt::ControlModifier);
				int checkExistingColumn = secondColumnSort ? 1 : 0;
				// Toggle the direction if this column is already in the second place
				bool toggleOrder = sortColumns.size() > checkExistingColumn && sortColumns[checkExistingColumn].first == section;
				for( QList<SortColumnPair>::iterator it = sortColumns.begin(); it != sortColumns.end(); ) {
					if( (*it).first == section ) {
						so = (*it).second;
						it = sortColumns.erase( it );
					} else
						++it;
				}
				if( toggleOrder )
					so = (so == Qt::DescendingOrder) ? Qt::AscendingOrder : Qt::DescendingOrder;
				
				sortColumns.insert( (secondColumnSort && sortColumns.size() >= 1) ? 1 : 0,SortColumnPair(section,so));
				sm->setSortColumns(sortColumns, /*forceResort = */ secondColumnSort );
				// We have to trick the damn QHeaderView into getting our sort direction correct, we make it think
				// it's already sorted on that column in the opposite order, so when the mouseRelease event is processed
				// it will reverse the order, send the correct order to the QTreeView, and call sort with the correct column and direction
				if( !secondColumnSort ) {
					header()->blockSignals(true);
					header()->setSortIndicator( section, so == Qt::AscendingOrder ? Qt::DescendingOrder : Qt::AscendingOrder );
					header()->blockSignals(false);
				}
				return secondColumnSort;
			}
		}
	}
	return false;
}


void ExtTreeView::showHeaderMenu( const QPoint & pos, int section )
{
	SuperModel * sm = dynamic_cast<SuperModel*>(model());

	QMenu * menu = new QMenu( this ), * columnVisibilityMenu = 0, * sortOrderMenu = 0;
	columnVisibilityMenu = menu->addMenu( "Column Visibility" );
	if( sm )
		sortOrderMenu = menu->addMenu( "Sort Order" );

	/* Column Visibility */
	QMap<QAction*,int> visibilityColumnMap, sortOrderColumnMap;
	QAction * sortBySelectionAction = 0;

	if( sm ) {
		sortBySelectionAction = sortOrderMenu->addAction( "Sort By Selection" );
		sortOrderMenu->addSeparator();
	}
	
	
	QStringList hls;// = mModel->headerLabels();
	QAbstractItemModel * mdl = model();
	for( int i = 0; i < mdl->columnCount(); ++i )
		hls += mdl->headerData(i,Qt::Horizontal,Qt::DisplayRole).toString();
	int i=0;
	foreach( QString hl, hls ) {
		QAction * act = columnVisibilityMenu->addAction( hl );
		act->setCheckable(true);
		if( !isColumnHidden(i) && columnWidth(i) > 0 )
			act->setChecked(true);
		visibilityColumnMap[act] = i++;
	}

	/* Sort order */
	if( sortOrderMenu && sm ) {
		QList<SortColumnPair> sortOrder = sm->sortColumns();
		foreach( SortColumnPair sc, sortOrder ) {
			QAction * act = sortOrderMenu->addAction( hls[sc.first] );
			sortOrderColumnMap[act] = sc.first;
		}
	}
	
	QAction * groupAction = 0, * ungroupAction = 0;
	if( sm && sm->grouper(false) ) {
		ModelGrouper * grouper = sm->grouper();;
		if( grouper->isGrouped() ) {
			int groupColumn = grouper->groupColumn();
			ungroupAction = menu->addAction( "Ungroup " + hls[groupColumn] );
		} else if( section >= 0 ) {
			groupAction = menu->addAction( "Group rows by " + hls[section] );
		} else 
			LOG_1( "Not a valid section" );
	} else
		LOG_1( "Not a grouping tree builder" );
	
	emit aboutToShowHeaderMenu( menu );
	
	QAction * ret = menu->exec( pos );
	delete menu;

	if( visibilityColumnMap.contains( ret ) ) {
		int ci = visibilityColumnMap[ret];
		bool currentlyHidden = isColumnHidden( ci ) || columnWidth( ci ) == 0;
		setColumnHidden( ci, !currentlyHidden );
		if( currentlyHidden && (columnAutoResize(ci) || columnWidth(ci) < 10) )
			resizeColumnToContents(ci);
		emit columnVisibilityChanged( ci, currentlyHidden );
	}
	else if( sortOrderColumnMap.contains( ret ) ) {
		int column = sortOrderColumnMap[ret];
		QList<SortColumnPair> sortColumns = sm->sortColumns();
		Qt::SortOrder so = Qt::DescendingOrder;
		// Toggle the direction if this column is already in the second place
		bool toggleOrder = sortColumns.size() >= 1 && sortColumns[0].first == column;
		for( QList<SortColumnPair>::iterator it = sortColumns.begin(); it != sortColumns.end(); ++it )
			if( (*it).first == column ) {
				so = (*it).second;
				it = sortColumns.erase( it );
			}
		if( toggleOrder )
			so = (so == Qt::DescendingOrder) ? Qt::AscendingOrder : Qt::DescendingOrder;
		sortColumns.push_front(SortColumnPair(column,so));
		sm->setSortColumns(sortColumns, /*forceResort = */ true );
	} else if( ret && ret == sortBySelectionAction ) {
		sortBySelection();
	} else if( ret && ret == groupAction ) {
		sm->grouper()->groupByColumn( section );
	} else if( ret && ret == ungroupAction ) {
		sm->grouper()->ungroup();
	}
}

void ExtTreeView::setColumnAutoResize( int column, bool acr )
{
	if( acr && !mColumnAutoResize ) {
		mColumnAutoResize = true;
		doAutoColumnConnections();
	}
	if( column == -1 ) {
		int cnt = model()->columnCount();
		mAutoResizeColumns.resize( cnt );
		for( int i=0; i<cnt; i++ )
			mAutoResizeColumns.setBit(i,acr);
		return;
	}
	if( acr ) {
		if( column >= mAutoResizeColumns.size() )
			mAutoResizeColumns.resize(column+1);
		mAutoResizeColumns.setBit(column);
	} else
		mAutoResizeColumns.setBit(column,false);
}


void ExtTreeView::doAutoColumnConnections()
{
	connect( model(), SIGNAL( dataChanged ( const QModelIndex &, const QModelIndex & ) ), SLOT( scheduleResizeAutoColumns() ) );
	connect( model(), SIGNAL( layoutChanged() ), SLOT( scheduleResizeAutoColumns() ) );
	connect( model(), SIGNAL( modelReset() ) , SLOT( scheduleResizeAutoColumns() ) );
}

void ExtTreeView::setShowGrid( bool showGrid )
{
	mShowGrid = showGrid;
}

bool ExtTreeView::showGrid() const
{
	return mShowGrid;
}

void ExtTreeView::setGridColors( const QColor & gridColor, const QColor & gridHighlightColor )
{
	mGridColor = gridColor;
	mGridColorHighlight = gridHighlightColor;
}

void ExtTreeView::getGridColors( QColor & gridColor, QColor & gridHighlightColor )
{
	gridColor = mGridColor;
	gridHighlightColor = mGridColorHighlight;
}

QModelIndexList ExtTreeView::selectedRows()
{
	QModelIndexList ret;
	QItemSelection sel = selectionModel()->selection();
	foreach( QItemSelectionRange range, sel ) {
		QModelIndex topLeft = range.topLeft();
		while(1) {
			ret += topLeft;
			if( topLeft.row() >= range.bottom() ) break;
			topLeft = topLeft.sibling(topLeft.row()+1,topLeft.column());
		}
	}
	return ret;
}

void ExtTreeView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
	if( mPropagateGroupSelection ) {
		SuperModel * sm = dynamic_cast<SuperModel*>(model());
		ModelGrouper * grp = sm->grouper(false);
		ModelDataTranslator * gt = grp ? grp->groupedItemTranslator() : 0;
		QItemSelectionModel * sel = selectionModel();
		if( sm && gt ) {
			QItemSelection selected2(selected);
			if( sel->selection().size() == 1 && selected.size() == 0 && deselected.size() )
				selected2 = sel->selection();
			QItemSelection toSelect, toDeselect;
			foreach( QItemSelectionRange range, selected2 ) {
				QModelIndex topLeft = range.topLeft();
				while(1) {
					if( gt == sm->translator(topLeft) )
						toSelect += QItemSelectionRange( topLeft.child( 0, topLeft.column() ), topLeft.child( sm->rowCount(topLeft) - 1, range.bottomRight().column() ) );
					if( topLeft.row() >= range.bottom() ) break;
					topLeft = topLeft.sibling(topLeft.row()+1,topLeft.column());
				}
			}
			foreach( QItemSelectionRange range, deselected ) {
				QModelIndex topLeft = range.topLeft();
				while(1) {
					if( gt == sm->translator(topLeft) )
						toDeselect += QItemSelectionRange( topLeft.child( 0, topLeft.column() ), topLeft.child( sm->rowCount(topLeft) - 1, range.bottomRight().column() ) );
					if( topLeft.row() >= range.bottom() ) break;
					topLeft = topLeft.sibling(topLeft.row()+1,topLeft.column());
				}
			}
			sel->select( toSelect, QItemSelectionModel::Select );
			sel->select( toDeselect, QItemSelectionModel::Deselect );
		}
	}

	if( !mShowGrid ) {
		QTreeView::selectionChanged(selected,deselected);
		return;
	}

	QRect rect = visualRegionForSelection(deselected).boundingRect();
	setDirtyRegion( QRegion( rect.adjusted(0,-1,0,0) ) );
	rect = visualRegionForSelection(selected).boundingRect();
	setDirtyRegion( QRegion( rect.adjusted(0,-1,0,0) ) );
}

void ExtTreeView::setModel( QAbstractItemModel * model )
{
	SuperModel * sm = dynamic_cast<SuperModel*>(model);
	if( sm ) {
		QList<SortColumnPair> sortColumns = sm->sortColumns();
		header()->setSortIndicator( sortColumns.size() ? sortColumns[0].first : 0, sm->sortOrder() );
		connect( model, SIGNAL( grouperChanged( ModelGrouper* ) ), SLOT( slotGrouperChanged( ModelGrouper * ) ) );
		// If the grouper is already set/created, then connect to it now
		if( sm->grouper(false) )
			slotGrouperChanged( sm->grouper() );
	}
	QTreeView::setModel( model );
	if( mColumnAutoResize )
		doAutoColumnConnections();
	header()->viewport()->installEventFilter( this );
	header()->installEventFilter( this );
}

bool ExtTreeView::columnAutoResize( int col ) const
{
	return col < mAutoResizeColumns.size() && col >= 0 && mAutoResizeColumns.at(col);
}

void ExtTreeView::scheduleResizeAutoColumns()
{
	if( !mAutoResizeScheduled ) {
		mAutoResizeScheduled = true;
		QTimer::singleShot( 0, this, SLOT( resizeAutoColumns() ) );
	}
}

void ExtTreeView::resizeAutoColumns()
{
	mAutoResizeScheduled = false;
	for( int i=0; i< qMin(model()->columnCount(),mAutoResizeColumns.size()); i++ ) {
		//LOG_5( "ExtTreeView::resizeAutoColumns: Checking column " + QString::number(i) );
		if( mAutoResizeColumns.at(i) ) {
			//LOG_5( "ExtTreeView::resizeAutoColumns: Resizing column " + QString::number(i) );
			resizeColumnToContents(i);
		}
	}
}

void ExtTreeView::expandRecursive( const QModelIndex & index, int levels )
{
	if( levels == 0 ) return;
	int rows = model()->rowCount(index);
	for( int i = 0; i < rows; i++ ) {
		QModelIndex child = model()->index( i,0,index );
		if( child.isValid() ) {
			setExpanded( child, true );
			if( levels > 1 || levels < 0 )
				expandRecursive( child, levels > 0 ? levels - 1 : -1 );
		}
	}
}

static bool modelIndexRowCmp( const QModelIndex & i1, const QModelIndex & i2 )
{
	return i1.row() < i2.row();
}

void ExtTreeView::sortBySelection()
{
	SuperModel * sm = dynamic_cast<SuperModel*>(model());
	QModelIndexList selected = selectedRows();
	qSort( selected.begin(), selected.end(), modelIndexRowCmp );
	if( sm )
		sm->moveToTop( selected );
}

int ExtTreeView::sizeHintForColumn( int column, int * height ) const
{
	QStyleOptionViewItem option = viewOptions();
	QAbstractItemDelegate *delegate = itemDelegate();
	QModelIndex index;
	QSize size;
	int w = 0;
	if( height ) *height = 0;
	ModelIter it( model(), ModelIter::Filter(ModelIter::Recursive | ModelIter::DescendOpenOnly), 0, const_cast<ExtTreeView*>(this) );
	for( ; it.isValid(); ++it ) {
		index = (*it).sibling((*it).row(), column);
		size = delegate->sizeHint(option, index);
		w = qMax(w, size.width() + (column == 0 ? indentation() * it.depth() : 0));
		if( height ) *height += size.height();
	}
	//LOG_5( "Calculated size hint: " + QString::number( w ) );
	return w;
}

int ExtTreeView::sizeHintForColumn ( int column ) const
{
	return sizeHintForColumn( column, 0 );
}

QSize ExtTreeView::allContentsSizeHint()
{
	int width = 0, height = 0;
	// Get height
	sizeHintForColumn(0,&height);
	height += header()->height();
	for( int i=0; i < model()->columnCount(); i++ )
		width += columnWidth(i);
	return QSize(width,height);
}

bool ExtTreeView::showBranches() const
{
	return mShowBranches;
}

void ExtTreeView::setShowBranches( bool showBranches )
{
	mShowBranches = showBranches;
	update();
}

void ExtTreeView::setPropagateGroupSelection( bool pgs )
{
	if( mPropagateGroupSelection != pgs ) {
		mPropagateGroupSelection = pgs;
	}
}

bool ExtTreeView::propagateGroupSelection() const
{
	return mPropagateGroupSelection;
}

QStyleOptionViewItem ExtTreeView::viewOptions() const
{
	QStyleOptionViewItem ret = QTreeView::viewOptions();
	if( !mShowGrid ) return ret;
	ret.state |= QStyle::StateFlag(State_ShowGrid);
	ret.palette.setColor( QPalette::Dark, mGridColor );
	ret.palette.setColor( QPalette::Light, mGridColorHighlight );
	return ret;
}

void ExtTreeView::startDrag ( Qt::DropActions supportedActions )
{
	QModelIndexList indexes = selectedIndexes();
	if (indexes.count() > 0) {
		QMimeData *data = model()->mimeData(indexes);
		if (!data)
			return;
		QRect rect;
		QDrag *drag = new QDrag(this);
		drag->setMimeData(data);
		drag->start(supportedActions);
	}
}

void ExtTreeView::drawBranches ( QPainter * p, const QRect & rect, const QModelIndex & index ) const
{
	const int indent = indentation();
	const int outer = rootIsDecorated() ? 0 : 1;

	QStyle::State extraFlags = QStyle::State_None;
	if (isEnabled())
		extraFlags |= QStyle::State_Enabled;
	if (window()->isActiveWindow())
		extraFlags |= QStyle::State_Active;
    const QAbstractItemModel * modelItem = index.model();
    QVariant bgColor = modelItem->data(index, Qt::BackgroundColorRole);

	QModelIndex parent = index.parent();
	int level = 0;
	{
		QModelIndex i = index.parent();
		while( i.isValid() ) {
			i = i.parent();
			level++;
		}
	}


	//RecordTreeView * rtc = const_cast<RecordTreeView*>(this);
	if( !mShowBranches )
	{
		if( level < outer || !model()->hasChildren(index) ) return;
		QRect primitive(rect.right() - indent, rect.top(), indent, rect.height());
		QStyleOption so(0);
		so.rect = primitive;
		so.state = QStyle::State_Item | extraFlags | QStyle::State_Children
			| (isExpanded(index) ? QStyle::State_Open : QStyle::State_None);
		QApplication::style()->drawPrimitive(QStyle::PE_IndicatorBranch, &so, p, this);
		return;
	}

	QPlastiqueStyle * ps = qobject_cast<QPlastiqueStyle*>(QApplication::style());
	if( ps ) {
		if( bgColor.isValid() && bgColor.type() == QVariant::Color && qvariant_cast<QColor>(bgColor).isValid() )
		{
			p->save();
			p->setPen(qvariant_cast<QColor>(bgColor));
			p->setBrush(qvariant_cast<QColor>(bgColor));
			p->drawRect(rect);
			p->restore();
		}

		QRect primitive(rect.right(), rect.top(), indent, rect.height());
	
		QModelIndex current = parent;
		QModelIndex ancestor = current.parent();
	
		QColor bc( 185, 195, 185 );

		//int origLevel = level;
		if (level >= outer) {
			// start with the innermost branch
			primitive.moveLeft(primitive.left() - indent);
	
			const bool expanded = isExpanded(index);
			const bool children = model()->hasChildren(index); // not layed out yet, so we don't know
			bool hasSiblingBelow = model()->rowCount(parent) - 1 > index.row();
			if( children ) {
				QStyleOption so(0);
				so.rect = primitive;
				so.state = QStyle::State_Item | extraFlags
                    | (hasSiblingBelow ? QStyle::State_Sibling : QStyle::State_None)
                    | (children ? QStyle::State_Children : QStyle::State_None)
                    | (expanded ? QStyle::State_Open : QStyle::State_None);
		        ps->drawPrimitive(QStyle::PE_IndicatorBranch, &so, p, this);
			} else {
				int h = primitive.top() + primitive.height()/2;
				int x = primitive.left() + primitive.width()/2;
				p->setPen( bc );
				p->drawLine( x, primitive.top(), x, hasSiblingBelow ? primitive.bottom() : h );
				p->drawLine( x, h, primitive.right(), h );
			}
		}
		// then go out level by level
		for (--level; level >= outer; --level) { // we have already drawn the innermost branch
			primitive.moveLeft(primitive.left() - indent);
			bool hasSiblingBelow = model()->rowCount(ancestor) - 1 > current.row();
			current = ancestor;
			ancestor = current.parent();
			//int h = primitive.top() + primitive.height()/2;
			int x = primitive.left() + primitive.width()/2;
			//int fade = 170;//20 + (origLevel-level)*20;
			p->setPen( bc );
			if( hasSiblingBelow )
				p->drawLine( x, primitive.top(), x, primitive.bottom() );
		}
		return;
	}
	QTreeView::drawBranches( p, rect, index );
}

void ExtTreeView::slotCustomContextMenuRequested( const QPoint & p )
{
	QModelIndex under = indexAt( p );
	emit showMenu( viewport()->mapToGlobal(p), under );
}

void ExtTreeView::setupColumns( IniConfig & ini, const ColumnStruct columns [] )
{
	QHeaderView * hdr = header();
	int cnt = 0;
	QStringList labels;
	for( cnt=0; columns[cnt].name; ++cnt );
	QVector<int> indexVec(cnt);
	for( int i=0; i<cnt; i++ ) {
		labels << QString::fromLatin1(columns[i].name);
		indexVec[i] = ini.readInt( columns[i].iniName + QString("Index"), columns[i].defaultPos );
	}
	SuperModel * sm = dynamic_cast<SuperModel*>(model());
	if( sm )
		sm->setHeaderLabels( labels );
	hdr->setStretchLastSection(false);
	for( int n=0; n<cnt; n++ ) {
		for( int i=0; i<cnt; i++ )
			if( indexVec[i] == n )
				hdr->moveSection( hdr->visualIndex(i), n );
	}
	hdr->resizeSections(QHeaderView::Stretch);
	for( int n=0; n<cnt; n++ ) {
		int size = ini.readInt( columns[n].iniName + QString("Size"), columns[n].defaultSize );
		hdr->resizeSection( n, size );
	}
	for( int n=0; n<cnt; n++ ) {
		bool hidden = ini.readBool( columns[n].iniName + QString("Hidden"), columns[n].defaultHidden );
		hdr->setSectionHidden( n, hidden );
	}
	hdr->setResizeMode( QHeaderView::Interactive );
}

void ExtTreeView::saveColumns( IniConfig & ini, const ColumnStruct columns [] )
{
	QHeaderView * hdr = header();
	for( int i=0; columns[i].name; i++ ) {
		ini.writeInt( columns[i].iniName + QString("Size"), hdr->sectionSize( i ) );
		ini.writeInt( columns[i].iniName + QString("Index"), hdr->visualIndex( i ) );
		ini.writeBool( columns[i].iniName + QString("Hidden"), hdr->isSectionHidden( i ) );

		QLineEdit *le = qobject_cast<QLineEdit*> (mRecordFilterWidget->mFilterMap[i]);
		if ( le )
			ini.writeString( columns[i].iniName + QString("ColumnFilter"), le->text() );
    }
}

void ExtTreeView::setupTreeView( IniConfig & ini, const ColumnStruct columns [] )
{
	setupColumns( ini, columns );
	SuperModel * sm = dynamic_cast<SuperModel*>(model());
	int sc = -1;
	if( sm ) {
		QList<SortColumnPair> sortColumns;
		QList<int> scl = ini.readIntList( "SortColumnOrder" );
		QStringList sortDirections= ini.readString( "SortColumnDirection" ).split( ',' );
		for( int i = 0; i < scl.size(); ++i )
			sortColumns.append( SortColumnPair( scl[i], (i < sortDirections.size()) && (sortDirections[i] == "Ascending") ? Qt::AscendingOrder : Qt::DescendingOrder ) );
		sm->setSortColumns( sortColumns );
		sc = scl.isEmpty() ? 0 : scl[0];
		mLastGroupColumn = ini.readInt( "GroupedByColumn", -1 );
		if( mLastGroupColumn >= 0 ) {
			ModelGrouper * grouper = sm->grouper();
			grouper->groupByColumn( mLastGroupColumn );
			foreach( QString key, ini.keys() )
				if( key.startsWith( "GroupExpandState:" ) )
					mGroupExpandState[key.mid(17)] = ini.readBool(key);
		}
	} else
		sc = ini.readInt("SortColumn", 0);
	Qt::SortOrder order(Qt::SortOrder(ini.readInt("SortOrder",Qt::AscendingOrder)));
	header()->setSortIndicator(sc,order);
	model()->sort(sc,order);
}

void ExtTreeView::setupRecordFilterWidget( IniConfig & ini, const ColumnStruct columns [] )
{
	if( !mRecordFilterWidget ) {
		mRecordFilterWidget = new RecordFilterWidget();
		addFilterLayout();
	}

	mRecordFilterWidget->setupFilters( this, columns, ini );
}

void ExtTreeView::setupTreeView( const QString & group, const QString & key, const ColumnStruct columns [] )
{
	Q_UNUSED(key);

	IniConfig & cfg = userConfig();
	cfg.pushSection( group );
	setupTreeView(cfg,columns);
	cfg.popSection();
}

void ExtTreeView::saveTreeView( IniConfig & ini, const ColumnStruct columns [] )
{
	saveColumns( ini, columns );
	SuperModel * sm = dynamic_cast<SuperModel*>(model());
	if( sm ) {
		QList<int> sortColumns;
		QStringList sortDirections;
		foreach( SortColumnPair sc, sm->sortColumns() ) {
			sortColumns.append( sc.first );
			sortDirections.append( sc.second == Qt::DescendingOrder ? "Descending" : "Ascending" );
		}
		ini.writeIntList( "SortColumnOrder", sortColumns );
		ini.writeString( "SortColumnDirection", sortDirections.join(",") );
		if( sm->grouper(false) && sm->grouper()->isGrouped() ) {
			ModelGrouper * grp = sm->grouper();
			ModelDataTranslator * git = grp->groupedItemTranslator();
			ini.writeInt( "GroupedByColumn", sm->grouper()->groupColumn() );
			for( QMap<QString,bool>::Iterator it = mGroupExpandState.begin(); it != mGroupExpandState.end(); ++it )
				ini.writeBool( "GroupExpandState:" + it.key(), it.value() );
			for( ModelIter it(sm); it.isValid(); ++it ) {
				QModelIndex idx(*it);
				if( sm->translator(idx) == git )
					ini.writeBool( "GroupExpandState:" + grp->groupValue(idx), isExpanded(idx) );
			}
				
		} else {
			ini.removeKey( "GroupedByColumn" );
			foreach( QString key, ini.keys( QRegExp("^GroupExpandState:") ) )
				ini.removeKey(key);
		}
		
	} else
		ini.writeInt( "SortColumn", header()->sortIndicatorSection() );
	ini.writeInt( "SortOrder", header()->sortIndicatorOrder() );
}

void ExtTreeView::saveTreeView( const QString & group, const QString & key, const ColumnStruct columns [] )
{
	Q_UNUSED(columns);

	IniConfig & cfg = userConfig();
	cfg.pushSection( group + ":" + key );
	cfg.popSection();
}

BusyWidget * ExtTreeView::busyWidget( bool autoCreate )
{
	if( !mBusyWidget && autoCreate ) {
		mBusyWidget = new BusyWidget( this, QPixmap( "images/rotating_head.png" ) );
		mBusyWidget->move(0,header()->height());
	}
	
	return mBusyWidget;
}

void ExtTreeView::resizeEvent(QResizeEvent *event)
{
	//LOG_3("resizeEvent");
	mHeaderClickIsResize = true;
	QWidget::resizeEvent(event);
	//if( mRecordFilterWidget )
	//    mRecordFilterWidget->resize( width(), 20 );
}

void ExtTreeView::slotGrouperChanged( ModelGrouper * grouper )
{
	if( grouper ) {
		connect( grouper, SIGNAL( groupCreated( const QModelIndex & ) ), SLOT( slotGroupCreated( const QModelIndex & ) ) );
		connect( grouper, SIGNAL( groupEmptied( const QModelIndex & ) ), SLOT( slotGroupEmptied( const QModelIndex & ) ) );
		connect( grouper, SIGNAL( groupPopulated( const QModelIndex & ) ), SLOT( slotGroupPopulated( const QModelIndex & ) ) );
		connect( grouper, SIGNAL( groupingChanged( bool ) ), SLOT( slotGroupingChanged() ) );
	}
}

void ExtTreeView::enableFilterWidget(bool enable)
{
	if(mRecordFilterWidget) {
		mRecordFilterWidget->setVisible(enable);
		mRecordFilterWidget->filterRows();
	}
}

void ExtTreeView::slotGroupCreated( const QModelIndex & idx )
{
	SuperModel * sm = dynamic_cast<SuperModel*>(model());
	if( sm && sm->grouper(false) ) {
		QMap<QString,bool>::Iterator it = mGroupExpandState.find( sm->grouper()->groupValue(idx) );
		if( (it != mGroupExpandState.end() && it.value()) || (it == mGroupExpandState.end() && sm->grouper()->expandNewGroups()) ) {
//			LOG_5( QString("Expanding new group, row %1").arg(idx.row()) );
			expand(idx);
		}
	}
}

void ExtTreeView::slotGroupEmptied( const QModelIndex & idx )
{
	SuperModel * sm = dynamic_cast<SuperModel*>(model());
	if( sm && sm->grouper(false) && sm->grouper()->emptyGroupPolicy() == ModelGrouper::HideEmptyGroups ) {
//		LOG_5( QString("Hiding empty group, row %1").arg(idx.row()) );
		setRowHidden( idx.row(), idx.parent(), true );
	}
}

void ExtTreeView::slotGroupPopulated( const QModelIndex & idx )
{
//	LOG_5( QString("Unhiding populated group, row %1").arg(idx.row()) );
	// Since this will only happen if the empty group state is hide or do nothing, we are safe to always call this
	setRowHidden( idx.row(), idx.parent(), false );
}

void ExtTreeView::slotGroupingChanged()
{
	SuperModel * sm = dynamic_cast<SuperModel*>(model());
	if( sm && sm->grouper(false) ) {
		ModelGrouper * grp = sm->grouper();
		if( grp->groupColumn() != mLastGroupColumn ) {
			// We only keep track of group expand states for a single group column
			// If the user groups by a different column they'll have to re-expand/collapse,
			// otherwise we could end up keeping a large amount of data if we keep track of expanded/collapsed
			// state for each possible group value for each possible group column
//			LOG_5( "Clearing saved group expand states" );
			mGroupExpandState.clear();
			mLastGroupColumn = grp->groupColumn();
		}
	}
}

RecordTreeView::RecordTreeView( QWidget * parent )
: ExtTreeView( parent, 0 )
{
	setContextMenuPolicy( Qt::CustomContextMenu );

	connect( this, SIGNAL( clicked( const QModelIndex & ) ), SLOT( slotClicked( const QModelIndex & ) ) );

	mDelegate = new RecordDelegate(this);
	setItemDelegate( mDelegate );
}

void RecordTreeView::setModel( QAbstractItemModel * model )
{
	ExtTreeView::setModel(model);
	QItemSelectionModel * sm = selectionModel();
	connect( sm, SIGNAL( currentChanged( const QModelIndex &, const QModelIndex & ) ),
		SLOT( slotCurrentChanged( const QModelIndex &, const QModelIndex & ) ) );
	connect( sm, SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
		SLOT( slotSelectionChanged( const QItemSelection &, const QItemSelection & ) ) );
}

QModelIndex RecordTreeView::findIndex( const Record & r, bool recursive, const QModelIndex & parent, bool loadChildren )
{
	RecordSuperModel * rsm = model();
	if( rsm )
		return rsm->findIndex( r, recursive, parent, loadChildren );
	return QModelIndex();
}

QModelIndexList RecordTreeView::findIndexes( RecordList rl, bool recursive, const QModelIndex & parent, bool loadChildren )
{
	RecordSuperModel * rsm = model();
	if( rsm )
		return rsm->findIndexes( rl, recursive, parent, loadChildren );
	return QModelIndexList();
}
	
Record RecordTreeView::getRecord(const QModelIndex & i)
{
	RecordSuperModel * rsm = model();
	if( rsm )
		return rsm->getRecord( i );
	return Record();
}

Record RecordTreeView::current()
{
	QModelIndex idx = selectionModel()->currentIndex();
	return idx.isValid() ? getRecord(idx) : Record();
}

RecordList RecordTreeView::selection()
{
	RecordSuperModel * rsm = model();
	if( rsm )
		return rsm->listFromIS( selectionModel()->selection() );
	return RecordList();
}

void RecordTreeView::setSelection( const RecordList & rl )
{
	QItemSelectionModel * sm = selectionModel();
	sm->clear();
	QModelIndexList il = findIndexes(rl,true);
	int lc = model()->columnCount()-1;
	foreach( QModelIndex i, il )
		sm->select( QItemSelection(i,i.sibling(i.row(),lc)), QItemSelectionModel::Select );
}

void RecordTreeView::setCurrent( const Record & r )
{
	QModelIndex i = findIndex( r );
	if( i.isValid() )
		selectionModel()->setCurrentIndex( i, QItemSelectionModel::NoUpdate );
}

void RecordTreeView::slotCurrentChanged( const QModelIndex & i, const QModelIndex & )
{
	emit currentChanged( getRecord(i) );
}

void RecordTreeView::slotSelectionChanged( const QItemSelection &, const QItemSelection & )
{
	emit selectionChanged( selection() );
}

void RecordTreeView::slotClicked( const QModelIndex & index )
{
	emit clicked( getRecord(index) );
}

void RecordTreeView::slotCustomContextMenuRequested( const QPoint & p )
{
	QModelIndex under = indexAt( p );
	RecordList sel = selection();
	emit showMenu( 
		viewport()->mapToGlobal(p),
		under.isValid() ? getRecord(under) : Record(),
		sel.isEmpty() ? RecordList( current() ) : sel
	);
	ExtTreeView::slotCustomContextMenuRequested(p);
}

void RecordTreeView::scrollTo( const Record & r )
{
	scrollTo( model()->findIndex( r ) );
}

void RecordTreeView::scrollTo( RecordList rl )
{
	scrollTo( model()->findFirstIndex( rl ) );
}

