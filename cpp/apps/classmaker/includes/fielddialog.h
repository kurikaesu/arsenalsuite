#ifndef FIELD_DIALOG_H
#define FIELD_DIALOG_H

#include <qdialog.h>

#include "ui_fielddialogui.h"

namespace Stone {
class TableSchema;
class Field;
}
using namespace Stone;

class FieldDialog : public QDialog
{
Q_OBJECT
public:
    FieldDialog( Field * field, QWidget * parent );
    virtual void applySettings();
    static Field * createField( QWidget * parent, TableSchema * schema );
    static bool modifyField( QWidget * parent, Field * field );
public slots:
    void typeChanged( const QString & );
    void nameChanged( const QString & );
    void methodNameChanged( const QString & );
	void editDocs();
protected:
    Field * mField;
	Ui::FieldDialogUI mUI;
	QString mLastMethodName;
};

#endif // FIELD_DIALOG_H

