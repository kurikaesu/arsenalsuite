
#include <qpushbutton.h>

#include "graphitesourceswidget.h"
#include "graphiteoptionswidget.h"
#include "graphitedialog.h"

GraphiteDialog::GraphiteDialog( QWidget * parent )
: QDialog( parent )
, mSourcesWidget( 0 )
, mOptionsWidget( 0 )
{
	setupUi(this);
	mSourcesWidget = new GraphiteSourcesWidget(mTabWidget);
	mTabWidget->addTab( mSourcesWidget, "Sources" );
	mOptionsWidget = new GraphiteOptionsWidget(mTabWidget);
	mTabWidget->addTab( mOptionsWidget, "Options" );
	
	connect( mButtonBox->button(QDialogButtonBox::Apply), SIGNAL( clicked() ), SLOT( apply() ) );
	connect( mButtonBox->button(QDialogButtonBox::Reset), SIGNAL( clicked() ), SLOT( reset() ) );
}

void GraphiteDialog::setGraphiteWidget( GraphiteWidget * graphiteWidget )
{
	mOptionsWidget->setGraphiteWidget( graphiteWidget );
	mSourcesWidget->setGraphiteWidget( graphiteWidget );
}

void GraphiteDialog::accept()
{
	apply();
	QDialog::accept();
}

void GraphiteDialog::reject()
{
	reset();
	QDialog::reject();
}

void GraphiteDialog::apply()
{
	mOptionsWidget->applyOptions();
	mSourcesWidget->apply();
}

void GraphiteDialog::reset()
{
	mOptionsWidget->resetOptions();
	mSourcesWidget->apply();
}

