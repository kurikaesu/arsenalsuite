
#ifndef WEB_VIEW_H
#define WEB_VIEW_H

#include "assfreezerview.h"

class QWebView;
class QNetworkReply;
class QAuthenticator;

class FREEZER_EXPORT WebView : public FreezerView
{
Q_OBJECT
public:
	WebView(QWidget * parent);
	~WebView();

	virtual QString viewType() const;
	
	virtual QToolBar * toolBar( QMainWindow * );
	virtual void populateViewMenu( QMenu * );
	
public slots:

	void applyOptions();
	void loadUrl();
	void authenticate( QNetworkReply * reply, QAuthenticator * authenticator  );
	
protected:
	void save( IniConfig & ini, bool );
	void restore( IniConfig & ini, bool );

	void populateViewSubMenu();

	QToolBar * mToolBar;
	QMenu * mViewSubMenu;

	QAction * mLoadUrlAction;
	
	QWebView * mWebView;
};

#endif // WEB_VIEW_H
