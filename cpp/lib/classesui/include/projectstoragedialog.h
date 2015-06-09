
#ifndef PROJECT_STORAGE_DIALOG_H
#define PROJECT_STORAGE_DIALOG_H

#include <qdialog.h>

#include "classesui.h"

#include "projectstorage.h"

#include "ui_projectstoragedialogui.h"

class CLASSESUI_EXPORT ProjectStorageDialog : public QDialog, public Ui::ProjectStorageDialog
{
Q_OBJECT
public:
	ProjectStorageDialog( QWidget * parent );

	void setProjectStorage( const ProjectStorage & ps );
	ProjectStorage projectStorage();

	virtual void accept();

protected:
	ProjectStorage mProjectStorage;
};

#endif // PROJECT_STORAGE_DIALOG_H

