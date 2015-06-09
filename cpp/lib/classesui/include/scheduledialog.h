

#ifndef SCHEDULE_DIALOG_H
#define SCHEDULE_DIALOG_H

#include <qdialog.h>
#include <qdatetime.h>

#include "classesui.h"

#include "ui_scheduledialogui.h"

#include "assettype.h"
#include "element.h"
#include "employee.h"
#include "project.h"
#include "schedule.h"

class ElementModel;

class CLASSESUI_EXPORT ScheduleDialog : public QDialog, public Ui::ScheduleDialogUI
{
Q_OBJECT
public:

	ScheduleDialog( QWidget * parent );

	virtual void accept();

public slots:
	/// By passing the asset type here, you don't need to pass it
	/// using the setAssetType function, and can avoid updates of
	/// the asset list
	void setElement( const Element &, const AssetType & at = AssetType() );

	void setAssetType( const AssetType & tt );
	void setEmployee( const Employee & emp );
	void setProject( const Project & project );
	void setDateRange( const QDate & start, const QDate & end );
	void setUseTypeFilter( bool );
	void setUseAssetsTypeFilter( bool );

	// Used to edit an existing schedule
	void setSchedule( const Schedule & );

	void chooseStartDate();
	void chooseEndDate();

signals:
	void dateRangedChanged( const QDate &, const QDate & );
	
protected slots:
	void projectSelected( const QString & );
	void assetTypeChanged( const Record & );
	void employeeSelected( const Record & );
	void startDateChanged( const QDate & );
	void endDateChanged( const QDate & );


protected:
	void updateAssets();
	void updateUsers();

	Project mProject;
	AssetType mAssetType;
	Employee mEmployee;
	ElementList mAssets;
	Schedule mSchedule;
	ElementModel * mAssetModel, * mUserModel;
	bool mDisableUpdates;
};

#endif // SCHEDULE_DIALOG_H

