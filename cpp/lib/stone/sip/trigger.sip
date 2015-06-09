
%VirtualErrorHandler TriggerVirtualErrorHandler
	throw PythonException();
%End

class Trigger /VirtualErrorHandler=TriggerVirtualErrorHandler/
{
%TypeHeaderCode
#include <trigger.h>
#include <pyembed.h>
%End
public:
	enum TriggerType
	{
		CreateTrigger			= 0x01,
		IncomingTrigger			= 0x02,
		PreInsertTrigger		= 0x04,
		PreUpdateTrigger		= 0x08,
		PreDeleteTrigger		= 0x10,
		PostInsertTrigger		= 0x20,
		PostUpdateTrigger		= 0x40,
		PostDeleteTrigger		= 0x80,
	};

	Trigger( int triggerTypes );
	virtual ~Trigger();
	
	int triggerTypes() const;

	virtual Record create( const Record & r );
	virtual RecordList incoming( RecordList rl );
	virtual RecordList preInsert( RecordList rl );
	virtual Record preUpdate( const Record & updated, const Record & before );
	virtual RecordList preDelete( RecordList rl );
	virtual void postInsert( RecordList );
	virtual void postUpdate( const Record & updated, const Record & before );
	virtual void postDelete( RecordList );
};
