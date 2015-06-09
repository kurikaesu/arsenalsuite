
class GraphiteSourcesWidget : QWidget
{
%TypeHeaderCode
#include <graphitesourceswidget.h>
%End
public:
	GraphiteSourcesWidget(QWidget * parent=0);

	void setGraphiteWidget( GraphiteWidget * );

	QStringList sources();
public slots:
	void apply();
	void reset();

	void setSources( QStringList );

	void showListMenu( const QPoint & point );
};