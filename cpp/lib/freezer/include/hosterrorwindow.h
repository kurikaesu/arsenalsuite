

#ifndef HOST_ERROR_WINDOW_H
#define HOST_ERROR_WINDOW_H

#include <qmainwindow.h>

#include "host.h"

#include "afcommon.h"

class RecordTreeView;
class ErrorModel;
class ErrorListTask;
class QEvent;

class FREEZER_EXPORT HostErrorWindow : public QMainWindow
{
Q_OBJECT
public:
	HostErrorWindow(QWidget * parent = 0);

	Host host() const { return mHost; }
	int limit() const { return mLimit; }

public slots:	
	void setHost( const Host & host );

	void setLimit( int limit );

	void refresh();
	
protected slots:
	void doRefresh();

	void showMenu( const QPoint &, const Record & underMouse, RecordList selected );
	void populateHeaderMenu( QMenu * );
	void setShowClearedErrors( bool sce );
	void slotGroupingChanged(bool grouped);

protected:
	void customEvent( QEvent * evt );
	
	Host mHost;
	int mLimit;
	bool mShowClearedErrors;
	ErrorListTask * mTask;
	bool mRefreshQueued, mRefreshScheduled;
	ErrorModel * mModel;
	RecordTreeView * mErrorTree;
};

#endif // HOST_ERROR_WINDOW_H
