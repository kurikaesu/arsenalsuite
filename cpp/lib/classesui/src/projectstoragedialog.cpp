

#include "projectstoragedialog.h"

ProjectStorageDialog::ProjectStorageDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi(this);
}

void ProjectStorageDialog::setProjectStorage( const ProjectStorage & ps )
{
	mStorageName->setText( ps.name() );
	mLocation->setText( ps.location() );
	mProjectStorage = ps;
}

ProjectStorage ProjectStorageDialog::projectStorage()
{
	mProjectStorage.setName( mStorageName->text() );
	mProjectStorage.setLocation( mLocation->text() );
	return mProjectStorage;
}

void ProjectStorageDialog::accept()
{
	projectStorage().commit();
	QDialog::accept();
}
