
#include <qmenu.h>

#include "graphitewidget.h"
#include "graphitesourceswidget.h"

GraphiteSourcesWidget::GraphiteSourcesWidget(QWidget * parent)
: QWidget( parent )
, mGraphiteWidget( 0 )
{
	setupUi(this);
	mSourceList->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( mSourceList, SIGNAL( customContextMenuRequested ( const QPoint & ) ), SLOT( showListMenu( const QPoint & ) ) );
}

void GraphiteSourcesWidget::setGraphiteWidget( GraphiteWidget * gw )
{
	mGraphiteWidget = gw;
	mOriginalSources = gw->desc().sources();
	reset();
}

QStringList GraphiteSourcesWidget::sources()
{
	QStringList ret;
	for( int i=0; i<mSourceList->count(); i++ )
		ret += mSourceList->item(i)->text();
	return ret;
}

void GraphiteSourcesWidget::setSources( QStringList sources )
{
	mSourceList->clear();
	foreach( QString source, sources )
		(new QListWidgetItem( source, mSourceList))->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled );
}

void GraphiteSourcesWidget::apply()
{
	if( mGraphiteWidget ) {
		mGraphiteWidget->setSources( sources() );
	}
}

void GraphiteSourcesWidget::reset()
{
	if( mGraphiteWidget ) {
		setSources( mOriginalSources );
		mGraphiteWidget->setSources( mOriginalSources );
	}
}

void GraphiteSourcesWidget::showListMenu( const QPoint & point )
{
	QMenu * menu = new QMenu(this);
	QAction * newSource = menu->addAction( "New Sources" ), * removeSource = 0, * moveToTop = 0, * moveToBottom = 0;
	if( mSourceList->selectedItems().size() ) {
		removeSource = menu->addAction( "Remove Selected Source" );
		moveToTop = menu->addAction( "Move Selected Source to Top" );
		moveToBottom = menu->addAction( "Move Selected Source To Bottom" );
	}
	if( QAction * result = menu->exec(mSourceList->mapToGlobal(point)) ) {
		if( result == newSource ) {
			QListWidgetItem * newItem = new QListWidgetItem( "New Source", mSourceList );
			newItem->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled );
			mSourceList->clearSelection();
			mSourceList->editItem( newItem );
		} else if( result == removeSource ) {
			foreach( QListWidgetItem * item, mSourceList->selectedItems() )
				delete item;
		} else if( result == moveToTop ) {
			foreach( QListWidgetItem * item, mSourceList->selectedItems() ) {
				mSourceList->takeItem( mSourceList->row(item) );
				mSourceList->insertItem( 0, item );
			}
		} else if( result == moveToBottom ) {
			foreach( QListWidgetItem * item, mSourceList->selectedItems() ) {
				mSourceList->takeItem( mSourceList->row(item) );
				mSourceList->insertItem( mSourceList->count(), item );
			}
		}
	}
	delete menu;
}
