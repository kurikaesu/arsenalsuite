
#ifndef PROJECT_WEIGHT_VIEW_H
#define PROJECT_WEIGHT_VIEW_H

#include "afcommon.h"
#include "recordtreeview.h"

class FREEZER_EXPORT ProjectWeightView : public RecordTreeView
{
Q_OBJECT
public:
	ProjectWeightView(QWidget * parent);

public slots:
	void refresh();
};

#endif // PROJECT_WEIGHT_VIEW_H
