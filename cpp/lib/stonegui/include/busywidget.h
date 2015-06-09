
#ifndef BUSY_WIDGET_H
#define BUSY_WIDGET_H

#include <qwidget.h>
#include <qpixmap.h>
#include <qdatetime.h>

#include "stonegui.h"

class QTimer;

class STONEGUI_EXPORT BusyWidget : public QWidget
{
Q_OBJECT
public:
	BusyWidget(QWidget * parent, const QPixmap & pixmap, double loopTime=1.6);

public slots:
	void start();
	void stop();
	void step();

protected:
	void paintEvent(QPaintEvent * pe);

	QPixmap mPixmap;
	double mLoopTime;
	int mSteps, mStep, mFadeIn, mFadeCurrent;
	QTimer * mTimer;
	QTime mStartTime;
};

#endif // BUSY_WIDGET_H

