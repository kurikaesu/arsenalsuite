

class DialogFactory : QObject
{
%TypeHeaderCode
#include <dialogfactory.h>
%End
public:
	static DialogFactory * instance();

	Project newProject( QWidget * pw = 0 );

	ElementList newAsset( const Element & parent, const AssetType &, QWidget * pw = 0  );

	ShotList newShot( const Element & parent, float suggest=0.0, QWidget * pw = 0  );

	ShotGroup newScene( const Element & parent, QWidget * pw = 0 );

	User newUser( QWidget * pw = 0 );


	void editAssetTemplates( QWidget * pw = 0 );

	void editPathTemplates( QWidget * pw = 0 );

	void editStatusSets( QWidget * pw = 0 );
	
	void editAssetTypes( QWidget * pw = 0 );

	void enterTimeSheetData( ElementList, const AssetType & at = AssetType(), QWidget * pw = 0 );

	void newTask( ElementList, QWidget * parent );

	void showConfigDBDialog( QWidget * pw=0 );

	void editPermissions( QWidget * pw = 0 );

	void editNotificationRoutes( QWidget * pw = 0 );
};
