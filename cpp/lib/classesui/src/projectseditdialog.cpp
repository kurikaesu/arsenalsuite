
#include <qdialog.h>

#include "recordtreeview.h"
#include "projectseditdialog.h"
#include "projectdialog.h"
#include "project.h"
#include "projectstatus.h"

class ProjectsEditItem : public RecordItemBase
{
public:
	Project project;
	ProjectsEditItem( const Project & p ) { setup(p); }
	ProjectsEditItem(){}
	void setup( const Record & r, const QModelIndex & = QModelIndex() ) {
		project = r;
	}
	QVariant modelData( const QModelIndex & i, int role ) {
		int col = i.column();
		switch( col ) {
			case 0:
			{
				if( role == Qt::DisplayRole )
					return project.key();
				break;
			}	
			case 1:
			{
				if( role == Qt::DisplayRole || role == Qt::EditRole )
					return project.name();
				else if( role == RecordDelegate::FieldNameRole )
					return "name";
				break;
			}
			case 2:
			{
				if( role == Qt::DisplayRole )
					return project.projectStatus().projectStatus();
				else if( role == Qt::EditRole ) {
					ProjectStatus none;
					none.setProjectStatus("None");
					ProjectStatusList psl = ProjectStatus::select();
					psl.insert(psl.begin(), none);
					return qVariantFromValue<RecordList>(psl);
				} else if( role == RecordDelegate::CurrentRecordRole )
					return qVariantFromValue<Record>(project.projectStatus());
				else if( role == RecordDelegate::FieldNameRole )
					return "status";
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
					project.setName(v.toString());
					break;
				}
				case 2:
				{
					ProjectStatus p = qvariant_cast<Record>(v);
					if (p.isValid())
						project.setProjectStatus(p);
					break;
				}
			}
			project.commit();
			return true;
		}
		return false;
	}
	Qt::ItemFlags modelFlags( const QModelIndex & index ) {
		if( index.column() > 1 )
			return Qt::ItemFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
		return Qt::ItemFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
	}
	Record getRecord() { return project; }
};

typedef TemplateRecordDataTranslator<ProjectsEditItem> ProjectsEditTranslator;

ProjectsEditDialog::ProjectsEditDialog( QWidget * parent )
: QDialog( parent )
{
	setupUi( this );
	mModel = new RecordSuperModel(mTreeView);
	new ProjectsEditTranslator(mModel->treeBuilder());
	mTreeView->setModel(mModel);
	mModel->setHeaderLabels( QStringList() << "Key" << "Name" << "Status");
	mModel->listen( Project::table() );
	connect( mTreeView, SIGNAL( currentChanged( const Record & ) ), SLOT( slotCurrentChanged( const Record & ) ) );
	connect( mNewMethodButton, SIGNAL( clicked() ), SLOT( slotNewMethod() ) );
	connect( mRemoveMethodButton, SIGNAL( clicked() ), SLOT( slotRemoveMethod() ) );
	refresh();
}

void ProjectsEditDialog::refresh()
{
	mModel->setRootList( Project::select() );
	slotCurrentChanged( mTreeView->current() );
}

void ProjectsEditDialog::slotNewMethod()
{
	ProjectDialog* pd = new ProjectDialog(this);
	pd->exec();
	delete pd;
}

void ProjectsEditDialog::slotRemoveMethod()
{
	mTreeView->current().remove();
}

void ProjectsEditDialog::slotCurrentChanged( const Record & r )
{
	mRemoveMethodButton->setEnabled( r.isValid() );
}

void ProjectsEditDialog::accept()
{
	QDialog::accept();
}

