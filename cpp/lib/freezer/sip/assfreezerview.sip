
class FreezerView : QWidget
{
%TypeHeaderCode
#include <assfreezerview.h>
%End
public:
	FreezerView( QWidget * parent );
	~FreezerView();

	virtual QString viewType() const = 0;

	QString viewName() const;
	void setViewName( const QString & );

	IniConfig & viewConfig();
	
	void restorePopup( QWidget * );

	virtual QToolBar * toolBar( QMainWindow * );

	virtual void populateViewMenu( QMenu * );

	virtual void applyOptions();

	virtual void save( IniConfig & ini, bool );
	virtual void restore( IniConfig & ini, bool );

	QString statusBarMessage() const;
	void setStatusBarMessage( const QString & );
	void clearStatusBar();

	int refreshCount() const;

public slots:
	void refresh();

signals:
	void statusBarMessageChanged( const QString & );

};