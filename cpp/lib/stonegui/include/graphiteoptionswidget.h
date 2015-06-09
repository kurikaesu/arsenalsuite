
#ifndef GRAPHITE_OPTIONS_WIDGET_H
#define GRAPHITE_OPTIONS_WIDGET_H

#include <qwidget.h>

#include "stonegui.h"

#include "ui_graphiteoptionswidgetui.h"

class GraphiteWidget;

class STONEGUI_EXPORT GraphiteOptionsWidget : public QWidget, public Ui::GraphiteOptionsWidgetUI
{
Q_OBJECT
public:
	GraphiteOptionsWidget( QWidget * parent = 0 );

	void setGraphiteWidget( GraphiteWidget * );

public slots:
	void applyOptions();
	void resetOptions();

protected slots:
	void timeRangeComboChanged();
	void valueRangeComboChanged();
	void showExtraListMenu( const QPoint & );
	
protected:
	GraphiteWidget * mGraphiteWidget;
};

#endif // GRAPHITE_OPTIONS_WIDGET_H
