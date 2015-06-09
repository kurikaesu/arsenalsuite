

#ifndef ASSET_TEMPLATE_COMBO_H
#define ASSET_TEMPLATE_COMBO_H

#include <qcombobox.h>

#include "classesui.h"

#include "assettemplate.h"
#include "assettype.h"
#include "project.h"
#include "recordlistmodel.h"

class CLASSESUI_EXPORT AssetTemplateCombo : public QComboBox
{
Q_OBJECT
public:
	AssetTemplateCombo( QWidget * parent );

	void setAssetType( const AssetType & );
	void setProject( const Project & );

	AssetType assetType();
	Project project();

	AssetTemplate assetTemplate();
	void setAssetTemplate( const AssetTemplate & at );

signals:
	void templateChanged( const AssetTemplate & at );

public slots:
	void indexActivated( int );
	void refresh();

protected:

	AssetTemplate mCurrent;
	RecordListModel * mModel;
	AssetType mAssetType;
	Project mProject;
};

#endif // ASSET_TEMPLATE_COMBO_H

