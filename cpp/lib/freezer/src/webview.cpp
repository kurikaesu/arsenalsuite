
#include <qaction.h>
#include <qinputdialog.h>
#include <qlayout.h>
#include <qmainwindow.h>
#include <qmenu.h>
#include <qnetworkaccessmanager.h>
#include <QtWebKit>
#include <qtoolbar.h>
#include <qurl.h>

#include "webview.h"

WebView::WebView(QWidget * parent)
: FreezerView( parent )
, mToolBar( 0 )
, mViewSubMenu( 0 )
, mLoadUrlAction( 0 )
, mWebView( 0 )
{
	QVBoxLayout * layout = new QVBoxLayout(this);
	mWebView = new QWebView( this );
	layout->addWidget( mWebView );
	layout->setContentsMargins(0,0,0,0);
	connect( mWebView->page()->networkAccessManager(), SIGNAL(authenticationRequired(QNetworkReply *, QAuthenticator *)), SLOT(authenticate(QNetworkReply *, QAuthenticator *)) );
	
	mLoadUrlAction = new QAction( "Load Url", this );
	
	connect( mLoadUrlAction, SIGNAL( triggered(bool) ), SLOT( loadUrl() ) );
}

WebView::~WebView()
{
}

QString WebView::viewType() const
{
	return "WebView";
}

QToolBar * WebView::toolBar( QMainWindow * mainWindow )
{
	if( !mToolBar ) {
		mToolBar = new QToolBar(mainWindow);
		mToolBar->addAction( mWebView->pageAction( QWebPage::Reload ) );
	}
	return mToolBar;
	
}

void WebView::populateViewMenu( QMenu * viewMenu )
{
	viewMenu->addAction( mWebView->pageAction( QWebPage::Reload ) );
	viewMenu->addAction( mLoadUrlAction );
}

void WebView::applyOptions()
{
}

void WebView::loadUrl()
{
	QString url = QInputDialog::getText( this, "Entry Url", "Enter the url to load", QLineEdit::Normal, mWebView->url().toString() );
	mWebView->load( QUrl( url ) );
}

void WebView::authenticate( QNetworkReply *, QAuthenticator * authenticator )
{
	QString username = QInputDialog::getText( this, "Entry Username", "Enter the username to authenticate", QLineEdit::Normal, mWebView->url().toString() );
	QString password = QInputDialog::getText( this, "Entry Password", "Enter the password for " + username, QLineEdit::Password, mWebView->url().toString() );
	authenticator->setUser( username );
	authenticator->setPassword( password );
}

void WebView::save( IniConfig & ini, bool forceFullSave )
{
	ini.writeString( "Url", mWebView->url().toString() );
	FreezerView::save(ini,forceFullSave);
}

void WebView::restore( IniConfig & ini, bool forceFullRestore )
{
	QString url = ini.readString( "Url" );
	if( !url.isEmpty() )
		mWebView->load( QUrl(url) );
	FreezerView::restore(ini,forceFullRestore);
}
