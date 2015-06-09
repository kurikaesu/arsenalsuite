/*
 * Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
 * SVN_META_HEADURL = "$HeadURL: http://svn/drd/apps/bach/trunk/view/BachKeywordView.h $"
 * SVN_META_ID = "$Id: BachKeywordView.h 9408 2010-03-03 22:35:49Z brobison $"
 */


#ifndef BACHKEYWORDVIEW_H_
#define BACHKEYWORDVIEW_H_

#include <qevent.h>
#include <qwidget.h>
#include <qitemdelegate.h>

#include "recordtreeview.h"
#include "recordsupermodel.h"
#include "bachkeyword.h"
#include "bachassetlist.h"


//-------------------------------------------------------------------------------------------------
class BachKeywordView : public RecordTreeView
{
Q_OBJECT
public:
	enum Columns
	{
		Column_Has = 0,
		Column_Name = 1,
		Column_Count = 2
	};

	BachKeywordView( QWidget * parent );
	virtual ~BachKeywordView() { }
	void saveState();
	void loadState();

	void setSelectedAssets( const BachAssetList & a_BachAssets );
	void keywordFilter( const QString & a_Filter );
	void setShowAll( bool a_Checked );

	static void LoadPixmaps();

public slots:
	void refresh();

protected:
	void dragEnterEvent( QDragEnterEvent * event );
	void dragLeaveEvent( QDragEnterEvent * event );
	void dragMoveEvent( QDragMoveEvent * event );
	void dropEvent( QDropEvent * event );

	void enterEvent( QEvent * event );
	void leaveEvent( QEvent * event );
	void mouseMoveEvent( QMouseEvent * event );

protected slots:
	void onClicked( const QModelIndex & a_Idx );

private:
	void enableCallbacks();
	void disableCallbacks();
	void setFlags( const QModelIndex & a_Idx, int a_Flags );
	int getFlags( const QModelIndex & a_Idx );
	void hoverLeave( const QModelIndex & a_Idx );
	void hoverEnter( const QModelIndex & a_Idx );
	void addHas( const QModelIndex & a_Idx );
	void deleteHas( const QModelIndex & a_Idx );

	RecordSuperModel * mKeywordsModel;
	BachKeyword mCurrentDropTarget;
	BachAssetList mCurrentSelection;
	QString mCurrentFilter;

	bool m_MouseIn;
	bool m_ShowAll;
	QModelIndex m_ItemUnderMouse;
	bool m_CallbacksEnabled;
};

//-------------------------------------------------------------------------------------------------
struct BachKeywordItem : public RecordItem
{
	BachKeyword mBachKeyword;

	void setup( const Record & r, const QModelIndex & );
	QVariant modelData( const QModelIndex & a_Idx, int a_Role ) const;
	bool setModelData( const QModelIndex & a_Idx, const QVariant & a_Value, int a_Role );
	QString sortKey( const QModelIndex & i ) const;
	int compare( const QModelIndex & a, const QModelIndex & b, int, bool );
	Qt::ItemFlags modelFlags( const QModelIndex & );
	Record getRecord();
};

//-------------------------------------------------------------------------------------------------
typedef TemplateRecordDataTranslator<BachKeywordItem> BachKeywordTranslator;

//-------------------------------------------------------------------------------------------------
class KeywordDelegate : public QItemDelegate
{
Q_OBJECT
public:
	KeywordDelegate( QObject * parent = NULL ) : QItemDelegate( parent ) {};
	~KeywordDelegate() {}

	void paint( QPainter * a_Painter, const QStyleOptionViewItem & a_Option, const QModelIndex & a_Idx ) const;
	QWidget * createEditor( QWidget * a_Editor, const QStyleOptionViewItem & a_Option, const QModelIndex & a_Idx ) const;
	void setEditorData( QWidget * a_Editor, const QModelIndex & a_Idx ) const;
	void updateEditorGeometry( QWidget * a_Editor, const QStyleOptionViewItem &option, const QModelIndex & a_Idx ) const;
	void setModelData( QWidget * a_Editor, QAbstractItemModel * a_Model, const QModelIndex & a_Idx ) const;

};
#endif /* BACHKEYWORDVIEW_H_ */
