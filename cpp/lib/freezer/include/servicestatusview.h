
#ifndef SERVICE_STATUS_VIEW
#define SERVICE_STATUS_VIEW

#include <qwidget.h>

#include "service.h"
#include "license.h"

#include "recordtreeview.h"
#include "recordsupermodel.h"

#include "afcommon.h"

struct FREEZER_EXPORT ServiceStatusItem : public RecordItemBase
{
	Service service;
	License license;
	int totalHosts, readyHosts;
	void setup( const Record & r, const QModelIndex & );
	QVariant modelData( const QModelIndex & i, int role ) const;
	int compare( const QModelIndex & a, const QModelIndex & b, int, bool );
	Qt::ItemFlags modelFlags( const QModelIndex & );
	Record getRecord();
};

typedef TemplateRecordDataTranslator<ServiceStatusItem> ServiceStatusTranslator;

class FREEZER_EXPORT ServiceStatusList : public RecordTreeView
{
Q_OBJECT
public:
	ServiceStatusList(QWidget * parent=0);
	~ServiceStatusList();

public slots:
	void refresh();

protected:
	RecordSuperModel * mModel;
	ServiceStatusTranslator * mTrans;
};

class FREEZER_EXPORT ServiceStatusView : public QWidget
{
Q_OBJECT
public:
	ServiceStatusView(QWidget * parent=0,Qt::WindowFlags f = Qt::Dialog);
	~ServiceStatusView();

	ServiceStatusList * serviceStatusList() const { return mServiceStatusList; }

	QSize allContentsSizeHint();
protected:
	ServiceStatusList * mServiceStatusList;
};

#endif // SERVICE_STATUS_VIEW
