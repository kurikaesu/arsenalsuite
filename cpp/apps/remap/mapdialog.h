

#include <qdialog.h>

#include "ui_mapdialogui.h"


class MapDialog : public QDialog, public Ui::MapDialogUI
{
Q_OBJECT
public:
	MapDialog( QWidget * parent = 0 );

public slots:
	void map();
	void unmap();
	void updateStatus();

protected:
};


