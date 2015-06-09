
#ifndef INDEX_DIALOG_H
#define INDEX_DIALOG_H

#include <qdialog.h>

#include "ui_indexdialogui.h"

namespace Stone {
class TableSchema;
class IndexSchema;
}
using namespace Stone;

class IndexDialog : public QDialog
{
Q_OBJECT
public:
    IndexDialog( IndexSchema * index, QWidget * parent );
    virtual void applySettings();
    static IndexSchema * createIndex( QWidget * parent, TableSchema * table );
    static bool modifyIndex( QWidget * parent, IndexSchema * index );
public slots:
    void addField();
protected:
    IndexSchema * mIndex;
	Ui::IndexDialogUI mUI;
};

#endif // INDEX_DIALOG_H

