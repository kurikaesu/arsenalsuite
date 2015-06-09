
#include <qlayout.h>
#include <qtimer.h>
#include <qsqlquery.h>

#include "database.h"
#include "servicestatusview.h"

static const ColumnStruct servicestatus_columns [] =
{
	{ "Service",			"Service",			120,	0,	false },	//0
	{ "Total Hosts",		"TotalHosts",		80,		1,	false }, 	//1
	{ "Ready Hosts",		"ReadyHosts",		90,		2,	false },	//2
	{ "Licenses", 			"Licenses",			65,		3,	false },	//3
	{ "Licenses In Use", 	"LicensesUsed", 	90,		4,	false }, 	//4
	{ 0, 					0, 					0, 		0, 	false }
};

void ServiceStatusItem::setup( const Record & r, const QModelIndex & )
{
	service = r;
	license = service.license();
}

QVariant ServiceStatusItem::modelData( const QModelIndex & i, int role ) const
{
	if( role == Qt::DisplayRole ) {
		switch( i.column() ) {
			case 0:
				return service.service();
			case 1:
				return totalHosts;
			case 2:
				return readyHosts;
			case 3:
				return QString::number(license.total() - license.reserved());
			case 4:
				return QString::number(license.inUse());
		}
	}
	return QVariant();
}

int ServiceStatusItem::compare( const QModelIndex & a, const QModelIndex & b, int col, bool asc )
{
	return RecordItemBase::compare(a,b,col,asc);
}

Qt::ItemFlags ServiceStatusItem::modelFlags( const QModelIndex & idx )
{
	return RecordItemBase::modelFlags( idx );
}

Record ServiceStatusItem::getRecord()
{
	return service;
}

ServiceStatusList::ServiceStatusList(QWidget * parent)
: RecordTreeView(parent)
, mModel( 0 )
, mTrans( 0 )
{
	mModel = new RecordSuperModel(this);
	mModel->sort(0);
	mModel->setAutoSort(true);
	mTrans = new ServiceStatusTranslator(mModel->treeBuilder());
	setModel(mModel);
	IniConfig & ini = userConfig();
	ini.pushSubSection("ServiceStatusList");
	setupTreeView(ini,servicestatus_columns);
	ini.popSection();

	//QTimer::singleShot( 0, this, SLOT(refresh()) );
}

ServiceStatusList::~ServiceStatusList()
{
}

void ServiceStatusList::refresh()
{
	QSqlQuery q = Database::current()->exec( "SELECT keyservice, count(*), sum((hoststatus.slavestatus='ready')::int) FROM hostservice "
											"INNER JOIN service ON fkeyservice=keyservice AND service.enabled=true "
											"INNER JOIN HostStatus ON hoststatus.fkeyhost=hostservice.fkeyhost AND HostStatus.slavestatus IN ('ready','assigned','copy','busy') "
											"WHERE hostservice.enabled=true GROUP BY keyservice, service" );

	ServiceList services;
	while( q.next() ) {
		Service service( q.value(0).toInt() );
		if( service.isRecord() ) services += service;
	}
	
	LicenseList licenses = License::table()->records( services.keys( Service::schema()->fieldPos( "fkeylicense" ) ), /*select=*/true, /*useCache=*/false );

	mModel->updateRecords( services );

	q.first();
	do {
		Service service( q.value(0).toInt() );
		if( !service.isRecord() ) continue;
		QModelIndex idx = mModel->findIndex( service );
		if( !idx.isValid() ) continue;
		ServiceStatusItem & item = ServiceStatusTranslator::data(idx);
		item.totalHosts = q.value(1).toInt();
		item.readyHosts = q.value(2).toInt();
	} while( q.next() );
}

ServiceStatusView::ServiceStatusView(QWidget * parent, Qt::WindowFlags f)
: QWidget( parent, f )
{
	mServiceStatusList = new ServiceStatusList(this);
	QVBoxLayout * layout = new QVBoxLayout(this);
	layout->setMargin(4);
	layout->addWidget(mServiceStatusList);
	mServiceStatusList->refresh();
}

ServiceStatusView::~ServiceStatusView()
{
}

QSize ServiceStatusView::allContentsSizeHint()
{
	QSize ret = mServiceStatusList->allContentsSizeHint();
	int l,r,t,b;
	layout()->getContentsMargins(&l,&t,&r,&b);
	ret += QSize( l + r, t + b + 4 );
	LOG_5( QString( "allContentsSizeHint, Width: %1, Height: %2" ).arg(ret.width()).arg(ret.height()) );
	return ret;
}
