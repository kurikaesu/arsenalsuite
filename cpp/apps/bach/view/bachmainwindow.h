
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
 * $Id: bachmainwindow.h 9408 2010-03-03 22:35:49Z brobison $
 */

#ifndef BACH_MAIN_WINDOW_H
#define BACH_MAIN_WINDOW_H

#include <qmainwindow.h>
#include <qtimer.h>
#include <qslider.h>
#include <qevent.h>

#include "ui_bachmainwindow.h"
#include "bachitems.h"
#include "BachSearchBox.h"
#include "BachDisplayOptions.h"

class QMenu;
class QAction;
class BachBucket;

class BachMainWindow : public QMainWindow, public Ui_BachMainWindow, public BachSearchBox::SearchStringCallback
{
Q_OBJECT

public:
	BachMainWindow(QSplashScreen * splash, QWidget * parent=0);
	~BachMainWindow();

	virtual void closeEvent( QCloseEvent * );
	BachAssetList selection() const;

	QSize thumbSize() const;

	static QString cacheRoot();
    static QSize tnSize();
	virtual QString getSearchString() const;
	virtual void setSearchString( const QString & a_SS );



public slots:
	void populateMenus();
	void populateTreeMenu();
	void showTreeMenu(const QPoint &);
	void treeEditItem(const QModelIndex &);

	void refreshView();
	void openFiles() const;
	void editTags();
	void showFiles() const;
	void copyPaths();
	void copyImage();
	void removeFromBucket();

	void selectionChanged();
	void collectionViewSelectionChanged();
	void collectionViewDoubleClicked();
	void foldersViewDoubleClicked( const QModelIndex & a_Idx );
	void keywordViewDoubleClicked();
	void changeFont();

	void tabChanged(int);

	void doTagSearch();
	void doBucketSearch( const BachBucket & bb );
	void doKeywordSearch( const BachKeyword & bk );
	void doDirectorySearch( const QString & a_Directory );
	bool doPreSearch();
	void doPostSearch();

	void cancelSearch();
	void updateSearchResults();
	void searchComplete();
	void searchButtonPressed();

	void collapseToSequence();

	void collectionAddBtnPressed(bool);
	void collectionNewBtnPressed(bool);
	void collectionDelBtnPressed(bool);
	void collectionRefreshBtnPressed(bool);
	void collectionRenameBtnPressed(bool);

	void keywordNewBtnPressed(bool);
	void keywordRenameBtnPressed(bool);
	void keywordDelBtnPressed(bool);
	void keywordRefreshBtnPressed(bool);

    void calculateTnSize(int i=0);
    void _calculateTnSize();

    void rotateTnCW();
    void rotateTnCCW();

    void toggleExcluded(bool);
    void toggleExcludeAsset();

    void OnKeywordsShowAllSBToggled( bool a_Toggled );

    void toggleFolderShowTotals( bool checked );

    void populatePropertyTree();

    // folder view
    void onFolderBrowserOpenAction();
    void onFolderBrowserAddAction();
    void showFolderViewMenu( const QPoint & a_Point );

	void onCollectionViewWebAction();
	void showCollectionViewMenu( const QPoint & a_Point );

    void onClearSearchBtnClicked( bool a_Clicked );

	void onKeywordFilterTextChanged( const QString & a_Text );
	void onBucketFilterTextChanged( const QString & a_Text );
protected:
    virtual void resizeEvent( QResizeEvent * );

private:
	QMenu* mTreeMenu;
	QMenu* mFolderBrowserMenu;
	QMenu* mCollectionViewMenu;
	QMenu* mFileMenu;
	QMenu* mViewMenu;

	QAction* mFontAction;
	QAction* mQuitAction;
	QAction* mOpenAction;
	QAction* mEditAction;
	QAction* mShowAction;
	QAction* mCopyPathAction;
	QAction* mCopyImageAction;
	QAction* mRemoveFromBucketAction;

	// for the folder view
	QAction * mFolderBrowserOpenAction;
	QAction * mFolderBrowserAddAction;

	// for the collection view
	QAction * mCollectionViewWebAction;

	QAction* mDblClickToPlayAction;
	QAction* mDblClickToEditAction;

	QAction* mCollapseToSequenceAction;
	BachSearchBox * mSearchBox;
	QVBoxLayout * m_Layout;

	QAction* mRotateCWAction;
	QAction* mRotateCCWAction;

	QAction* mExcludeAction;
    bool mShowExcluded;

    bool mFolderShowTotals;

	int mResults;
	int mResultsShown;
	int mMissing;
	RecordSuperModel * mThumbnailModel;

	BachAssetList mFound;
	BachAssetIter mUpdateIter;
	QTime mSearchTime;
	bool mSearchInProgress;
	bool mSearchUpdateInProgress;
	bool mSearchCanceled;

	void initSearchEdit();
	QTimer * mSearchTimer;
	QString _cacheRoot();

	void initDisplayOptions();
    BachDisplayOptions * mDisplayOptions;

    void rotateTn(int);

    void loadState();
    void saveState();
    bool mTnSizeIsChanging;
};

#endif // MAIN_WINDOW_H

