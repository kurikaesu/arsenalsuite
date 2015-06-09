/*
 *
 * Copyright 2003, 2004 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Resin is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Resin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef NEW_ASSET_DIALOG_H
#define NEW_ASSET_DIALOG_H

#include <qdialog.h>

#include "ui_assetdialogui.h"

#include "assettype.h"
#include "assettemplate.h"
#include "element.h"
#include "project.h"

class ElementCheckTree;
class TasksWidget;

/**
 * AssetDialog - Dialog for creating and editting assets
 *
 * Inherited from AssetDialogUI in ui/assetdialogui.ui
 **/

class CLASSESUI_EXPORT AssetDialog : public QDialog, public Ui::AssetDialogUI
{
Q_OBJECT
public:
	AssetDialog( const Element & parEl, QWidget * parent=0 );

	void setAssetType( const AssetType & at );

	ElementList created();

	virtual void accept();
	virtual void reject();

	// If this is true the asset template combo
	// will be displayed allowing the user to choose
	// an asset template.  The created asset will
	// have it's fkeyassettemplate set to this value
	void setAssetTemplatesEnabled( bool );
	
	// If true and asset templates are enabled(above),
	// then the asset will be created using the asset
	// template. This defaults to true.
	void setCreateAssetTemplates( bool );

	void setPathTemplatesEnabled( bool );

	// Calling this function will hide the create
	// button, disable the assetcombo, and
	// allow editting an existing asset.
	void setAsset( const Element & );

public slots:
	void assetTypeChanged( const QString & );

	void assetTemplateChanged( const AssetTemplate & at );

	void create();

	void updateStatusLabel( const QString & );

protected:
	ElementList mCreated;
	Element mParent;
	TasksWidget * mTasksWidget;
	bool mAssetTemplatesEnabled, mCreateAssetTemplates, mPathTemplatesEnabled;
	Element mAsset;
	AssetType mLastAssetType;
	AssetTemplate mLastAssetTemplate;
};

#endif // NEW_ASSET_DIALOG_H

