
#include <qlayout.h>
#include <qmenu.h>
#include <qpainter.h>

#include "blurqt.h"
#include "iniconfig.h"

#include "schedulepanel.h"
#include "schedulewidget.h"
#include "schedulerow.h"
#include "recordtreeview.h"

struct RowPanelItem : public ItemBase
{
	ScheduleRow * row;
	int pos;
	QPixmap img;

	RowPanelItem()
	: row( 0 )
	, pos( 0 ) {}

	void setup( ScheduleRow * r, int p )
	{
		row = r;
		pos = p;
		img = row->image();
	}
	QVariant modelData( const QModelIndex & i, int role ) const {
		int col = i.column();
		if( role == Qt::DisplayRole || role == Qt::EditRole ) {
			switch( col ) {
				case 0: return row->name();
				case 1: return row->startDate();
				case 2: return row->endDate();
				case 3: return row->duration();
			}
		} else if( role == Qt::DecorationRole && col == 0 )
			return img;
		return QVariant();
	}
	bool setModelData( const QModelIndex & i, const QVariant & v, int role ) {
		int col = i.column();
		if( role == Qt::EditRole ) {
			switch( col ) {
				case 1:
					row->setStartDate( v.toDate() );
					break;
				case 2:
				{
					row->setEndDate( v.toDate() );
					break;
				}
				case 3:
					row->setEndDate( row->startDate().addDays( v.toInt() ) );
					break;
			}
			return true;
		}
		return false;
	}
	Qt::ItemFlags flags( const QModelIndex & i ) {
		switch( i.column() ) {
			case 0:
				return Qt::ItemFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
			case 1:
			case 2:
			case 3:
				return Qt::ItemFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable );
		}
		return Qt::ItemFlags();
	}
};

typedef TemplateDataTranslator<RowPanelItem> RowPanelTranslator;

class RowItemTreeBuilder : public ModelTreeBuilder
{
public:
	RowItemTreeBuilder( SuperModel * model )
	: ModelTreeBuilder( model )
	{ new RowPanelTranslator(this); }

	bool hasChildren( const QModelIndex & parentIndex, SuperModel * model )
	{
		return false;
	}
	
	void loadChildren( const QModelIndex & parentIndex, SuperModel * model )
	{
	}

	int load( const QModelIndex & parentIndex, SuperModel * model, QList<ScheduleRow*> children, int pos=0 )
	{
		int cnt = children.size();
		model->clearChildren(parentIndex);
		SuperModel::InsertClosure closure(model);
		model->append(parentIndex,cnt);
		for( int i=0; i<cnt; i++ ) {
			QModelIndex idx = model->index(i,0,parentIndex);
			ScheduleRow * row = children[i];
			RowPanelTranslator::data(idx).setup(row,pos++);
			QList<ScheduleRow*> kids = row->children();
			if( kids.size() )
				pos = load( idx, model, kids, pos );
		}
		return pos;
	}
};


class PanelTreeDelegate : public ExtDelegate
{
public:
	PanelTreeDelegate( QObject * parent, ScheduleWidget * sw )
	: ExtDelegate( parent )
	, mSchedule( sw )
	{}

	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		QSize size = ExtDelegate::sizeHint(option,index);
		int pos = RowPanelTranslator::data(index).pos;
		int height = mSchedule->rowHeight(pos);
		return QSize( size.width(), height );
	}
protected:
	ScheduleWidget * mSchedule;
};

SchedulePanel::SchedulePanel( ScheduleWidget * scheduleWidget, QWidget * parent )
: QWidget( parent )
, mHighlightStart( -1 )
, mScheduleWidget( scheduleWidget )
{
	connect( sched(), SIGNAL( highlightChanged( const QRect & ) ), SLOT( highlightChanged( const QRect & ) ) );
	mTree = new ExtTreeView(this);
	mTree->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	mTree->setFrameShape( QFrame::NoFrame );
	mTree->setItemDelegate( new PanelTreeDelegate( mTree, scheduleWidget ) );
	mTree->setIndentation( 12 );
	mTree->setShowBranches( false );
	mTree->setAlternatingRowColors( false );
	mTree->setShowGrid( true );
	mTree->setGridColors( mScheduleWidget->mCellBorder, mScheduleWidget->mCellBorderHighlight );
	mTree->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );

	connect( mTree, SIGNAL( showMenu( const QPoint&, const QModelIndex & ) ), SLOT( showMenu( const QPoint&, const QModelIndex & ) ) );
	
	QPalette p = mTree->palette();
	p.setColor( QPalette::Base, QColor( 192, 192, 192 ) );
	p.setColor( QPalette::Highlight, mScheduleWidget->mCellBackgroundHighlight );//QColor( 184, 189, 192 ) );
	mTree->setPalette( p );

	mModel = new SuperModel( mTree );
	mTreeBuilder = new RowItemTreeBuilder(mModel);
	mModel->setTreeBuilder( mTreeBuilder );
	mModel->setAutoSort( false );
	mModel->setHeaderLabels( QStringList() << "Asset" << "Start" << "End" << "Days" );
	connect( mModel, SIGNAL( layoutChanged() ), SLOT( rowOrderChanged() ) );
	mTree->setModel( mModel );
	connect( sched()->scrollArea()->verticalScrollBar(), SIGNAL( valueChanged( int ) ), mTree->verticalScrollBar(), SLOT( setValue( int ) ) );
	connect( mTree->verticalScrollBar(), SIGNAL( valueChanged( int ) ), sched()->scrollArea()->verticalScrollBar(), SLOT( setValue( int ) ) );
	connect( mTree, SIGNAL( expanded( const QModelIndex & ) ), SLOT( scheduleItemExpanded( const QModelIndex & ) ) );
	connect( mTree, SIGNAL( collapsed( const QModelIndex & ) ), SLOT( scheduleItemExpanded( const QModelIndex & ) ) );

	QLayout * layout = new QHBoxLayout(this);
	layout->addWidget(mTree);
	layout->setMargin(0);
	connect( sched(), SIGNAL( rowsChanged() ), SLOT( rowsChanged() ) );
	connect( sched(), SIGNAL( layoutChanged() ), SLOT( layoutChanged() ) );

}

SchedulePanel::~SchedulePanel()
{
}

ScheduleWidget * SchedulePanel::sched() const
{
	return mScheduleWidget;
}

void SchedulePanel::rowOrderChanged()
{
	QList<ScheduleRow*> rows;
	int pos = 0;
	for( ModelIter it( mTree->model(), ModelIter::Recursive ); it.isValid(); ++it ) {
		RowPanelItem * item = &RowPanelTranslator::data( *it );
		item->pos = pos++;
		rows += item->row;
	}
	if( !rows.isEmpty() ) {
		SW_DEBUG( "SchedulePanel::rowOrderChanged: Setting rows, count is: " + QString::number( rows.size() ) );
		sched()->setRows(rows);
		mTree->doItemsLayout();
	}
}

void SchedulePanel::highlightChanged( const QRect & rect )
{
	if( rect.y() == mHighlightRect.y() && rect.bottom() == mHighlightRect.bottom() )
		return;
	QRect damage = rect | mHighlightRect;
	mHighlightRect = rect;
	int y = sched()->rowPos( damage.y() );
	int h = sched()->rowPos( damage.bottom() + 1 ) - y;
//	y -= off;
	y = qMax( y, 0 );
	h = qMin( h, height()-y );
	update( 0, y, width(), h );
}

void SchedulePanel::rowsChanged()
{
	ScheduleWidget * s = sched();
	QList<ScheduleRow*> rows = s->rows(), rootRows;
	int row=0;
	foreach( ScheduleRow * srow, rows )
		if( !srow->parent() || !rows.contains(srow->parent()) )
			rootRows.append(srow);
	mModel->clear();
	mTreeBuilder->load( QModelIndex(), mModel, rootRows );
	mTree->expandRecursive();
}

void SchedulePanel::scheduleItemExpanded( const QModelIndex & index )
{
	RowPanelTranslator::data(index).row->setExpanded(mTree->isExpanded(index));
	sched()->layoutCells();
}

void SchedulePanel::layoutChanged()
{
	//mTree->header()->setFixedHeight( sched()->header()->height() - sched()->header()->splitHeight() );
}

void SchedulePanel::showMenu( const QPoint & pos, const QModelIndex & underMouse )
{
	LOG_5( "SchedulePanel::showMenu" );
	if( underMouse.isValid() ) {
	LOG_5( "SchedulePanel::showMenu: Valid index" );
		QMenu * menu = new QMenu( this );
		RowPanelTranslator::data( underMouse ).row->populateMenu( menu, pos, QDate(), QDate(), 0 );
		menu->exec(pos);
		delete menu;
	}
}

// Returns the width
int SchedulePanel::print( QPainter * p, int offset, int height )
{
	QAbstractItemDelegate * del = mTree->itemDelegate();
	QStyleOptionViewItem style;
	style.initFrom( mTree );
	style.font = mScheduleWidget->mDisplayOptions.panelFont;
	style.fontMetrics = QFontMetrics(style.font,p->device());
	style.decorationSize = QSize(0,0);
	style.displayAlignment = Qt::AlignVCenter | Qt::AlignLeft;

	int columnCount = 1;//mTree->model()->columnCount(mTree->rootIndex());
	QVector<int> columnSizes(columnCount,0);
	
	for( ModelIter it( mTree->model(), ModelIter::Recursive ); it.isValid() ;++it ) {
		QModelIndex row_index = *it;
		for( int c = 0; c < columnCount; c++ ) {
			QModelIndex index = row_index.sibling(row_index.row(),c);
			columnSizes[c] = qMax( columnSizes[c], del->sizeHint( style, index ).width() );
		}
	}
	
	int y = 0;
	for( ModelIter it( mTree->model(), ModelIter::Recursive ); it.isValid() ;++it ) {
		QModelIndex row_index = *it;
		QSize size = del->sizeHint( style, row_index );
		if( y + size.height() > offset + height ) break;
		if( y + size.height() > offset ) {
			int x = 0;
			for( int c = 0; c < columnCount; c++ ) {
				int offset_x = c==0 ? 5 * it.depth() * p->device()->logicalDpiX() / 72 : 0;
				QModelIndex index = row_index.sibling(row_index.row(),c);
				size = del->sizeHint( style, index );
				style.rect = QRect( QPoint(x + offset_x,y-offset), QSize(columnSizes[c]-offset_x,size.height()) );
				del->paint( p, style, index );
				p->setPen( Qt::black );
				p->drawRect( style.rect.adjusted(-offset_x,0,0,0) );
				x += columnSizes[c];
			}
		}
		y += size.height();
	}
	
	int width = 0;
	for( int i=0; i<columnCount; i++ )
		width += columnSizes[i];
	return width;
}

