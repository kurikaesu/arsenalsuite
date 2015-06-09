
#ifndef FRAME_NTH_DIALOG
#define FRAME_NTH_DIALOG

#include <qdialog.h>

#include "afcommon.h"

#include "ui_framenthdialogui.h"

class FREEZER_EXPORT FrameNthDialog : public QDialog, Ui::FrameNthDialogUI
{
public:
	FrameNthDialog( QWidget * parent, int startFrame, int endFrame, int frameNth, int copyMode);

	void getSettings( int & startFrame, int & endFrame, int & frameNth, int & mode );
};


#endif // FRAME_NTH_DIALOG
