
class ModelGrouper : QObject
{
%TypeHeaderCode
#include <modelgrouper.h>
%End
public:
	ModelGrouper( SuperModel * model );
	
	enum GroupItemRoles {
		GroupingColumn,
		GroupingValue,
		GroupingUpdate,
	};
	
	enum EmptyGroupPolicy {
		HideEmptyGroups,
		RemoveEmptyGroups,
		LeaveEmptyGroups
	};
	
	SuperModel * model() const;
	ModelDataTranslator * groupedItemTranslator() const;
	void setGroupedItemTranslator( ModelDataTranslator * trans /Transfer/ );
	
	bool isGrouped() const;
	int groupColumn() const;
	
	void groupByColumn( int column );
	void ungroup();

	void setColumnGroupRegex( int column, const QRegExp & = QRegExp() );
	QRegExp columnGroupRegex( int column ) const;
	
	QString groupValue( const QModelIndex & idx );

	EmptyGroupPolicy emptyGroupPolicy() const;
	void setEmptyGroupPolicy( EmptyGroupPolicy policy );
	
	void setExpandNewGroups(bool openNewGroups);
	bool expandNewGroups() const;
	
signals:
	void groupingChanged( bool grouped );
	void grouped();
	void ungrouped();
	
	void groupCreated( const QModelIndex & );
	void groupEmptied( const QModelIndex & );
	void groupPopulated( const QModelIndex & );
};
