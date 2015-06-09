
#ifndef STATUS_DIALOG_H
#define STATUS_DIALOG_H

#include <qdialog.h>
#include <qitemdelegate.h>

#include "classesui.h"

#include "ui_statusdialogui.h"

#include "elementstatus.h"
#include "statusset.h"

class CLASSESUI_EXPORT StatusDialog : public QDialog, public Ui::StatusDialogUI
{
Q_OBJECT
public:
	StatusDialog( QWidget * parent = 0 );

	void accept();
	void reject();

	StatusSet statusSet();
	void setStatusSet( const StatusSet & );

public slots:
	void addStatus();
	void removeStatus();
	void moveUp();
	void moveDown();

//	void statusChanged( const QString & );

protected:
	void swap( const QModelIndex &, const QModelIndex & );

	void updateStatuses();

	StatusSet mStatusSet;
	ElementStatus mStatus; // current status
	ElementStatusList mStatuses;
};

#endif // STATUS_DIALOG_H

