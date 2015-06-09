
#ifndef ASSET_TEMPLATES_DIALOG_H
#define ASSET_TEMPLATES_DIALOG_H

#include <qdialog.h>

#include "ui_assettemplatesdialogui.h"

#include "classesui.h"

#include "assettemplate.h"
#include "assettype.h"
#include "project.h"

class CLASSESUI_EXPORT AssetTemplatesDialog : public QDialog, public Ui::AssetTemplatesDialogUI
{
Q_OBJECT
public:
	AssetTemplatesDialog( QWidget * parent = 0 );

	void setProject( const Project & );
	void setAssetType( const AssetType & );
	
	void accept();
	void reject();

public slots:
	void projectChanged( const Record & project );
	void assetTypeChanged( const Record & assetType );
	void addTemplate();
	void removeTemplate();
	void editTemplate();

	void slotCurrentChanged( const Record & );

protected:
	void updateTemplates();

	Project mProject;
	AssetType mAssetType;
	AssetTemplate mTemplate;
	AssetTemplateList mTemplates, mAdded, mRemoved;
};

#endif // ASSET_TEMPLATES_DIALOG_H

