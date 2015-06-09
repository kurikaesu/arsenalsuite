
class GraphiteDialog : QDialog
{
%TypeHeaderCode
#include <graphitedialog.h>
%End
public:
	GraphiteDialog( QWidget * parent = 0 );
	
	void setGraphiteWidget( GraphiteWidget * );

	void accept();
	void reject();
	
public slots:
	void apply();
	void reset();
};
