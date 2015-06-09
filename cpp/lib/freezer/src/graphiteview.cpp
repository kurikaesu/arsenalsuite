
#include <qinputdialog.h>
#include <qlayout.h>
#include <qmainwindow.h>
#include <qmdiarea.h>
#include <qmdisubwindow.h>
#include <qmenu.h>
#include <qtoolbar.h>

#include "graphitedialog.h"
#include "graphitesavedialog.h"
#include "graphitesource.h"
#include "graphitewidget.h"

#include "graphiteview.h"

GraphiteView::GraphiteView(QWidget * parent)
: FreezerView( parent )
, mMdiArea( 0 )
, mToolBar( 0 )
, mViewSubMenu( 0 )
{
	QLayout * layout = new QVBoxLayout(this);
	mMdiArea = new QMdiArea( this );
	layout->addWidget( mMdiArea );
	mMdiArea->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( mMdiArea, SIGNAL( customContextMenuRequested( const QPoint & ) ), SLOT( showMdiAreaMenu( const QPoint & ) ) );

	RefreshGraphsAction = new QAction( "Refresh Graphs(s)", this );
	RefreshGraphsAction->setIcon( QIcon( ":/images/refresh" ) );
	
	NewGraphAction = new QAction( "New Graph...", this );
	NewGraphFromUrlAction = new QAction( "New Graph From Url...", this );
	
	connect( RefreshGraphsAction, SIGNAL( triggered(bool) ), SLOT( refresh() ) );
	connect( NewGraphAction, SIGNAL( triggered(bool) ), SLOT( newGraph() ) );
	connect( NewGraphFromUrlAction, SIGNAL( triggered(bool) ), SLOT( newGraphFromUrl() ) );
}

GraphiteView::~GraphiteView()
{
}

QString GraphiteView::viewType() const
{
	return "GraphiteView";
}

QToolBar * GraphiteView::toolBar( QMainWindow * mw )
{
	if( !mToolBar ) {
		mToolBar = new QToolBar(mw);
		mToolBar->addAction( RefreshGraphsAction );
	}
	return mToolBar;
}

void GraphiteView::populateViewSubMenu()
{
	if( !mViewSubMenu ) {
		mViewSubMenu = new QMenu( "Graphite" );
		mViewSubMenu->addAction( RefreshGraphsAction );
		mViewSubMenu->addSeparator();
		mViewSubMenu->addAction( NewGraphAction );
		QMenu * loadMenu = new GraphiteLoadMenu( "Load Graph", mViewSubMenu );
		connect( loadMenu, SIGNAL( loadGraph( const GraphiteSavedDesc & ) ), SLOT( loadSavedGraph( const GraphiteSavedDesc & ) ) );
		mViewSubMenu->addMenu( loadMenu );
		mViewSubMenu->addAction( NewGraphFromUrlAction );
	}
}

void GraphiteView::populateViewMenu( QMenu * viewMenu )
{
	populateViewSubMenu();
	viewMenu->addMenu( mViewSubMenu );
}

void GraphiteView::showMdiAreaMenu( const QPoint & pos )
{
	populateViewSubMenu();
	mViewSubMenu->exec( mMdiArea->mapToGlobal( pos ) );
}

void GraphiteView::populateGraphiteWidgetMenu( QMenu * menu )
{
	GraphiteWidget * gw = qobject_cast<GraphiteWidget*>(sender());
	if( gw ) {
		GraphiteSaveAction * gsa = new GraphiteSaveAction( gw, menu );
		menu->addAction(gsa);
		connect( gsa, SIGNAL( saved( GraphiteWidget *, const GraphiteSavedDesc & ) ), SLOT( slotGraphSaved( GraphiteWidget *, const GraphiteSavedDesc & ) ) );
		GraphiteGenerateSeriesAction * ggs = new GraphiteGenerateSeriesAction( gw, menu );
		menu->addAction(ggs);
		connect( ggs, SIGNAL( generateSeries( QList<GraphiteDesc> ) ), SLOT( generateSeries( QList<GraphiteDesc> ) ) );
	}
}

void GraphiteView::slotGraphSaved( GraphiteWidget * gw, const GraphiteSavedDesc & sd )
{
	QMdiSubWindow * subWindow = subWindowFromWidget(gw);
	if( subWindow )
		subWindow->setWindowTitle( sd.group() + " - " + sd.name() );
}

void GraphiteView::generateTimeSeries( const GraphiteDesc & desc, int count )
{
	QList<GraphiteDesc> descriptions = desc.generateTimeSeries(count);
	foreach( GraphiteDesc gd, descriptions ) {
		newGraph( gd );
	}
}

void GraphiteView::applyOptions()
{
	
}

void GraphiteView::newGraph()
{
	GraphiteWidget * gw = newGraph(GraphiteDesc());
	gw->showDialog();
}

void GraphiteView::newGraphFromUrl()
{
	bool okay = true;
	QString url = QInputDialog::getText( window(), "Enter Graphite Url", "Enter Graphite Graph Url", QLineEdit::Normal, QString(), &okay );
	if( okay )
		newGraph( GraphiteDesc::fromUrl(url) );
}

void GraphiteView::loadSavedGraph( const GraphiteSavedDesc & desc )
{
	newGraph( GraphiteDesc::fromUrl(desc.url()), QRect(), desc.group() + " - " + desc.name() );
}

QString findGraphTitle( const QString & _url )
{
	QString title;
	QString url(_url);
	url.replace( QRegExp("width=\\d+"), "width=\\d+" );
	url.replace( QRegExp("height=\\d+"), "height=\\d+" );
	url.replace( "?", "\\?" );
	GraphiteSavedDescList sdl = GraphiteSavedDesc::c.Url.regexSearch(url).select();
	if( sdl.size() ) {
		GraphiteSavedDesc sd = sdl[0];
		title = sd.group() + " - " + sd.name();
	}
	if( title.isEmpty() )
		title = "Unsaved Graph";
	return title;
}

GraphiteWidget * GraphiteView::widgetFromSubWindow( QMdiSubWindow * subWindow ) const
{
	return qobject_cast<GraphiteWidget*>(subWindow->widget());
}

QMdiSubWindow * GraphiteView::subWindowFromWidget( GraphiteWidget * graphiteWidget ) const
{
	return qobject_cast<QMdiSubWindow*>(graphiteWidget->parent());
}

QList<GraphiteWidget*> GraphiteView::graphiteWidgets() const
{
	QList<GraphiteWidget*> ret;
	foreach( QMdiSubWindow * subWindow, mMdiArea->subWindowList() ) {
		GraphiteWidget * gw = widgetFromSubWindow( subWindow );
		if( gw ) ret += gw;
	}
	return ret;
}

GraphiteWidget * GraphiteView::newGraph( const GraphiteDesc & desc, const QRect & rect, const QString & _title )
{
	GraphiteWidget * gw = new GraphiteWidget( mMdiArea );
	gw->setGraphiteDesc(desc);
	connect( gw, SIGNAL( aboutToShowMenu( QMenu * ) ), SLOT( populateGraphiteWidgetMenu( QMenu * ) ) );
	QMdiSubWindow * subWindow = mMdiArea->addSubWindow( gw );
	if( rect.isValid() )
		subWindow->setGeometry( rect );
	else
		subWindow->resize( desc.size() );
	QString title(_title);
	if( title.isEmpty() )
		title = findGraphTitle( gw->desc().buildUrl().toString() );
	subWindow->setWindowTitle( title );
	gw->show();
	return gw;
}

void GraphiteView::doRefresh()
{
	FreezerView::doRefresh();
	foreach( GraphiteWidget * gw, graphiteWidgets() )
		gw->refresh();
}

void GraphiteView::save( IniConfig & ini, bool forceFullSave )
{
	int graphCount = 0;
	foreach( GraphiteWidget * gw, graphiteWidgets() ) {
		QString graphName = "Graph_" + QString::number( graphCount );
		QMdiSubWindow * subWindow = subWindowFromWidget(gw);
		ini.writeRect( graphName + "_Rect", subWindow->geometry() );
		ini.writeString( graphName + "_Url", gw->desc().buildUrl().toString() );
		if( subWindow->windowTitle() != "Unsaved Graph" && subWindow->windowTitle().size() )
			ini.writeString( graphName + "_Title", subWindow->windowTitle() );
		graphCount++;
	}
	ini.writeInt( "GraphCount", graphCount );
	FreezerView::save(ini,forceFullSave);
}

void GraphiteView::restore( IniConfig & ini, bool forceFullRestore )
{
	int graphCount = ini.readInt( "GraphCount", 0 );
	for( int i=0; i<graphCount; i++ ) {
		QString graphName = "Graph_" + QString::number( i );
		newGraph( GraphiteDesc::fromUrl(ini.readString( graphName + "_Url" )), ini.readRect( graphName + "_Rect" ), ini.readString( graphName + "_Title") );
	}
	FreezerView::restore(ini,forceFullRestore);
}
