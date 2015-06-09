

#include "iniconfig.h"

#include "assfreezerview.h"
#include "viewmanager.h"

ViewManager * ViewManager::mInstance = 0;

ViewManager::ViewManager()
{}

ViewManager * ViewManager::instance()
{
	if( !mInstance ) mInstance = new ViewManager();
	return mInstance;
}

void ViewManager::readSavedViews( IniConfig & ini )
{
	int viewCount = ini.readInt( "Count" );
	for( int i=1; i<=viewCount; i++ ) {
		QString savedViewName = ini.readString( QString("ViewName%1").arg(i) );
		QString savedViewCode = ini.readString( QString("ViewCode%1").arg(i) );
		// Backward compat, need to give views a view code if they don't have one
		if( savedViewCode.isEmpty() ) {
			savedViewCode = FreezerView::generateViewCode();
			QString viewSectionName = "View_" + savedViewName;
			foreach( QString section, ini.sections() ) {
				// Section matches exactly or is a sub section
				if( section.startsWith( viewSectionName ) && (section.size() == viewSectionName.size() || section[viewSectionName.size()] == ':'))
					ini.copySection( section, "View_" + savedViewCode + section.mid(viewSectionName.size()) );
			}
		}
		if( !savedViewName.isEmpty() )
			mSavedViews << qMakePair<QString,QString>(savedViewName,savedViewCode);
	}
}
	
void ViewManager::writeSavedViews( IniConfig & ini )
{
	ini.writeInt( "Count", mSavedViews.size() );
	int i=1;
	foreach( StringPair p, mSavedViews ) {
		ini.writeString( QString("ViewName%1").arg(i), p.first );
		ini.writeString( QString("ViewCode%1").arg(i), p.second );
		i++;
	}
}

void ViewManager::addSavedView( const QString & viewName, const QString & viewCode )
{
	removeSavedViewByCode(viewCode);
	mSavedViews << qMakePair<QString,QString>(viewName,viewCode);
}

void ViewManager::removeSavedView( const QString & viewName )
{
	foreach( StringPair p, mSavedViews )
		if( p.first == viewName ) {
			mSavedViews.removeAll(p);
			return;
		}
}

void ViewManager::removeSavedViewByCode( const QString & viewCode )
{
	foreach( StringPair p, mSavedViews )
		if( p.second == viewCode ) {
			mSavedViews.removeAll(p);
			return;
		}
}

QString ViewManager::generateViewName( const QString & suggestion )
{
	QString viewName = suggestion;
	int viewNum = 1;
	while( savedViewNames().contains( viewName ) ) viewName = suggestion + "_" + QString::number(viewNum++);
	return viewName;
}

QList<ViewManager::StringPair> ViewManager::savedViews()
{
	return mSavedViews;
}

bool ViewManager::hasSavedView( const QString & viewCode )
{
	foreach( StringPair p, mSavedViews ) {
		if( p.second == viewCode )
			return true;
	}
	return false;
}

QStringList ViewManager::savedViewNames()
{
	QStringList ret;
	foreach( StringPair p, mSavedViews )
		ret.append(p.first);
	return ret;
}

