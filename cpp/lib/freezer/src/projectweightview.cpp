
#include <qtimer.h>

#include "database.h"
#include "tableschema.h"
#include "schema.h"
#include "recordimp.h"
#include "recordsupermodel.h"

#include "classes.h"
#include "group.h"
#include "project.h"
#include "projectweightschedule.h"
#include "user.h"
#include "usergroup.h"

#include "projectweightview.h"

Table * projectWeightTable()
{
	static Table * ret = 0;
	if( !ret ) {
		TableSchema * schema = new TableSchema( classesSchema() );
		new Field( schema, "fkeyproject", "Project" );
		new Field( schema, "assburnerWeight", Field::Double );
		new Field( schema, "currentAllocation", Field::Double );
		ret = schema->table();
	}
	return ret;
}

struct ProjectWeightItem : public RecordItemBase
{
	Record record;
	Project project;
	void setup( const Record & r, const QModelIndex & ) {
		record = r;
		project = record.foreignKey( "fkeyproject" );
	}
	QVariant modelData( const QModelIndex & idx, int role ) {
		int col = idx.column();
		if( role == Qt::DisplayRole || role == Qt::EditRole ) {
			switch( col ) {
				case 0:
					return project.name();
				case 1:
					return record.getValue("assburnerWeight");
				case 2:
					return record.getValue("currentAllocation");
			}
		}
		return QVariant();
	}
	bool setModelData ( const QModelIndex & idx, const QVariant & val,  int role = Qt::EditRole )
	{
		if( role == Qt::EditRole && idx.column() == 1 ) {
			project.setAssburnerWeight( val.toDouble() );
			project.commit();
			record.setValue( "assburnerWeight", val );
			ProjectWeightSchedule pws;
			pws.setProject( project );
			pws.setColumnLiteral( "dateTime", "NOW()" );
			pws.setWeight( val.toDouble() );
			pws.commit();
			return true;
		}
		return false;
	}
	Record getRecord() const { return record; }
    Qt::ItemFlags modelFlags( const QModelIndex & idx ) const {
		bool isAdministrator = false;

		User currentUser = User::currentUser();
		if (currentUser.userGroups().size() > 0) {
		UserGroupList groups = currentUser.userGroups();
		for (UserGroupIter it = groups.begin(); it != groups.end(); ++it) {
			if ((*it).group().name() == "RenderOps" || (*it).group().name() == "Admin") {
				isAdministrator = true;
				break;
			}
		}
	}
		Qt::ItemFlags ret( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
		if( idx.column() == 1 && isAdministrator )
			ret = Qt::ItemFlags( ret | Qt::ItemIsEditable );
		return ret;
	}
};

typedef TemplateRecordDataTranslator<ProjectWeightItem> ProjectWeightTranslator;

ProjectWeightView::ProjectWeightView( QWidget * parent )
: RecordTreeView( parent )
{
	RecordSuperModel * model = new RecordSuperModel(this);
	new ProjectWeightTranslator(model->treeBuilder());
	model->setHeaderLabels( QStringList() << "Project" << "Weight" << "Current Allocation" );
	setModel( model );
	model->sort(1);
	QTimer::singleShot( 0, this, SLOT( refresh() ) );
}

void ProjectWeightView::refresh()
{
	RecordList records;
	QSqlQuery q = Database::current()->exec( "SELECT project.keyelement, coalesce(assburnerweight,0), coalesce(tempo,0) FROM projecttempo RIGHT JOIN project ON projecttempo.fkeyproject=keyelement WHERE assburnerweight > 0 or fkeyprojectstatus=4 or tempo > 0" );
	while( q.next() )
		records.append( Record( new RecordImp( projectWeightTable(), q ) ) );
	model()->setRootList( records );
}
