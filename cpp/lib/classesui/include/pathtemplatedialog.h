

#ifndef PATH_TEMPLATE_DIALOG_H
#define PATH_TEMPLATE_DIALOG_H

#include <qdialog.h>

#include "classesui.h"

#include "ui_pathtemplatedialogui.h"
#include "pathtemplate.h"

class CLASSESUI_EXPORT PathTemplateDialog : public QDialog, public Ui::PathTemplateDialogUI
{
Q_OBJECT
public:
	PathTemplateDialog( QWidget * parent = 0 );

	void accept();

	void setPathTemplate( const PathTemplate & );
	PathTemplate pathTemplate();

public slots:

	void slotUseScriptToggled( bool );
	void slotEditScript();

protected:

	PathTemplate mTemplate;
};

#endif // PATH_TEMPLATE_DIALOG_H


