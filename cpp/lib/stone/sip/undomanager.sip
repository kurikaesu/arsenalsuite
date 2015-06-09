

class UndoManager : QObject
{
%TypeHeaderCode
#include <undomanager.h>
%End
public:
	UndoManager( QObject * parent = 0 );
	~UndoManager();

	bool canUndo();
	bool canRedo();

	static UndoManager * instance();
	
//	bool insideUndoRedo();
	
	QStringList undoTitles( int count = -1 );
	QStringList redoTitles( int count = -1 );
	
public slots:
	void undo( int count = 1 );
	void redo( int count = 1 );
	void startBlock( const QString & title = QString::null );
	void commitBlock();
	void rollbackBlock();
	void pushTitle( const QString & );
	void popTitle();
	
//	void addOperation( UndoOperation * );
	
signals:
	void undoRedoChange( bool canUndo, bool canRedo );

};

/*
class UndoRedoToolButton :QToolButton
{
%TypeHeaderCode
#include <undomanager.h>
%End

Q_OBJECT
public:
	UndoRedoToolButton( QWidget * parent, UndoManager *, bool isUndo );
	
	void updatePopup();
	
public slots:
	void undoRedoChange( bool, bool );
	void menuItemClicked( int );
	
protected:
	bool mIsUndo;
	UndoManager * mUndoManager;
};
*/



