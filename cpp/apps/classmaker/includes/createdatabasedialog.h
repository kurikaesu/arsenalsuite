

#ifndef CREATE_DATABASE_DIALOG_H
#define CREATE_DATABASE_DIALOG_H

#include <qdialog.h>

#include "ui_createdatabasedialogui.h"

namespace Stone {
class Database;
class Schema;
class TableSchema;
class Table;
}
using namespace Stone;

class CreateDatabaseDialog : public QDialog
{
Q_OBJECT
public:
	CreateDatabaseDialog( Schema * schema, QWidget * parent=0 );

public slots:
	void setTableSchema( TableSchema * tableSchema );

	void editConnection();
	void verify();
	void create();
	QString getNextMigrationFile();
	void alterTableMigration( const QString& migrationFile, Table * table, QString * output );
	void createTableMigration( const QString& migrationFile, TableSchema * tableSchema, QString * output );
	void createTablesMigration( const QString& migrationFile, QString * output );
	void updateConnectionLabel();

protected:
	Schema * mSchema;
	Database * mDatabase;
	TableSchema * mTableSchema;
	Ui::CreateDatabaseDialogUI mUI;
	QString mMigrationsDirectory;
};

#endif // CREATE_DATABASE_DIALOG_H

