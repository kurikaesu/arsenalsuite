
class ThreadTask : QEvent
{
%TypeHeaderCode
#include <freezercore.h>
%End
public:
	ThreadTask( int type = 0, QObject * reciever = 0 ) /Transfer/;
	
	virtual void run();

	bool canceled() const;
	void cancel();

	QObject * reciever();
};

class FreezerCore : QObject
{
%TypeHeaderCode
#include <freezercore.h>
%End
public:
	FreezerCore();
	~FreezerCore();

	static FreezerCore * instance();
	
	static void shutdown();

	void shutdownThreads();

	static void addTask( ThreadTask * /Transfer/ );
	static void wakeup();

	static void setDatabaseForThread( Database * db, Connection * c /Transfer/ );
	void cancelObjectTasks( QObject * );
	static void cancelTask( ThreadTask * task );
};

