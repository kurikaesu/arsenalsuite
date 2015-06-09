
#ifndef GRAPHITE_VIEW_H
#define GRAPHITE_VIEW_H

#include <qstring.h>
#include <qmap.h>
#include <qlist.h>

#include "graphitesource.h"
#include "graphitesaveddesc.h"

#include "assfreezerview.h"

class QAction;
class QMdiArea;
class QMdiSubWindow;
class QMenu;
class GraphiteWidget;

class FREEZER_EXPORT GraphiteView : public FreezerView
{
Q_OBJECT
public:
	GraphiteView(QWidget * parent);
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
	
	void generateTimeSeries( const GraphiteDesc & desc, int count );
protected:
	/// refreshes the host list from the database
	void doRefresh();

	void save( IniConfig & ini, bool );
	void restore( IniConfig & ini, bool );

	void populateViewSubMenu();

	QMdiArea * mMdiArea;
	QToolBar * mToolBar;
	QMenu * mViewSubMenu;
};

#endif // GRAPHITE_VIEW_H
