

#include <qcombobox.h>
#include <qpushbutton.h>

#include "rangefiletracker.h"
#include "versionfiletracker.h"

#include "filetrackerdialog.h"
#include "pathtemplatesdialog.h"

FileTrackerDialog::FileTrackerDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	refreshTemplates();
	connect( mEditPathTemplatesButton, SIGNAL( clicked() ), SLOT( editTemplates() ) );
	connect( mSequenceGroup, SIGNAL( toggled( bool ) ), SLOT( typeChanged() ) );
	connect( mRangeRadio, SIGNAL( toggled( bool ) ), SLOT( typeChanged() ) );
	connect( mVersionRadio, SIGNAL( toggled( bool ) ), SLOT( typeChanged() ) );
	connect( mPathTemplateGroup, SIGNAL( toggled( bool ) ), SLOT( usingTemplateChanged( bool ) ) );
}

void FileTrackerDialog::refreshTemplates()
{
	mTemplates = PathTemplate::select().sorted( "name" );
	mPathTemplateCombo->clear();
	mPathTemplateCombo->addItems( mTemplates.names() );
}

void FileTrackerDialog::accept()
{
	QDialog::accept();
}

void FileTrackerDialog::setFileTracker( const FileTracker & ft )
{
	mTracker = ft;
	
	PathTemplate pt = ft.pathTemplate();
	mPathTemplateGroup->setChecked( pt.isRecord() );
	mPathTemplateCombo->setCurrentIndex( pt.isRecord() ? mTemplates.findIndex( pt ) : 0 );
	usingTemplateChanged( pt.isRecord() );

	mName->setText( ft.name() );
	mPathEdit->setText( ft.path() );
	mFileNameEdit->setText( ft.fileName() );

	VersionFileTracker vft( ft );
	RangeFileTracker rft( ft );
	if( vft.isRecord() ) {
		mSequenceGroup->setChecked( true );
		mVersionRadio->setChecked( true );
		mVersionSpin->setValue( vft.version() );
		mIterationSpin->setValue( vft.iteration() );
	} else if( rft.isRecord() ) {
		mSequenceGroup->setChecked( true );
		mRangeRadio->setChecked( true );
		mFrameStartSpin->setValue( rft.frameStart() );
		mFrameEndSpin->setValue( rft.frameEnd() );
	} else {
		mSequenceGroup->setChecked( false );
	}
	typeChanged();
}

void FileTrackerDialog::typeChanged()
{
	bool re = mSequenceGroup->isChecked() && mRangeRadio->isChecked();
	bool ve = mSequenceGroup->isChecked() && mVersionRadio->isChecked();
	if( !re && !ve && mSequenceGroup->isChecked() ) {
		re = true;
		mRangeRadio->setChecked( true );
	}

	mFrameRangeLabel->setShown( re );
	mFrameToLabel->setShown( re );
	mFrameStartSpin->setShown( re );
	mFrameEndSpin->setShown( re );

	mVersionLabel->setShown( ve );
	mVersionSpin->setShown( ve );
	mIterationLabel->setShown( ve );
	mIterationSpin->setShown( ve );
}

void FileTrackerDialog::usingTemplateChanged( bool ut )
{
	mPathEdit->setReadOnly( ut );
	mFileNameEdit->setReadOnly( ut );	
}

Record coerce( Record & src, Table * table )
{
	if( src.table() == table ) return src;
	Record ret = table->load();
	FieldList fl = src.table()->schema()->fields();
	foreach( Field * f, fl )
		ret.setValue( f->name(), src.getValue( f->name() ) );
	if( src.isRecord() ) src.remove();
	return ret;
}

FileTracker FileTrackerDialog::fileTracker()
{
	if( mSequenceGroup->isChecked() ) {
		mTracker = coerce( mTracker, mVersionRadio->isChecked() ? (Table*)VersionFileTracker::table() : (Table*)RangeFileTracker::table() );
		if( mVersionRadio->isChecked() ) {
			VersionFileTracker vft( mTracker );
			vft.setVersion( mVersionSpin->value() );
			vft.setIteration( mIterationSpin->value() );
		} else {
			RangeFileTracker rft( mTracker );
			rft.setFrameStart( mFrameStartSpin->value() );
			rft.setFrameEnd( mFrameEndSpin->value() );
		}
	} else
		mTracker = coerce( mTracker, FileTracker::table() );

	mTracker.setName( mName->text() );
	if( mPathTemplateGroup->isChecked() ) {
		mTracker.setPathTemplate( mTemplates[mPathTemplateCombo->currentIndex()] );
	} else {
		mTracker.setPathTemplate( PathTemplate() );
		mTracker.setPath( mPathEdit->text() );
		mTracker.setFileName( mFileNameEdit->text() );
	}
	return mTracker;
}

void FileTrackerDialog::editTemplates()
{
	PathTemplatesDialog ptd( this );
	ptd.exec();
}


