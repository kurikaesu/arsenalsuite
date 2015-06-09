

#ifndef ASSET_TEMPLATE_DIALOG_H
#define ASSET_TEMPLATE_DIALOG_H

#include <qtreewidget.h>

#include "classesui.h"

#include "ui_assettemplatedialogui.h"

#include "assettype.h"
#include "assettemplate.h"
#include "element.h"
#include "filetracker.h"


class CLASSESUI_EXPORT AssetTemplateModel : public RecordSuperModel
{
Q_OBJECT
public:
	AssetTemplateModel( QObject * parent = 0 );

	static Element getElement( const QModelIndex & index );

	// If build is false, it will not build the tree internally
	// to find the element, in other words, if the element isn't
	// displayed, then it won't be found.
	QModelIndex findElement( const Element &, bool build = true );
	QModelIndex findFileTracker( const FileTracker &, bool build = true );
	virtual QStringList mimeTypes() const;

protected:
	bool dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );

public slots:
	virtual void added( RecordList );
	virtual void removed( RecordList );
	virtual void updated( Record, Record );
};

class CLASSESUI_EXPORT AssetTemplateDialog : public QDialog, public Ui::AssetTemplateDialogUI
{
Q_OBJECT
public:
	AssetTemplateDialog( QWidget * parent = 0 );
	
	void accept();
	void reject();
	
public slots:
	void setTemplate( const AssetTemplate & );

	void showContextMenu( const QPoint &, const QModelIndex & );

protected:
	AssetTemplate mTemplate;
	ElementList mAdded, mDeleted;
};

#endif // ASSET_TEMPLATE_DIALOG_H

