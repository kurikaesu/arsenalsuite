
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Arsenal.
 *
 * Arsenal is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Arsenal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arsenal; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 */

#include <qtextstream.h>
#include <qstring.h>
#include <QDialog>
#include <qsystemtrayicon.h>

#include "ui_maindialogui.h"
#include "iniconfig.h"
#include "slave.h"
#include "common.h"

class QTimer;
class QFile;
class QMenu;
class QCloseEvent;

const QString VERSION("1.5.20");

/* Note:
 * init() must be called after construction
 * if init returns false, then an error occured
 * and the application should be shut down
 */

/// \ingroup ABurner
/// @{

class MainDialog : public QDialog, public Ui::MainDialogBase
{
Q_OBJECT

public:
	MainDialog(Slave * s, QWidget * parent=0);
	~MainDialog();
	
	void readConfig();

	Slave * slave() const { return mSlave; }

protected slots:

	void updateTray();
	void slotTrayIconActivated( QSystemTrayIcon::ActivationReason );

	// Connected to mSlave->statusChange
	void setStatus( const QString & );

	void showOptions();
	void showClientLog();
	void slotDisablePressed();

	void showAssfreezer();
	void slotShowAssignments( bool );
	void slotAssignmentsChanged( JobAssignmentList );

	void updateSize();
protected:
	void setDisplay( const QString &, const QString &, const QString & );

	void closeEvent( QCloseEvent * );

	void keyPressEvent( QKeyEvent * );

	// Configuration Variables
	QString cClientLogFile, cLogCommand, cAFPath, cAppName;

	Slave * mSlave;
	QSystemTrayIcon * mTrayIcon;
	QMenu * mTrayMenu;

	QAction * mTrayMenuToggleAction;

	int mCurrentWidget;
	bool mBringToTop;

//	RecordSuperModel * mModel;
//	JobAssignmentTranslator * mTrans;
};

/// @}

