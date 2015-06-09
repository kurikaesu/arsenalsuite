


#ifndef DATE_CHOOSER_WIDGET_H
#define DATE_CHOOSER_WIDGET_H

#include "classesui.h"

#include <qwidget.h>
#include <qdatetime.h>
#include <qdialog.h>

class ScheduleWidget;

class CLASSESUI_EXPORT DateChooserWidget : public QWidget
{
Q_OBJECT
public:
	DateChooserWidget( QWidget * parent );

	void setDate( const QDate & );
	QDate date() const;

	virtual QSize sizeHint() const { return QSize( 200, 200 ); }
	virtual QSize minimumSizeHint() const { return QSize( 150, 150 ); }

protected:
	ScheduleWidget * mScheduleWidget;
};

class CLASSESUI_EXPORT DateChooserDialog : public QDialog
{
Q_OBJECT
public:
	DateChooserDialog( QWidget * parent );

	static QDate getDate(QWidget*parent,const QDate & date);
protected:
	DateChooserWidget * mDateChooserWidget;
};

#endif // DATE_CHOOSER_WIDGET_H

