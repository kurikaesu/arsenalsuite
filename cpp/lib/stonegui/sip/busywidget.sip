
class BusyWidget : QWidget
{
%TypeHeaderCode
#include <busywidget.h>
%End
public:
	BusyWidget(QWidget * parent, const QPixmap & pixmap, double loopTime=1.6);

public slots:
	void start();
	void stop();
	void step();
};


