

#include <qapplication.h>
#include "kdatepicker.h"

int main( int argc, char * argv[] )
{
	QApplication a(argc, argv);
	KDatePicker dp(0);
	dp.setCloseButton( true );
	a.setMainWidget(&dp);
	dp.show();
	return a.exec();
}
