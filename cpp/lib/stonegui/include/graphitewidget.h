
#ifndef GRAPHITE_WIDGET_H
#define GRAPHITE_WIDGET_H

#include <limits.h>

#include <qlabel.h>
#include <qimage.h>

#include "stonegui.h"
#include "graphitesource.h"

class QTimer;
class QMenu;

class STONEGUI_EXPORT GraphiteWidget : public QLabel
{
Q_OBJECT
public:
	GraphiteWidget(QWidget * parent = 0);

	GraphiteDesc desc() const;
	
	GraphiteImageSource * imageSource() const;

	QSize minimumSizeHint() const;

signals:
	// Emitted immediately before the context menu is shown
	// menu will be recreated each time before this is called
	void aboutToShowMenu( QMenu * menu );

public slots:
	void refresh();

	void setGraphiteDesc( const GraphiteDesc & );
	void setSources( QStringList sources );
	void setAreaMode( GraphiteDesc::AreaMode );
	void setDateRange( const QDateTime & start, const QDateTime & end );
	void setValueRange( int minValue = INT_MAX, int maxValue = INT_MAX );

	void showDialog();
	void showContextMenu( const QPoint & );
	void copyUrlToClipboard();
	
protected slots:
	void slotImageFinished( const QImage & );
	void doRefresh();
	
protected:
	void resizeEvent( QResizeEvent * );
	void mouseDoubleClickEvent( QMouseEvent * );
	
	QTimer * mResizeTimer;
	GraphiteImageSource * mImageSource;
	GraphiteDesc mDesc;
	bool mRefreshScheduled;
};

#endif // GRAPHITE_WIDGET_H
