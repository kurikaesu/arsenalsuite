

#ifndef PROJECT_COMBO_H
#define PROJECT_COMBO_H

#include <qcombobox.h>

#include "classesui.h"
#include "project.h"
#include "recordcombo.h"
#include "projectstatus.h"
#include "recordlistmodel.h"

class CLASSESUI_EXPORT ProjectCombo : public RecordCombo
{
Q_OBJECT
public:
	ProjectCombo( QWidget * parent );

	Project project() const;
	ProjectStatusList statusFilters() const;

	/// Defaults to true
	bool showingSpecialItem() const;

	/// Defaults to "All"
	void setSpecialItemText( const QString & );
	QString specialItemText() const;

signals:
	void projectChanged( const Project & p );

public slots:
	void setShowSpecialItem( bool );
	void setStatusFilters( ProjectStatusList );
	void setProject( const Project & );
	void refresh();

protected slots:
	void indexChanged( int );
	void doRefresh();

protected:
	bool mShowSpecialItem;
	QString mSpecialItemText;
	ProjectStatusList mStatusFilters;
	Project mCurrent;
	RecordListModel * mModel;
	bool mRefreshRequested;
};

#endif // PROJECT_COMBO_H

