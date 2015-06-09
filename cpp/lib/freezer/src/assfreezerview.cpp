
#include <qtimer.h>
#include <qevent.h>

#include "blurqt.h"
#include "freezercore.h"
#include "iniconfig.h"

#include "group.h"
#include "user.h"
#include "usergroup.h"

#include "assfreezerview.h"
#include "viewmanager.h"

FreezerView::FreezerView( QWidget * parent )
: QWidget( parent )
, mRefreshScheduled( false )
, mRefreshCount( 0 )
, mIniConfig()
{
	mIniConfig = userConfig();
}

FreezerView::~FreezerView()
{
	FreezerCore::instance()->cancelObjectTasks(this);
}

QString FreezerView::generateViewCode()
{
	static int viewNum = 0;
	QDateTime ctd = QDateTime::currentDateTime();
	return QString::number( ctd.toTime_t() ) + QString::number( ctd.time().msec() ) + QString::number( viewNum++ );
}

QString FreezerView::viewCode() const
{
	if( mViewCode.isEmpty() )
		mViewCode = generateViewCode();
	return mViewCode;
}

void FreezerView::setViewCode( const QString & viewCode )
{
	mViewCode = viewCode;
}

void FreezerView::refresh()
{
	if( !mRefreshScheduled ) {
		mRefreshScheduled = true;
		mRefreshCount++;
		uint timeToRefresh = 1;
		if( User::currentUser().userGroups().groups().contains( Group::recordByName("RenderOps") ) )
			if( mRefreshLast.secsTo(QDateTime::currentDateTime()) < 60 )
				timeToRefresh = 60 - mRefreshLast.secsTo(QDateTime::currentDateTime());
		if( User::currentUser().userGroups().groups().contains( Group::recordByName("RenderOps") ) )
			timeToRefresh = 1;

		QTimer::singleShot( 1000*timeToRefresh, this, SLOT( doRefresh() ) );
		mRefreshLast = QDateTime::currentDateTime();
		QTimer::singleShot( 0, this, SLOT( doRefresh() ) );
	}
}

int FreezerView::refreshCount() const
{
	return mRefreshCount;
}

void FreezerView::doRefresh()
{
	mRefreshScheduled = false;
}

IniConfig & FreezerView::viewConfig()
{
	IniConfig & config = userConfig();
	config.setSection( "View_" + viewCode() );
	return config;
}

void FreezerView::restorePopup( QWidget * w )
{
#ifndef Q_OS_WIN
	w->hide();
	w->setWindowFlags( w->windowFlags() | Qt::Tool );
	w->setAttribute( Qt::WA_DeleteOnClose, true );
	w->show();
#endif // !Q_OS_WIN
	w->installEventFilter( this );
	QString popupName = "Popup" + QString::number( mNextPopupNumber );
	IniConfig & cfg = userConfig();
	cfg.pushSection(popupName);
	QRect frameRect = cfg.readRect( "WindowGeometry", QRect( 0, 0, 200, 400 ) );
	w->resize( frameRect.size() );
	w->move( frameRect.topLeft() );
	cfg.popSection();
	mPopupList[w] = mNextPopupNumber;
	QList<int> values = mPopupList.values();
	while( values.contains( mNextPopupNumber ) ) mNextPopupNumber++;
}

bool FreezerView::eventFilter( QObject * o, QEvent * e )
{
	QWidget * w = qobject_cast<QWidget*>(o);
	if( w && mPopupList.contains( w ) ) {
		QEvent::Type type = e->type();
		if( type == QEvent::Resize || type == QEvent::Move ) {
			IniConfig & cfg = userConfig();
			cfg.pushSection( "Popup" + QString::number( mPopupList[w] ) );
			cfg.writeRect( "WindowGeometry", QRect( w->pos(), w->size() ) );
			cfg.popSection();
		}
		if( type == QEvent::Close ) {
			mNextPopupNumber = qMin( mNextPopupNumber, mPopupList[w] );
			mPopupList.remove( w );
		}
	}
	return QWidget::eventFilter( o, e );
}

QString FreezerView::statusBarMessage() const
{
	return mStatusBarMessage;
}

void FreezerView::setStatusBarMessage( const QString & sbm )
{
	mStatusBarMessage = sbm;
	emit statusBarMessageChanged( mStatusBarMessage );
}

void FreezerView::clearStatusBar()
{
	setStatusBarMessage(QString());
}

QString FreezerView::viewName() const
{
	if( mViewName.isEmpty() )
		const_cast<FreezerView*>(this)->setViewName( ViewManager::instance()->generateViewName( viewType() ) );
	return mViewName;
}

void FreezerView::setViewName( const QString & viewName )
{
	mViewName = viewName;
}

void FreezerView::save( IniConfig & ini, bool )
{
	ini.writeString("ViewName",mViewName);
	ini.writeString("ViewType",viewType());
}

void FreezerView::restore( IniConfig &, bool )
{
	applyOptions();
}
