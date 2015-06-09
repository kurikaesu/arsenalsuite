
#ifndef PROJECT_WEIGHT_DIALOG_H
#define PROJECT_WEIGHT_DIALOG_H

#include "afcommon.h"
#include "ui_projectweightdialogui.h"

class FREEZER_EXPORT ProjectWeightDialog : public QDialog, public Ui::ProjectWeightDialogUI
{
Q_OBJECT
public:
	ProjectWeightDialog(QWidget * parent=0);

};

#endif // PROJECT_WEIGHT_DIALOG_H
