/*
 *
 * Copyright 2003, 2004 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Resin is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Resin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#include <stdlib.h>

#include <QPaintEvent>
#include <QStyleOptionButton>
#include <qapplication.h>
#include <qcursor.h>
#include <qfontdialog.h>
#include <qheaderview.h>
#include <qlabel.h>
#include <qmenu.h>
#include <qpainter.h>
#include <qprintdialog.h>
#include <qprinter.h>
#include <qpushbutton.h>
#include <qsplitter.h>
#include <qstyle.h>
#include <qtooltip.h>
#include <qtimer.h>
#include <qtreewidget.h>
#include <qlayout.h>

#include "blurqt.h"
#include "recordsupermodel.h"
#include "iniconfig.h"
#include "recordtreeview.h"

#include "schedulewidget.h"
#include "schedulepanel.h"
#include "scheduleheader.h"
#include "schedulerow.h"
#include "scheduleentry.h"
#include "scheduleselection.h"
#include "elementschedulecontroller.h"

void ScrollArea::scrollContentsBy( int x, int y )
{
	emit contentsMoving( horizontalScrollBar()->value(), verticalScrollBar()->value() );
	QScrollArea::scrollContentsBy(x,y);
}

ScheduleCanvas::ScheduleCanvas( QWidget * parent, ScheduleWidget * sw )
: QWidget( parent )
, mSchedulePopup( 0 )
, mSched( sw )
, mDragging( false )
{
	setMouseTracking( true );
	installEventFilter( this );
	setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding ) );
	setBackgroundRole( QPalette::NoRole );
//	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_NoSystemBackground);
	setAttribute(Qt::WA_NoBackground);
}

QSize ScheduleCanvas::sizeHint() const
{
	return size();
}

ScheduleWidget * ScheduleCanvas::sched()
{
	return mSched;
}

void ScheduleCanvas::paint( QPainter * p, const QRect & rect, bool paintBackgrounds )
{
	/* Each paint event we'll use a new id
	 * So that any span can draw itself all at once
	 * instead of redrawing for each cell,
	 * or they can ignore this, and draw each cell
	 * individually.
	 */
	static int paintId = 1;
	int cx = rect.x(), cy = rect.y(), cw = rect.width(), ch = rect.height();

	ScheduleWidget * sw = sched();
	int rows = sw->rowCount();
	int cols = sw->columnCount();

	ScheduleSelection sel = sw->selection();

	ScheduleEntry::PaintOptions po;
	po.paintId = paintId;
	po.backgroundColor = sw->mCellBackground;
	po.painter = p;
	po.displayOptions = &sw->mDisplayOptions;

	int startRow = 0;
	while( startRow < rows && sw->rowPos( startRow ) + sw->rowHeight( startRow ) < cy ) startRow++;

	int startCol = 0;
	while( startCol < cols && sw->colPos( startCol ) + sw->colWidth( startCol ) < cx ) startCol++;
	
	int y=0, h=0;
	
	for( int stage = 0; stage <= 1; stage++ )
	{
		for( int row = startRow; row < rows; row++ )
		{
			bool done = false;
			paintId++;
			po.paintId = paintId;

			// Vertical clipping
			y = sw->rowPos( row );
			h = sw->rowHeight( row );

			if( y > cy + ch )
				done = true;

			if( h == 0 ) continue;

			for( int col = startCol; col < cols; col++ )
			{
				// Horizontal clipping
				int x( sw->colPos( col ) ), w( sw->colWidth( col ) );
				QDate date = sw->dateAtCell( row, col );

				if( !done ) {
					if( x > cx + cw ) break;
					if( x + w < cx ) continue;
	
					int cell = row * cols + col;
	
					bool valid = sw->isCellValid( row, col );
					
					// Backgrounds
					if( stage == 0 ) {
						if( paintBackgrounds ) {
							QColor bg = valid
										?	(date.dayOfWeek() > 5 ? sw->mCellWeekendBackground : sw->mCellBackground)
										:   (palette().color( QPalette::Background ));
							p->fillRect( x, y, w, h, bg );
						}
					}
					// Foreground
					else if( stage == 1 && valid ) {
						if( sw->displayMode() == ScheduleWidget::Month ) {
							if( date.month() == sw->endDate().month() )
								p->setPen( Qt::black );
							else
								p->setPen( QColor( 180, 180, 180 ) );
							p->setFont( sw->mDisplayOptions.cellDayFont );
							p->drawText( x + 3, y + p->fontMetrics().lineSpacing(), QString::number( date.day() ) );
						}
						QList<ScheduleEntry*> entries = sw->cellEntries( row, col );
						foreach( ScheduleEntry * ent, entries )
						{
							p->save();
							// Draw the entry
							RowLayout & rl = ent->mLayouts[row];
							QRect r( rl.mRect );
							r.translate( 0, y );
							po.spanRect = r;
							po.cellRect = QRect(x,y,w,h);
							po.startDate = sw->dateAtCell( row, rl.mColStart );
							po.endDate = sw->dateAtCell( row, rl.mColEnd );
							po.cellDate = date;
							po.startColumn = rl.mColStart;
							po.endColumn = rl.mColEnd;
							po.column = col;
							po.selection = sel.getEntrySelection( cell, ent );
							if( !po.selection.isValid() || !mDragging )
								ent->paint(po);
							p->restore();
						}
					}
				}
				if( stage == 0 && (done || row + 1 == rows) ) {
					p->setPen( sw->mCellBorder );
					p->drawLine( x + w - 1, cy, x + w - 1, qMin(cy + ch-1,sw->rowPos(rows-1)+sw->rowHeight(rows-1)-2) );
				}
			}
			if( done ) break;
			if( stage == 0 ) {
				p->setPen( sw->mCellBorder );
				p->drawLine( cx, y-1, qMin(cx + cw,sw->colPos(cols-1) + sw->colWidth(cols-1)-1), y-1 );
			}
		}
		if( stage == 0 ) {
			QVector<bool> selectedCells(cols*rows,false);
			foreach( ScheduleCellSelection scs, sel.mCellSelections ) {
				for( int i=scs.mCellStart; i <= scs.mCellEnd; i++ )
					selectedCells[i] = true;
			}
			for( int c=0; c<cols*rows; c++ ) {
				if( !selectedCells[c] ) continue;

				int row = c / cols;
				int col = c % cols;
				int above = c - cols;
				int below = c + cols;
				int right = c + 1;
				int left = c - 1;
				
				int x = sw->colPos(col);
				int w = sw->colWidth(col);
				int y = sw->rowPos(row);
				int h = sw->rowHeight(row);

				p->fillRect( x, y, w-1, h-1, sw->mCellBackgroundHighlight );
	
				p->setPen( sw->mCellBorderHighlight );

				if( above >= 0 && !selectedCells[above] )
					p->drawLine( x-1, y-1, x + w - 1, y-1 );
				if( below < rows*cols && !selectedCells[below] )
					p->drawLine( x-1, y + h - 1, x + w - 1, y + h - 1 );
				if( right / cols == row && !selectedCells[right] )
					p->drawLine( x + w - 1, y-1, x + w - 1, y + h - 1 );
				if( left > 0 && left / cols == row && !selectedCells[left] )
					p->drawLine( x-1, y-1, x - 1, y + h -1 );
			}
		}
	}
	
	if( y + h < cy + ch ) {
		int top = cy > y + h ? cy : y + h;
		p->fillRect( cx, top, cw, cy - top + ch, palette().color( QPalette::Background ) );
	}

	if( mDragging ) {
		po.paintId++;
		QPoint offset = mDragPosition - mDragStartPosition;
		//LOG_5( "Painting dragged entries with offset: " + QString::number(offset.x()) + ", " + QString::number(offset.y()) );
		QRect dragRect;
		p->setClipping(false);
		foreach( ScheduleEntrySelection ses, sel.mEntrySelections ) {
			ScheduleEntry * entry = ses.mEntry;
			for( int cell = ses.mCellStart; cell <= ses.mCellEnd; ++cell ) {
				int row = cell / cols;
				int col = cell % cols;
				int row_y = sw->rowPos( row );
				int col_x = sw->colPos( col );
				int row_h = sw->rowHeight( row );
				int col_w = sw->colWidth( col );
				QDate date = sw->dateAtCell( row, col );
				p->save();
				p->translate( mDragPosition - mDragStartPosition );
				// Draw the entry
				RowLayout & rl = entry->mLayouts[row];
				QRect r( rl.mRect );
				r.translate( 0, row_y );
				po.spanRect = r;
				po.cellRect = QRect(col_x,row_y,col_w,row_h);
				dragRect |= po.cellRect;
				po.startDate = sw->dateAtCell( row, rl.mColStart );
				po.endDate = sw->dateAtCell( row, rl.mColEnd );
				po.cellDate = date;
				po.startColumn = rl.mColStart;
				po.endColumn = rl.mColEnd;
				po.column = col;
				po.selection = ses;
				entry->paint(po);
				p->restore();
			}
		}
		dragRect.translate( offset );
		mLastDragRect = dragRect.adjusted(-1,-1,2,2);
	}
}

void ScheduleCanvas::paintEvent( QPaintEvent * pe )
{
	SW_DEBUG( "ScheduleCanvas::paintEvent" );
	QPainter painter( this );
	QPainter * p = &painter;
	paint( p, pe->rect() );
}

ScheduleEntry * ScheduleCanvas::findHandle( const QPoint & pos, bool * right )
{
	QPoint inCell;
	int row, col;
	bool ptc = sched()->pointToCell( pos, &row, &col, &inCell );
	if( ptc ) {
		QList<ScheduleEntry*> entries = sched()->cellEntries( row, col );
		foreach( ScheduleEntry * se, entries ) {
			RowLayout & rl = se->mLayouts[row];
			if( inCell.y() < rl.mRect.y() || inCell.y() > rl.mRect.bottom() ) {
				continue;
			}
			int allowResize = se->allowResize();
			if( (allowResize & ScheduleEntry::ResizeLeft) && abs( pos.x() - rl.mRect.x() ) < 4 ) {
				if( right ) *right = false;
				return se;
			}
			if( (allowResize & ScheduleEntry::ResizeRight) && abs( pos.x() - rl.mRect.right() ) < 4 ) {
				if( right ) *right = true;
				return se;
			}
		}
	}
	return 0;
}

static int menuId = 1;

void ScheduleCanvas::mouseDoubleClickEvent( QMouseEvent * me )
{
	LOG_3( "ScheduleCanvas::mouseDoubleClickEvent" );
	QPoint p( me->pos() );
	int row, col;
	ScheduleEntry * se = sched()->entryAtPos( p );
	bool ptc = sched()->pointToCell( me->pos(), &row, &col, 0 );
	if( me->button() == Qt::LeftButton && ptc ) {
		if( se ) {
			ScheduleSelection ss = sched()->selection();
			if( !(me->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier)) )
				ss.clear();
			int start = sched()->dateCell( se->dateStart(), se->row() );
			int end = sched()->dateCell( se->dateEnd(), se->row() );
			for( int i=start; i<=end; ++i )
				ss.select( i, se, ScheduleSelection::AddSelection );
			sched()->setSelection( ss );
		}
	}
}

void ScheduleCanvas::mousePressEvent( QMouseEvent * me )
{
	QPoint p( me->pos() );
	int row, col;
	ScheduleEntry * se = sched()->entryAtPos( p );
	bool ptc = sched()->pointToCell( me->pos(), &row, &col, 0 );
	int cell = row * sched()->columnCount() + col;
	
	// Context Menu
	if( me->button() == Qt::RightButton && ptc ) {
		ScheduleSelection sel = sched()->selection();
		if( !sel.isEntrySelected(cell,se) && !sel.isCellSelected(cell) ) {
			if( !(me->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier)) )
				sel.clear();
			sel.select( cell, se, ScheduleSelection::AddSelection );
			sched()->setSelection(sel);
		}
		if( se ) {
			sched()->mShowingPopup = true;
			se->popup( this, me->globalPos(), sched()->dateAtCell( row, col ), sel );
			sched()->mShowingPopup = false;
		} else {
			QList<ScheduleRow*> rows = sched()->scheduleRowsForRow( row );
			if( !rows.isEmpty() ) {
				QMenu * menu = new QMenu( this );
				QDate ds = sched()->dateAtCell( row, col );
				QDate de = ds;
				if( sel.isCellSelected( cell ) ) {
					ScheduleCellSelection cs = sel.getCellSelection( cell );
					ds = cs.startDate();
					de = cs.endDate();
				}
				menuId++;
				foreach( ScheduleRow * row, rows ) {
					row->populateMenu( menu, me->pos(), ds, de, menuId );
				}
				if( !menu->actions().isEmpty() )
					menu->exec(me->globalPos());
				delete menu;
			}
		}
	} else if( me->button() == Qt::LeftButton && ptc ) {
		bool isRight;
		ScheduleEntry * entry = findHandle( p, &isRight );
		mDragStartPosition = p;

		if( entry ) {
			//qWarning( QString(isRight ? "Right" : "Left") + " Handle grabbed" );
			sched()->mResizeEntry = new ScheduleWidget::ResizeEntry( isRight, row, col, entry );
			// Keep a reference to this entry
			entry->ref();
		} else if( se && sched()->displayMode() == ScheduleWidget::Year ) {
			// Popup a week view of this cell
			ElementScheduleController * esc = new ElementScheduleController();
			esc->setShowTimeSheets( false );
			ScheduleWidget * sw = new ScheduleWidget( esc, this );
			sw->setShowWeekends( true );
			sw->setDisplayMode( ScheduleWidget::Week );
			sw->setDate( sched()->dateAtCell( row, col ) );
			int x = qMax(me->globalPos().x()-300, 0);
			int y = sched()->rowPos( row ) + sched()->rowHeight( row ) + mapToGlobal(QPoint(0,0)).y();
			sw->setGeometry( x, y, 600, 150 );
			esc->setElementList( ((ElementScheduleRow*)se->row())->element() );
			// HACK
			sw->show();
			sw->resize( sw->width(), sw->contentsHeight() );
			mSchedulePopup = sw;
			mSchedulePopup->installEventFilter( this );
		} else {
			sched()->moveCurrent( cell, se, me->modifiers() );
		}
	}
	QWidget::mousePressEvent(me);
}

void ScheduleCanvas::mouseMoveEvent( QMouseEvent * me )
{
	int row, col;
	bool ptc = sched()->pointToCell( me->pos(), &row, &col, 0 );
	int cell = row * sched()->columnCount() + col;
	QDate date = sched()->dateAtCell( row, col );

	if( me->buttons() == Qt::NoButton && ptc ) {
		ScheduleEntry * se = findHandle( me->pos() );
		if( se && !qApp->overrideCursor() )
			qApp->setOverrideCursor( QCursor( Qt::SizeHorCursor ) );
		else if( !se )
			qApp->restoreOverrideCursor();
		return;
	}
	ScheduleWidget::ResizeEntry * re = sched()->mResizeEntry;
	if( re && ptc && (re->mCurRow != row || re->mCurCol != col) ) {
		re->mCurRow = row;
		re->mCurCol = col;
		QDate start = re->mEntry->dateStart(), end = re->mEntry->dateEnd();
		bool needsUpdated = false;
		if( re->mRightSide && date >= re->mEntry->dateStart() && date != re->mEntry->dateEnd() ) {
			re->mEntry->setDateEnd( date );
			needsUpdated = true;
			//qWarning( "Moving dateEnd" );
		} else if( !re->mRightSide && date <= re->mEntry->dateEnd() && date != re->mEntry->dateStart() ) {
			re->mEntry->setDateStart( date );
			needsUpdated = true;
			//qWarning( "Moving dateStart" );
		}
		if( needsUpdated ) {
			SW_DEBUG( "Moved date to: " + date.toString() + " dateStart: " + re->mEntry->dateStart().toString() + " dateEnd: "+ re->mEntry->dateEnd().toString() );
			
			sched()->removeCellEntries( re->mEntry, start, end );
			if( start > re->mEntry->dateStart() ) start = re->mEntry->dateStart();
			if( end < re->mEntry->dateEnd() ) end = re->mEntry->dateEnd();
			sched()->addCellEntries( re->mEntry, re->mEntry->dateStart(), re->mEntry->dateEnd() );
			int startRow, endRow;
			sched()->dateCell( start, re->mEntry->row(), &startRow );
			sched()->dateCell( end, re->mEntry->row(), &endRow );
			bool rowChanges = sched()->layoutCells( startRow, endRow );
			if( rowChanges ) {
				update();
				sched()->panel()->tree()->doItemsLayout();
			} else {
				int y = sched()->rowPos(startRow);
				update( QRect( 0, y, width(), sched()->rowPos(endRow+1)-y ) );
			}
		}
		return;
	}
 
	if( mDragging ) {
		//LOG_5( "ScheduleCanvas::mouseMoveEvent:: Checking to see if we can drop here" );
		QPoint offset = me->pos() - mDragPosition;
		mDragPosition = me->pos();
		ScheduleSelection ss = sched()->selection();
		QList<ScheduleRow*> rows = sched()->scheduleRowsForRow( row );
		bool canDrop = true;
		foreach( ScheduleEntrySelection ses, ss.mEntrySelections ) {
			if( !ses.mEntry->canDrop( ses, mDragStartDate, date, rows.size() == 1 ? rows[0] : ses.mEntry->row() ) ) {
				canDrop = false;
				break;
			}
		}
		setCursor( canDrop ? Qt::ArrowCursor : Qt::ForbiddenCursor );
		mLastDragRect |= mLastDragRect.translated( offset );
		update(mLastDragRect);
		return;
	}

	// If the user clicked on an entry and drags a certain distance
	// then we'll start a drag operation
	if( !re && (me->buttons() & Qt::LeftButton) && (me->modifiers() && Qt::ShiftModifier) ) {
		QPoint distance = me->pos() - mDragStartPosition;
		if( sched()->selection().mEntrySelections.size() && distance.manhattanLength() > qApp->startDragDistance()  ) {
			LOG_5( "ScheduleCanvas::mouseMoveEvent: Checking to see if we can drag selection" );
			ScheduleSelection ss = sched()->selection();
			bool canDrag = true;
			foreach( ScheduleEntrySelection ses, ss.mEntrySelections ) {
				if( !ses.mEntry->allowDrag( ses ) ) {
					canDrag = false;
					break;
				}
			}
			if( canDrag ) {
				LOG_5( "ScheduleCanvas::mouseMoveEvent: Starting Entry Drag operation" );
				mDragging = true;
				mDragStartDate = date;
			}
		}
	}

	if( ptc && !re ){
		ScheduleEntry * se = sched()->entryAtPos( me->pos() );
		sched()->moveCurrent( cell, se, me->modifiers() );
	}
}

bool ScheduleCanvas::eventFilter( QObject * o, QEvent * ev )
{
	if( o == this  && ev->type() == QEvent::Leave ) {
		if( qApp->overrideCursor() )
			qApp->restoreOverrideCursor();
	}
	if( o && o == mSchedulePopup ) {
		if( !mSchedulePopup->mShowingPopup && ev->type() == QEvent::FocusOut ) {
			mSchedulePopup->deleteLater();
			mSchedulePopup = 0;
		}
	}
	if( ev->type() == QEvent::ToolTip ) {
		QHelpEvent * he = (QHelpEvent*)ev;
		QRect rect;
		QPoint p = he->pos();
		ScheduleEntry * entry = sched()->entryAtPos( p , &rect );
		if( entry ) {
			int row, col;
			if( !sched()->pointToCell( p, &row, &col ) )
				return false;
			QToolTip::showText( he->globalPos(), entry->toolTip( sched()->dateAtCell( row, col ), sched()->displayMode() ), this );
		}
	}

	return QWidget::eventFilter( o, ev );
}


void ScheduleCanvas::mouseReleaseEvent( QMouseEvent * me )
{
	if( sched()->mResizeEntry ) {
		sched()->mResizeEntry->mEntry->applyChanges( true );
		sched()->mResizeEntry->mEntry->deref();
		delete sched()->mResizeEntry;
		sched()->mResizeEntry = 0;
		qApp->restoreOverrideCursor();
	}
	
	if( mDragging ) {
		int row, col;
		bool ptc = sched()->pointToCell( me->pos(), &row, &col, 0 );
		QDate date = sched()->dateAtCell( row, col );
		ScheduleSelection ss = sched()->selection();
		QList<ScheduleRow*> rows = sched()->scheduleRowsForRow( row );
		bool canDrop = true;
		foreach( ScheduleEntrySelection ses, ss.mEntrySelections ) {
			ScheduleRow * row = rows.size() == 1 ? rows[0] : ses.mEntry->row();
			if( !ses.mEntry->canDrop( ses, mDragStartDate, date, row ) ) {
				canDrop = false;
				break;
			}
		}
		setCursor( QCursor() );
		if( canDrop ) {
			LOG_5( "ScheduleCanvas::mouseReleaseEvent: Executing Drop" );
			foreach( ScheduleEntrySelection ses, ss.mEntrySelections ) {
				ScheduleRow * row = rows.size() == 1 ? rows[0] : ses.mEntry->row();
				ses.mEntry->drop( ses, mDragStartDate, date, row );
			}
		}
		mDragging = false;
		sched()->setSelection( ScheduleSelection(sched()) );
		mLastDragRect = QRect();
		update();
		return;
	}
	//mHighlightStart = QPoint( -1, -1 );
	//sched()->setHighlight( QRect( -1,-1,0,0 ) );
	QWidget::mouseReleaseEvent( me );
}

void ScheduleCanvas::resizeEvent( QResizeEvent * re )
{
	SW_DEBUG( "ScheduleCanvas::resizeEvent: Resized to " + QString::number( re->size().width() ) + " x " + QString::number( re->size().height() ) );
	QWidget::resizeEvent(re);
	update();
}

bool ScheduleWidget::RowEntry::operator<( const RowEntry & other ) const { return entry->cmp(other.entry); }
bool ScheduleWidget::RowEntry::operator==(const RowEntry & other ) const { return entry == other.entry; }

ScheduleWidget::ScheduleWidget( ScheduleController * sc, QWidget * parent )
: QWidget( parent )
, mResizeEntry( 0 )
, mCellBackground( 210, 212, 225 )
, mCellWeekendBackground( 205, 208, 217 )
, mCellBackgroundHighlight( 201, 211, 240 )
, mCellBorder( 160, 162, 175 )
, mCellBorderHighlight( 31, 133, 195 )
, mHeaderBackground( 220, 223, 232 )
, mHeaderBackgroundHighlight( 226, 221, 203 )
, mPanelBackground( 210, 212, 226 )
, mPanelBackgroundHighlight( 226, 221, 203 )
, mWeekStartDay( 0 )
, mCellBorderWidth( 1 )
, mCellBorderWidthHighlight( 2 )
, mCellLeadHeight( 0 )
, mCellMinHeight( 0 )
, mEntryBorderWidth( 2 )
, mEntryBorderWidthHighlight( 3 )
, mEntryHeight( 25 )
, mMonthCellSize( 40 )
, mPanelWidth( 100 )
, mShowingPopup( false )
, mRowCount( 0 )
, mColumnCount( 0 )
, mCellCount( 0 )
, mUpdatingCellEntries( false )
, mContentsYOffset( 0 )
, mContentsXOffset( 0 )
, mDisplayMode( Month )
, mSelectionMode( ExtendedSelect )
, mSelection( this )
, mCurrent( this )
, mController( sc )
, mHideWeekends( false )
, mZoom( 100 )
{
	mSplitter = new QSplitter( this );
	QLayout * mainLayout = new QHBoxLayout( this );
	mainLayout->addWidget( mSplitter );
	mainLayout->setMargin( 0 );
	
	mHeaderCanvasContainer = new QWidget( mSplitter );
	mPanelContainer = new QWidget( mSplitter );
	mHeaderCanvasContainer->installEventFilter( this );
	QSizePolicy sp( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );
	sp.setHorizontalStretch( 5 );
	mHeaderCanvasContainer->setSizePolicy( sp );
	mHeaderCanvasContainer->setMinimumSize( QSize( 150, 150 ) );
	mPanelContainer->installEventFilter( this );

	// The canvas widget goes inside the scroll view
	mScrollView = new ScrollArea( mHeaderCanvasContainer );
	mCanvas = new ScheduleCanvas( mScrollView->viewport(), this );
	mHeader = new ScheduleHeader( this, mHeaderCanvasContainer );
	
	mPanel = new SchedulePanel( this, mPanelContainer );
	
	mSplitter->addWidget( mPanelContainer );
	mSplitter->addWidget( mHeaderCanvasContainer );
	mSplitter->setCollapsible( 1, false );
	{
		QFont canvasFont = mCanvas->font();
		canvasFont.setPointSize( canvasFont.pointSize() - 1 );
		mCanvas->setFont( canvasFont );
	}
	mScrollView->setWidget( mCanvas );
	mScrollView->setWidgetResizable( true );
	mScrollView->viewport()->setBackgroundRole( QPalette::NoRole );
	mScrollView->setBackgroundRole( QPalette::NoRole );
	mScrollView->setFrameStyle( QFrame::NoFrame );


//	mDateLabel = new QLabel( this );
//	mDateLabel->setPaletteBackgroundColor( mHeaderBackground );
//	mDateLabel->setAlignment( Qt::AlignHCenter | Qt::AlignTop );
//	mDateLabel->setFrameStyle( QFrame::Plain | QFrame::Box );
//	mDateLabel->setLineWidth( 1 );
	if( mController )
		connect( mController, SIGNAL( rowsChanged() ), SLOT( slotRowsChanged() ) );

	mForward = new QPushButton( ">", this );
	mBack = new QPushButton( "<", this );
	mYearForward = new QPushButton( ">>", this );
	mYearBack = new QPushButton( "<<", this );
	mTodayButton = new QPushButton( "Today", this );

	mYearBack->setGeometry( 5, 5, 25, 25 );
	mBack->setGeometry( 35, 5, 25, 25 );
	int todayWidth = fontMetrics().size(Qt::TextSingleLine,"Today").width() + 20;
	mTodayButton->setGeometry( 65, 5, todayWidth, mTodayButton->sizeHint().height() );
	mForward->setGeometry( todayWidth + 70, 5, 25, 25 );
	mYearForward->setGeometry( todayWidth + 100, 5, 25, 25 );

	connect( mForward, SIGNAL( clicked() ), SLOT( forward() ) );
	connect( mBack, SIGNAL( clicked() ), SLOT( back() ) );
	connect( mYearForward, SIGNAL( clicked() ), SLOT( yearForward() ) );
	connect( mYearBack, SIGNAL( clicked() ), SLOT( yearBack() ) );
	connect( mTodayButton, SIGNAL( clicked() ), SLOT( today() ) );

	connect( mScrollView, SIGNAL( contentsMoving( int, int ) ), SLOT( contentsMoved( int, int ) ) );

	QDate cur = QDate::currentDate();
	setDisplayMode( Month );
	setDate( cur );

	installEventFilter( this );

	QFont f = font();
	f.setPointSize( 7 );
	mDisplayOptions.entryHoursFont = f;
	f.setPointSize( 8 );
	mDisplayOptions.entryFont = f;
}

ScheduleWidget::~ScheduleWidget()
{
	IniConfig & c = userConfig();
	c.pushSection( "Calendar" );
	c.writeInt( "PanelWidth", mPanel->width() );
	c.popSection();
	delete mController;
}

//
//  Grid Layout Functions
//
bool ScheduleWidget::pointToCell( const QPoint & pos, int * row, int * col, QPoint * celPos )
{
	int r = 0, c = 0;
	if( row || celPos ) {
		for( ; r < rowCount(); r++ )
			if( mRowPositions[r] > pos.y() )
				break;
		if( r == rowCount() )
			return false;
		*row = r;
	}
	if( col || celPos ) {
		for( ; c < columnCount(); c++ )
			if( mColumnPositions[c] > pos.x() )
				break;
		if( c == columnCount() )
			return false;
		*col = c;
	}
	if( celPos ) {
		QPoint cp( pos.x() - colPos(c), pos.y() - rowPos(r) );
		*celPos = cp;
	}
	return true;
}

int ScheduleWidget::colPos( int col ) const
{
	if( col <= 0 || col > columnCount() )
		return 0;
	return mColumnPositions[col-1];
}

int ScheduleWidget::colWidth( int col ) const
{
	return colPos( col + 1 ) - colPos( col );
}

int ScheduleWidget::rowPos( int row ) const
{
	if( row <= 0 || row > rowCount() )
		return 0;
	return mRowPositions[row-1];
}

int ScheduleWidget::rowHeight( int row ) const
{
	return rowPos( row+1 ) - rowPos( row );
}

QRect ScheduleWidget::posToPixels( const QRect & pos ) const
{
	int x = colPos( pos.x() ), y = rowPos( pos.y() );
	return QRect( x, y, colPos( pos.right() + 1 ) - x, rowPos( pos.bottom() + 1 ) - y );
}

//
// Updates the column positions based on viewport
// width and view mode.
//
void ScheduleWidget::updateColumnPositions( int desiredWidth )
{
	SW_DEBUG( "ScheduleWidget::updateColumnPositions" );
	int nc = columnCount(), vw = mScrollView->viewport()->width();
	if( desiredWidth > 0 ) vw = desiredWidth;
	SW_DEBUG( "ScheduleWidget::updateColumnPositions: Desired Width: " + QString::number( vw ) );
	mColumnPositions.resize( nc );
	int last = 0;
	int minColWidth = 0;
	switch( mDisplayMode ) {
		case Week:
			minColWidth = 50 * mZoom / 100;
			break;
		case Month:
			minColWidth = 50 * mZoom / 100;
			break;
		case MonthFlat:
			minColWidth = 50 * mZoom / 100;
			break;
		case Year:
			minColWidth = 20 * mZoom / 100;
			break;
	}
	minColWidth = qMax(15,minColWidth);
	for( int col = 0; col<nc; col++ ) {
		int next = ((col+1)*vw/nc);
		next = qMax( last + minColWidth, next );
		mColumnPositions[col] = last = next;
	}
//	mCanvas->setMinimumWidth( last );
	mCanvas->resize( last, mCanvas->height() );
}

void ScheduleWidget::updateDimensions()
{
	{
		SW_DEBUG( "ScheduleWidget::updateColumnCount" );
		switch( mDisplayMode ) {
			case Year: 
				mColumnCount = 52;
				break;
			case MonthFlat:
				{
					int ret = mDate.daysTo( endDate() ) + 1;
					if( mHideWeekends ) {
						QDate start = mDate, end = endDate();
						while( start <= end ) {
							if( start.dayOfWeek() >= 6 )
								ret--;
							start = start.addDays( 1 );
						}
					}
					mColumnCount = ret;
					break;
				}
			default:
				mColumnCount = mHideWeekends ? 5 : 7;
		}
	}

	{
		SW_DEBUG( "ScheduleWidget::updateRowCount" );
		int rows = 0;
		if( mDisplayMode == Month ) {
			// In month mode, the first date usually isn't in the current month
			QDate firstDayOfMonth = QDate( mOrigDate.year(), mOrigDate.month(), 1 );
			rows = (dateColumn( firstDayOfMonth ) + firstDayOfMonth.daysInMonth() - 1) / 7 + 1;
		} else {
			rows = mRows.count();
		}
		mRowPositions.resize( rows );
		mRowHeights.resize( rows );
		mRowCount = rows;
	}

	mCellCount = rowCount() * columnCount();
	updateCellEntries();
}

//
// Date related functions
//

//
// Returns the first date shown on the calendar
//
QDate ScheduleWidget::date() const
{
	return mDate;
}

//
// Returns the last date shown on the calendar
//
QDate ScheduleWidget::endDate() const
{
	if( mDisplayMode==Week )
		return mDate.addDays( 6 );
	else if( mDisplayMode==Month )
		return QDate( mOrigDate.year(), mOrigDate.month(), mOrigDate.daysInMonth() );
	else if( mDisplayMode == MonthFlat )
		return mEndDate;
	else if( mDisplayMode==Year )
		return QDate( mOrigDate.year()+1, 1, 1 ).addDays( -1 );
	return QDate();
}

//
// Returns true if the coordinate falls on a valid date on the calendar
//
bool ScheduleWidget::isCellValid( int row, int col )
{
	QDate date = dateAtCell( row, col );
	return date.isValid() && date >= mDate && date <= endDate();
}

//
// Returns the date for the coordinate.
//
QDate ScheduleWidget::dateAtCell( int row, int col )
{
	QDate ret;
	if( row < 0 || row > rowCount() )
		return ret;
	if( col < 0 || col > columnCount() )
		return ret;
	if( mDisplayMode == Week || mDisplayMode == MonthFlat ) {
		if( mHideWeekends ) {
			QDate ret = mDate;
			do {
				while( ret.dayOfWeek() > 5 )
					ret = ret.addDays(1);
				if( col <= 0 ) break;
				col--;
				ret = ret.addDays(1);
			} while( 1 );
			return ret;
		}
		ret = mDate.addDays( col );
		return ret;
	} else if( mDisplayMode == Month ) {
		ret = mDate;
		
		return ret.addDays( row * 7 + col );
	} else if( mDisplayMode == Year ) {
		ret = mDate.addDays( col * 7 );
	}
	return ret;
}

//
// Returns the column number for a specific date
//
int ScheduleWidget::dateColumn( const QDate & date )
{
	if( !date.isValid() || date < mDate || date > endDate() )
		return -1;
	if( mDisplayMode == Week )
		return mDate.daysTo( date ) % 7;
	else if( mDisplayMode == MonthFlat )
		return mDate.daysTo( date );
	else if( mDisplayMode == Month )
		return mDate.daysTo( date );
	else
		return mDate.daysTo( date ) / 7;
}

//
// Returns the cell for a specific date,
// and optionally a specific ScheduleRow
// 
int ScheduleWidget::dateCell( const QDate & date, ScheduleRow * sr, int * row, int * col )
{
	int r = sr ? mRows.indexOf( sr ) : 0;
	if( mDisplayMode == Week ) {
		int pos = qMax(0,mDate.daysTo( date ));
		pos = qMin(pos,columnCount()-1);
		if( row ) *row = r;
		if( col ) *col = pos % 7;
		return r * columnCount() + qMin(pos,6);
	} else if( mDisplayMode == Month) {
		if( mHideWeekends && date.dayOfWeek() > 5 )
			return -1;
		int pos = qMax(0,mDate.daysTo( date ));
		//LOG_5( date.toString() + " is " + QString::number( pos ) + " days from " + mDate.toString() );
		if( row ) *row = pos / 7;
		if( col ) *col = pos % 7;
		if( mHideWeekends ) pos = ((pos/7)*5) + pos % 7;
		if( pos > cellCount() - 1 ) return -1;
		return pos;
	} else if( mDisplayMode == MonthFlat ) {
		int c = qMax(0,qMin(mDate.daysTo( date ),columnCount()-1));
		if( row ) *row = r;
		if( mHideWeekends ) {
			int dow = mDate.dayOfWeek()-1, neg = 0;
			for( int i = 0; i <= c; i++ )
				if( (dow++ % 7) >= 5 ) neg++;
			c -= neg;
			if( c < 0 ) c = 0;
		}
		if( col ) *col = c;
		return r * columnCount() + c;
	} else if( mDisplayMode == Year ) {
		int yr;
		int wn = date.weekNumber( &yr );
		if( yr != date.year() ) wn = 1;
		if( col ) *col = wn - 1;
		if( row ) *row = r;
		return r * columnCount() + wn - 1;
	}
	return -1;
}

//
// Set the calendars date range so that date is visible
// using the current view mode
//
void ScheduleWidget::setDate( const QDate & date )
{
	SW_DEBUG( "ScheduleWidget::setDate" );
	if( !date.isValid() )
		return;
	mOrigDate = date;
	mDate = date;
	if( mDisplayMode == Week ){
		if( mDate.dayOfWeek() != mWeekStartDay ) {
			int skip = mWeekStartDay - mDate.dayOfWeek();
			if( skip > 0 )
				skip = skip-7;
			mDate = mDate.addDays( skip );
		}
		while( mHideWeekends && mDate.dayOfWeek() > 5 ) mDate = mDate.addDays(1);
		//mDateLabel->setText( "Week " + QString::number( mDate.weekNumber() ) + ", " + QString::number( mOrigDate.year() ) );
	} else if( mDisplayMode == Month || mDisplayMode == MonthFlat ){
		mDate = QDate( mDate.year(), mDate.month(), 1 );
		while( mHideWeekends && mDate.dayOfWeek() > 5 ) mDate = mDate.addDays(1);
		//mDateLabel->setText( QDate::longMonthName( mDate.month() ) + ", " + QString::number( mDate.year() ) );
		if( mDisplayMode == Month ) {
			while( (mDate.dayOfWeek() % 7) != (mWeekStartDay % 7) )	mDate = mDate.addDays( -1 );
			while( mHideWeekends && mDate.dayOfWeek() > 5 ) mDate = mDate.addDays(1);
		}
		if( mDisplayMode == MonthFlat ) {
			QDate nextMonth = mDate.addDays( mDate.daysInMonth()-1 );
			if( mEndDate < nextMonth )
				mEndDate = nextMonth;
			if( mDate.daysTo( mEndDate ) > 365 ) // Limit to 1 yr
				mEndDate = mDate.addDays( 365 );
		}
	} else if( mDisplayMode == Year ) {
		mDate = QDate( mDate.year(), 1, 1 );
		mDate.addDays( (8 - mDate.dayOfWeek()) % 7 );
		//mDateLabel->setText( QString::number( mDate.year() ) );
	}
	LOG_5( "ScheduleWidget::setDate: mDate set to " + mDate.toString() );
	slotRowsChanged();
	QDate today( date );
	ScheduleSelection ss = selection();
	ss.clear();
	if( (mDisplayMode == Week || mDisplayMode == Month) && today >= mDate && today <= endDate() ) {
		int r, c;
		if( dateCell( today, 0, &r, &c ) >= 0 )
			ss.select( r * columnCount() + c, 0, ScheduleSelection::AddSelection );
	}
	setSelection( ss );
}

void ScheduleWidget::setEndDate( const QDate & endDate )
{
	mEndDate = endDate;
	if( mDisplayMode == MonthFlat )
		setDate( mOrigDate );
}

//
// Date navigation functions
//
void ScheduleWidget::back()
{
	if( mDisplayMode == Week )
		setDate( mOrigDate.addDays( -7 ) );
	else if( mDisplayMode == Month || mDisplayMode == MonthFlat )
		setDate( mOrigDate.addMonths( -1 ) );
	else if( mDisplayMode == Year )
		setDate( mOrigDate.addYears( -1 ) );
}

void ScheduleWidget::forward()
{
	if( mDisplayMode == Week )
		setDate( mOrigDate.addDays( 7 ) );
	else if( mDisplayMode == Month || mDisplayMode == MonthFlat )
		setDate( mOrigDate.addMonths( 1 ) );
	else if( mDisplayMode == Year )
		setDate( mOrigDate.addYears( 1 ) );
}

void ScheduleWidget::yearBack()
{
	setDate( mOrigDate.addYears(-1) );
}

void ScheduleWidget::yearForward()
{
	setDate( mOrigDate.addYears(1) );
}

void ScheduleWidget::today()
{
	setDate( QDate::currentDate() );
}

QList<ScheduleRow*> gatherRows( QList<ScheduleRow*> baseRows )
{
	QList<ScheduleRow*> ret;
	foreach( ScheduleRow * row, baseRows )
	{
		ret += row;
		ret += gatherRows( row->children() );
	}
	return ret;
}

void ScheduleWidget::setRows( QList<ScheduleRow*> rows )
{
	SW_DEBUG( "ScheduleWidget::setRows" );
	mRows = rows;
	updateDimensions();
	updateColumnPositions();
	layoutCells();
//	emit rowsChanged();
	emit layoutChanged();
	mCanvas->update();
	mHeader->update();
	mPanel->update();
}

void ScheduleWidget::slotRowsChanged()
{
	SW_DEBUG( "ScheduleWidget::slotRowsChanged" );
	QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
	mSelection.clear();
	mCurrent.clear();
	for( int i = 0; i < mRows.size(); i++ )
		mRows.at(i)->disconnect( SIGNAL( change( ScheduleRow*, int ) ), this );
	mRows.clear();
	mRows = gatherRows( mController->dataSources() );
	foreach( ScheduleRow * row, mRows )
		connect( row, SIGNAL( change( ScheduleRow *, int ) ), SLOT( slotRowChanged( ScheduleRow *, int ) ) );
	updateDimensions();
	updateColumnPositions();
	layoutCells();
	emit rowsChanged();
	emit layoutChanged();
	mCanvas->update();
	mHeader->update();
	mPanel->update();
	QApplication::restoreOverrideCursor();
}

void ScheduleWidget::slotRowChanged( ScheduleRow *, int change )
{
	if( change == ScheduleRow::SpanChange ) {
		updateCellEntries();
		layoutCells();
		emit layoutChanged();
		mPanel->update();
		mCanvas->update();
	}
}

QList<ScheduleRow*> ScheduleWidget::scheduleRowsForRow( int row )
{
	if( mDisplayMode == Month ) return mRows;
	QList<ScheduleRow*> ret;
	if( row >= 0 && row < mRows.size() )
		ret += mRows[row];
	return ret;
}

QList<ScheduleRow*> ScheduleWidget::rows() const
{
	return mRows;
}

QList<ScheduleEntry *> ScheduleWidget::cellEntries( int row, int col )
{
	int cell = row*columnCount() + col;
	if( cell < 0 || cell >= mCellEntries.size() ) return QList<ScheduleEntry *>();
	return mCellEntries[cell];
}

void ScheduleWidget::addCellEntries( ScheduleEntry * se, const QDate & _startDate, const QDate & _endDate )
{
	QDate startDate = (date() > _startDate) ? date() : _startDate,
		endDate = (this->endDate() < _endDate) ? this->endDate() : _endDate;
	if( (mDisplayMode == Month || mDisplayMode == MonthFlat) && mHideWeekends ) {
		int lastRow = -1;
		int firstCol = -1, lastCol = -1;
		QDate date = startDate;
		LOG_5( "Adding Weekendless entry, start date: " + startDate.toString() + " endDate: " + endDate.toString() );
		while( date <= endDate ) {
			int row, col;
			int cell = dateCell( date, se->row(), &row, &col );
			LOG_5( "Date " + date.toString() + " cell: " + QString::number( cell ) + " row: " + QString::number( row ) + " col: " + QString::number( col ) );

			if( cell >= 0 )
				mCellEntries[cell] += se;

			if( (row != lastRow || cell < 0) && lastRow >= 0 ) {
				mRowEntries[lastRow] += RowEntry(se, firstCol, lastCol);
				LOG_5( "Adding row entry, row: " + QString::number( lastRow ) + " start col: " + QString::number( firstCol ) + " last col: " + QString::number( lastCol ) );
				lastRow = firstCol = lastCol = -1;
			}

			if( cell >= 0 ) {
				if( lastRow == -1 ) {
					lastRow = row;
					firstCol = col;
				}
				lastCol = col;
			}

			date = date.addDays(1);
		}
		if( lastRow >= 0 ) {
			LOG_5( "Adding row entry, row: " + QString::number( lastRow ) + " start col: " + QString::number( firstCol ) + " last col: " + QString::number( lastCol ) );
			mRowEntries[lastRow] += RowEntry(se, firstCol, lastCol);
		}
	} else {
		int startRow, startCol, endRow, endCol;
		int start = dateCell( startDate, se->row(), &startRow, &startCol );
		int end = dateCell( endDate, se->row(), &endRow, &endCol );
		if( end < start ) end = start;
		start = qMax(0,start);
		end = qMin(end,mCellEntries.size()-1);
		for( int i = start; i <= end; i++ )
			mCellEntries[i] += se;
		for( int i = startRow; i <= endRow; i++ ) {
			int row_startCol = (i == startRow) ? startCol : 0;
			int row_endCol = (i == endRow) ? endCol : columnCount() - 1;
			mRowEntries[i] += RowEntry(se,row_startCol,row_endCol);
		}
	}
}

void ScheduleWidget::removeCellEntries( ScheduleEntry * se, const QDate & startDate, const QDate & endDate )
{
	int start = dateCell( startDate, se->row() );
	int end = dateCell( endDate, se->row() );
	if( end < start ) end = start;
	end = qMin(end,mCellEntries.size()-1);
	if( start >= 0 )
		for( int i = start; i <= end; i++ )
			mCellEntries[i].removeAll( se );
	int startRow = start / columnCount();
	int endRow = end / columnCount();
	for( int i = startRow; i < endRow; i++ )
		mRowEntries[i].removeAll(RowEntry(se,0,0));
}

void ScheduleWidget::updateCellEntries()
{
	// Clear the existing vector, and create a new one
	// with the correct number of cells
	if( !mUpdatingCellEntries ) {
		mUpdatingCellEntries = true;
		mCellEntries = QVector<QList<ScheduleEntry*> >( qMax(0,cellCount()) );
		mRowEntries = QVector<QList<RowEntry> >( rowCount() );
		foreach( ScheduleRow * sr, mRows ) {
			sr->setDateRange( mDate, endDate() );
			QList<ScheduleEntry*> entries = sr->range();
			foreach( ScheduleEntry * entry, entries ) {
				entry->mLayouts.clear();
				addCellEntries( entry, entry->dateStart(), entry->dateEnd() );
			}
		}
		for( int i=rowCount()-1; i>=0; i-- )
			qSort(mRowEntries[i]);
		mUpdatingCellEntries = false;
	}
}

// Returns true if the row heights have changed
bool ScheduleWidget::layoutCells( int rowStart, int rowEnd, QPaintDevice * device, int desiredHeight )
{
	if( desiredHeight <= 0 )
		desiredHeight = mScrollView->viewport()->height();

	if( rowEnd == -1 )
		rowEnd = rowCount()-1;

	int heightUsed = 0;
	QMap<int,QList<int> > rowsByHeight;
	int minColWidth = INT_MAX;

	// Make sure we have a paint device for dpi values
	if( !device )
		device = mCanvas;

	// There also QPaintDevice::physicalDpi[XY], but Qt's docs
	// don't say what the difference is...
	int dpix = device->logicalDpiX();
	//int dpiy = device->logicalDpiY();

	// Size stuff at 72 dpi pixel by default, so that it stays
	// pixel perfect at the average screen resolution
	int spacing_x = 2 * dpix / 72;
	int spacing_y = 2 * dpix / 72;

	QFontMetrics fontMet( mDisplayOptions.cellDayFont, device );

	mCellMinHeight = fontMet.lineSpacing();

	if( mCellLeadHeight > 0 )
		mCellLeadHeight = int( fontMet.lineSpacing() * 1.3 );

	SW_DEBUG( "ScheduleWidget::layoutCells: mCellMinHeight: " + QString::number(mCellMinHeight) + " mCellLeadHeight: " + QString::number( mCellLeadHeight ) );
	for( int row = 0; row < rowCount(); row++ ) {
		if( row < rowStart || row > rowEnd ) {
			int rh = qMax(mCellMinHeight,rowHeight(row));
			mRowHeights[row] = rh;
			QList<int> & hl = rowsByHeight[rh];
			hl += row;
			heightUsed += rh;
		}
	}

	for( int row = rowStart; row <= rowEnd; row++ ) {
		int rowHeight = 0;
		bool hidden = false;
		if( mDisplayMode != Month ) {
			QList<ScheduleRow*> rows = scheduleRowsForRow( row );
			if( rows.size() == 1 ) {
				ScheduleRow * r = rows[0]->parent();
				while( r ) {
					if( !r->expanded() ) {
						hidden = true;
						break;
					}
					r = r->parent();
				}
			}
		}
		if( !hidden ) {
			bool compact = true;
			QMap<int/*col*/,int/*colpos*/> colFill;
			int rowPos = spacing_y + mCellLeadHeight;
			int posInRow = 0;

			QList<RowEntry> & rowEntries = mRowEntries[row];
			foreach( RowEntry re, rowEntries ) {
				RowLayout & rl = re.entry->mLayouts[row];
				rl.mPosInRow = posInRow++;
				rl.mColStart = re.colStart;
				rl.mColEnd = re.colEnd;
				int startPos = colPos(rl.mColStart);
				int width = colPos(rl.mColEnd) + colWidth(rl.mColEnd) - startPos;
				int height = re.entry->heightForWidth( width-spacing_x*2, device, mDisplayOptions, colWidth(rl.mColEnd) );
				int y = rowPos;
				if( compact ) {
					for( int i=re.colStart; i<=re.colEnd; i++ ) {
						if( y < colFill[i] )
							y = colFill[i];
					}
					for( int i=re.colStart; i<=re.colEnd; i++ )
						colFill[i] = y + spacing_y + height;
				}
				rl.mRect = QRect( startPos + spacing_x, y, colPos(rl.mColEnd) + colWidth(rl.mColEnd) - startPos - spacing_x * 2 - 2, height );
				if( !compact )
					rowPos += height + spacing_y;
			}

			if( compact )
				for( int i=0; i < columnCount(); i++ )
					if( rowPos < colFill[i] )
						rowPos = colFill[i];

			rowHeight = qMax(mCellMinHeight,rowPos+3);
			QList<int> & hl = rowsByHeight[rowHeight];
			hl += row;
			heightUsed += rowHeight;
		}
		SW_DEBUG( "Row: " + QString::number(row) + " height: " + QString::number( rowHeight ) );
		mRowHeights[row] = rowHeight;
	}
//	SW_DEBUG( "minColWidth :" + QString::number( minColWidth ) );
//	SW_DEBUG( "viewport height: " + QString::number( mScrollView->viewport()->height() ) + " heightUsed: " + QString::number( heightUsed ) );
	if( mDisplayMode == Month ) {
		while( heightUsed < desiredHeight && !rowsByHeight.isEmpty() ) {
			int spaceLeft = desiredHeight - heightUsed;
			QList<int> rl = rowsByHeight.begin().value();
			int height = rowsByHeight.begin().key();
	
			// We dont want cells that are taller than they are wide
			if( height >= minColWidth ) break;
	
			rowsByHeight.remove(height);
			int maxAdd = spaceLeft / rl.size();
	//		if( maxAdd == 0 ) break;
	
			if( !rowsByHeight.isEmpty() )
				maxAdd = qMin(maxAdd,rowsByHeight.begin().key() - height);
			
			int n = 0;
			foreach( int i, rl ) {
				SW_DEBUG( "Expanding row: " + QString::number( i ) + " by " + QString::number( maxAdd ) + " to " + QString::number( height + maxAdd ) );
				int add = maxAdd;
				if( n++ < spaceLeft % rl.size() )
					add++;
				if( add == 0 ) continue;
				mRowHeights[i] = height + add;
				rowsByHeight[mRowHeights[i]] += i;
				heightUsed += add;
			}
		}
	}
	SW_DEBUG( "Final Height: " + QString::number( heightUsed ) );

	//
	// Updates the row positions and resizes the canvas
	// based on the row heights.
	//
	SW_DEBUG( "ScheduleWidget::updateRowPositions" );
	bool changes = false;
	int rows = rowCount();
	int pos = 0, row = 0;
	for( int i=0; i<rows; i++ )
	{
		pos += mRowHeights[row];
		if( !changes && mRowPositions[row] != pos )
			changes = true;
		mRowPositions[row++] = pos;
	}
	mCanvas->setMinimumHeight( pos );
	mCanvas->resize( mCanvas->width(), pos );
	return changes;

/*
	QList<ScheduleEntry*> lastEntries;
	int nc = columnCount();
	if( startCell > 0 ) lastEntries = cellEntries( (startCell - 1)/nc, (startCell - 1)%nc );
	for( int i=startCell; i < endCell; i++ ) {
		int col = i % nc, row = i / nc;
		int cp = colPos( col ), cw = colWidth( col );
	
		// Start layout over if we are starting a new row
		// Year view is entries per week, each is separate
		if( col == 0 || mDisplayMode == Year ) lastEntries.clear();
		
		QList<ScheduleEntry *> entries = cellEntries( row, col );
		QList<ScheduleEntry *> newEntries;
		QMap<uint, ScheduleEntry*> used;
		foreach( ScheduleEntry * se, entries ) {
			if( !lastEntries.contains( se ) )
				newEntries += se;
			else {
				RowLayout & rl = se->mLayouts[row];
				used[rl.mPosInRow] = se;
				if( dateAtCell( row, col ) == se->dateEnd() || col == nc - 1 ) {
					rl.mRect.setRight( cp + cw - 5 );
					rl.mColEnd = col;
				}
			}
		}
		int i=0;
		foreach( ScheduleEntry * se, newEntries ) {
			while( used.contains( i ) ) i++;
			used[i] = se;
			RowLayout & rl = se->mLayouts[row];
			rl.mPosInRow = i;
			rl.mColStart = rl.mColEnd = col;
			rl.mRect = QRect( cp + 2, mCellLeadHeight + i * (mEntryHeight + 3) + 2, cw - 6, mEntryHeight );
			mRowHeights[row] = qMax( mRowHeights[row], mCellLeadHeight + (i + 1) * (mEntryHeight + 3) + 2 );
		}
		mRowHeights[row] = qMax( mRowHeights[row], qMax(mCellMinHeight,mCellLeadHeight) );
		lastEntries = entries;
	}
*/
}

ScheduleEntry * ScheduleWidget::entryAtPos( const QPoint & pos, QRect * entryRect )
{
	int row, col;
	QPoint inner;
	if( !pointToCell( pos, &row, &col, &inner ) )
		return 0;
	QList<ScheduleEntry*> entries = cellEntries( row, col );
	foreach( ScheduleEntry * se, entries ) {
		RowLayout & rl = se->mLayouts[row];
		if( rl.mRect.contains( QPoint( pos.x(), inner.y() ) ) ) {
			if( entryRect ) {
				*entryRect = rl.mRect;
				entryRect->translate( 0, rowPos( row ) );
			}
			return se;
		}
	}
	return 0;
}

//
// Display Mode and options
//

void ScheduleWidget::setDisplayMode( int mode )
{
	SW_DEBUG( "ScheduleWidget::setDisplayMode" );

	if( mode < Week && mode > Month )
		return;

	mDisplayMode = mode;
	if( mDisplayMode == Month ) {
		// Will be adjusted to the font's lineSpacing in layoutCells
		mCellLeadHeight = 1;
		mPanelContainer->hide();
	} else {
		mCellLeadHeight = 0;
		mCellMinHeight = qMax(mCellMinHeight,fontMetrics().lineSpacing() + 5);
		mPanelContainer->show();
		QList<int> sizes = mSplitter->sizes();
		if( sizes[0] < 50 ) {
			sizes[1] = sizes[1] - (100-sizes[0]);
			sizes[0] = 100;
			mSplitter->setSizes( sizes );
		}
	}
	setDate( mOrigDate );
}

int ScheduleWidget::displayMode() const
{
	return mDisplayMode;
}

int ScheduleWidget::selectionMode() const
{
	return mSelectionMode;
}

void ScheduleWidget::setSelectionMode( int sm )
{
	switch( sm ) {
		case SingleSelect:
		case ExtendedSelect:
			mSelectionMode = sm;
			// TODO: Update the selection
	};
}

ScheduleSelection ScheduleWidget::selection()
{
	return mSelection;
}

void ScheduleWidget::setSelection( const ScheduleSelection & ss )
{
	mSelection = ss;
	mCanvas->update();
}

ScheduleSelection ScheduleWidget::current()
{
	return mCurrent;
}

bool ScheduleWidget::moveCurrent( int cell, ScheduleEntry * se, Qt::KeyboardModifiers modifiers )
{
	ScheduleSelection cur = current();
	ScheduleSelection ss = selection();

	// If we are in cell selection mode, ignore any entries
	if( cur.mCellSelections.size() && (modifiers & (Qt::ShiftModifier | Qt::ControlModifier)) )
		se = 0;

	int selectStart = cell;

	if( mSelectionMode == ExtendedSelect && (modifiers & Qt::ShiftModifier) ) {
		bool isContinuous = true;
		if( se ) {
			isContinuous = false;
			foreach( ScheduleEntrySelection ses, cur.mEntrySelections ) {
				if( ses.mEntry == se )
					isContinuous = true;
			}
		}
		if( isContinuous ) {	
			int currentCell = -1;
			if( cur.mCellSelections.size() )
				currentCell = cur.mCellSelections[0].mCellStart;
			else if( cur.mEntrySelections.size() )
				currentCell = cur.mEntrySelections[0].mCellStart;
			if( currentCell != -1 )
				selectStart = currentCell;
		}
	}

	if( mSelectionMode == SingleSelect || !(modifiers & (Qt::ShiftModifier | Qt::ControlModifier)) )
		ss.clear();

	int min = qMin( selectStart, cell );
	int max = qMax( selectStart, cell );

	if( mDisplayMode == ScheduleWidget::Month ) {
		for( int i=min; i<=max; i++ ) {
			ss.select( i, se, (modifiers & Qt::ControlModifier) ? ScheduleSelection::InvertSelection : ScheduleSelection::AddSelection );
		}
	} else {
		int cc = columnCount();
		int col_start = qMin( min % cc, max % cc );
		int col_end = qMax( min % cc, max % cc );
		int row_start = qMin( min / cc, max / cc );
		int row_end = qMax( min / cc, max / cc );
		for( int r = row_start; r <= row_end; ++r ) {
			int rowStartCell = r * cc;
			if( se ) {
				for( int c = col_start; c<=col_end; c++ )
					ss.select( rowStartCell + c, se, ScheduleSelection::AddSelection );
			} else
				ss.mCellSelections += ScheduleCellSelection( this, rowStartCell + col_start, rowStartCell + col_end );
		}
	}

	setSelection( ss );
	cur.clear();
	cur.select( cell, se, ScheduleSelection::AddSelection );
	mCurrent = cur;
	return true;
}

void ScheduleWidget::setShowWeekends( bool showWeekends )
{
	if( showWeekends != mHideWeekends ) return;
	SW_DEBUG( "ScheduleWidget::setShowWeekends" );
	mHideWeekends = !showWeekends;
	setDate( mOrigDate );
}

bool ScheduleWidget::showWeekends() const
{
	return !mHideWeekends;
}

int ScheduleWidget::weekStartDay() const
{
	return mWeekStartDay;
}

void ScheduleWidget::setWeekStartDay( int wsd )
{
	mWeekStartDay = wsd % 7;
	setDate( mOrigDate );
}

void ScheduleWidget::setZoom( int zoom )
{
	if( zoom > 0 && zoom < 10000 && zoom != mZoom )
	{
		SW_DEBUG( "ScheduleWidget::setZoom" );
		mZoom = zoom;
		updateColumnPositions();
		layoutCells();
		mCanvas->update();
		mHeader->update();
		mPanel->update();
	}
}

int ScheduleWidget::zoom() const
{
	return mZoom;
}

void ScheduleWidget::contentsMoved( int x, int y )
{
	SW_DEBUG( "ScheduleWidget::contentsMoved" );
	if( y != mContentsYOffset ) {
		mContentsYOffset = y;
		mPanel->update();
	}
	if( x != mContentsXOffset ) {
		mContentsXOffset = x;
		mHeader->update();
	}
}

void ScheduleWidget::resizePanel()
{
	int panelHeight = mScrollView->viewport()->height() + mPanel->tree()->header()->height();
	if( mScrollView->horizontalScrollBar()->isVisible() && mPanel->tree()->horizontalScrollBar()->isVisible() )
		panelHeight += mPanel->tree()->horizontalScrollBar()->height();
	mPanel->setGeometry( 0, mHeader->height() - mPanel->tree()->header()->height(), mPanelContainer->width(), panelHeight );
}

bool ScheduleWidget::eventFilter( QObject * o, QEvent * event )
{
	if( event->type() == QEvent::Resize ) {
		QResizeEvent * re = (QResizeEvent*)event;
		if( o == mHeaderCanvasContainer )
		{
			SW_DEBUG( "ScheduleWidget::eventFilter: Header Canvas Container Resized to " + QString::number( re->size().width() ) + " x " + QString::number( re->size().height() ) );
			int headerHeight = mHeader->sizeHint().height();
			mHeader->setGeometry( 0, 0, re->size().width(), headerHeight );
			mScrollView->setGeometry( 0, headerHeight, re->size().width(), re->size().height() - headerHeight );
			updateColumnPositions();
			layoutCells();
			resizePanel();
		}
		if( o == mPanelContainer )
		{
			SW_DEBUG( "ScheduleWidget::eventFilter: Panel Container Resized to " + QString::number( re->size().width() ) + " x " + QString::number( re->size().height() ) );
			resizePanel();
		}
	}
	if( event->type() == QEvent::Show ) {
		IniConfig & c = userConfig();
		c.pushSection( "Calendar" );
		int panelWidth = c.readInt( "PanelWidth" );
		QList<int> splitterSizes;
		splitterSizes << panelWidth << qMax(1,width() - panelWidth);
		mSplitter->setSizes( splitterSizes );
		c.popSection();
	}
	return QWidget::eventFilter( o, event );
}

void ScheduleWidget::print( QPrinter * printer, PrintOptions & po )
{
	if( !printer )
		return;

	setUpdatesEnabled( false );

	//options
	int horizPages = po.horizontalPages;

	bool headerOnEveryPage = po.headerOnEveryPage;
	bool panelOnEveryPage = po.panelOnEveryPage;
	bool printHeader = po.printHeader;
	bool printPanel = po.printPanel;
	bool avoidRowSplits = po.avoidSplitRows;

	if( displayMode() == ScheduleWidget::Month )
		printPanel = false;

	int oldHeaderHeights[3];
	mHeader->getHeights( oldHeaderHeights[0], oldHeaderHeights[1], oldHeaderHeights[2] );
	
	mHeader->setupHeights(
		0,
		QFontMetrics(mDisplayOptions.headerMonthYearFont,printer).lineSpacing(),
		QFontMetrics(mDisplayOptions.headerDayWeekFont,printer).lineSpacing() * 2 );

	int scheduleHeaderHeight = mHeader->height();
	//int schedulePanelWidth = 0;
	int headerHeight = 0, footerHeight = 0;
	int headerFooterFontHeight = QFontMetrics( po.headerFooterFont, printer ).lineSpacing();

	if( !po.leftHeaderText.isEmpty() || !po.middleHeaderText.isEmpty() || !po.rightHeaderText.isEmpty() )
		headerHeight = headerFooterFontHeight;

	if( !po.leftFooterText.isEmpty() || !po.middleFooterText.isEmpty() || !po.rightFooterText.isEmpty() )
		footerHeight = headerFooterFontHeight;

	QSize pageSize(printer->pageRect().size());
	mCanvas->resize(QSize(pageSize.width() * horizPages,pageSize.height()-1) );
	updateColumnPositions( pageSize.width() * horizPages );

	layoutCells(0,-1, printer, pageSize.height() - scheduleHeaderHeight - headerHeight - footerHeight );
	
	QPainter p( printer );
	//p.setRenderHint( QPainter::TextAntialiasing, false );

	
	int pageHeight = pageSize.height(), pageWidth = pageSize.width();
	SW_DEBUG( "PageHeight: " + QString::number( pageHeight ) );
	
	//int pageY = 0;
	
	int currentRow = 0;
	int nextRowOffset = 0;
	/*
	 *	Loop until all the rows are printed
     */
	while( currentRow < rowCount() ) {
		int spaceOnPage = pageHeight;
		int contentsHeight = 0;
		int nextRow = currentRow;
		int currentRowOffset = 0;
		int x = 0;
		
		spaceOnPage -= headerHeight + footerHeight;

		// Figure out the size taken by the header
		if( printHeader && (currentRow == 0 || headerOnEveryPage) ) {
			spaceOnPage -= scheduleHeaderHeight;
		}

		while( nextRow < rowCount() ) {
			int height = rowHeight(nextRow);
			if( nextRowOffset ) {
				currentRowOffset = nextRowOffset;
				height -= nextRowOffset;
				nextRowOffset = 0;
			}
			if( height > spaceOnPage ) {
				LOG_3( "Row " + QString::number(nextRow) + " doesnt fit by " + QString::number( height - spaceOnPage ) + " pixels" );
				if( avoidRowSplits && nextRow != currentRow )
					break;
				nextRowOffset = spaceOnPage;
				if( nextRow == currentRow )
					nextRowOffset += currentRowOffset;
				contentsHeight += spaceOnPage;
				spaceOnPage = 0;
				break;
			}
			contentsHeight += height;
			spaceOnPage -= height;
			nextRow++;
		}
		
		SW_DEBUG( "Current Row: " + QString::number( currentRow ) + " Offset: " + QString::number( currentRowOffset ) );
		SW_DEBUG( "Painting Size: " + QString::number( pageHeight - spaceOnPage ) + " out of: " + QString::number( pageHeight ) );
		
		// Paint each page from left to right for these rows
		for( int pageX = 0; pageX < horizPages; pageX++ )
		{
			bool pagePrintHeader = ( printHeader && (currentRow==0 || headerOnEveryPage) );
			bool pagePrintPanel = ( printPanel && (pageX == 0 || panelOnEveryPage) );

			int y = rowPos(currentRow) + currentRowOffset;
			int pagePosX = 0, pagePosY = 0;
			
			if( headerHeight > 0 ) {
				p.setFont( po.headerFooterFont );
				if( !po.leftHeaderText.isEmpty() )
					p.drawText( 0, 0, pageWidth, headerHeight, Qt::AlignLeft, po.leftHeaderText );
				if( !po.middleHeaderText.isEmpty() )
					p.drawText( 0, 0, pageWidth, headerHeight, Qt::AlignCenter, po.middleHeaderText );
				if( !po.rightHeaderText.isEmpty() )
					p.drawText( 0, 0, pageWidth, headerHeight, Qt::AlignRight, po.rightHeaderText );
				p.translate( 0, headerHeight );
				pagePosY += headerHeight;
			}

			// Paint the panel
			if( pagePrintPanel ) {
				p.save();
				p.translate( 0, (pagePrintHeader ? scheduleHeaderHeight : 0) - 1 );
				pagePosX += mPanel->print( &p, y, contentsHeight );
				p.restore();
				p.resetMatrix();
				if( pagePrintHeader ) {
					p.setPen( Qt::black );
					p.drawLine( pagePosX, 0, pagePosX, scheduleHeaderHeight );
				}
			}
		
			// Paint the header
			if( pagePrintHeader ) {
				p.resetMatrix();
				p.setClipRect( pagePosX, pagePosY, pageWidth - pagePosX, scheduleHeaderHeight );
				p.translate( pagePosX, pagePosY );
				QRect r( 0, 0, pageWidth-pagePosX, scheduleHeaderHeight );
				mHeader->draw( &p, QRegion(r), r, x, pageWidth-pagePosX, scheduleHeaderHeight, false );
				p.translate( 0, scheduleHeaderHeight );
				pagePosY += scheduleHeaderHeight;
			}

			// Paint the contents
			p.resetMatrix();
			p.setClipRect( QRect( QPoint(pagePosX,pagePosY),QSize(pageWidth-pagePosX,contentsHeight)) );
			p.translate( pagePosX - x, pagePosY - y );
			mCanvas->paint( &p, QRect( x,y,pageWidth-pagePosX,contentsHeight), false );
			pagePosY += contentsHeight;

			p.resetMatrix();
			p.setClipping( false );

			printf( "Page Pos Y: %i  Page Height: %i\n", pagePosY, pageHeight );

			if( footerHeight > 0 ) {
				p.setFont( po.headerFooterFont );
				if( !po.leftFooterText.isEmpty() )
					p.drawText( 0, pageHeight - footerHeight, pageWidth, footerHeight, Qt::AlignLeft, po.leftFooterText );
				if( !po.middleFooterText.isEmpty() )
					p.drawText( 0, pageHeight - footerHeight, pageWidth, footerHeight, Qt::AlignCenter, po.middleFooterText );
				if( !po.rightFooterText.isEmpty() )
					p.drawText( 0, pageHeight - footerHeight, pageWidth, footerHeight, Qt::AlignRight, po.rightFooterText );
			}
			// Draw a frame
/*			p.setClipRect( QRect( QPoint(pagePosX,0),QSize(pageWidth-pagePosX,pageHeight)) );
			p.setPen( Qt::black );
			p.setBrush( QBrush( Qt::NoBrush ) );
			p.drawRect( QRect( QPoint(0,0), pageSize ) );
*/
			// Extend horiz pages to fit all of the contents
			x += pageWidth - pagePosX;
			if( pageX + 1 == horizPages && x < mCanvas->width() )
				horizPages++;

			// Update x coord and get ready for next page
			if( pageX + 1 < horizPages || nextRow < rowCount() )
				printer->newPage();
		}
		currentRow = nextRow;
	}

	mHeader->setupHeights( oldHeaderHeights[0], oldHeaderHeights[1], oldHeaderHeights[2] );
	updateColumnPositions();
	layoutCells();
	setUpdatesEnabled( true );
}


