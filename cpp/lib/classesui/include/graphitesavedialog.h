

#ifndef GRAPHITE_SAVE_DIALOG_H
#define GRAPHITE_SAVE_DIALOG_H

#include <qaction.h>
#include <qdialog.h>
#include <qmenu.h>

#include "graphitesource.h"
#include "graphitesaveddesc.h"

#include "classesui.h"

#include "ui_graphitesavedialogui.h"

class GraphiteWidget;

class CLASSESUI_EXPORT GraphiteSaveDialog : public QDialog, Ui::GraphiteSaveDialogUI
{
Q_OBJECT
public:
	/// Pass a new GraphiteSavedDesc class with URL set, or an existing
	/// one in order to modify it's name and group
	GraphiteSaveDialog( const GraphiteSavedDesc &, QWidget * parent = 0 );

	void accept();
	
	GraphiteSavedDesc savedDesc() const { return mSavedDesc; }
protected:
	GraphiteSavedDesc mSavedDesc;
};

class CLASSESUI_EXPORT GraphiteSaveAction : public QAction
{
Q_OBJECT
public:
	GraphiteSaveAction( GraphiteWidget * widget, QObject * parent );
	
signals:
	void saved( GraphiteWidget * widget, const GraphiteSavedDesc & );
	
protected slots:
	void slotTriggered();
	
protected:
	GraphiteWidget * mGraphiteWidget;
};

class CLASSESUI_EXPORT GraphiteLoadMenu : public QMenu
{
Q_OBJECT
public:
	GraphiteLoadMenu( const QString & title, QWidget * parent );
	
signals:
	void loadGraph( const GraphiteSavedDesc & );
	
protected slots:
	void slotAboutToShow();
	void slotAboutToShowGroupMenu();
	void slotSavedDescTriggered( QAction * action );
	
protected:
	QMap<QString,GraphiteSavedDescList> mSavedDescMap;
	
};

class CLASSESUI_EXPORT GraphiteGenerateSeriesAction : public QAction
{
Q_OBJECT
public:
	GraphiteGenerateSeriesAction( GraphiteWidget * widget, QObject * parent );
	
signals:
	void generateSeries( QList<GraphiteDesc> descriptions );
	
protected slots:
	void slotTriggered();
	
protected:
	GraphiteWidget * mGraphiteWidget;
};


#endif // GRAPHITE_SAVE_DIALOG_H
