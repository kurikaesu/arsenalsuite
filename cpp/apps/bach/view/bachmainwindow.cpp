
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Bach.
 *
 * Bach is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Bach is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Bach; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: bachmainwindow.cpp 9408 2010-03-03 22:35:49Z brobison $
 */

#include <qaction.h>
#include <qcombobox.h>
#include <qimage.h>
#include <qlabel.h>
#include <qmessagebox.h>
#include <qmenu.h>
#include <qpushbutton.h>
#include <qprintdialog.h>
#include <qprinter.h>
#include <qregexp.h>
#include <qsplitter.h>
#include <qstatusbar.h>
#include <qtabbar.h>
#include <qtabwidget.h>
#include <qmenubar.h>
#include <qtoolbar.h>
#include <qurl.h>
#include <qfontdialog.h>
#include <qscrollbar.h>
#include <qclipboard.h>
#include <qprocess.h>
#include <qprogressdialog.h>
#include <qpixmapcache.h>
#include <qinputdialog.h>
#include <qfile.h>
#include <qsize.h>
#include <qfuture.h>
#include <qfuturewatcher.h>
#include <algorithm>

#include "blurqt.h"
#include "database.h"
#include "iniconfig.h"
#include "process.h"

#include "bachasset.h"

#include "svnrev.h"
#include "bachmainwindow.h"
#include "bachitems.h"
#include "bachbucket.h"
#include "bachbucketmap.h"
#include "bachbucketmaplist.h"
#include "bachkeyword.h"
#include "bachkeywordmap.h"
#include "bachkeywordmaplist.h"
#include "bachthumbnailloader.h"
#include "BachDirModel.h"
#include "utils.h"

static QString sCacheRoot;
static QSize sTnSize;

BachMainWindow::BachMainWindow( QSplashScreen * splash, QWidget * parent )
: QMainWindow( parent )
,mFolderBrowserMenu( NULL )
,mCollectionViewMenu( NULL )
,mShowExcluded(false)
,mResults(0)
,mResultsShown(0)
,mMissing(0)
,mSearchInProgress(false)
,mSearchUpdateInProgress(false)
,mSearchCanceled(false)
,mTnSizeIsChanging(false)
{
	setupUi( this );
	setWindowIcon( QIcon("images/bachicon.jpg") );
	setWindowTitle( QString("Bach 1.2") + ", build "+ SVN_REVSTR );

//    m_Layout = new QVBoxLayout( searchBoxContainer );
//    m_Layout->setSpacing(2);
//    m_Layout->setMargin(2);
//    m_Layout->setObjectName( QString::fromUtf8("Layout"));
	mSearchBox = new BachSearchBox( searchBoxContainer );
	verticalLayout_7->addWidget( mSearchBox );

    loadState();

    // db preload
    splash->showMessage( "Loading database tables" );
    BachKeyword::select();
    BachKeywordMap::select();
	BachBucket::select();
	BachBucketMap::select();

	 // actions
    splash->showMessage( "Creating application actions" );
	mFontAction = new QAction( "Change &Font...", this );
	connect( mFontAction, SIGNAL( triggered() ), SLOT(changeFont()) );

	mQuitAction = new QAction( "&Quit", this );
	connect( mQuitAction, SIGNAL( triggered() ), qApp, SLOT(quit()) );

	mOpenAction = new QAction( "Open", this );
	connect( mOpenAction, SIGNAL( triggered() ), SLOT(openFiles()) );
	mShowAction = new QAction( "Show folder", this );
	connect( mShowAction, SIGNAL( triggered() ), SLOT(showFiles()) );
	mCopyPathAction = new QAction( "Copy path", this );
	connect( mCopyPathAction, SIGNAL( triggered() ), SLOT(copyPaths()) );
	mCopyImageAction = new QAction( "Copy image", this );
	connect( mCopyImageAction, SIGNAL( triggered() ), SLOT(copyImage()) );

	mRemoveFromBucketAction = new QAction( "Remove from collection", this );
	connect( mRemoveFromBucketAction, SIGNAL( triggered() ), SLOT(removeFromBucket()) );

	mRotateCWAction = new QAction( "Rotate Thumbnail CW", this );
	connect( mRotateCWAction, SIGNAL( triggered() ), SLOT(rotateTnCW()) );
	mRotateCCWAction = new QAction( "Rotate Thumbnail CCW", this );
	connect( mRotateCCWAction, SIGNAL( triggered() ), SLOT(rotateTnCCW()) );

	mEditAction = new QAction( "Edit Tags", this );
	connect( mEditAction, SIGNAL( triggered() ), SLOT(editTags()) );

	mDblClickToPlayAction = new QAction( "Double click to play movies", this );
	mDblClickToPlayAction->setCheckable(true);
	mDblClickToPlayAction->setChecked(true);

	mDblClickToEditAction = new QAction( "Double click to edit tags", this );
	mDblClickToEditAction->setCheckable(true);
	mDblClickToEditAction->setChecked(false);

    connect( mExcludedBox, SIGNAL( toggled(bool) ), SLOT( toggleExcluded(bool) ) );
	mExcludeAction = new QAction( "Exclude Asset", this );
	connect( mExcludeAction, SIGNAL( triggered() ), SLOT(toggleExcludeAsset()) );

    connect( mFolderShowTotalsCB, SIGNAL( toggled(bool) ), SLOT( toggleFolderShowTotals(bool) ) );

    connect( mKeywordsShowAllCB, SIGNAL( toggled(bool) ), SLOT( OnKeywordsShowAllSBToggled(bool) ) );

	// folder browser stuff
#define NEW_AND_CONNECT(action,name,slot) action=new QAction(name,this); connect(action,SIGNAL(triggered()),SLOT(slot));
	NEW_AND_CONNECT( mFolderBrowserOpenAction, "Open Folder", onFolderBrowserOpenAction() );
	NEW_AND_CONNECT( mFolderBrowserAddAction, "Add Folder to Bach", onFolderBrowserAddAction() );

	NEW_AND_CONNECT( mCollectionViewWebAction, "View web version", onCollectionViewWebAction() );
#undef NEW_AND_CONNECT

	populateMenus();

    splash->showMessage( "Creating thumbnail views" );
	initDisplayOptions();
	{
		mThumbnailModel = new RecordSuperModel( mThumbDetailsView );
		new BachAssetTranslator( mThumbnailModel->treeBuilder() );
		mThumbnailModel->setAutoSort(false);

		mThumbDetailsView->setModel( mThumbnailModel );
		WrapTextDelegate * wtd = new WrapTextDelegate;
		mThumbDetailsView->setItemDelegate( wtd );
		setupAssetTree( mThumbDetailsView );

		mThumbsView->setModel( mThumbnailModel );
		mThumbsView->setViewMode( QListView::IconMode );
		ThumbDelegate * thumbGate = new ThumbDelegate;
		mThumbsView->setItemDelegate( thumbGate );
		mThumbsView->setModelColumn(1);

		mKeywordsListView->loadState();
		mCollectionView->loadState();
		mFoldersView->loadState();
	}

	connect( mThumbDetailsView, SIGNAL( selectionChanged(RecordList) ), SLOT( selectionChanged() ) );
	connect( mThumbsView, SIGNAL( selectionChanged(RecordList) ), SLOT( selectionChanged() ) );
	connect( mCollectionView, SIGNAL( selectionChanged(RecordList) ), SLOT( collectionViewSelectionChanged() ) );

	connect( mThumbDetailsView, SIGNAL( doubleClicked(const QModelIndex &) ), SLOT(treeEditItem(const QModelIndex &)));
	connect( mThumbsView, SIGNAL( doubleClicked(const QModelIndex &) ), SLOT(treeEditItem(const QModelIndex &)));
	connect( mCollectionView, SIGNAL( doubleClicked(const QModelIndex &) ), SLOT(collectionViewDoubleClicked()));
	connect( mFoldersView, SIGNAL( doubleClicked(const QModelIndex &) ), SLOT(foldersViewDoubleClicked(const QModelIndex &)));
	connect( mKeywordsListView, SIGNAL( doubleClicked(const QModelIndex &) ), SLOT(keywordViewDoubleClicked()));

	connect( mRightTabWidget, SIGNAL( currentChanged(int) ), SLOT( tabChanged(int) ) );

	connect( mCollectionAddBtn,     SIGNAL( clicked(bool) ), SLOT( collectionAddBtnPressed(bool) ) );
	connect( mCollectionNewBtn,     SIGNAL( clicked(bool) ), SLOT( collectionNewBtnPressed(bool) ) );
	connect( mCollectionDelBtn,     SIGNAL( clicked(bool) ), SLOT( collectionDelBtnPressed(bool) ) );
	connect( mCollectionRefreshBtn, SIGNAL( clicked(bool) ), SLOT( collectionRefreshBtnPressed(bool) ) );
	connect( mCollectionRenameBtn,  SIGNAL( clicked(bool) ), SLOT( collectionRenameBtnPressed(bool) ) );

	connect( mKeywordNewBtn,     	SIGNAL( clicked(bool) ), SLOT( keywordNewBtnPressed(bool) ) );
	connect( mKeywordRenameBtn,    	SIGNAL( clicked(bool) ), SLOT( keywordRenameBtnPressed(bool) ) );
	connect( mKeywordDelBtn,    	SIGNAL( clicked(bool) ), SLOT( keywordDelBtnPressed(bool) ) );
	connect( mKeywordRefreshBtn,    SIGNAL( clicked(bool) ), SLOT( keywordRefreshBtnPressed(bool) ) );

	connect( mClearSearchBtn,    	SIGNAL( clicked(bool) ), SLOT( onClearSearchBtnClicked(bool) ) );

	connect( mKeywordFilter, SIGNAL( textChanged( const QString & ) ), SLOT( onKeywordFilterTextChanged( const QString & ) ) );
	connect( mBucketFilter, SIGNAL( textChanged( const QString & ) ), SLOT( onBucketFilterTextChanged( const QString & ) ) );

	searchBoxContainer->setVisible( false );
	mSearchBox->SetSearchStringCallback( this );

    splash->showMessage( "Creating collection view" );
	mCollectionView->refresh();

    splash->showMessage( "Creating keyword view" );
    BachKeywordView::LoadPixmaps();
    mKeywordsListView->refresh();

    QPixmapCache::setCacheLimit( 1024 * 2000 ); // 2 GB
    sCacheRoot = _cacheRoot();

    splash->showMessage( "Creating tag cloud" );
    initSearchEdit();
}

BachMainWindow::~BachMainWindow()
{
    closeEvent(new QCloseEvent());
	delete mFontAction;
	delete mQuitAction;
	delete mOpenAction;
	delete mShowAction;
	delete mCopyPathAction;
	delete mCopyImageAction;
	delete mRemoveFromBucketAction;
	delete mEditAction;
	delete mDblClickToPlayAction;
	delete mDblClickToEditAction;
	delete mThumbnailModel;
}

void BachMainWindow::loadState()
{
	IniConfig & config = userConfig();
	config.pushSection("BachMainWindow");

    QString mss = config.readString( "MainSplitterPos", "324,600,300" );
    QList<int> sizes;
    sizes += mss.section(',',0,0).toInt();
    sizes += mss.section(',',1,1).toInt();
    sizes += mss.section(',',2,2).toInt();
    splitter->setSizes( sizes );

    QString rss = config.readString( "RightSplitterPos", "600,300" );
    QList<int> sizes_2;
    sizes_2 += rss.section(',',0,0).toInt();
    sizes_2 += rss.section(',',1,1).toInt();
    splitter_2->setSizes( sizes_2 );

	qApp->setFont( config.readFont("AppFont") );

	config.pushSection( "SearchHistory" );
	int numberOfSavedSearches = config.readInt( "numberOfSavedSearches", 0 );
	QStringList history;
	for ( int idx = 0 ; idx < numberOfSavedSearches ; ++idx )
	{
		history << config.readString( "search_"+QString::number( idx ), "" );
	}
	mSearchHistory->addItems( history );
	config.popSection();

    config.popSection();
}

void BachMainWindow::closeEvent( QCloseEvent * ce )
{
    LOG_3("closeEvent() saving app state");
	saveAssetTree(mThumbDetailsView);
	mKeywordsListView->saveState();
	mCollectionView->saveState();
	mFoldersView->saveState();

	IniConfig & config = userConfig();
	config.pushSection("BachMainWindow");
	config.writeString( "FrameGeometry",
	  QString("%1,%2,%3,%4").arg( pos().x() ).arg( pos().y() ).arg( size().width() ).arg( size().height() )
	);

    //LOG_3("AppFont: " + QVariant(qApp->font()).toString() );
	config.writeFont( "AppFont", qApp->font() );

    QList<int> sizes = splitter->sizes();
    if( sizes.size() >= 3 )
        config.writeString( "MainSplitterPos", QString("%1,%2,%3").arg(sizes[0]).arg(sizes[1]).arg(sizes[2]) );

    QList<int> sizes_2 = splitter_2->sizes();
    if( sizes_2.size() >= 2 )
        config.writeString( "RightSplitterPos", QString("%1,%2").arg(sizes_2[0]).arg(sizes_2[1]) );

	config.pushSection( "SearchHistory" );
	config.writeInt( "numberOfSavedSearches", mSearchHistory->count() );
	for ( int idx = 0 ; idx < mSearchHistory->count() ; ++idx )
	{
		config.writeString( "search_"+QString::number( idx ), mSearchHistory->itemText( idx ) );
	}
	config.popSection();

	config.popSection();

	QMainWindow::closeEvent(ce);
}

void BachMainWindow::changeFont() {
	qApp->setFont( QFontDialog::getFont(0, qApp->font()) );
}

void BachMainWindow::populateMenus()
{
	QMenuBar * mb = menuBar();
	mb->clear();
	mFileMenu = mb->addMenu( "&File" );
	mFileMenu->addAction( mQuitAction );

	mViewMenu = mb->addMenu( "&View" );
	mFileMenu->addAction( mFontAction );
	mViewMenu->addAction( mDblClickToPlayAction );
	mViewMenu->addAction( mDblClickToEditAction );

	mTreeMenu = new QMenu;
	populateTreeMenu();
	connect( mThumbDetailsView, SIGNAL( customContextMenuRequested(const QPoint &) ), SLOT(showTreeMenu( const QPoint & )));
	connect( mThumbsView, SIGNAL( customContextMenuRequested(const QPoint &) ), SLOT(showTreeMenu( const QPoint & )));

	mSearchTimer = new QTimer(this);
	connect( mSearchTimer, SIGNAL( timeout() ), SLOT( updateSearchResults() ) );

	mFolderBrowserMenu = new QMenu();
	mFolderBrowserMenu->clear();
	mFolderBrowserMenu->addAction( mFolderBrowserOpenAction );
	mFolderBrowserMenu->addAction( mFolderBrowserAddAction );

	mCollectionViewMenu = new QMenu();
	mCollectionViewMenu->clear();
	mCollectionViewMenu->addAction( mCollectionViewWebAction );

	connect( mFoldersView, SIGNAL( customContextMenuRequested(const QPoint &) ), SLOT(showFolderViewMenu( const QPoint & )));
	connect( mCollectionView, SIGNAL( customContextMenuRequested(const QPoint &) ), SLOT(showCollectionViewMenu( const QPoint & )));
}

void BachMainWindow::populateTreeMenu()
{
	mTreeMenu->clear();
 	mTreeMenu->addAction(mOpenAction);
 	mTreeMenu->addAction(mEditAction);
 	mTreeMenu->addAction(mShowAction);
 	mTreeMenu->addSeparator();
 	mTreeMenu->addAction(mCopyPathAction);
 	mTreeMenu->addAction(mCopyImageAction);
 	mTreeMenu->addSeparator();
 	mTreeMenu->addAction(mRotateCWAction);
 	mTreeMenu->addAction(mRotateCCWAction);
 	mTreeMenu->addSeparator();
 	mTreeMenu->addAction(mExcludeAction);
 	mTreeMenu->addAction(mRemoveFromBucketAction);
}

void BachMainWindow::showTreeMenu( const QPoint & )
{
	QAction * result = mTreeMenu->exec(QCursor::pos());
	if( !result )
		return;
}

void BachMainWindow::showFolderViewMenu( const QPoint & /*a_Point*/ )
{
	QAction * result = mFolderBrowserMenu->exec(QCursor::pos());
	if( !result )
		return;
}

BachAssetList BachMainWindow::selection() const
{
	if( mRightTabWidget->currentIndex() == 0 )
		return mThumbDetailsView->selection();
	else if( mRightTabWidget->currentIndex() == 1 )
		return mThumbsView->selection();
	return BachAssetList();
}

void BachMainWindow::openFiles() const
{
	BachAssetList selected = selection();

	foreach( BachAsset ba, selected ) {
#ifdef Q_OS_LINUX
		QProcess::startDetached("kfmclient", QStringList("exec") << ba.path());
#endif
#ifdef Q_OS_MAC
		QProcess::startDetached("open", QStringList() << ba.path());
#endif
#ifdef Q_OS_WIN
		QString path = Stone::Path("T:"+ba.path()).path();
		openURL( path );
#endif
	}
}

void BachMainWindow::showFiles() const
{
	BachAssetList selected = selection();
	foreach( BachAsset ba, selected ) {
#ifdef Q_OS_LINUX
		QProcess::startDetached("konqueror", QStringList() << "--select" << ba.path());
#endif
#ifdef Q_OS_MAC
        QString cmd("/usr/bin/osascript /drd/software/int/apps/bach/osx/showandtell.osa \"%1\" \"%2\"");
        LOG_3(cmd.arg(QFileInfo( ba.path() ).absoluteDir().absolutePath()).arg(ba.path()));
		QProcess::startDetached(cmd.arg(QFileInfo( ba.path() ).absoluteDir().absolutePath()).arg(ba.path()));
#endif
#ifdef Q_OS_WIN
		QString path = Stone::Path("T:"+ba.path()).dirPath();
		openExplorer( path );
#endif
	}
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::copyPaths()
{
	QStringList paths;
	BachAssetList selected = selection();
	foreach( BachAsset ba, selected ) {
#ifdef Q_OS_WIN
		QString path = Path("T:"+ba.path()).path();
		paths << path;
#else
		paths << ba.path();
#endif
	}
	QClipboard * cb = QApplication::clipboard();
	cb->setText( paths.join("\n") );
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::copyImage()
{
	QStringList paths;
	BachAssetList selected = selection();
	if( selected.size() == 0 )
		return;

#ifdef Q_OS_WIN
	QString path = Path("T:"+selected[0].path()).path();
	QImage img = ThumbnailLoader::loadFromDisk( path );
#else
	QImage img = ThumbnailLoader::loadFromDisk( selected[0].path() );
#endif
	QClipboard * cb = QApplication::clipboard();
	cb->setImage( img );
}


//-------------------------------------------------------------------------------------------------
void BachMainWindow::removeFromBucket()
{
	BachBucketList bbl = mCollectionView->selection();
	if ( bbl.count() == 0 )
		return;

	BachAssetList bal = selection();
	if( bal.count() == 0 )
		return;

	int removed = 0;
	foreach( BachAsset ba, bal )
	{
		BachBucketMap bbm = BachBucketMap::recordByBucketAndAsset( bbl[0], ba );
		removed += bbm.remove()==1?1:0;
	}

	mCollectionView->refresh();
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::refreshView()
{
	int currentSlidePos = mThumbDetailsView->verticalScrollBar()->sliderPosition();

 //	mThumbDetailsView->refresh();

	mThumbDetailsView->verticalScrollBar()->setSliderPosition(currentSlidePos);
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::updateSearchResults()
{
	if( mSearchCanceled ) {
		searchComplete();
		return;
	}
	if( mSearchUpdateInProgress )
		return;
	mSearchUpdateInProgress = true;

	//QTime updateTime = QTime::currentTime();

	int toUpdate = qMin((uint)75, mFound.size()-mResultsShown);
	int results = 0;
	BachAssetList toShow;
	while( mUpdateIter != mFound.end() && results++ <= toUpdate ) {
		toShow += *mUpdateIter;
		++mUpdateIter;
	}
	mThumbnailModel->append( toShow );
	//LOG_3("msecs to update:"+QString::number(updateTime.msecsTo(QTime::currentTime())));

	mResultsShown += toUpdate;
	if( mUpdateIter == mFound.end() )
		searchComplete();
	mSearchUpdateInProgress = false;
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::searchButtonPressed()
{
	if( mSearchInProgress )
		cancelSearch();
	else
		doTagSearch();
}

//-------------------------------------------------------------------------------------------------
QString getArg( const QString & a_Var, const QString & a_Name )
{
	return a_Var.mid( a_Name.size() );
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::doTagSearch()
{
	QString searchText = mSearchHistory->currentText();
	if ( searchText.isEmpty() )
		return;

	if ( !doPreSearch() )
		return;
	//QString searchText = mSearchEdit->text();
	int idx = -1;
	while( ( idx = mSearchHistory->findText( searchText ) ) != -1 )
	{
		mSearchHistory->removeItem( idx );
	}
	mSearchHistory->insertItem( 0, searchText );
	mSearchHistory->setCurrentIndex( 0 );


	QStringList tokens;
	SmartTokenise( searchText, tokens );
	/*
	DBG( ">>>>>>>>>:"+searchText );
	for( int idx = 0 ; idx < tokens.size() ; ++idx )
		DBG( tokens[ idx ] );
	DBG( "<<<<<<<<<" );
	*/

	QStringList queries;
	VarList args;

	for( int idx = 0 ; idx < tokens.size() ; ++idx )
	{
		QString token = tokens[ idx ];
		if ( token.startsWith( "file:" ) )
		{
			QString arg = getArg( token, "file:" );
			queries << QString( "lower(path) like '%"+arg.toLower()+"%'" );
		}
		else if ( token.startsWith( "proj:" ) )
		{
			QString arg = getArg( token, "proj:" );
			queries << QString( "lower(path) like '/drd/jobs/"+arg.toLower()+"%'" );
		}
		else if ( token.startsWith( "exif:" ) )
		{
			QString arg = getArg( token, "exif:" );
			queries << QString( "exif like '%"+arg+"%'" );
		}
		else if ( token.startsWith( "keyword:" ) )
		{
			QString arg = getArg( token, "keyword:" );
			queries << QString( "lower(cachedkeywords) like '%"+arg.toLower()+"%'" );
		}
		else if ( token.startsWith( "haskeyword:" ) )
		{
			bool noKeyword = token.startsWith( "haskeyword:no" );
			if ( noKeyword )
				queries << QString( "(cachedkeywords='' or cachedkeywords is null)" );
		}
		else if ( token.startsWith( "from:" ) )
		{
			QString arg = getArg( token, "from:" );
			queries << QString( "creationdatetime > '"+arg+"'" );
		}
		else if ( token.startsWith( "to:" ) )
		{
			QString arg = getArg( token, "to:" );
			queries << QString( "creationdatetime < '"+arg+"'" );
		}
		else if ( token.startsWith( "from_import:" ) )
		{
			QString arg = getArg( token, "from_import:" );
			queries << QString( "importeddatetime > '"+arg+"'" );
		}
		else if ( token.startsWith( "to_import:" ) )
		{
			QString arg = getArg( token, "to_import:" );
			queries << QString( "importeddatetime < '"+arg+"'" );
		}
		else if ( token.startsWith( "width" ) )
		{
			queries << token;
		}
		else if ( token.startsWith( "height" ) )
		{
			queries << token;
		}
		else if ( token.startsWith( "iso" ) )
		{
			queries << token;
		}
		else if ( token.startsWith( "aperture" ) )
		{
			queries << token;
		}
		else if ( token.startsWith( "lens" ) )
		{
			queries << token;
		}
		else if ( token.startsWith( "shutter" ) )
		{
			queries << token;
		}

		// THE else
		else
		{
		    queries << "fti_tags @@ to_tsquery(?)";
		    args << token.replace(" ","&");
		}
	}
    QStringList fileTypes;
    if( mDisplayOptions->mSequenceCheck->isChecked() )
        fileTypes << "1";
    if( mDisplayOptions->mImageCheck->isChecked() )
        fileTypes << "2";
    if( mDisplayOptions->mMovieCheck->isChecked() )
        fileTypes << "3";
    queries << "filetype IN("+fileTypes.join(",")+")";

    if( !mShowExcluded )
    	queries << "exclude=false";


    QString query = queries.join( " AND " );

    QVariant limit = mLimitCB->itemData( mLimitCB->currentIndex() );
    if ( limit.isValid() && limit.toInt() != -1 )
    {
		query.append( " LIMIT " + limit.toString() );
    }

	PRNT( "Query: ["+query+"]" );

	mFound = BachAsset::select( query, args );
	doPostSearch();
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::doBucketSearch( const BachBucket & bb )
{
	if ( !doPreSearch() )
		return;

	mFound = BachAssetList();
	GetAssets( bb, mShowExcluded, mFound );

	mThumbsView->setShowingCollection( true );
	mThumbsView->setCollection( bb, mShowExcluded );

	doPostSearch();
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::doKeywordSearch( const BachKeyword & bk )
{
	if ( !doPreSearch() )
		return;

	mFound = bk.bachKeywordMaps().bachAssets().filter("exclude", QVariant(mShowExcluded));
	doPostSearch();
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::doDirectorySearch( const QString & a_Directory )
{
	if ( !doPreSearch() )
		return;

	mFound = BachAsset::select( "directory=(?)", VarList() << a_Directory );
	doPostSearch();
}

//-------------------------------------------------------------------------------------------------
bool BachMainWindow::doPreSearch()
{
	if( mSearchInProgress )
		return false;

	mSearchInProgress = true;
	mSearchButton->setText("Cancel");
	statusBar()->showMessage("Searching... ");
	qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
	mSearchTime = QTime::currentTime();
	mThumbsView->setShowingCollection( false );

	return true;
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::doPostSearch()
{
	BachAssetList tmp;
	int position = 0;
	foreach ( BachAsset ba, mFound )
	{
		ba.setPosition( position++ );
		tmp.append( ba );
	}
	mFound = tmp;
	mUpdateIter = BachAssetIter(mFound);
	mResults = mFound.size();
	LOG_3("msecs to load from db:"+QString::number(mSearchTime.msecsTo(QTime::currentTime())));
	LOG_3("Searching... "+QString::number(mResults)+" records found");
	mSearchTime = QTime::currentTime();

	mResultsShown = 0;
	mThumbnailModel->updateRecords( BachAssetList() );
	statusBar()->showMessage("Searching... "+QString::number(mResults)+" records found");
	mSearchTimer->start(50);
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::cancelSearch()
{
	mSearchCanceled = true;
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::searchComplete()
{
	mSearchTimer->stop();
	qApp->restoreOverrideCursor();

	int msecs = mSearchTime.msecsTo(QTime::currentTime());
	LOG_3("msecs to update views:"+QString::number(msecs));
	statusBar()->showMessage("Showing "+QString::number(mResultsShown)+" out of "+QString::number(mResults)+" valid records found. Search took "+QString::number(msecs)+" msecs");
	mSearchButton->setText("Search");
	mSearchInProgress = false;
	mSearchCanceled = false;
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::selectionChanged()
{
	BachAssetList selected = selection();
	statusBar()->showMessage(QString::number(selected.size())+" of "+QString::number(mResults) +" records selected");

	mKeywordsListView->setSelectedAssets( selected );
	mKeywordsListView->refresh();
    populatePropertyTree();
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::collectionViewSelectionChanged()
{
	BachBucketList selected = mCollectionView->selection();
    /*
	DBG( "BachAssets" );
	for ( BachBucketIter it = selected.begin() ; it != selected.end() ; ++it )
	{
		const BachBucket & bb = *it;
		DBG( "\tBachAsset: " + bb.name() );
	}
    */
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::collectionViewDoubleClicked()
{
	// borg borg borg
	BachBucketList bbl = mCollectionView->selection();
	if ( bbl.count() == 0 )
	{
		return;
	}
	BachBucket bb = *bbl.at( 0 );
	doBucketSearch( bb );
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::keywordViewDoubleClicked()
{
	// borg borg borg
	BachKeywordList bkl = mKeywordsListView->selection();
	if ( bkl.count() == 0 )
	{
		return;
	}
	BachKeyword bk = *bkl.at( 0 );
	doKeywordSearch( bk );
}


//-------------------------------------------------------------------------------------------------
void BachMainWindow::foldersViewDoubleClicked( const QModelIndex & a_Idx )
{
	QString filePath = mFoldersView->getDirModel()->filePath( a_Idx );
	if ( !filePath.endsWith( '/' ) )
		filePath.append( '/' );
	doDirectorySearch( filePath );
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::treeEditItem(const QModelIndex & index)
{
	if( (index.column() == 2 && mDblClickToEditAction->isChecked()) ) { // ||
			//(index.column() == 1 && mDblClickToPlayAction->isChecked()) ) {
        qWarning( "=== enable editing for row " + QString::number(index.row()).toAscii() + ", col " + QString::number(index.column()).toAscii() );
		mThumbDetailsView->setCurrentIndex(index);
		mThumbDetailsView->edit(index);
	} else
		openFiles();
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::tabChanged(int tab)
{
	//if( tab == 1 )
	//	mThumbsView->setGridSize(mThumbsView->gridSize());
    if( tab == 0 ) {
        // lock the thumbnail size
        disconnect( mDisplayOptions->mThumbCountSlider, SIGNAL(valueChanged(int)) );
        disconnect( splitter, SIGNAL(splitterMoved(int,int)) );
        sTnSize = QSize(256,256);
        mThumbDetailsView->setIconSize(sTnSize);

        // maintain selection
        QItemSelectionModel * selectModel = mThumbsView->selectionModel();
        selectModel->select( selectModel->selection(), QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows );
        mThumbDetailsView->setSelectionModel( selectModel );

        if( selectModel->selectedIndexes().size() > 0 )
            mThumbDetailsView->scrollTo( selectModel->selectedIndexes()[0] );
    } else if ( tab == 1 ) {
        _calculateTnSize();
        connect( mDisplayOptions->mThumbCountSlider, SIGNAL(valueChanged(int)), SLOT(calculateTnSize(int)));
        connect( splitter, SIGNAL(splitterMoved(int,int)), SLOT(calculateTnSize(int)));

        // maintain selection
        QItemSelectionModel * selectModel = mThumbDetailsView->selectionModel();
        mThumbsView->setSelectionModel( selectModel );

        if( selectModel->selectedRows(1).size() > 0 )
            mThumbsView->scrollTo( selectModel->selectedRows(1)[0] );
    }
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::editTags()
{
	BachAssetList selected = selection();
	QString tags;
	if( selected.size() > 0 )
		tags = selected[0].tags();

	bool ok;
	QString text = QInputDialog::getText(this, tr("Edit Tags"),
										tr("Tags:"),
										QLineEdit::Normal,
										tags,
										&ok);
	if (ok && !text.isEmpty()) {
		selected.setTags(text);
		selected.commit();
	}
}

//-------------------------------------------------------------------------------------------------
QString BachMainWindow::cacheRoot()
{
	return sCacheRoot;
}

//-------------------------------------------------------------------------------------------------
QString BachMainWindow::_cacheRoot()
{
#ifdef Q_OS_WIN
	QString temp( "T:/reference/.thumbnails/" );
#else
	QString temp( "/drd/reference/.thumbnails/" );
#endif // Q_OS_WIN

	QStringList environment =  QProcess::systemEnvironment();
	foreach( QString key, environment )
		if( key.startsWith("BACHCACHEPATH=") )
			temp = key.replace("BACHCACHEPATH=","");

	// Make sure the thumbnail cache directory exists
	if( !QDir().mkpath(temp) ) {
		LOG_3( "Couldn't create thumbnailcache directory: " + temp );
	}
	return temp;
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::collapseToSequence()
{
	BachAssetList selected = selection();
}

//-------------------------------------------------------------------------------------------------
void
BachMainWindow::collectionAddBtnPressed(bool)
{
	BachBucketList bbl = mCollectionView->selection();
	if ( bbl.count() == 0 )
	{
		return;
	}

	BachBucket bb = *bbl.at( 0 );
	BachAssetList selected = selection();
	BachBucketMapList bbml = bb.bachBucketMaps();

	BachBucketMapList toAdd;

	for ( RecordIter it = selected.begin() ; it != selected.end() ; ++it )
	{
		BachAsset ba = *it;

		BachBucketMap bbm;
		bbm.setBachBucket( bb );
		bbm.setBachAsset( ba );
		toAdd.append( bbm );
	}

	bbml |= toAdd;
	bbml.commit();

	mCollectionView->refresh();
}

//-------------------------------------------------------------------------------------------------
void
BachMainWindow::collectionNewBtnPressed(bool)
{
	QString name;
	bool ok = false;
	QString text = QInputDialog::getText(this, tr("New Collection"),
										tr("Name:"),
										QLineEdit::Normal,
										name,
										&ok);
	if ( ok && !text.isEmpty() )
	{
		BachBucket bb;
		bb.setName( text );
		bb.commit();
		mCollectionView->refresh();
	}
}

//-------------------------------------------------------------------------------------------------
void
BachMainWindow::collectionDelBtnPressed(bool)
{
	BachBucketList bbl = mCollectionView->selection();
	if ( bbl.count() == 0 )
	{
		return;
	}

	BachBucket bb = bbl[ 0 ];

	QMessageBox msgBox;
	msgBox.setWindowTitle( "Delete bucket \""+bb.name()+"\"?" );
	msgBox.setText( "Area you sure you want to delete the selected bucket?" );
	msgBox.setInformativeText( "This will delete the collection and all the mappings, \nbut the images will remain in the pool.\n\n"
			"Note: this action can NOT be undone" );
	msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel );
	msgBox.setDefaultButton( QMessageBox::Ok );
	int ret = msgBox.exec();
	if ( ret == QMessageBox::Cancel )
		return;

	BachBucketMapList bbml = BachBucketMap::recordsByBachBucket( bb );
	foreach ( BachBucketMap bbm, bbml )
		bbm.remove();
	bb.remove();

	mCollectionView->refresh();
}

//-------------------------------------------------------------------------------------------------
void
BachMainWindow::collectionRefreshBtnPressed(bool)
{
	mCollectionView->refresh();
	collectionViewDoubleClicked();
}

//-------------------------------------------------------------------------------------------------
void
BachMainWindow::keywordNewBtnPressed(bool)
{
	QString name;
	bool ok = false;
	QString text = QInputDialog::getText(this, tr("New Keyword"),
										tr("Name:"),
										QLineEdit::Normal,
										name,
										&ok);
	if ( ok && !text.isEmpty() )
	{
		BachKeyword bk;
		bk.setName( text );
		Record & rec = bk.commit();
		if ( !rec.isRecord() )
		{
			QMessageBox msgBox;
			msgBox.setWindowTitle( "Could not create new keyword \""+bk.name()+"\"" );
			msgBox.setText( "The DB returned an error trying to create that keyword." );
			msgBox.setInformativeText( "Maybe it already exists" );
			msgBox.setStandardButtons( QMessageBox::Ok );
			msgBox.setDefaultButton( QMessageBox::Ok );
			msgBox.exec();
		}
		mKeywordsListView->refresh();
	}
}

//-------------------------------------------------------------------------------------------------
void
BachMainWindow::keywordRenameBtnPressed(bool)
{
    QModelIndexList selected = mKeywordsListView->selectionModel()->selectedRows( BachKeywordView::Column_Name );
    if( selected.size() > 0 )
        mKeywordsListView->edit( selected[0] );
}

//-------------------------------------------------------------------------------------------------
void
BachMainWindow::collectionRenameBtnPressed(bool)
{
    QModelIndexList selected = mCollectionView->selectionModel()->selectedRows( BachCollectionView::Column_Name );
    if( selected.size() > 0 )
    	mCollectionView->edit( selected[0] );
}

//-------------------------------------------------------------------------------------------------
void
BachMainWindow::keywordDelBtnPressed(bool)
{
	BachKeywordList bkl = mKeywordsListView->selection();
    if( bkl.isEmpty() )
    	return;

    QStringList keywords;
    foreach( BachKeyword bk, bkl )
    {
    	keywords.append( bk.name() );
    }
	QMessageBox::StandardButton button = QMessageBox::warning( this,
		"Delete",
		"This will delete the selected keywords - it cannot be undone:\n[" + keywords.join( ", " ) + "]",
		QMessageBox::Ok | QMessageBox::Cancel );

	if ( button != QMessageBox::Ok )
		return;

	foreach( BachKeyword bk, bkl )
	{
		BachKeywordMapList bkml = bk.bachKeywordMaps();
		foreach ( BachKeywordMap bkm, bkml )
			bkm.remove();
		bk.remove();
	}
	mKeywordsListView->refresh();
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::keywordRefreshBtnPressed( bool /*a_Pressed*/ )
{
	mKeywordsListView->refresh();
}

//-------------------------------------------------------------------------------------------------
void BachMainWindow::initDisplayOptions() {
	mDisplayOptions = new BachDisplayOptions(this);
	statusBar()->addPermanentWidget(mDisplayOptions);

	connect( mDisplayOptions->mThumbCountSlider, SIGNAL(valueChanged(int)), SLOT(calculateTnSize(int)));
	connect( splitter, SIGNAL(splitterMoved(int,int)), SLOT(calculateTnSize(int)));
	calculateTnSize();
}

//-------------------------------------------------------------------------------------------------

void BachMainWindow::calculateTnSize(int)
{
    if( mTnSizeIsChanging )
        return;
    mTnSizeIsChanging = true;
    QTimer::singleShot(200, this, SLOT(_calculateTnSize()));
}

void BachMainWindow::_calculateTnSize()
{
    // mThumbSlider holds the value of how many images to show horizontally
	// but go from 24 (left) to 1 (right)
    int imagesToShow = mDisplayOptions->mThumbCountSlider->value();

    // widget of mRightTabWidget is amount of space we have in total
    int amountOfSpace = mThumbsView->width()-10;

    int pixelsPerImage = (int)((float)amountOfSpace / (float)imagesToShow) - 12;
    sTnSize = QSize(pixelsPerImage, pixelsPerImage);
    mThumbsView->setIconSize(sTnSize);
		//LOG_3("scroll by:" + QString::number(mThumbsView->horizontalScrollBar()->singleStep()));
		//mThumbsView->horizontalScrollBar()->setSingleStep(5);
    // maintain selection

    QItemSelectionModel * selectModel = mThumbsView->selectionModel();
    if( selectModel->selectedIndexes().size() > 0 )
        mThumbsView->scrollTo( selectModel->selectedIndexes()[0] );

    mTnSizeIsChanging = false;
}

QSize BachMainWindow::tnSize()
{
    return sTnSize;
}

void BachMainWindow::initSearchEdit() {
	QString sql("SELECT word, ndoc FROM ts_stat('SELECT fti_tags FROM bachasset') \
	ORDER BY ndoc DESC \
	LIMIT 1000;");
	QStringList wordList;
	QSqlQuery query = Database::current()->exec(sql);
	while (query.next()) {
		wordList << query.value(0).toString();
	}
	QCompleter * c = new QCompleter(wordList, this);
	c->setCaseSensitivity( Qt::CaseInsensitive );
/////	mSearchEdit->setCompleter(c);
	mSearchHistory->setCompleter(c);

//////	connect( mSearchEdit, SIGNAL(returnPressed()), SLOT(doTagSearch()));
	connect( mSearchHistory->lineEdit(), SIGNAL(returnPressed()), SLOT(doTagSearch()));
	connect( mSearchButton, SIGNAL(pressed()), SLOT(searchButtonPressed()));
/////	mSearchEdit->setFocus( Qt::OtherFocusReason );
	mSearchHistory->setFocus( Qt::OtherFocusReason );
	onClearSearchBtnClicked( false );

	mLimitCB->addItem( "20 Items", QVariant( 20 ) );
	mLimitCB->addItem( "50 Items", QVariant( 50 ) );
	mLimitCB->addItem( "100 Items", QVariant( 100 ) );
	mLimitCB->addItem( "200 Items", QVariant( 200 ) );
	mLimitCB->addItem( "500 Items", QVariant( 500 ) );
	mLimitCB->addItem( "1000 Items", QVariant( 1000 ) );
	mLimitCB->addItem( "5000 Items", QVariant( 5000 ) );
	mLimitCB->addItem( "No Limit", QVariant( -1 ) );
	mLimitCB->setCurrentIndex( 3 ); // the '200' one.
}

void BachMainWindow::resizeEvent( QResizeEvent * event )
{
    if( mRightTabWidget->currentIndex() == 1 )
        calculateTnSize();
    QWidget::resizeEvent(event);
    event->accept();
}

//-------------------------------------------------------------------------------------------------
QString
BachMainWindow::getSearchString() const
{
/////	return mSearchEdit->text();
	return mSearchHistory->currentText();
}

//-------------------------------------------------------------------------------------------------
void
BachMainWindow::setSearchString( const QString & a_SS )
{
/////	mSearchEdit->setText( a_SS );
	mSearchHistory->setEditText( a_SS );
}

void BachMainWindow::rotateTnCW() {
    rotateTn( 90 );
}

void BachMainWindow::rotateTnCCW() {
    rotateTn( -90 );
}

void BachMainWindow::rotateTn( int angle )
{
//	LOG_3("rotateTn(): "+QString::number(angle));
    BachAssetList assets = selection();
    foreach( BachAsset ba, assets ) {
        ba.setTnRotate( ba.tnRotate() + angle );
        ba.commit();
    }
}

void BachMainWindow::toggleExcluded( bool checked )
{
    mShowExcluded = checked;
}

void BachMainWindow::toggleFolderShowTotals( bool checked )
{
    mFolderShowTotals = checked;
	mFoldersView->showTotals( mFolderShowTotals );
}

void BachMainWindow::toggleExcludeAsset()
{
    BachAssetList assets = selection();
    foreach( BachAsset ba, assets ) {
        ba.setExclude( ba.exclude() ? false : true );
        ba.commit();
    }
}

void BachMainWindow::populatePropertyTree()
{
    mPropertyTree->clear();

    BachAssetList selected = selection();
    if( selected.size() == 0 )
        return;

    BachAsset ba = selected[0];
    mPropertyTree->addTopLevelItem( new QTreeWidgetItem(QStringList() << "Path" << ba.path()) );
    mPropertyTree->addTopLevelItem( new QTreeWidgetItem(QStringList() << "Created" << ba.creationDatetime().toString()) );
    mPropertyTree->addTopLevelItem( new QTreeWidgetItem(QStringList() << "Imported" << ba.importedDatetime().toString()) );
    mPropertyTree->addTopLevelItem( new QTreeWidgetItem(QStringList() << "Width" << QString::number(ba.width())) );
    mPropertyTree->addTopLevelItem( new QTreeWidgetItem(QStringList() << "Height" << QString::number(ba.height())) );
    mPropertyTree->addTopLevelItem( new QTreeWidgetItem(QStringList() << "Size" << QString::number(ba.filesize())) );
    QTreeWidgetItem * camera = new QTreeWidgetItem(QStringList() << "Camera Data");
    camera->setBackground( 0, QBrush(Qt::lightGray) );
    mPropertyTree->addTopLevelItem( camera );
    camera->setFirstColumnSpanned(true);
    camera->addChild( new QTreeWidgetItem(QStringList() << "Aperture" << QString::number( ba.aperture() )) );
    camera->addChild( new QTreeWidgetItem(QStringList() << "Focal Length" << QString::number( ba.focalLength() )) );
    camera->addChild( new QTreeWidgetItem(QStringList() << "ISO Speed" << QString::number( ba.isoSpeedRating() )) );
    camera->addChild( new QTreeWidgetItem(QStringList() << "Shutter Speed" << QString::number( ba.shutterSpeed() )) );
    camera->addChild( new QTreeWidgetItem(QStringList() << "Serial #" << QString::number( ba.cameraSN() )) );
    camera->setExpanded(true);

    QTreeWidgetItem * exif = new QTreeWidgetItem(QStringList() << "EXIF Data");
    exif->setBackground( 0, QBrush(Qt::lightGray) );
    mPropertyTree->addTopLevelItem( exif );
    exif->setFirstColumnSpanned(true);

    QStringList exifLines = ba.exif().split("\n");
    foreach( QString line, exifLines ) {
        QStringList lineData = line.split(":");
        if( lineData.size() == 2 )
            exif->addChild( new QTreeWidgetItem(QStringList() << lineData[0] << lineData[1]) );
    }
    exif->sortChildren(0, Qt::AscendingOrder);
    exif->setExpanded(true);
}

//-------------------------------------------------------------------------------------------------
void
BachMainWindow::onFolderBrowserOpenAction()
{
	QString path = mFoldersView->getSelectedPath();
#ifdef Q_OS_LINUX
	QProcess::startDetached("konqueror", QStringList() << path);
#endif
#ifdef Q_OS_MAC
	QProcess::startDetached("open", QStringList() << path);
#endif
#ifdef Q_OS_WIN
	openExplorer( path );
#endif
}

//-------------------------------------------------------------------------------------------------
void
BachMainWindow::onFolderBrowserAddAction()
{
	QString path = mFoldersView->getSelectedPath();
	QMessageBox::StandardButton button = QMessageBox::question( this, "Importing '"+path+"'",
			"Warning, this process could take a long time, continue?\n"
			"(This will open up an window to do the actual import)",
			QMessageBox::Ok|QMessageBox::Cancel );
	if ( button == QMessageBox::Cancel )
	{
		return;
	}

	if ( !QProcess::startDetached( "python2.5", QStringList() << QDir::currentPath()+"/data_export/DirectoryImportGUI.py" << path ) )
	{
		QMessageBox::information( this, "Error launching importer", "Could not launch importer!!" );
	}
	mFoldersView->forgetPath( path );
}


//-------------------------------------------------------------------------------------------------
void
BachMainWindow::onCollectionViewWebAction()
{
	QString selection = mCollectionView->getSelectedCollection();
	QString url = "http://bach/collection/" + selection;
	QDesktopServices::openUrl( url );
}


//-------------------------------------------------------------------------------------------------
void
BachMainWindow::showCollectionViewMenu( const QPoint & /*a_Point*/ )
{
	QAction * result = mCollectionViewMenu->exec(QCursor::pos());
	if( !result )
		return;
}


//-------------------------------------------------------------------------------------------------
void
BachMainWindow::onClearSearchBtnClicked( bool /*a_Clicked*/ )
{
	int idx = -1;
	while( ( idx = mSearchHistory->findText( "" ) ) != -1 )
	{
		mSearchHistory->removeItem( idx );
	}
	mSearchHistory->insertItem( 0, "" );
	mSearchHistory->setCurrentIndex( 0 );
}

//-------------------------------------------------------------------------------------------------
void
BachMainWindow::onKeywordFilterTextChanged( const QString & a_Text )
{
	mKeywordsListView->keywordFilter( a_Text );
	mKeywordsListView->refresh();
	mKeywordFilterRemove->setEnabled( !a_Text.isEmpty() );

}

//-------------------------------------------------------------------------------------------------
void
BachMainWindow::onBucketFilterTextChanged( const QString & a_Text )
{
	mCollectionView->collectionFilter( a_Text );
	mBucketFilterRemove->setEnabled( !a_Text.isEmpty() );
}

//-------------------------------------------------------------------------------------------------
void
BachMainWindow::OnKeywordsShowAllSBToggled( bool a_Checked )
{
	mKeywordsListView->setShowAll( a_Checked );
}
