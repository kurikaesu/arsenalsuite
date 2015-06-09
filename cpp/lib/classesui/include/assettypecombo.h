

#ifndef ASSET_TYPE_COMBO_H
#define ASSET_TYPE_COMBO_H

#include "classesui.h"

#include "recordcombo.h"
#include "assettype.h"

class CLASSESUI_EXPORT AssetTypeCombo : public RecordCombo
{
Q_OBJECT
public:
	AssetTypeCombo( QWidget * parent );

	void setTagFilters( QStringList );
	QStringList tagFilters() const { return mTagFilters; }

	void setShowDisabled( bool );
	bool showDisabled() const { return mShowDisabled; }

	void setShowFirst( AssetTypeList atl );
	AssetTypeList showingFirst() const { return mShowingFirst; }

protected:
	void refresh();

	QStringList mTagFilters;
	bool mShowDisabled;
	AssetTypeList mShowingFirst;
};

#endif // ASSET_TYPE_COMBO_H
