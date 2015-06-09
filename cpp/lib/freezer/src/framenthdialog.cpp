
#include "framenthdialog.h"

FrameNthDialog::FrameNthDialog( QWidget * parent, int startFrame, int endFrame, int frameNth, int frameNthMode)
: QDialog( parent )
{
	setupUi(this);
	
	connect( mRenderFrameNthRadio, SIGNAL( toggled( bool ) ), mCopyOptionsGroup, SLOT( setEnabled( bool ) ) );

	mStartFrameSpin->setValue( startFrame );
	mEndFrameSpin->setValue( endFrame );
	mFrameNthSpin->setValue( frameNth );
	if( frameNthMode == 4 )
		mRenderFillFramesRadio->setChecked( true );
	else {
		mRenderFrameNthRadio->setChecked(true);
		if( frameNthMode == 1 )
			mCopyToMissingRadio->setChecked(true);
		else if( frameNthMode == 2 )
			mCopyToAllRadio->setChecked(true);
		else
			mNoCopyRadio->setChecked(true);
	}
}

void FrameNthDialog::getSettings( int & startFrame, int & endFrame, int & frameNth, int & mode )
{
	startFrame = mStartFrameSpin->value();
	endFrame = mEndFrameSpin->value();
	frameNth = mFrameNthSpin->value();
	mode = 0;
	if( mRenderFillFramesRadio->isChecked() )
		mode = 4;
	else if( mCopyToMissingRadio->isChecked() )
		mode = 1;
	else if( mCopyToAllRadio->isChecked() )
		mode = 2;
}
