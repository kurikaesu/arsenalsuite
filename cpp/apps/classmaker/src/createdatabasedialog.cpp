
#include <iostream>

#include <qpushbutton.h>
#include <qtablewidget.h>
#include <qlabel.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <qdir.h>
#include <qfile.h>

#include "blurqt.h"
#include "createdatabasedialog.h"
#include "configdbdialog.h"
#include "connection.h"
#include "database.h"
#include "freezercore.h"
#include "iniconfig.h"
#include "schema.h"

CreateDatabaseDialog::CreateDatabaseDialog( Schema * schema, QWidget * parent )
: QDialog( parent )
, mSchema( schema )
, mDatabase( new Database( schema, Connection::createFromIni( config(), "Database" ) ) )
, mTableSchema( 0 )
{
	mUI.setupUi( this );

	connect( mUI.mVerifyButton, SIGNAL( clicked() ), SLOT( verify() ) );
	connect( mUI.mCreateButton, SIGNAL( clicked() ), SLOT( create() ) );
	connect( mUI.mCloseButton, SIGNAL( clicked() ), SLOT( reject() ) );
	connect( mUI.mEditConnectionButton, SIGNAL( clicked() ), SLOT( editConnection() ) );

	mMigrationsDirectory = config().readString("MigrationsDirectory", ".");

	updateConnectionLabel();
}

void CreateDatabaseDialog::setTableSchema( TableSchema * tableSchema )
{
	mTableSchema = tableSchema;
}

void CreateDatabaseDialog::editConnection()
{
	ConfigDBDialog * cdb = new ConfigDBDialog( this );
	if( cdb->exec() == QDialog::Accepted ) {
		Connection * c = Connection::createFromIni( config(), "Database" );
		c->checkConnection();
		mDatabase->setConnection( c );
		updateConnectionLabel();
	}
	delete cdb;
}

void CreateDatabaseDialog::verify()
{
	QString output;
	if( mTableSchema ) {
		Table * table = mDatabase->tableFromSchema( mTableSchema );
		table->verifyTable( false, &output );
	} else {
		mDatabase->verifyTables( &output );
	}
	mUI.mHistoryEdit->setText( output );
}

void CreateDatabaseDialog::create()
{
	QString output;
	QString newMigrationFile = getNextMigrationFile();
	if( mTableSchema ) {
		Table * table = mDatabase->tableFromSchema( mTableSchema );
		if( table->exists() ) {
			alterTableMigration( newMigrationFile, table, &output);
			table->verifyTable( true, &output );
		} else {
			createTableMigration( newMigrationFile, table->schema(), &output );
			table->createTable( &output );
		}
	} else {
		createTablesMigration( newMigrationFile, &output );
		mDatabase->createTables( &output );
	}
	if (output.size() > 0) 
		if (output.contains("CREATE") || output.contains("ALTER"))
			output = "Added to migrations file " + newMigrationFile + "\n" + output;

	mUI.mHistoryEdit->setText( output );
}

QString CreateDatabaseDialog::getNextMigrationFile()
{
	QDir migrationDirectory(mMigrationsDirectory);
	QStringList fileList = migrationDirectory.entryList(QStringList("*.sql"));

	uint currentIndex = 0;
	for (QStringList::Iterator it = fileList.begin(); it != fileList.end(); ++it)
	{
		uint indexNumber = it->split('.')[0].toUInt();
		currentIndex = (currentIndex > indexNumber)?currentIndex:++indexNumber;
	}

	QString newMigrationIndex;
	newMigrationIndex.setNum(currentIndex);

	while (newMigrationIndex.size() < 4) newMigrationIndex = "0" + newMigrationIndex;

	return newMigrationIndex + ".sql";
}

void CreateDatabaseDialog::alterTableMigration( const QString& migrationFile, Table * table, QString * output )
{
	FieldList fl = table->schema()->columns();
	QMap<QString, Field*> fieldMap;

	QString info_query( "select att.attname, typ.typname, des.description from pg_class cla "
			"inner join pg_attribute att on att.attrelid=cla.oid "
			"inner join pg_type typ on att.atttypid=typ.oid "
			"left join pg_description des on cla.oid=des.classoid AND att.attnum=des.objsubid "
			"where cla.relkind='r' AND cla.relnamespace=2200 AND att.attnum>0 AND cla.relname='" );

	info_query += table->tableName().toLower() + "';";

	QSqlQuery q = mDatabase->exec( info_query );

	for( FieldIter it = fl.begin(); it != fl.end(); ++it)
		fieldMap[(*it)->name().toLower()] = *it;

	while( q.next() )
	{
		QString fieldName = q.value(0).toString();
		QMap<QString, Field*>::Iterator fi = fieldMap.find( fieldName );

		if( fi == fieldMap.end() )
			continue;

		fieldMap.remove ( fieldName );
	}

	if( !fieldMap.isEmpty() )
	{
		QFile file(migrationFile);
		file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
		QTextStream out(&file);
		for( QMap<QString, Field*>::Iterator it = fieldMap.begin(); it != fieldMap.end(); ++it) {
			Field * f = it.value();
			QString cc = "ALTER TABLE " + table->schema()->tableName() + " ADD COLUMN \"" + f->name().toLower() + "\" " + f->dbTypeString() + ";";
			(*output) += cc + "\n";
			out << cc << "\n";
		}
		file.close();
	}

	(*output) += "\n";
}

void CreateDatabaseDialog::createTableMigration( const QString& migrationFile, TableSchema * schema, QString * output)
{
	QString cre("CREATE TABLE ");
	cre += schema->tableName().toLower() + "  (";
	QStringList columns;
	FieldList fl = schema->ownedColumns();
	foreach( Field * f, fl ) {
		QString ct("\"" + f->name().toLower() + "\"");
		if( f->flag( Field::PrimaryKey ) ) {
			ct += " SERIAL PRIMARY KEY";
			columns.push_front( ct );
		} else {
			ct += " " + f->dbTypeString();
			columns += ct;
		}
	}
	cre += columns.join(",") + ")";
	if( schema->parent() )
		cre += " INHERITS (" + schema->parent()->tableName().toLower() + ")";
	cre += ";";

	(*output) += cre + "\n\n";
	QFile file(migrationFile);
	file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
	QTextStream out(&file);
	out << cre << "\n";
	file.close();
}

void CreateDatabaseDialog::createTablesMigration( const QString& migrationFile, QString * output )
{
	TableSchemaList tables = mSchema->tables();
	for ( TableSchemaList::iterator it = tables.begin(); it != tables.end(); ++it)
	{
		Table * table = mDatabase->tableFromSchema( (*it) );
		if ( table->exists() )
		{
			alterTableMigration( migrationFile, table, output );
		}
		else
		{
			createTableMigration( migrationFile, (*it), output );
		}
	}
}

void CreateDatabaseDialog::updateConnectionLabel()
{
	QString text;
	if ( !mDatabase->connection() ) return;
	mDatabase->connection()->checkConnection();
	IniConfig & cfg = config();
	cfg.pushSection( "Database" );
	if( mDatabase->connection()->isConnected() ) {
		text = "Connected: ";
	} else
		text = "Connection Failed: ";
	text += cfg.readString( "DatabaseName" ) + " on " + cfg.readString( "User" ) + ":" + cfg.readString( "Password" ).replace( QRegExp("."), "x" ) +
		"@"  + cfg.readString( "Host" );
	mUI.mConnectionStatus->setText( text );
	cfg.popSection();
}
