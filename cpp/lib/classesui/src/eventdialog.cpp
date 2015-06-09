
#include "calendarcategory.h"
#include "eventdialog.h"
#include "recordcombo.h"
#include "user.h"

EventDialog::EventDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi(this);
	mCategoryCombo->setColumn( "name" );
	mCategoryCombo->setItems( CalendarCategory::select() );
	mProjectCombo->setSpecialItemText( "None" );
	mProjectCombo->setStatusFilters( ProjectStatusList() + ProjectStatus::recordByName( "Production" ) );
}

EventDialog::~EventDialog()
{}

void EventDialog::setEvent( const Calendar & c )
{
	mEvent = c;
	mDescriptionEdit->setPlainText( mEvent.name() );
	mWhenDateTimeEdit->setDateTime( mEvent.dateTime() );
	mCategoryCombo->setCurrent( mEvent.category() );
	mProjectCombo->setProject( mEvent.project() );
}

void EventDialog::accept()
{
	mEvent.setName( mDescriptionEdit->toPlainText() );
	mEvent.setDateTime( mWhenDateTimeEdit->dateTime() );
	mEvent.setUser( User::currentUser() );
	mEvent.setCategory( mCategoryCombo->current() );
	mEvent.setProject( mProjectCombo->project() );
	mEvent.commit();
	QDialog::accept();
}

void EventDialog::reject()
{
	QDialog::reject();
}
