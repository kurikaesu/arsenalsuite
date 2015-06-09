
class GroupsDialog : QDialog
{
%TypeHeaderCode
#include <groupsdialog.h>
%End
public:
	GroupsDialog( QWidget * parent /TransferThis/, ChangeSet changeSet );
	
	void setUser( const User & );
	User user();
	
	GroupList checkedGroups();
	void setCheckedGroups( GroupList );
	
	ChangeSet changeSet() const;
	
public slots:
	void reset();
	void newGroup();
	void deleteGroup();

	void groupsAdded(RecordList);
	void groupsRemoved(RecordList);
	void groupUpdated(Record,Record);
	
	void userGroupsAdded(RecordList);
	void userGroupsRemoved(RecordList);
};

