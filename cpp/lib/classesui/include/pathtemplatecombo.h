

#ifndef PATH_TEMPLATE_COMBO_H
#define PATH_TEMPLATE_COMBO_H

#include <qcombobox.h>

#include "classesui.h"

#include "pathtemplate.h"
#include "recordlistmodel.h"

class CLASSESUI_EXPORT PathTemplateCombo : public QComboBox
{
Q_OBJECT
public:
	PathTemplateCombo( QWidget * parent );

	PathTemplate pathTemplate();
	void setPathTemplate( const PathTemplate & pt );

signals:
	void templateChanged( const PathTemplate & at );

public slots:
	void indexActivated( int );
	void refresh();

protected:

	PathTemplate mCurrent;
	RecordListModel * mModel;
};

#endif // PATH_TEMPLATE_COMBO_H

