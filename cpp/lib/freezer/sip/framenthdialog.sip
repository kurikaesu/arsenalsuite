

class FrameNthDialog : QDialog
{
%TypeHeaderCode
#include <framenthdialog.h>
%End

public:
	FrameNthDialog( QWidget * parent, int startFrame, int endFrame, int frameNth, int copyMode);

	void getSettings( int & startFrame /Out/, int & endFrame /Out/, int & frameNth /Out/, int & mode /Out/ );
};
