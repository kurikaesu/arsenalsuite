/*
 *
 * Copyright 2003, 2004 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Resin is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Resin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef TIME_ENTRY_DIALOG_H
#define TIME_ENTRY_DIALOG_H

#include <qdialog.h>
#include <qmap.h>

#include "assettype.h"
#include "element.h"
#include "timesheet.h"
#include "user.h"
#include "project.h"
#include "ui_timeentrydialogui.h"

class QPushButton;
class ElementModel;

class CLASSESUI_EXPORT TimeEntryDialog : public QDialog, public Ui::TimeEntryDialogUI
{
Q_OBJECT
public:
	TimeEntryDialog( QWidget * parent = 0 );

	virtual void accept();

	static bool recordTimeSheet( QWidget * parent = 0, ElementList elements = ElementList(), const QDate & start = QDate::currentDate(), const AssetType & tt = AssetType(), const QDate & end = QDate() );

	static ElementList filterAssets( ElementList, const QString & );
public slots:

	void setupSuggestedTimeSheet();
	void setDateRange( const QDate & start, const QDate & end = QDate() );
	void setElementList( ElementList );
	void setAssetType( const AssetType & );
	void setProject( const Project & );
	void setTimeSheet( const TimeSheet & );

protected slots:
	void chooseStartDate();
	void chooseEndDate();
	void projectSelected( const QString & );
	void assetTypeChanged( const Record & );
	void slotFilterAssets();

protected:
	void updateAssets();

	Project mProject;
	AssetType mAssetType;
	ElementList mAssets;
	TimeSheet mTimeSheet;
	ElementModel * mAssetModel;
	User mUser;

	QStringList mForcedProjectCategories;
	Project mVirtualProject;
	bool mForceVirtualProject;
};


#endif // TIME_ENTRY_DIALOG_H

