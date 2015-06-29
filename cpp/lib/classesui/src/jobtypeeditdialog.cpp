
#include <qdialog.h>

#include "recordtreeview.h"
#include "jobtypeeditdialog.h"
#include "service.h"

class JobTypeEditItem : public RecordItemBase
{
public:
	JobType jtype;
	JobTypeEditItem( const JobType & jt ) { setup(jt); }
	JobTypeEditItem(){}
	void setup( const Record & r, const QModelIndex & = QModelIndex() ) {
		jtype = r;
	}
	QVariant modelData( const QModelIndex & i, int role ) {
		int col = i.column();
		switch( col ) {
			case 0:
			{
				if( role == Qt::DisplayRole )
					return jtype.key();
				break;
			}	
			case 1:
			{
				if( role == Qt::DisplayRole || role == Qt::EditRole )
					return jtype.name();
				else if ( role == RecordDelegate::FieldNameRole )
					return "jobtype";
				break;
			}
			case 2:
			{
				if( role == Qt::DisplayRole )
					return jtype.service().service();
				else if( role == Qt::EditRole ) {
					Service none;
					none.setService( "None" );
					ServiceList sl = Service::select().sorted("service");
					sl.insert(sl.begin(),none);
					return qVariantFromValue<RecordList>(sl);
				} else if( role == RecordDelegate::CurrentRecordRole )
					return qVariantFromValue<Record>(jtype.service());
				else if( role == RecordDelegate::FieldNameRole )
					return "service";
				break;
			}
		}
		return QVariant();
	}
	bool setModelData( const QModelIndex & i, const QVariant & v, int role ) {
		int col = i.column();
		if( role == Qt::EditRole ) {
			switch( col ) {
				case 0:
					return false;
				case 1:
				{
					jtype.setName(v.toString());
					break;
				}
				case 2:
				{
					Service s = qvariant_cast<Record>(v);
					if( s.isValid() )
						jtype.setService( s );
					break;
				}
			}
			jtype.commit();
			return true;
		}
		return false;
	}
	Qt::ItemFlags modelFlags( const QModelIndex & index ) {
		if( index.column() > 1 )
			return Qt::ItemFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
		return Qt::ItemFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
	}
	Record getRecord() { return jtype; }
};

typedef TemplateRecordDataTranslator<JobTypeEditItem> JobTypeEditTranslator;

JobTypeEditDialog::JobTypeEditDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	mModel = new RecordSuperModel(mTreeView);
	new JobTypeEditTranslator(mModel->treeBuilder());
	mTreeView->setModel(mModel);
	mModel->setHeaderLabels( QStringList() << "Key" << "Job Type" << "Service" );
	mModel->listen( JobType::table() );
	connect( mTreeView, SIGNAL( currentChanged( const Record & ) ), SLOT( slotCurrentChanged( const Record & ) ) );
	connect( mNewMethodButton, SIGNAL( clicked() ), SLOT( slotNewMethod() ) );
	connect( mRemoveMethodButton, SIGNAL( clicked() ), SLOT( slotRemoveMethod() ) );
	refresh();
}

void JobTypeEditDialog::refresh()
{
	mModel->setRootList( JobType::select() );
	slotCurrentChanged( mTreeView->current() );
}

void JobTypeEditDialog::slotNewMethod()
{
	JobType jt;
	mModel->append( jt );
}

void JobTypeEditDialog::slotRemoveMethod()
{
	mTreeView->current().remove();
}

void JobTypeEditDialog::slotCurrentChanged( const Record & r )
{
	mRemoveMethodButton->setEnabled( r.isValid() );
}

void JobTypeEditDialog::accept()
{
	QDialog::accept();
}

