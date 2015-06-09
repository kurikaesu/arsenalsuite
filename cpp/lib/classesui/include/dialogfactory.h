

#ifndef DIALOG_FACTORY_H
#define DIALOG_FACTORY_H

#include <qwidget.h>

#include "classesui.h"

#include "assettype.h"
#include "project.h"
#include "shot.h"
#include "shotgroup.h"
#include "user.h"

class CLASSESUI_EXPORT DialogFactory : public QObject
{
Q_OBJECT
public:
	static DialogFactory * instance();

public slots:
	/**
	 * Displays an empty ProjectDialog allowing
	 * the user to enter a new Project.
	 * Commits the project to the database if
	 * the user pressed OK
	 **/
	Project newProject( QWidget * pw = 0 );

	/**
	 * Displays an empty AssetDialog allowing
	 * the user to create a new Asset.
	 * Commits the asset to the database on OK
	 **/
	ElementList newAsset( const Element & parent, const AssetType &, QWidget * pw = 0  );

	/**
	 * Displays an empty ShotDialog, allowing
	 * the user to create a new Shot.
	 * Commits the shot to the database on OK
	 **/
	ShotList newShot( const Element & parent, float suggest=0.0, QWidget * pw = 0  );

	/**
	 * Displays an empty SceneDialog, allowing
	 * the user to create a new Scene.
	 * Commits the ShotGroup to the database on OK
	 **/
	ShotGroup newScene( const Element & parent, QWidget * pw = 0 );

	/**
	 * Displays an empty UserDialog, allowing
	 * the user to create a new User.
	 * Commits the Usr to the database on OK
	 **/
	User newUser( QWidget * pw = 0 );


	void editAssetTemplates( QWidget * pw = 0 );

	void editPathTemplates( QWidget * pw = 0 );

	void editStatusSets( QWidget * pw = 0 );
	
	void editAssetTypes( QWidget * pw = 0 );
	
	/**
	 * Pops up a dialog that allows the user
	 * to enter a timesheet record. The user
	 * can entry the number of hours, date,
	 * and the user who did the time
	 **/
	void enterTimeSheetData( ElementList, const AssetType & at = AssetType(), QWidget * pw = 0 );

	void newTask( ElementList, QWidget * parent );

	void showConfigDBDialog( QWidget * pw=0 );

	void editPermissions( QWidget * pw = 0 );

	void editNotificationRoutes( QWidget * pw = 0 );

	void viewNotifications( QWidget * pw = 0 );
};

#endif // DIALOG_FACTORY_H
