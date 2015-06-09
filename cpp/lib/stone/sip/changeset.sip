
class ChangeSet
{
%TypeHeaderCode
#include <changeset.h>
%End

public:
	enum ChangeType {
		Change_Insert,
		Change_Update,
		Change_Remove,
		// Indicates the change is a nested changeset
		Change_Nested,
		// This will be returned from changeType if the record has no changes, or is otherwise invalid
		Change_Invalid
	};
	
	static QString changeTypeString(ChangeType);

	enum ReadMode {
		Read_Current,
		Read_Direct,
		Read_Pristine,
		Read_Default
	};

	enum State {
		Invalid,
		Unchanged,
		ChangesPending,
		Committed,
		Undone
	};
	static QString stateString(State);

	//static ChangeType changeType( const Record &);
	
	static ChangeSet current();
	static ChangeSet create( const QString & title = QString(), QUndoStack * = 0 );
	ChangeSet createChild( const QString & title = QString(), QUndoStack * = 0 );
	
	ChangeSet();
	ChangeSet( const ChangeSet & other );
	~ChangeSet();
	
	//ChangeSet & operator=( const ChangeSet & other );
	
	bool operator==( const ChangeSet & other ) const;
	bool operator!=( const ChangeSet & other ) const;

	bool isValid();

	QString title() const;
	void setTitle( const QString & title );
	
	ChangeSet parent() const;
	ChangeSet::State state() const;

	ReadMode readMode() const;

	ChangeSet commit( bool detach = true );
	void revert();

	void undo();
	void redo();

	struct Change {
		ChangeSet::ChangeType type;
		ChangeSet changeSet() const;
		MappedRecordList records() const;
	private:
		Change( ChangeSet::ChangeType type, RecordList records );
		Change( ChangeSet );
	};
	
	QList<ChangeSet::Change> changes();

	void visibleRecordsChanged( RecordList * added /Out/, RecordList * updated /Out/, RecordList * removed /Out/, QList<Table*> filter = QList<Table*>() );

	QString debug(int tab=0);
	
	bool isAnscestor( const ChangeSet & cs ) const;
	
	int depth() const;

	ChangeSet topLevel() const;

	bool isVisible( const ChangeSet & other ) const;

	static ChangeSet commonParent( ChangeSet a, ChangeSet b );
};

class ChangeSetUndoCommand : QUndoCommand
{
public:
	virtual void redo();
	virtual void undo();
	
	ChangeSet changeSet() const;
protected:
	ChangeSetUndoCommand( ChangeSet cs );
};

class ChangeSetEnabler
{
%TypeHeaderCode
#include <changeset.h>
%End

public:
	ChangeSetEnabler( const ChangeSet & changeSet, ChangeSet::ReadMode readMode = ChangeSet::Read_Default );
	~ChangeSetEnabler();
	
	ChangeSet changeSet() const;
	
	bool enabled() const;
	
	void enable();
	void disable();

	void setReadMode( ChangeSet::ReadMode );
};

class ChangeSetNotifier : QObject
{
%TypeHeaderCode
#include <changeset.h>
%End

public:
	ChangeSetNotifier( ChangeSet cs, Table * table, QObject * parent /TransferThis/ = 0 );
	ChangeSetNotifier( ChangeSet cs, QObject * parent /TransferThis/ = 0 );
	
	ChangeSet changeSet() const;
	
	Table * table() const;
	
signals:
	void added( RecordList );
	void removed( RecordList );
	void updated( Record, Record );
	
};
