
#include "dialogfactory.h"

#include "blurqt.h"
#include "asset.h"
#include "assettypedialog.h"
#include "assettemplatesdialog.h"
#include "assetdialog.h"
#include "configdbdialog.h"
//#include "elementtasksdialog.h"
#include "freezercore.h"
#include "iniconfig.h"
#include "dialogfactory.h"
#include "notificationroutedialog.h"
#include "notificationwidget.h"
#include "pathtemplatesdialog.h"
#include "permsdialog.h"
#include "project.h"
#include "projectdialog.h"
#include "scenedialog.h"
#include "shot.h"
#include "shotdialog.h"
#include "shotgroup.h"
#include "statussetdialog.h"
#include "database.h"
//#include "taskdialog.h"
#include "timeentrydialog.h"
#include "updatemanager.h"
#include "user.h"
#include "userdialog.h"

DialogFactory * DialogFactory::instance()
{
	static DialogFactory * self = 0;
	if( !self ) self = new DialogFactory();
	return self;
}

Project DialogFactory::newProject( QWidget * parent )
{
	if( !parent && qApp->activeWindow() )
		parent = qApp->activeWindow();
	//Database::instance()->beginTransaction( "Create Project" );
	ProjectDialog * pd = new ProjectDialog( parent );
	pd->exec();
	Project ret = pd->project();
	delete pd;
	//Database::instance()->commitTransaction();
	return ret;
}

ElementList DialogFactory::newAsset( const Element & parent, const AssetType & at, QWidget * pw )
{
	if( !pw && qApp->activeWindow() )
		pw = qApp->activeWindow();
	//Database::instance()->beginTransaction( "Create Asset" );
	AssetDialog * ad = new AssetDialog( parent, pw );
	ad->setAssetTemplatesEnabled( true );
	ad->setAssetType( at );
	ad->exec();
	ElementList ret = ad->created();
	delete ad;
	//Database::instance()->commitTransaction();
	return ret;
}


ShotList DialogFactory::newShot( const Element & parent, float suggest, QWidget * pw )
{
	if( !pw && qApp->activeWindow() )
		pw = qApp->activeWindow();
	//Database::instance()->beginTransaction( "Create Shot(s)" );
	ShotDialog * sd = new ShotDialog( parent, pw );
	ElementList shots = parent.children( Shot::type() );
	if( suggest <= 0.0 ){
		float maxShot=0;
		foreach( Shot s, shots ){
			float sn = s.shotNumber();
			if( sn > maxShot ) {
				maxShot = sn;
			}
		}
		suggest = (int)maxShot + 1;
	}
	sd->setShotNumber( suggest );
	sd->exec();
	ShotList ret = sd->createdShots();
	delete sd;
	//Database::instance()->commitTransaction();
	return ret;
}

ShotGroup DialogFactory::newScene( const Element & parent, QWidget * pw )
{
	if( !pw && qApp->activeWindow() )
		pw = qApp->activeWindow();
	//Database::instance()->beginTransaction( "Create Scene" );
	ShotGroup sg;
	sg.setParent( parent );
	sg.setProject( parent.project() );
	SceneDialog * sd = new SceneDialog( pw );
	sd->setShotGroup( sg );
	sd->exec();
	ShotGroup ret = sd->shotGroup();
	delete sd;
	//Database::instance()->commitTransaction();
	return ret;
}

User DialogFactory::newUser( QWidget * pw )
{
	if( !pw && qApp->activeWindow() )
		pw = qApp->activeWindow();
	//Database::instance()->beginTransaction( "Create User" );
	UserDialog * ud = new UserDialog( pw );
	ud->exec();
	User ret = ud->user();
	delete ud;
	//Database::instance()->commitTransaction();
	return ret;
}

void DialogFactory::editAssetTemplates( QWidget * pw )
{
	if( !pw && qApp->activeWindow() )
		pw = qApp->activeWindow();
	//Database::instance()->beginTransaction( "Edit Asset Sets" );
	AssetTemplatesDialog * atd = new AssetTemplatesDialog( pw );
	atd->exec();
	delete atd;
	//Database::instance()->commitTransaction();
}

void DialogFactory::editPathTemplates( QWidget * pw )
{
	if( !pw && qApp->activeWindow() )
		pw = qApp->activeWindow();
	//Database::instance()->beginTransaction( "Edit Asset Sets" );
	PathTemplatesDialog * atd = new PathTemplatesDialog( pw );
	atd->exec();
	delete atd;
	//Database::instance()->commitTransaction();	
}

void DialogFactory::editStatusSets( QWidget * pw )
{
	if( !pw && qApp->activeWindow() )
		pw = qApp->activeWindow();
	//Database::instance()->beginTransaction( "Edit Status Sets" );
	StatusSetDialog * ssd = new StatusSetDialog( pw );
	ssd->exec();
	delete ssd;
	//Database::instance()->commitTransaction();
}
void DialogFactory::editAssetTypes( QWidget * pw )
{
	if( !pw && qApp->activeWindow() )
		pw = qApp->activeWindow();
	//Database::instance()->beginTransaction( "Edit Asset/Task Types" );
	AssetTypeDialog * atd = new AssetTypeDialog( pw );
	atd->exec();
	delete atd;
	//Database::instance()->commitTransaction();
}

void DialogFactory::enterTimeSheetData( ElementList el, const AssetType & tt, QWidget * pw )
{
	if( !pw && qApp->activeWindow() )
		pw = qApp->activeWindow();
	//Database::instance()->beginTransaction( "Enter Time Sheet" );
	TimeEntryDialog ted( pw );
	ted.setAssetType( tt );
	ted.setElementList( el );
	ted.exec();
	//Database::instance()->commitTransaction();
}

void DialogFactory::showConfigDBDialog( QWidget * pw )
{
	if( !pw && qApp->activeWindow() )
		pw = qApp->activeWindow();
	ConfigDBDialog cdb( pw );
	cdb.exec();
}

void DialogFactory::newTask( ElementList parents, QWidget * pw )
{
	//Database::instance()->beginTransaction( "Create Task" );
	AssetDialog td( parents[0], pw );
	td.exec();
	//Database::instance()->commitTransaction();
}

void DialogFactory::editPermissions( QWidget * pw )
{
	if( !pw && qApp->activeWindow() )
		pw = qApp->activeWindow();
	PermsDialog * pm = new PermsDialog(pw);
	pm->exec();
	delete pm;
}

void DialogFactory::editNotificationRoutes( QWidget * pw )
{
	if( !pw && qApp->activeWindow() )
		pw = qApp->activeWindow();
	NotificationRouteDialog * nrd = new NotificationRouteDialog( pw );
	nrd->exec();
	delete nrd;
}

void DialogFactory::viewNotifications( QWidget * pw )
{
	if( !pw && qApp->activeWindow() )
		pw = qApp->activeWindow();
	NotificationDialog * nd = new NotificationDialog( pw );
	nd->exec();
	delete nd;
}
