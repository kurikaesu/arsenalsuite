
#ifndef GRAPHITE_DIALOG_H
#define GRAPHITE_DIALOG_H

#include <qdialog.h>

#include "stonegui.h"

#include "ui_graphitedialogui.h"

class GraphiteSourcesWidget;
class GraphiteOptionsWidget;
class GraphiteWidget;

class STONEGUI_EXPORT GraphiteDialog : public QDialog, public Ui::GraphiteDialogUI
{
Q_OBJECT
public:
	GraphiteDialog( QWidget * parent = 0 );
	
	void setGraphiteWidget( GraphiteWidget * );

	void accept();
	void reject();
	
public slots:
	void apply();
	void reset();
	
protected:
	GraphiteSourcesWidget * mSourcesWidget;
	GraphiteOptionsWidget * mOptionsWidget;
};

#endif // GRAPHITE_DIALOG_H
