
#include <qheaderview.h>
#include <qlistwidget.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qinputdialog.h>
#include <qitemdelegate.h>
#include <qpainter.h>

#include "statusdialog.h"
#include "resinerror.h"
#include "recordsupermodel.h"

struct StatusItem : public RecordItemBase
{
	ElementStatus status;
	void setup( const Record & r, const QModelIndex & = QModelIndex() ) {
		status = r;
	}
	QVariant modelData( const QModelIndex & i, int role ) const {
		if( role == Qt::DisplayRole || role == Qt::EditRole ) {
			switch( i.column() ) {
				case 0: return status.name();
				case 1: {
					QColor c;
					c.setNamedColor( status.color() );
					return c;
				}
			}
		}
		return QVariant();
	}
	bool setModelData( const QModelIndex & i, const QVariant & v, int role ) {
		if( role == Qt::EditRole ) {
			switch( i.column() ) {
				case 0:
					status.setName( v.toString() );
					break;
				case 1:
					status.setColor( qvariant_cast<QColor>(v).name() );
					break;
			}
			status.commit();
			return true;
		}
		return false;
	}
	int compare( const QModelIndex &, const QModelIndex &, int, bool );
	Qt::ItemFlags modelFlags( const QModelIndex & ) { return Qt::ItemFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable ); }
	Record getRecord() { return status; }
};

typedef TemplateRecordDataTranslator<StatusItem> StatusTranslator;

int StatusItem::compare( const QModelIndex &, const QModelIndex & idx2, int, bool )
{
	return status.order() - StatusTranslator::data(idx2).status.order();
}

StatusDialog::StatusDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	
	connect( mAddButton, SIGNAL( clicked() ),  SLOT( addStatus() ) );
	connect( mRemoveButton, SIGNAL( clicked() ), SLOT( removeStatus() ) );
	connect( mUpButton, SIGNAL( clicked() ), SLOT( moveUp() ) );
	connect( mDownButton, SIGNAL( clicked() ), SLOT( moveDown() ) );

	QStringList headerLabels;
	headerLabels << "Status" << "Color";

	RecordSuperModel * sm = new RecordSuperModel(mStatusTree);
	new StatusTranslator(sm->treeBuilder());
	sm->setAutoSort(true);
	mStatusTree->setModel( sm );
	sm->setHeaderLabels( headerLabels );
	mStatusTree->setRootIsDecorated( false );
	mStatusTree->header()->setClickable(false);
	mStatusTree->setColumnAutoResize( 0, true );
//	connect( mStatusTree, SIGNAL( currentTextChanged( const QString & ) ), SLOT( templateChanged( const QString & ) ) );
}


StatusSet StatusDialog::statusSet()
{
	return mStatusSet;
}

void StatusDialog::setStatusSet( const StatusSet & ss )
{
	if( ss != mStatusSet ) {
		mStatusSet = ss;
		mNameEdit->setText( mStatusSet.name() );
		updateStatuses();
	}
}

void StatusDialog::accept()
{
	// Commit the set record
	mStatusSet.setName( mNameEdit->text() );
	mStatusSet.commit();

	// Lets iterate through and make sure the status order column is correct
	int last = mStatusTree->model()->rowCount()-1;
	ElementStatusList toCommit;
	for( int i=0; i <= last; i++ ) {
		ElementStatus es = mStatusTree->model()->getRecord(mStatusTree->model()->index(i,0));
		es.setOrder(i);
		es.setStatusSet(mStatusSet);
		toCommit += es;
	}
	toCommit.commit();

	QDialog::accept();
}

void StatusDialog::reject()
{
	QDialog::reject();
}

/*void StatusDialog::templateChanged( const QString & tn )
{
	AssetTemplateList match = mTemplates.filter( "name", tn );
	if( match.size() == 1 ) {
		mTemplate = match[0];
		mEditButton->setEnabled( true );
		mRemoveButton->setEnabled( tn != "Default" );
	} else {
		mEditButton->setEnabled( false );
		mRemoveButton->setEnabled( false );
	}
}
*/

void StatusDialog::addStatus()
{
	mStatusTree->selectionModel()->setCurrentIndex(mStatusTree->model()->append(),QItemSelectionModel::ClearAndSelect);
}

void StatusDialog::removeStatus()
{
	QModelIndex i = mStatusTree->selectionModel()->currentIndex();
	if( i.isValid() ) {
		ElementStatus es = mStatusTree->model()->getRecord(i);
		es.remove();
		mStatusTree->model()->remove(i);
	}
}

void StatusDialog::swap( const QModelIndex & a, const QModelIndex & b )
{
	ElementStatus as = mStatusTree->model()->getRecord(a);
	ElementStatus bs = mStatusTree->model()->getRecord(b);
	as.setOrder(as.order()+1);
	bs.setOrder(bs.order()-1);
	as.commit();
	bs.commit();
	mStatusTree->model()->swap(a,b);
}

void StatusDialog::moveUp()
{
	QModelIndex i = mStatusTree->selectionModel()->currentIndex();
	if( i.isValid() && i.row() > 0 ) {
		QModelIndex a = i.sibling(i.row()-1,i.column());
		swap(a,i);
	}
}

void StatusDialog::moveDown()
{
	QModelIndex i = mStatusTree->selectionModel()->currentIndex();
	if( i.isValid() && i.row() < mStatusTree->model()->rowCount()-1 ) {
		QModelIndex a = i.sibling(i.row()+1,i.column());
		swap(i,a);
	}
}

void StatusDialog::updateStatuses()
{
	ElementStatusList esl = mStatusSet.elementStatuses();
	mStatusTree->model()->setRootList( esl );
}
