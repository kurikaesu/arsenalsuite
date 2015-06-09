

/* $Author: newellm $
 * $LastChangedDate: 2008-05-02 11:35:13 +1000 (Fri, 02 May 2008) $
 * $Rev: 6487 $
 * $HeadURL: svn://svn.blur.com/blur/branches/concurrent_burn/cpp/lib/classesui/include/hostlistsdialog.h $
 */

#ifndef HOST_LISTS_DIALOG_H
#define HOST_LISTS_DIALOG_H

#include <qdialog.h>

#include "classesui.h"

#include "ui_hostlistsdialogui.h"

class CLASSESUI_EXPORT HostListsDialog : public QDialog, public Ui::HostListsDialogUI
{
Q_OBJECT
public:
	HostListsDialog( QWidget * parent = 0 );


public slots:
	void refresh();
	void showMenu( const QPoint &, const Record &, RecordList );

protected:

	RecordSuperModel * mModel;
};

#endif // HOST_LISTS_DIALOG_H
