

#ifndef NOTIFICATION_WIDGET_H
#define NOTIFICATION_WIDGET_H

#include <qdialog.h>

#include "ui_notificationwidgetui.h"

class NotificationWidget : public QWidget, public Ui::NotificationWidgetUI
{
Q_OBJECT
public:
	NotificationWidget( QWidget * parent = 0 );
	
};

class NotificationDialog : public QDialog
{
Q_OBJECT
public:
	NotificationDialog( QWidget * parent = 0 );

};

#endif // NOTIFICATION_WIDGET_H
