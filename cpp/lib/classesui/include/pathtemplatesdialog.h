

#ifndef PATH_TEMPLATES_DIALOG_H
#define PATH_TEMPLATES_DIALOG_H

#include <qdialog.h>

#include "classesui.h"

#include "ui_pathtemplatesdialogui.h"
#include "pathtemplate.h"

class PathTemplateItem;

class CLASSESUI_EXPORT PathTemplatesDialog : public QDialog, public Ui::PathTemplatesDialogUI
{
Q_OBJECT
public:
	PathTemplatesDialog( QWidget * parent = 0 );

	void accept();
	void reject();

public slots:
	void refresh();

	void currentChanged( QTreeWidgetItem * );

	void addTemplate();
	void editTemplate();
	void removeTemplate();

protected:
	PathTemplate mCurrent;
	PathTemplateItem * mCurrentItem;
};

#endif // PATH_TEMPLATES_DIALOG_H
