
#include <qinputdialog.h>
#include <qsqlquery.h>

#include "database.h"

#include "graphitewidget.h"

#include "user.h"

#include "graphitesavedialog.h"

GraphiteSaveDialog::GraphiteSaveDialog( const GraphiteSavedDesc & savedDesc, QWidget * parent )
: QDialog( parent )
, mSavedDesc( savedDesc )
{
	setupUi(this);
	QStringList groups;
	QSqlQuery q = Database::current()->exec( "SELECT \"group\" FROM graphitesaveddesc GROUP BY \"group\" ORDER BY \"group\" DESC" );
	while( q.next() )
		groups.append( q.value(0).toString() );
	mGroupCombo->addItems( groups );
	mGroupCombo->setEditable( true );
}

void GraphiteSaveDialog::accept()
{
	if( !mSavedDesc.url().isEmpty() ) {
		mSavedDesc.setName( mNameEdit->text() );
		mSavedDesc.setGroup( mGroupCombo->currentText() );
		mSavedDesc.setUser( User::currentUser() );
		mSavedDesc.setModified( QDateTime::currentDateTime() );
		mSavedDesc.commit();
	}
	QDialog::accept();
}

GraphiteSaveAction::GraphiteSaveAction( GraphiteWidget * widget, QObject * parent )
: QAction( "Save Graph...", parent )
, mGraphiteWidget( widget )
{
	connect( this, SIGNAL( triggered() ), SLOT( slotTriggered() ) );
}

void GraphiteSaveAction::slotTriggered()
{
	GraphiteSaveDialog * gsd = new GraphiteSaveDialog( GraphiteSavedDesc().setUrl(mGraphiteWidget->desc().buildUrl().toString()), mGraphiteWidget->window() );
	if( gsd->exec() == QDialog::Accepted ) {
		emit saved( mGraphiteWidget, gsd->savedDesc() );
	}
	delete gsd;
}

GraphiteLoadMenu::GraphiteLoadMenu( const QString & title, QWidget * parent )
: QMenu( title, parent )
{
	connect( this, SIGNAL( aboutToShow() ), SLOT( slotAboutToShow() ) );
}

void GraphiteLoadMenu::slotAboutToShow()
{
	mSavedDescMap = GraphiteSavedDesc::select().groupedBy<QString,GraphiteSavedDescList>( "group" );
	clear();
	QStringList groups(mSavedDescMap.keys());
	groups.sort();
	foreach( QString group, groups ) {
		QMenu * menu = new QMenu( group, this );
		addMenu( menu );
		connect( menu, SIGNAL( aboutToShow() ), SLOT( slotAboutToShowGroupMenu() ) );
		connect( menu, SIGNAL( triggered(QAction*) ), SLOT( slotSavedDescTriggered(QAction*) ) );
	}
}

void GraphiteLoadMenu::slotAboutToShowGroupMenu()
{
	QMenu * groupMenu = qobject_cast<QMenu*>(sender());
	if( groupMenu ) {
		groupMenu->clear();
		QString groupName = groupMenu->title();
		foreach( GraphiteSavedDesc sd, mSavedDescMap[groupName].sorted( "name" ) ) {
			QAction * action = groupMenu->addAction( sd.name() );
			action->setProperty( "savedDesc", qVariantFromValue<Record>(sd) );
		}
	}
}

void GraphiteLoadMenu::slotSavedDescTriggered( QAction * action )
{
	GraphiteSavedDesc sd = qvariant_cast<Record>(action->property( "savedDesc" ));
	if( sd.isRecord() )
		emit loadGraph(sd);
}

GraphiteGenerateSeriesAction::GraphiteGenerateSeriesAction( GraphiteWidget * widget, QObject * parent )
: QAction( "Generate Series...", parent )
, mGraphiteWidget( widget )
{
	connect( this, SIGNAL( triggered() ), SLOT( slotTriggered() ) );
}
	
void GraphiteGenerateSeriesAction::slotTriggered()
{
	bool okay = false;
	int count = QInputDialog::getInt( mGraphiteWidget, "How many graphs in the series?", "How many graphs do you want to generate?", 1, 1, 50, 1, &okay );
	if( okay )
		emit generateSeries( mGraphiteWidget->desc().generateTimeSeries(count) );
}
