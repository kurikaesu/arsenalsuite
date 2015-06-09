
class GraphiteWidget : QLabel
{
%TypeHeaderCode
#include <graphitewidget.h>
%End
public:
	GraphiteWidget(QWidget * parent = 0);
	
	GraphiteDesc desc() const;

	GraphiteImageSource * imageSource() const;

signals:
	// Emitted immediately before the context menu is shown
	// menu will be recreated each time before this is called
	void aboutToShowMenu( QMenu * menu );

public slots:
	void refresh();
	void setSources( QStringList sources );
	void setAreaMode( GraphiteDesc::AreaMode );
	void setDateRange( const QDateTime & start, const QDateTime & end );
	void setValueRange( int minValue = INT_MAX, int maxValue = INT_MAX );
	void setGraphiteDesc( const GraphiteDesc & );

protected:
	void resizeEvent( QResizeEvent * );

};
