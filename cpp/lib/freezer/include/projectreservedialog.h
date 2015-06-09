
#ifndef PROJECT_RESERVE_DIALOG_H
#define PROJECT_RESERVE_DIALOG_H

#include "afcommon.h"
#include "ui_projectreservedialogui.h"

class FREEZER_EXPORT ProjectReserveDialog : public QDialog, public Ui::ProjectReserveDialogUI
{
Q_OBJECT
public:
	ProjectReserveDialog(QWidget * parent=0);

};

#endif // PROJECT_RESERVE_DIALOG_H
