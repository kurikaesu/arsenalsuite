
#include <qmenu.h>

#include "graphitewidget.h"
#include "graphiteoptionswidget.h"

GraphiteOptionsWidget::GraphiteOptionsWidget( QWidget * parent )
: QWidget( parent )
, mGraphiteWidget( 0 )
{
	setupUi(this);
	connect( mTimeRangeCombo, SIGNAL( currentIndexChanged(int) ), SLOT( timeRangeComboChanged() ) );
	connect( mValueRangeCombo, SIGNAL( currentIndexChanged(int) ), SLOT( valueRangeComboChanged() ) );
	mTimeRangeGroup->hide();
	mValueRangeGroup->hide();

	mExtraList->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( mExtraList, SIGNAL( customContextMenuRequested ( const QPoint & ) ), SLOT( showExtraListMenu( const QPoint & ) ) );
}

void GraphiteOptionsWidget::setGraphiteWidget( GraphiteWidget * graphiteWidget )
{
	mGraphiteWidget = graphiteWidget;
	resetOptions();
}

void GraphiteOptionsWidget::applyOptions()
{
	if( !mGraphiteWidget ) return;
	int minValue = INT_MAX, maxValue = INT_MAX;
	GraphiteDesc::AreaMode areaMode = GraphiteDesc::None;
	GraphiteDesc gd = mGraphiteWidget->desc();
	
	QString trt = mTimeRangeCombo->currentText();
	if( trt == "Last Week" ) {
		gd.setRelativeStart( Interval::fromString( "-7 days" ) );
	} else if( trt == "Last Month" ) {
		gd.setRelativeStart( Interval::fromString( "-1 month" ) );
	} else if( trt == "Last Year" ) {
		gd.setRelativeStart( Interval::fromString( "-1 year" ) );
	} else if( trt == "Specify Range..." ) {
		QDateTime start, end;
		end = mEndDateTimeEdit->dateTime();
		start = mStartDateTimeEdit->dateTime();
		gd.setDateRange(start,end);
	}
	
	QString vrt = mValueRangeCombo->currentText();
	if( vrt.startsWith( "Percentage" ) ) {
		minValue = 0;
		maxValue = 100;
	} else if( vrt == "Specify Range..." ) {
		minValue = mMinValueSpin->value();
		maxValue = mMaxValueSpin->value();
	}

	areaMode = GraphiteDesc::areaModeFromString(mAreaModeCombo->currentText());
	
	gd.setAreaMode( areaMode );
	gd.setValueRange( minValue, maxValue );
	
	QList< QPair<QString,QString> > extraItems;
	for( int i=0; i<mExtraList->count(); i++ ) {
		QString extra = mExtraList->item(i)->text();
		QStringList parts = extra.split("=");
		if( parts.size() == 2 )
			extraItems.append( QPair<QString,QString>(parts[0],parts[1]) );
		else
			extraItems.append( QPair<QString,QString>(extra,QString()) );
	}
	gd.setExtraQueryItems(extraItems);

	mGraphiteWidget->setGraphiteDesc(gd);
	mGraphiteWidget->refresh();
}

void GraphiteOptionsWidget::resetOptions()
{
	if( !mGraphiteWidget ) return;
	GraphiteDesc gd = mGraphiteWidget->desc();
	
	// Qt::MatchFixedString is case insensitive unless also passed with Qt::MatchCaseSensitive
	mAreaModeCombo->setCurrentIndex( mAreaModeCombo->findText(GraphiteDesc::areaModeToString(gd.areaMode()), Qt::MatchFixedString) );
	
	if( gd.minValue() == 0 && gd.maxValue() == 100 )
		mValueRangeCombo->setCurrentIndex( 1 );
	else if( gd.minValue() != INT_MAX || gd.maxValue() != INT_MAX )
		mValueRangeCombo->setCurrentIndex( 2 );
	else
		mValueRangeCombo->setCurrentIndex( 0 );
	
	// absolute start is null, absolute end is null
	bool asin = gd.start().isNull(), aein = gd.end().isNull();
	int trci = 0;
	
	if( asin && aein && gd.relativeEnd() == Interval() ) {
		Interval rs = gd.relativeStart();
		// Default to Last Week if nothing else matches exactly, until the dialog
		// has proper support for specifying intervals
		if( rs == Interval::fromString( "-1 week" ) )
			trci = 1;
		else if( rs == Interval::fromString( "-1 month" ) )
			trci = 2;
		else if( rs == Interval::fromString( "-1 year" ) )
			trci = 3;
	} else if( !asin && !aein ) {
		trci = 3;
		mStartDateTimeEdit->setDateTime( gd.start() );
		mEndDateTimeEdit->setDateTime( gd.end() );
	}
	mTimeRangeCombo->setCurrentIndex(trci);
	
	mExtraList->clear();
	QPair<QString,QString> extra;
	foreach( extra, gd.extraQueryItems() )
		(new QListWidgetItem( extra.first + "=" + extra.second, mExtraList))->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled );
}

void GraphiteOptionsWidget::timeRangeComboChanged()
{
	bool manual = mTimeRangeCombo->currentText() == "Specify Range...";
	if( manual )
		mTimeRangeGroup->show();
	else
		mTimeRangeGroup->hide();
}

void GraphiteOptionsWidget::valueRangeComboChanged()
{
	bool manual = mValueRangeCombo->currentText() == "Specify Range...";
	if( manual )
		mValueRangeGroup->show();
	else
		mValueRangeGroup->hide();
}

void GraphiteOptionsWidget::showExtraListMenu( const QPoint & point )
{
	QMenu * menu = new QMenu(this);
	QAction * newItem = menu->addAction( "New Item" ), * removeItem = 0;
	if( mExtraList->selectedItems().size() ) {
		removeItem = menu->addAction( "Remove Selected Item" );
	}
	if( QAction * result = menu->exec(mExtraList->mapToGlobal(point)) ) {
		if( result == newItem ) {
			QListWidgetItem * newItem = new QListWidgetItem( "New Item", mExtraList );
			newItem->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled );
			mExtraList->clearSelection();
			mExtraList->editItem( newItem );
		} else if( result == removeItem ) {
			foreach( QListWidgetItem * item, mExtraList->selectedItems() )
				delete item;
		}
	}
	delete menu;
}
