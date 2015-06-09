
#include <qapplication.h>
#include <qdatetime.h>

#include "blurqt.h"
#include "database.h"
#include "field.h"
#include "freezercore.h"
#include "record.h"
#include "schema.h"

#include "assettype.h"
#include "jobtask.h"
#include "filetracker.h"
#include "pathtemplate.h"
#include "project.h"
#include "versionfiletracker.h"

bool test1();
bool test2();
bool test3();
bool test4();
bool test5();
bool test6();
//bool test7();
bool test8();

#define TEST(x)	{ \
	QTime t; \
	t.start(); \
	bool res = test##x(); \
	int time = t.elapsed(); \
	if( !res ) abort(); \
	LOG_3( "Test " #x " Passed in " + QString::number( time ) + " ms" ); \
	}
	
int main( int argc, char * argv [] )
{
	QApplication a( argc, argv );
	initConfig( "test.ini" );
	blurqt_loader();
	TEST(1);
	TEST(2);
	TEST(3);
	TEST(4);
	TEST(5);
	TEST(6);
	TEST(8);
	shutdown();
}

bool testString( const QString & target, const QString & actual, const QString test, bool expectEqual = true )
{
	if( (target != actual) == expectEqual ) {
		qWarning( "Test " + test + " failed" );
		qWarning( "Expected: " + target );
		qWarning( "Got: " + actual );
		return false;
	}
	return true;
}

bool testInt( int target, int actual, const QString & test, bool expectEqual = true )
{
	if( (target != actual) == expectEqual ) {
		qWarning( "Test " + test + " failed" );
		qWarning( "Expected: " + QString::number( target ) );
		qWarning( "Got: " + QString::number( actual ) );
		return false;
	}
	return true;
}

bool testBool( bool target, bool actual, const QString & test )
{
	if( target != actual ) {
		qWarning( "Test " + test + " failed" );
		qWarning( "Expected: " + QString( target ? "true" : "false" ) );
		qWarning( "Got: " + QString( actual ? "true" : "false" ) );
		return false;
	}
	return true;
}

bool test1()
{
	QString test( "Database Connection" );
	Connection * c = Database::current()->connection();
	return testBool( true, c->checkConnection(), test );
}

Database * d = 0;
Table * t = 0;
bool test2()
{
	QString test( "Table::mergeXmlSchema" );
	d = Database::current();
	d->schema()->mergeXmlSchema( "../schema.xml" );
	t = d->schema()->tableByName( "JobTask" )->table();
	return testInt( 0, (int)t, test, false );
}

Record r;
bool test3()
{
	QString test( "JobTask insertion" );
	r = t->load();
	r.setValue( "jobTask", 666 );
	r.setValue( "fkeyHost", 777 );
	r.setValue( "fkeyJob", 111 );
	r.commit();
	return testInt( 0, r.key(), test, false );
}

#define RETFAIL(x) do{ if( !x ) return false; } while(0)
bool test4()
{
	QString test( "JobTask values" );
	RETFAIL( testInt( 666, r.getValue( "jobTask" ).toInt(), test + " jobTask" ) );
	RETFAIL( testInt( 777, r.getValue( "fkeyHost" ).toInt(), test + " fkeyHost" ) );
	RETFAIL( testInt( 111, r.getValue( "fkeyJob" ).toInt(), test + " fkeyJob" ) );
	return true;
}

bool test5()
{
	QString test( "Record Ref Counting" );
	RETFAIL( testInt( 1, r.imp()->refCount(), test + " 1" ) );
	Record mod = r;
	RETFAIL( testInt( 2, r.imp()->refCount(), test + " 2" ) );
	mod.setValue( "fkeyJob", 112 );
	RETFAIL( testInt( 1, mod.imp()->refCount(), test + " 3" ) );
	RETFAIL( testInt( 1, r.imp()->refCount(), test + " 4" ) );
	mod.commit();
	RETFAIL( testInt( 112, r.getValue( "fkeyJob" ).toInt(), test + " 5" ) );
	mod = t->record( r.key(), true, false );
	RETFAIL( testInt( (int)mod.imp(), (int)r.imp(), test + " 6" ) );
	return true;
}

bool test6()
{
	QString test( "checkForUpdate test" );
	Record same = t->record( r.key(), true, false );
	RETFAIL( testInt( (int)r.imp(), (int)same.imp(), test + " 1" ) );
	int key = r.key();
	same = Record();
	r = Record();
	same = t->record( key, false, true );
	RETFAIL( testInt( (int)same.isRecord(), 0, test + " 2" ) );
	return true;
}

bool test8()
{
	QString test( "field list creation" );
	FieldList fl = FieldList() << JobTaskFields::Key << JobTaskFields::FrameNumber << JobTaskFields::Host;
	RETFAIL( testInt( 3, fl.size(), test ) );
	return true;
}

/*
bool test7()
{
	QString test( "PathTemplate methods" );

	PathTemplate project_template;
	project_template.setName( "test_project_template" );
	project_template.setPathTemplate( "[Name]/" );
	project_template.commit();

	Project p = AssetType::recordByName( "Project" ).construct();
	p.setName( "A_Project" );
	p.setWipDrive( "G:" );
	p.setPathTemplate( project_template );
	p.commit();
	RETFAIL( testString( "G:/A_Project/", p.path(), test + ": project", true ) );

	PathTemplate asset_template;
	asset_template.setName( "test_asset_template" );
	asset_template.setPathTemplate( "[Parent.Path][Name:/ /_/]/" );
	asset_template.commit();

	Element ag = AssetType::recordByName( "Asset Group" ).construct();
	ag.setName( "An Asset Group" );
	ag.setParent( p );
	ag.setProject( p );
	ag.setPathTemplate( asset_template );
	ag.commit();
	RETFAIL( testString( "G:/A_Project/An_Asset_Group/", ag.path(), test + ": asset group", true ) );

	Element c = AssetType::recordByName( "Character" ).construct();
	c.setName( "A Character" );
	c.setParent( ag );
	c.setPathTemplate( asset_template );
	c.commit();
	RETFAIL( testString( "G:/A_Project/An_Asset_Group/A_Character/", c.path(), test + ": character", true ) );

	Element t = AssetType::recordByName( "Modeling" ).construct();
	t.setName( "Modeling" );
	t.setParent( c );
	t.setPathTemplate( asset_template );
	t.commit();
	RETFAIL( testString( "G:/A_Project/An_Asset_Group/A_Character/Modeling/", t.path(), test + ": modeling", true ) );

	PathTemplate ft_template;
	ft_template.setName( "test_filetracker_template" );
	ft_template.setPathTemplate( "[Path]" );
	ft_template.setFileNameTemplate( "[climb_to_type(Character).Name:/ /_/]_Mesh.max" );
	ft_template.commit();

	FileTracker ft;
	ft.setName( "modeling_final" );
	ft.setPathTemplate( ft_template );
	ft.setElement( t );
	ft.commit();
	RETFAIL( testString( "G:/A_Project/An_Asset_Group/A_Character/Modeling/A_Character_Mesh.max", ft.filePath(), test + ": modeling_final", true ) );

	PathTemplate ft2_template;
	ft2_template.setName( "test_filetracker2_template" );
	ft2_template.setPathTemplate( "[Path]" );
	ft2_template.setFileNameTemplate( "[climb_to_istask(false).Name:/ /_/]_Rig.max" );
	ft2_template.commit();

	FileTracker ft2;
	ft2.setName( "rigging_final" );
	ft2.setPathTemplate( ft2_template );
	ft2.setElement( t );
	ft2.commit();
	RETFAIL( testString( "G:/A_Project/An_Asset_Group/A_Character/Modeling/A_Character_Rig.max", ft2.filePath(), test + ": rigging_final", true ) );

	PathTemplate version_template;
	version_template.setName( "test_versionfiletracker_template" );
	version_template.setPathTemplate( "[Path]" );
	version_template.setFileNameTemplate( "[climb_to_istask(false).Name:/ /_/]_v[Version]_[Iteration].max" );
	version_template.commit();

	VersionFileTracker ft_wip;
	ft_wip.setName( "max_modeling_wip" );
	ft_wip.setPathTemplate( version_template );
	ft_wip.setElement( t );
	ft_wip.setVersion( 1 );
	ft_wip.setIteration( 2 );
	ft_wip.commit();
	RETFAIL( testString( "G:/A_Project/An_Asset_Group/A_Character/Modeling/A_Character_v1_2.max", ft_wip.filePath(), test + ": rigging_final", true ) );
	
	return true;
}*/



