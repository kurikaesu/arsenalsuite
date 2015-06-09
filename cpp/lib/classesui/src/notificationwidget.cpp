
#include <qdialogbuttonbox.h>

#include "notificationwidget.h"

NotificationWidget::NotificationWidget( QWidget * parent )
: QWidget( parent )
{
	setupUi( this );

	connect( mComponentEdit, SIGNAL( textChanged( const QString & ) ), mNotificationView, SLOT( setComponentFilter( const QString & ) ) );
	connect( mMethodEdit, SIGNAL( textChanged( const QString & ) ), mNotificationView, SLOT( setMethodFilter( const QString & ) ) );
	connect( mSubjectEdit, SIGNAL( textChanged( const QString & ) ), mNotificationView, SLOT( setSubjectFilter( const QString & ) ) );

	connect( mStartDateTime, SIGNAL( dateTimeChanged( const QDateTime & ) ), mNotificationView, SLOT( setStartDateTime( const QDateTime & ) ) );
	connect( mEndDateTime, SIGNAL( dateTimeChanged( const QDateTime & ) ), mNotificationView, SLOT( setEndDateTime( const QDateTime & ) ) );
	
	connect( mLimitSpin, SIGNAL( valueChanged( int ) ), mNotificationView, SLOT( setLimit( int ) ) );

	connect( mRefreshButton, SIGNAL( clicked() ), mNotificationView, SLOT( refresh() ) );

	QDateTime curDT = QDateTime::currentDateTime();
	mStartDateTime->setDateTime( curDT.addYears(-1) );
	mEndDateTime->setDateTime( curDT );
}


NotificationDialog::NotificationDialog( QWidget * parent )
: QDialog( parent )
{
	QVBoxLayout * l = new QVBoxLayout( this );
	l->addWidget( new NotificationWidget( this ) );
	QDialogButtonBox * dbb = new QDialogButtonBox( QDialogButtonBox::Close, Qt::Horizontal, this );
	l->addWidget( dbb );
	connect( dbb->button( QDialogButtonBox::Close ), SIGNAL( clicked() ), SLOT( accept() ) );
}
