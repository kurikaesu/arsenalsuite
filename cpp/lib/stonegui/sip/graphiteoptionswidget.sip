
class GraphiteOptionsWidget : QWidget
{
%TypeHeaderCode
#include <graphiteoptionswidget.h>
%End
public:
	GraphiteOptionsWidget( QWidget * parent = 0 );

	void setGraphiteWidget( GraphiteWidget * );

public slots:
	void applyOptions();
	void resetOptions();

};
