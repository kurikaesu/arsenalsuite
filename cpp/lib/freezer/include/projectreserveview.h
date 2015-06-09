
#ifndef PROJECT_RESERVE_VIEW_H
#define PROJECT_RESERVE_VIEW_H

#include "afcommon.h"
#include "recordtreeview.h"

class FREEZER_EXPORT ProjectReserveView : public RecordTreeView
{
Q_OBJECT
public:
	ProjectReserveView(QWidget * parent);

public slots:
	void refresh();
};

#endif // PROJECT_RESERVE_VIEW_H
