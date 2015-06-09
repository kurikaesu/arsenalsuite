
#ifndef STATUS_SET_DIALOG_H
#define STATUS_SET_DIALOG_H

#include <qdialog.h>

#include "classesui.h"

#include "ui_statussetdialogui.h"

#include "statusset.h"

class CLASSESUI_EXPORT StatusSetDialog : public QDialog, public Ui::StatusSetDialogUI
{
Q_OBJECT
public:
	StatusSetDialog( QWidget * parent = 0 );

	void accept();
	void reject();

public slots:
	void addSet();
	void removeSet();
	void editSet();

	void setChanged( const QString & );

protected:
	void updateSets();

	StatusSet mSet;
	StatusSetList mSets, mAdded, mRemoved;
};

#endif // STATUS_SET_DIALOG_H

