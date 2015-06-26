#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <qaction.h>
#include <qmainwindow.h>

#include "user.h"

#include "recordsupermodel.h"
#include "recordtreeview.h"

#include "afcommon.h"

#include "ui_userpermissionseditorui.h"

class FREEZER_EXPORT UserPermissionsModel : public RecordSuperModel
{
Q_OBJECT
public:
	UserPermissionsModel( QObject * parent = 0 );
	
public slots:
	void userPermissionsUpdated(Record up, Record);
protected:

}

class FREEZER_EXPORT UserPermissionsView : public RecordTreeView
{
Q_OBJECT
public:
	UserPermissionsView( QWidget * parent = 0 )
	UserPermissionsModel * getModel() const;
		
protected:
	UserPermissionsModel * mModel;
}

class FREEZER_EXPORT UserPermissionsWidget : public QWidget, public Ui::UserPermissionsWidgetUi
{
Q_OBJECT
public:
	UserPermissionsWidget( QWidget * parent = 0 );
	
public slots:

protected:
	UserPermissionsView * mView;
}

class FREEZER_EXPORT UserPermissionsWindow : public QMainWindow
{
Q_OBJECT
public:
	UserPermissionsWindow( QWidget * parent = 0 );
	
}

#endif