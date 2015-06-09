
#include <qapplication.h>

#include "mapdialog.h"

int main(int argc, char * argv[])
{
	QApplication * a = new QApplication( argc, argv );

	MapDialog * md = new MapDialog();
	md->show();
	int res = a->exec();
	delete a;
	return res;
}

