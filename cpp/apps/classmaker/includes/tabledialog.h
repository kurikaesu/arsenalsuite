
#ifndef TABLE_DIALOG_H
#define TABLE_DIALOG_H

#include <qdialog.h>
#include "ui_tabledialogui.h"

namespace Stone {
class TableSchema;
class Schema;
}
using namespace Stone;

class TableDialog : public QDialog
{
Q_OBJECT
public:
    TableDialog( TableSchema * table, QWidget * parent );
    virtual void applySettings();
    static TableSchema * createTable( QWidget * parent, Schema * schema, TableSchema * parTable=0 );
    static bool modifyTable( QWidget * parent, TableSchema * table );
public slots:
	void useProjectPreload( bool );
	void slotNameChanged( const QString & );
	void editDocs();
protected:
    TableSchema * mTable;
	Ui::TableDialogUI mUI;
};

#endif // TABLE_DIALOG_H

