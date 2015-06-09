

#ifndef FILE_TRACKER_DIALOG_H
#define FILE_TRACKER_DIALOG_H

#include <qdialog.h>

#include "classesui.h"

#include "ui_filetrackerdialogui.h"
#include "filetracker.h"
#include "pathtemplate.h"

class CLASSESUI_EXPORT FileTrackerDialog : public QDialog, public Ui::FileTrackerDialogUI
{
Q_OBJECT
public:
	FileTrackerDialog( QWidget * parent );

	void accept();
	void setFileTracker( const FileTracker & );
	FileTracker fileTracker();

public slots:
	void editTemplates();

	void typeChanged();

	void usingTemplateChanged( bool );

protected:
	void refreshTemplates();

	FileTracker mTracker;
	PathTemplateList mTemplates;
};

#endif // FILE_TRACKER_DIALOG_H

