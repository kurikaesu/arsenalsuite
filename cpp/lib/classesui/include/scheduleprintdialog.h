

#ifndef SCHEDULE_PRINT_DIALOG_H
#define SCHEDULE_PRINT_DIALOG_H

#include <qdialog.h>

#include "classesui.h"

#include "ui_scheduleprintdialogui.h"

#include "schedulewidget.h"

class CLASSESUI_EXPORT SchedulePrintDialog : public QDialog, public Ui::SchedulePrintDialogUI
{
Q_OBJECT
public:
	SchedulePrintDialog( ScheduleWidget * widget );


public slots:
	void preview();
	void print();
	void savePdf();
	void updateFont( QTreeWidgetItem * );

protected:
	void doPrint(QPrinter *);
	void setupPrinter(QPrinter & printer);
	void setupPdfPrinter(QPrinter & printer);
	ScheduleWidget * mScheduleWidget;
	ScheduleDisplayOptions mDisplayOptions;
};

#endif // SCHEDULE_PRINT_DIALOG_H

