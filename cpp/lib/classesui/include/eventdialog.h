

#ifndef EVENT_DIALOG_H
#define EVENT_DIALOG_H

#include "ui_eventdialogui.h"

#include "classesui.h"

#include "calendar.h"

class CLASSESUI_EXPORT EventDialog : public QDialog, public Ui::EventDialogUI
{
Q_OBJECT
public:
	EventDialog( QWidget * parent = 0 );
	~EventDialog();

	void setEvent( const Calendar & );
	virtual void accept();
	virtual void reject();
	
	Calendar mEvent;
};

#endif // EVENT_DIALOG_H
