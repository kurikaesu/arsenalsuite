

#include <qlayout.h>
#include <qpushbutton.h>
#include <qlayout.h>

#include "datechooserwidget.h"
#include "elementschedulecontroller.h"
#include "schedulewidget.h"
#include "scheduleheader.h"


DateChooserWidget::DateChooserWidget( QWidget * parent )
: QWidget( parent )
{
	QLayout * layout = new QHBoxLayout( this );
	layout->setMargin( 0 );
	
	mScheduleWidget = new ScheduleWidget( new ElementScheduleController(), this );
	layout->addWidget( mScheduleWidget );

	mScheduleWidget->mCellMinHeight = 0;
	mScheduleWidget->mCellLeadHeight = 0;
	mScheduleWidget->setDisplayMode( ScheduleWidget::Month );
	mScheduleWidget->setZoom( 1 );
	mScheduleWidget->setSelectionMode( ScheduleWidget::SingleSelect );
	mScheduleWidget->header()->setupHeights( 30, 30, 30 );
}

void DateChooserWidget::setDate( const QDate & date )
{
	mScheduleWidget->setDate( date );
}

QDate DateChooserWidget::date() const
{
	ScheduleSelection ss = mScheduleWidget->selection();
	if( !ss.mCellSelections.isEmpty() )
		return ss.mCellSelections[0].startDate();
	return mScheduleWidget->date();
}

DateChooserDialog::DateChooserDialog( QWidget * parent )
: QDialog( parent )
, mDateChooserWidget( 0 )
{
	mDateChooserWidget = new DateChooserWidget( this );
	QPushButton * ok = new QPushButton( "&OK", this );
	QPushButton * cancel = new QPushButton( "&Cancel", this );

	connect( ok, SIGNAL( clicked() ), SLOT( accept() ) );
	connect( cancel, SIGNAL( clicked() ), SLOT( reject() ) );

	QBoxLayout * layout = new QVBoxLayout( this );
	layout->setSpacing( 2 );
	layout->setMargin( 4 );
	
	layout->addWidget( mDateChooserWidget );
	QBoxLayout * hlayout = new QHBoxLayout();
	hlayout->setSpacing( 4 );
	hlayout->setMargin( 0 );
	layout->addLayout( hlayout );
	hlayout->addStretch();
	hlayout->addWidget( ok );
	hlayout->addWidget( cancel );

}

QDate DateChooserDialog::getDate(QWidget*parent,const QDate & date)
{
	QDate ret = date;
	DateChooserDialog * dcd = new DateChooserDialog( parent );
	dcd->mDateChooserWidget->setDate( date );
	if( dcd->exec() == QDialog::Accepted )
		ret = dcd->mDateChooserWidget->date();
	delete dcd;
	return ret;
}
