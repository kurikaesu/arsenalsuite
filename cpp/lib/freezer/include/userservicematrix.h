
#ifndef USER_SERVICE_MATRIX_H
#define USER_SERVICE_MATRIX_H

#include "user.h"
#include "userservice.h"
#include "service.h"

#include "recordsupermodel.h"
#include "recordtreeview.h"

#include "afcommon.h"

#include "ui_userservicematrixwindowui.h"

class UserServiceModel;

class FREEZER_EXPORT UserServiceModel : public RecordSuperModel
{
Q_OBJECT
public:
	UserServiceModel( QObject * parent = 0 );

	UserService findUserService( const User & user, int column ) const;
	UserService findUserService( const QModelIndex & ) const;
	QVariant serviceData ( const User &, int column, int role ) const;

	void setUserFilter( const QString & );

public slots:
	void updateServices();

protected:

	ServiceList mServices;
};

class FREEZER_EXPORT UserServiceMatrix : public RecordTreeView
{
Q_OBJECT
public:
	UserServiceMatrix( QWidget * parent = 0 );

public slots:
	void setUserFilter( const QString & );
	void setServiceFilter( const QString & );

	void slotShowMenu( const QPoint & pos, const QModelIndex & underMouse );

	void updateServices();
protected:

	QString mUserFilter, mServiceFilter;
	UserServiceModel * mModel;
};

class FREEZER_EXPORT UserServiceMatrixWindow : public QMainWindow, public Ui::UserServiceMatrixWindowUi
{
Q_OBJECT
public:
	UserServiceMatrixWindow( QWidget * parent = 0 );

protected:
	UserServiceMatrix * mView;
};

#endif // USER_SERVICE_MATRIX_H
