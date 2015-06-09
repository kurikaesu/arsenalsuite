
#include <qclipboard.h>
#include <qevent.h>
#include <qmenu.h>
#include <qtimer.h>

#include "blurqt.h"
#include "graphitedialog.h"
#include "graphitewidget.h"

GraphiteWidget::GraphiteWidget( QWidget * parent )
: QLabel( parent )
, mResizeTimer( 0 )
, mImageSource( 0 )
, mRefreshScheduled( false )
{
	setScaledContents( true );
	mImageSource = new GraphiteImageSource(this);
	connect( mImageSource, SIGNAL( getImageFinished( const QImage & ) ), SLOT( slotImageFinished( const QImage & ) ) );
	
	setContextMenuPolicy( Qt::CustomContextMenu );
	connect( this, SIGNAL( customContextMenuRequested( const QPoint & ) ), SLOT( showContextMenu( const QPoint & ) ) );
}

// QLabel won't let you resize smaller than the pixmap without this
QSize GraphiteWidget::minimumSizeHint() const
{
	return QSize( 50, 50 );
}

GraphiteImageSource * GraphiteWidget::imageSource() const
{
	return mImageSource;
}

GraphiteDesc GraphiteWidget::desc() const
{
	return mDesc;
}

void GraphiteWidget::setGraphiteDesc( const GraphiteDesc & desc )
{
	mDesc = desc;
}

void GraphiteWidget::setSources( QStringList sources )
{
	mDesc.setSources( sources );
}

void GraphiteWidget::setAreaMode( GraphiteDesc::AreaMode areaMode )
{
	mDesc.setAreaMode( areaMode );
}

void GraphiteWidget::setDateRange( const QDateTime & start, const QDateTime & end )
{
	mDesc.setDateRange( start, end );
}

void GraphiteWidget::setValueRange( int minValue, int maxValue )
{
	mDesc.setValueRange( minValue, maxValue );
}

void GraphiteWidget::slotImageFinished( const QImage & image )
{
	setPixmap( QPixmap::fromImage(image) );
}

void GraphiteWidget::refresh()
{
	if( !mRefreshScheduled ) {
		LOG_5( "Scheduling refresh" );
		mRefreshScheduled = true;
		QTimer::singleShot( 0, this, SLOT( doRefresh() ) );
	}
}

void GraphiteWidget::showDialog()
{
	GraphiteDialog * gd = new GraphiteDialog(this);
	gd->setAttribute( Qt::WA_DeleteOnClose, true );
	gd->setGraphiteWidget(this);
	gd->show();
}

void GraphiteWidget::showContextMenu( const QPoint & pos )
{
	QMenu * menu = new QMenu(this);
	menu->addAction( "Refresh", this, SLOT( refresh() ) );
	menu->addSeparator();
	menu->addAction( "Modify Graph...", this, SLOT( showDialog() ) );
	menu->addAction( "Copy Graph URL to Clipboard", this, SLOT( copyUrlToClipboard() ) );
	emit aboutToShowMenu( menu );
	menu->exec( mapToGlobal(pos) );
	delete menu;
}

void GraphiteWidget::copyUrlToClipboard()
{
	QApplication::clipboard()->setText( mDesc.buildUrl( mImageSource->graphiteServer(), mImageSource->graphitePort() ).toString() );
}

void GraphiteWidget::doRefresh()
{
	mRefreshScheduled = false;
	mDesc.setSize( size() );
	mImageSource->getImage( mDesc );
}

void GraphiteWidget::resizeEvent( QResizeEvent * re )
{
	if( !mResizeTimer ) {
		mResizeTimer = new QTimer(this);
		mResizeTimer->setSingleShot( true );
		connect( mResizeTimer, SIGNAL( timeout() ), SLOT( refresh() ) );
	}
	mResizeTimer->start( 500 );
}

void GraphiteWidget::mouseDoubleClickEvent( QMouseEvent * )
{
	showDialog();
}
