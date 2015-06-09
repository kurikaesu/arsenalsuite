
#include <qpainter.h>
#include <qtimer.h>

#include "busywidget.h"


BusyWidget::BusyWidget(QWidget * parent, const QPixmap & pixmap, double loopTime)
: QWidget( parent )
, mPixmap( pixmap )
, mLoopTime( loopTime )
, mSteps( 0 )
, mStep( 0 )
, mFadeIn( 500 )
, mFadeCurrent( 0 )
, mTimer( 0 )
{
	if( !mPixmap.isNull() && pixmap.height() ) {
		mSteps = pixmap.height() / pixmap.width();
		mStep = 0;
		mTimer = new QTimer(this);
		connect(mTimer, SIGNAL(timeout()), SLOT(step()) );
		resize(pixmap.width(),pixmap.width());
	}
	hide();
}

void BusyWidget::start()
{
	show();
	if( mTimer ) {
		mTimer->start(40);
		mStartTime.restart();
		mFadeCurrent = 0;
	}
}

void BusyWidget::stop()
{
	if( mTimer ) {
		mTimer->stop();
		update();
	}
	hide();
}

void BusyWidget::step()
{
	mStep = int((mStartTime.elapsed() / (mLoopTime*1000.0)) * mSteps) % mSteps;
	mFadeCurrent = qMin(mStartTime.elapsed(), mFadeIn);
	update();
}

void BusyWidget::paintEvent(QPaintEvent * pe)
{
	QPainter p(this);
	int w = mPixmap.width();
	p.setOpacity( mFadeCurrent / double(mFadeIn) );
	p.drawPixmap(QPoint(0,0),mPixmap,QRect(0,w * mStep,w,w));
}