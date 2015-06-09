
#include "database.h"
#include "tableschema.h"
#include "schema.h"
#include "recordimp.h"
#include "recordsupermodel.h"

#include "classes.h"
#include "project.h"
#include "user.h"
#include "usergroup.h"
#include "group.h"

#include "projectreserveview.h"

Table * projectReserveTable()
{
    static Table * ret = 0;
    if( !ret ) {
        TableSchema * schema = new TableSchema( classesSchema() );
        new Field( schema, "fkeyproject", "Project" );
        new Field( schema, "arsenalSlotReserve", Field::Double );
        new Field( schema, "currentAllocation", Field::Double );
        ret = schema->table();
    }
    return ret;
}

struct ProjectReserveItem : public RecordItemBase
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
                    return record.getValue("arsenalSlotReserve");
                case 2:
                    return record.getValue("currentAllocation");
            }
        }
        return QVariant();
    }
    bool setModelData ( const QModelIndex & idx, const QVariant & val,  int role = Qt::EditRole )
    {
        if( role == Qt::EditRole && idx.column() == 1 ) {
            project.setArsenalSlotReserve( val.toDouble() );
            project.commit();
            record.setValue( "arsenalSlotReserve", val );
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

typedef TemplateRecordDataTranslator<ProjectReserveItem> ProjectReserveTranslator;

ProjectReserveView::ProjectReserveView( QWidget * parent )
: RecordTreeView( parent )
{
    RecordSuperModel * model = new RecordSuperModel(this);
    new ProjectReserveTranslator(model->treeBuilder());
    model->setHeaderLabels( QStringList() << "Project" << "Reserve" << "Current Allocation" );
    setModel( model );
    model->sort(1);
}

void ProjectReserveView::refresh()
{
    RecordList records;
    QSqlQuery q = Database::current()->exec( "select project.keyelement, project_slots_current.arsenalslotreserve, sum from project_slots_current join project on project.name=project_slots_current.name;" );
    while( q.next() )
        records.append( Record( new RecordImp( projectReserveTable(), q ) ) );
    model()->setRootList( records );
}
