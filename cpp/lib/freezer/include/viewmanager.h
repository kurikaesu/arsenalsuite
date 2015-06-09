
#ifndef VIEW_MANAGER_H
#define VIEW_MANAGER_H

#include <qstringlist.h>

class IniConfig;
class FreezerView;

class ViewManager
{
public:
	ViewManager();

	static ViewManager * instance();

	void readSavedViews( IniConfig & );
	
	void writeSavedViews( IniConfig & );

	void addSavedView( const QString & viewName, const QString & viewCode );
	void removeSavedViewByCode( const QString & viewCode );
	void removeSavedView( const QString & viewName );

	QString generateViewName( const QString & suggestion );

	QList< QPair< QString, QString > > savedViews();

	QStringList savedViewNames();
	
	bool hasSavedView( const QString & viewCode );
	
protected:

	typedef QPair<QString,QString> StringPair;
	QList<StringPair> mSavedViews;

	static ViewManager * mInstance;
};


#endif // VIEW_MANAGER_H
