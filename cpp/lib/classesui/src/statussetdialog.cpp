

#include <qlistwidget.h>
#include <qpushbutton.h>

#include "statussetdialog.h"
#include "statusdialog.h"
#include "resinerror.h"

StatusSetDialog::StatusSetDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	
	connect( mAddButton, SIGNAL( clicked() ),  SLOT( addSet() ) );
	connect( mEditButton, SIGNAL( clicked() ), SLOT( editSet() ) );
	connect( mRemoveButton, SIGNAL( clicked() ), SLOT( removeSet() ) );
	
	connect( mSetList, SIGNAL( currentTextChanged( const QString & ) ), SLOT( setChanged( const QString & ) ) );
	
	updateSets();
}

void StatusSetDialog::accept()
{
	mRemoved.remove();
	QDialog::accept();
}

void StatusSetDialog::reject()
{
	mAdded.remove();
	QDialog::reject();
}

void StatusSetDialog::setChanged( const QString & tn )
{
	StatusSetList match = mSets.filter( "name", tn );
	if( match.size() == 1 ) {
		mSet = match[0];
		mEditButton->setEnabled( true );
		mRemoveButton->setEnabled( tn != "Default" );
	} else {
		mEditButton->setEnabled( false );
		mRemoveButton->setEnabled( false );
	}
}

void StatusSetDialog::addSet()
{
	StatusDialog * sd = new StatusDialog( this );
	if( sd->exec() == QDialog::Accepted ) {
		mAdded += sd->statusSet();
		updateSets();
	}
	delete sd;
}

void StatusSetDialog::removeSet()
{
	if( mSet.isRecord() ) {
		if( mAdded.contains( mSet ) ) {
			mAdded -= mSet;
			mSet.remove();
		} else
			mRemoved += mSet;
		updateSets();
	}
}

void StatusSetDialog::editSet()
{
	StatusDialog * atd = new StatusDialog( this );
	atd->setStatusSet( mSet );
	atd->exec();
	delete atd;
	updateSets();
}

void StatusSetDialog::updateSets()
{
	mSets = StatusSet::select() - mRemoved;
	mSetList->clear();
	mSetList->addItems( mSets.sorted( "name" ).names() );
}

