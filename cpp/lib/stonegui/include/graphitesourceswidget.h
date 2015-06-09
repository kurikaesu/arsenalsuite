
#ifndef GRAPHITE_SOURCES_WIDGET_H
#define GRAPHITE_SOURCES_WIDGET_H

#include <qwidget.h>

#include "stonegui.h"

#include "ui_graphitesourceswidgetui.h"

class GraphiteWidget;

class STONEGUI_EXPORT GraphiteSourcesWidget : public QWidget, Ui::GraphiteSourcesWidgetUI
{
Q_OBJECT
public:
	GraphiteSourcesWidget(QWidget * parent=0);

	void setGraphiteWidget( GraphiteWidget * );

	QStringList sources();
public slots:
	void apply();
	void reset();
	void setSources( QStringList );

	void showListMenu( const QPoint & point );
	
protected:
	GraphiteWidget * mGraphiteWidget;
	QStringList mOriginalSources;
};

#endif // GRAPHITE_SOURCES_WIDGET_H
