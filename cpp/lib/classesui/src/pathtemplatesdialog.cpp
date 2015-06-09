
#include <qtreewidget.h>
#include <qpushbutton.h>

#include "pathtemplatesdialog.h"
#include "pathtemplatedialog.h"
#include "resinerror.h"
#include "element.h"

class PathTemplateItem : public QTreeWidgetItem
{
public:
	PathTemplateItem( QTreeWidget * , const PathTemplate & );

	static const int Type = QTreeWidgetItem::UserType;
protected:
	PathTemplate mTemplate;
	friend class PathTemplatesDialog;
};

PathTemplateItem::PathTemplateItem( QTreeWidget * tw, const PathTemplate & pt )
: QTreeWidgetItem( tw, Type )
, mTemplate( pt )
{
	setText( 0, pt.name() );
	setText( 1, pt.pathTemplate() );
	setText( 2, pt.fileNameTemplate() );
}


PathTemplatesDialog::PathTemplatesDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	QStringList hl;
	hl << "Name" << "Path" << "FileName";
	mTree->setHeaderLabels( hl );
	connect( mTree, SIGNAL( currentItemChanged( QTreeWidgetItem *, QTreeWidgetItem * ) ), SLOT( currentChanged( QTreeWidgetItem * ) ) );
	connect( mAddButton, SIGNAL( clicked() ), SLOT( addTemplate() ) );
	connect( mEditButton, SIGNAL( clicked() ), SLOT( editTemplate() ) );
	connect( mRemoveButton, SIGNAL( clicked() ), SLOT( removeTemplate() ) );

	connect( PathTemplate::table(), SIGNAL( added( RecordList ) ), SLOT( refresh() ) );
	connect( PathTemplate::table(), SIGNAL( removed( RecordList ) ), SLOT( refresh() ) );
	connect( PathTemplate::table(), SIGNAL( updated( Record, Record ) ), SLOT( refresh() ) );
	refresh();

	// Not sure if this is needed
	currentChanged( mTree->currentItem() );
}

void PathTemplatesDialog::accept()
{
	QDialog::accept();
}

void PathTemplatesDialog::reject()
{
	QDialog::reject();
}

void PathTemplatesDialog::refresh()
{
	mTree->clear();
	PathTemplateList all = PathTemplate::select().sorted( "name" );
	foreach( PathTemplate pt, all )
		new PathTemplateItem( mTree, pt );
}

void PathTemplatesDialog::currentChanged( QTreeWidgetItem * item )
{
	bool isItem( item!=0 );
	mEditButton->setEnabled( isItem );
	mRemoveButton->setEnabled( isItem );
	mCurrentItem = (PathTemplateItem*)item;
	if( mCurrentItem )
		mCurrent = mCurrentItem->mTemplate;
	else
		mCurrent = PathTemplate();
}

void PathTemplatesDialog::addTemplate()
{
	PathTemplateDialog * ptd = new PathTemplateDialog( this );
	if( ptd->exec() == QDialog::Accepted ) {
		ptd->pathTemplate().commit();
		refresh();
	}
	delete ptd;
}

void PathTemplatesDialog::editTemplate()
{
	if( !mCurrentItem ) return;
	PathTemplateDialog * ptd = new PathTemplateDialog( this );
	ptd->setPathTemplate( mCurrent );
	if( ptd->exec() == QDialog::Accepted ) {
		ptd->pathTemplate().commit();
		Element::invalidatePathCache();
	}
	delete ptd;
}

void PathTemplatesDialog::removeTemplate()
{
	if( !mCurrentItem || !ResinError::deleteConfirmation( this ) ) return;
	mCurrent.remove();
	Element::invalidatePathCache();
	refresh();
}
