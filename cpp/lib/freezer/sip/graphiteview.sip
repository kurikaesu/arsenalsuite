

class GraphiteView : FreezerView
{
%TypeHeaderCode
#include <graphiteview.h>
%End
public:
	GraphiteView(QWidget * parent /TransferThis/);
	~GraphiteView();

	virtual QString viewType() const;

	QAction* RefreshGraphsAction;
	QAction* NewGraphAction;
	QAction* NewGraphFromUrlAction;
	
	virtual QToolBar * toolBar( QMainWindow * );
	virtual void populateViewMenu( QMenu * );

	GraphiteWidget * newGraph( const GraphiteDesc & desc, const QRect & rect = QRect(), const QString & title = QString() );

	GraphiteWidget * widgetFromSubWindow( QMdiSubWindow * ) const;
	QMdiSubWindow * subWindowFromWidget( GraphiteWidget * ) const;

	QList<GraphiteWidget*> graphiteWidgets() const;
	
public slots:

	void applyOptions();

	void newGraph();
	void newGraphFromUrl();
	void loadSavedGraph( const GraphiteSavedDesc & );

	void showMdiAreaMenu( const QPoint & pos );
	
	void populateGraphiteWidgetMenu( QMenu * menu );
	
	void slotGraphSaved( GraphiteWidget *, const GraphiteSavedDesc & );
	
	void generateTimeSeries(const GraphiteDesc & desc, int count);
};

