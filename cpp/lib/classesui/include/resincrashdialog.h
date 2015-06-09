
#ifndef RESIN_CRASH_DIALOG_H
#define RESIN_CRASH_DIALOG_H

#include <qdialog.h>

#include "classesui.h"

#include "sqlerrorhandler.h"

#include "ui_resincrashdialogui.h"

class CLASSESUI_EXPORT ResinCrashDialog : public QDialog, public Ui::ResinCrashDialog
{
public:
	ResinCrashDialog( QWidget * parent = 0 );
	void setError( const QString & ) {}

};

class CLASSESUI_EXPORT ResinSqlErrorHandler : public SqlErrorHandler
{
public:
	virtual ~ResinSqlErrorHandler(){}
	virtual void handleError( const QString & error );

	static void install();
};

#endif // RESIN_CRASH_DIALOG_H

