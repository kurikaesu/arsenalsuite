
class ProjectCombo : RecordCombo
{
%TypeHeaderCode
#include <projectcombo.h>
%End
public:
	ProjectCombo( QWidget * parent /TransferThis/ );

	Project project() const;
	ProjectStatusList statusFilters() const;

	bool showingSpecialItem() const;
	void setSpecialItemText( const QString & );
	QString specialItemText() const;

signals:
	void projectChanged( const Project & p );

public slots:
	void setShowSpecialItem( bool );
	void setStatusFilters( ProjectStatusList );
	void setProject( const Project & );
	void refresh();
};


