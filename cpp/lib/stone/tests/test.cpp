
#include <QtTest/QtTest>

#include "blurqt.h"
#include "connection.h"
#include "database.h"
#include "freezercore.h"
#include "record.h"
#include "schema.h"
#include "trigger.h"

class StoneTest : public QObject
{
    Q_OBJECT
public:

private slots:
    void initTestCase();
    void databaseConnection();
    void schemaImport();
    void recordCommit();
    void recordRefCounts();
    void checkForUpdate();
	void queryException();
	void triggers();
	void columnLiterals();
    void traxBug();
    void indexes();
	void changeSet();
	void changeSetSelect();
	void changeSetBug1();
	void changeSetBug2();
	void changeSetBug3();
	void changeSetBug4();
	void changeSetBug5();
	void nestedChangeSet();
	void nestedChangeSetIndexes();
	void expressions();
	void changeSetExpressions();
	void expressionInheritanceSelect();
    void schemaDelete();
};

void StoneTest::initTestCase()
{
    initConfig( "db.ini" );
}

Connection * conn = 0;
Schema * schema = 0;
void StoneTest::databaseConnection()
{
	schema = new Schema();
	conn = Connection::createFromIni( config(), "Database" );
    QVERIFY(conn);
	Database::setCurrent( new Database( schema, conn ) );
	QCOMPARE( (int)conn->checkConnection(), 1 );
}

Table * t = 0;
TableSchema * ts = 0;
Database * d = 0;
void StoneTest::schemaImport()
{
	QVERIFY(d = Database::current());
	schema->mergeXmlSchema( "../../classes/schema.xml" );
	d->setEchoMode( Database::EchoInsert | Database::EchoSelect | Database::EchoUpdate );
	ts = schema->tableByName( "JobTask" );
	t = ts->table();
    QVERIFY(t);
}

Record r;
void StoneTest::recordCommit()
{
	QString test( "JobTask insertion" );
	r = t->load();
	r.setValue( "jobTask", 666 );
	r.setValue( "fkeyHost", 777 );
	r.setValue( "fkeyJob", 111 );
	r.commit();
    QVERIFY( r.key() );
    QCOMPARE( 666, r.getValue( "jobTask" ).toInt() );
    QCOMPARE( 777, r.getValue( "fkeyHost" ).toInt() );
    QCOMPARE( 111, r.getValue( "fkeyJob" ).toInt() );
	r.setValue( "jobtask", 667 );
	r.setValue( "fkeyhost", 778 );
	r.setValue( "fkeyJob", 112 );
    QCOMPARE( 667, r.getValue( "jobTask" ).toInt() );
    QCOMPARE( 778, r.getValue( "fkeyHost" ).toInt() );
    QCOMPARE( 112, r.getValue( "fkeyJob" ).toInt() );
	// r2's imp has a ref to r, the pristine
	QCOMPARE( r.imp()->refCount(), 1 );
	r.commit();
	QCOMPARE( r.imp()->refCount(), 1 );
    QCOMPARE( 667, r.getValue( "jobTask" ).toInt() );
    QCOMPARE( 778, r.getValue( "fkeyHost" ).toInt() );
    QCOMPARE( 112, r.getValue( "fkeyJob" ).toInt() ); 
}

void StoneTest::recordRefCounts()
{
	// One ref from r
	QCOMPARE(r.imp()->refCount(), 1);
	Record mod = t->record(r.key());
	qDebug() << mod.dump();
	// Two refs, one each from r and mod
	QCOMPARE(r.imp()->refCount(), 2);
	mod.setValue( "fkeyJob", 113 );
	// Mod now holds a ref to a copy, and the copy holds a ref to the pristine, along with r
	QCOMPARE(mod.imp()->refCount(), 1);
	QCOMPARE(r.imp()->refCount(), 2);
	mod.commit();
	// Mod now points to the pristine again, the copy is out of scope and drops it's ref to the pristine
	QCOMPARE(r.imp()->refCount(), 2);
	QCOMPARE(r.getValue( "fkeyJob" ).toInt(), 113);
	mod = t->record( r.key(), true, false );
	QCOMPARE( mod.imp(), r.imp() );
	mod.setValue( "fkeyJob", 114 );
	QCOMPARE(mod.imp()->refCount(), 1);
	QCOMPARE(r.imp()->refCount(), 2);
	mod.commit();
	QCOMPARE(r.getValue( "fkeyJob" ).toInt(), 114);
	mod = t->record( r.key(), true, false );
	QCOMPARE( mod.imp(), r.imp() );
	mod = Record();
	// Mod goes out of scope and should drop it's reference to the pristine
	QCOMPARE(r.imp()->refCount(), 1);
}

void StoneTest::traxBug()
{
    schema->mergeXmlSchema( "trax.schema" );
    Table * deliveryTable = schema->tableByName("Delivery")->table();
    QVERIFY(deliveryTable);
    Record d = deliveryTable->record(4564122);
    if( !d.isRecord() ) {
        d = deliveryTable->load();
        d.setValue("keyelement",4564122);
        d.commit();
    }
    d.setValue("daysEstimated",1.0);
    QCOMPARE(d.getValue("daysEstimated").toDouble(), 1.0);
    d.commit();
    QCOMPARE(d.getValue("daysEstimated").toDouble(), 1.0);
    d = deliveryTable->record(4564122);
    QCOMPARE(d.getValue("daysEstimated").toDouble(), 1.0);
    d.setValue("daysEstimated",2.0);
    QCOMPARE(d.getValue("daysEstimated").toDouble(), 2.0);
    d.commit();
    QCOMPARE(d.getValue("daysEstimated").toDouble(), 2.0);
}

void StoneTest::checkForUpdate()
{
	Record same = t->record( r.key(), true, false );
	QCOMPARE( r.imp(), same.imp() );
	int key = r.key();
	same = Record();
	r = Record();
	same = t->record( key, false, true );
	QCOMPARE( same.isRecord(), false );
}

void StoneTest::queryException()
{
	try {
		Database::current()->exec( "SELECT error;" );
	} catch (const SqlException & se) {
		qDebug() << se.what();
	}
}

class TestTrigger : public Trigger
{
public:
	TestTrigger()
	: Trigger( 0xff /* All trigger types implemented */ )
	, gotPostInsert(false)
	, gotPostUpdate(false)
	, gotPostDelete(false)
	{}
	
	virtual Record create( Record r )
	{
		qDebug() << "Running create trigger";
		r.setValue( "jobtask", 1 );
		return r;
	}
	virtual RecordList incoming( RecordList rl )
	{
		RecordList ret;
		for( int i=0; i<rl.size(); ++i )
			if( rl[i].getValue( "jobtask" ).toInt() == 666 )
				ret.append(rl[i].setValue( "jobtask", 667 ));
			else
				ret.append(rl[i]);
		return ret;
	}
	virtual RecordList preInsert( RecordList rl ) {
		foreach( Record r, rl ) {
			if( r.getValue( "jobtask" ).toInt() == 100 )
				throw std::runtime_error( "Hello World" );
			if( r.getValue( "jobtask" ).toInt() == 101 )
				r.setValue( "jobtask", 102 );
		}
		return rl;
		
	}
	virtual Record preUpdate( const Record & updated, const Record & /*before*/ ) {
		if( updated.getValue("jobtask") == 103 )
			throw std::runtime_error( "Hello World" );
		if( updated.getValue("jobtask") == 104 )
			Record(updated).setValue("jobtask",105);
		return updated;
	}
	virtual RecordList preDelete( RecordList rl )
	{
		foreach( Record r, rl )
			if( r.getValue("jobtask") == 105 )
				throw std::runtime_error( "Hello World" );
		// Filter out records that have jobtask=106
		rl = rl.filter("jobtask",106,false);
		qDebug() << rl.debug();
		return rl;
	}
	virtual void postInsert( RecordList rl )
	{
		gotPostInsert = true;
	}
	virtual void postUpdate( const Record & /*updated*/, const Record & /*before*/ )
	{
		gotPostUpdate = true;
	}
	virtual void postDelete( RecordList )
	{
		gotPostDelete = true;
	}
	
	bool gotPostInsert, gotPostUpdate, gotPostDelete;
};

void StoneTest::triggers()
{
	TestTrigger * testTrigger = new TestTrigger();
	ts->addTrigger(testTrigger);

	// create trigger
	Record r = t->load();
	QCOMPARE( r.getValue("jobtask").toInt(), 1 );
	
	// incoming trigger
	foreach( Record r, t->select( "jobtask=666" ) )
		QCOMPARE( r.getValue("jobtask").toInt(), 667 );

	// preInsert trigger exception
	r.setValue("jobtask",100);
	bool caughtException=false;
	try {
		r.commit();
	} catch( const std::runtime_error & re ) {
		caughtException = true;
	}
	QVERIFY(caughtException);
	// preInsert trigger value change
	r.setValue( "jobtask", 101 );
	r.commit();
	QCOMPARE(r.getValue("jobtask").toInt(),102);
	// postInsert trigger was called
	QVERIFY(testTrigger->gotPostInsert);
	
	// preUpdate trigger exception
	caughtException = false;
	r.setValue( "jobtask", 103 );
	try {
		r.commit();
	} catch( const std::runtime_error & re ) {
		caughtException = true;
	}
	QVERIFY(caughtException);
	// preUpdate trigger value change
	r.setValue( "jobtask", 104 );
	r.commit();
	QCOMPARE(r.getValue("jobtask").toInt(),105);
	// postUpdate trigger was called
	QVERIFY(testTrigger->gotPostUpdate);
	
	// preDelete trigger exception
	caughtException = false;
	try {
		r.remove();
	} catch( const std::runtime_error & re ) {
		caughtException = true;
	}
	QVERIFY(caughtException);
	// preDelete record skipping
	r.setValue("jobtask",106);
	r.remove();
	QVERIFY(r.isRecord());
	r.setValue("jobtask",107);
	r.remove();
	qDebug() << r.debug();
	QVERIFY(!r.isRecord());
	QVERIFY(testTrigger->gotPostDelete);
	
	ts->removeTrigger(testTrigger);
	delete testTrigger;
}

void StoneTest::columnLiterals()
{
	// Single record, literal on insert
	{
		Record jt = t->load();
		jt.setColumnLiteral( "startedts", "NOW()" );
		jt.commit();
		
		QDateTime dt = jt.getValue("startedts").toDateTime();
		QVERIFY( !dt.isNull() );
		
		jt.setValue( "jobTask", 1 );
		jt.commit();
		
		QCOMPARE( jt.getValue("jobTask").toInt(), 1 );
		QCOMPARE( jt.getValue("startedts").toDateTime(), dt );
	}
	
	
	// Single record, literal on update
	{
		Record jt = t->load();
		jt.key(true);
		jt.commit();
		
		jt.setColumnLiteral( "startedts", "NOW()" );
		jt.commit();
		
		QDateTime dt = jt.getValue("startedts").toDateTime();
		QVERIFY( !dt.isNull() );
		
		jt.setValue( "jobTask", 1 );
		jt.commit();
		
		QCOMPARE( jt.getValue("jobTask").toInt(), 1 );
		QCOMPARE( jt.getValue("startedts").toDateTime(), dt );
	}

	// RecordList with 1 record, literal on insert
	{
		RecordList rl;
		rl.append(t->load());
		rl.setColumnLiteral( "startedts", "NOW()" );
		rl.commit();
		
		QDateTime dt = rl[0].getValue("startedts").toDateTime();
		QVERIFY( !dt.isNull() );
		
		rl.setValue( "jobTask", 1 );
		rl.commit();
		
		QCOMPARE( rl[0].getValue("jobTask").toInt(), 1 );
		QCOMPARE( rl[0].getValue("startedts").toDateTime(), dt );
	}

		// RecordList with 1 record, literal on update
	{
		RecordList rl;
		Record r = t->load();
		r.key(true);
		r.commit();
		
		rl.append(r);
		rl.setColumnLiteral( "startedts", "NOW()" );
		rl.commit();
		
		QDateTime dt = rl[0].getValue("startedts").toDateTime();
		QVERIFY( !dt.isNull() );
		
		rl.setValue( "jobTask", 1 );
		rl.commit();
		
		QCOMPARE( rl[0].getValue("jobTask").toInt(), 1 );
		QCOMPARE( rl[0].getValue("startedts").toDateTime(), dt );
	}
}

void StoneTest::indexes()
{
	// Host
	Record h = d->tableByName( "Host" )->load();
	h.setValue( "name", "Test7" );

	// JobTask
	Record jt = t->load();
	jt.setValue( "jobTask", 666 );
	jt.setValue( "fkeyJob", 111 );
	jt.setForeignKey( "host", h );

	jt.commit();
	h.commit();
	QVERIFY2( jt.foreignKey("host").key() == h.key(), "Uncommitted foreign key relationships" );
}

void StoneTest::changeSet()
{
	QUndoStack * undoStack = new QUndoStack();
	ChangeSet cs = ChangeSet::create( "Test", undoStack );
	Table * config = Database::current()->tableByName( "Config" );

	ChangeSetNotifier csn(cs);
	
	QSignalSpy addedSpy(&csn, SIGNAL(added(RecordList)));
	QSignalSpy updatedSpy(&csn, SIGNAL(updated(Record,Record)));
	
	Record c = config->load();
	c.setValue( "config", "test" );
	c.setValue( "value", "value" );
	c.commit();

	QCOMPARE(addedSpy.count(), 1);
	QList<QVariant> arguments = addedSpy.takeFirst();
	QVERIFY(qVariantValue<RecordList>(arguments.at(0)) == RecordList(c));

	{
		ChangeSetEnabler cse(cs);
		c.setValue( "value", "other" );
		QCOMPARE( c.getValue("value").toString(), QString("other") );
		cse.setReadMode( ChangeSet::Read_Pristine );
		QCOMPARE( c.getValue("value").toString(), QString("value") );
		cse.setReadMode( ChangeSet::Read_Direct );
		QCOMPARE( c.getValue("value").toString(), QString("other") );
		cse.setReadMode( ChangeSet::Read_Current );
		QCOMPARE( c.getValue("value").toString(), QString("other") );
		c.commit();
		QCOMPARE( c.getValue("value").toString(), QString("other") );
	}
	
	QCOMPARE( c.getValue( "value" ).toString(), QString("value") );
	
	QCOMPARE(updatedSpy.count(), 1);
	arguments = updatedSpy.takeFirst();
	QCOMPARE(qVariantValue<Record>(arguments.at(0)), c);
	
	c.setValue( "config", "test2" );
	c.commit();
	
	QCOMPARE(updatedSpy.count(), 1);
	arguments = updatedSpy.takeFirst();
	QCOMPARE(qVariantValue<Record>(arguments.at(0)), c);
	
	// cs now a new changeset ready to go
	ChangeSet old = cs.commit(true);
	
	QCOMPARE( c.getValue( "value" ).toString(), QString("other") );
	QCOMPARE( c.getValue( "config" ).toString(), QString("test2") );
	{
		ChangeSetEnabler cse(old);
		QCOMPARE( c.getValue( "value" ).toString(), QString("other") );
		QVERIFY( old.readMode() == ChangeSet::Read_Pristine );
	}
	
	QCOMPARE( undoStack->count(), 1 );
	
	QCOMPARE(updatedSpy.count(), 0);
	undoStack->undo();
	
	QCOMPARE( c.getValue( "value" ).toString(), QString("value") );
	QCOMPARE(updatedSpy.count(), 1);
	
	undoStack->redo();
	
	QCOMPARE( c.getValue( "value" ).toString(), QString("other") );
	QCOMPARE(updatedSpy.count(), 2);
	
	{
		ChangeSetEnabler cse(cs);
		c.setValue( "config", "test3" );
		c.commit();
	}
	
	QCOMPARE(updatedSpy.count(), 3);
}

void StoneTest::changeSetSelect()
{
	// Cleanup previous runs
	Database::current()->exec( "DELETE FROM Config WHERE config IN ('test_select_remove','test_select_update','test_select_insert')" );
	
	Table * config = Database::current()->tableByName( "Config" );
	Record r_remove = config->load();
	r_remove.setValue( "config", "test_select_remove" );
	r_remove.commit();
	
	Record r_update = config->load();
	r_update.setValue( "config", "test_select_update" );
	r_update.setValue( "value", "test_select_value1" );
	r_update.commit();
	
	ChangeSet cs = ChangeSet::create("test");
	ChangeSetEnabler cse(cs);
	Record r_insert = config->load();
	r_insert.setValue( "config", "test_select_insert" );
	r_insert.commit();
	
	r_update.setValue( "value", "test_select_value2" );
	r_update.commit();
	
	r_remove.remove();
	
	RecordList allConfig = config->select();
	QVERIFY( allConfig.contains(r_insert) );
	QVERIFY( !allConfig.contains(r_remove) );
	QVERIFY( allConfig.filter( "value", "test_select_value1" ).size() == 0 );
	QVERIFY( allConfig.filter( "value", "test_select_value2" ).size() == 1 );
}

void StoneTest::changeSetBug1()
{
	ChangeSet cs = ChangeSet::create("test");
	ChangeSetEnabler cse(cs);
	Record r = Database::current()->tableByName( "Config" )->load();
	r.setValue( "config", "test_bug_1" );
	r.commit();
	r.remove();
}

void StoneTest::changeSetBug2()
{
	ChangeSet cs = ChangeSet::create("test");
	ChangeSetEnabler cse(cs);
	ChangeSetNotifier csn(cs);
	QSignalSpy ad(&csn, SIGNAL(added(RecordList)));
	QSignalSpy up(&csn, SIGNAL(updated(Record,Record)));
	Record r = Database::current()->tableByName( "Config" )->load();
	r.setValue( "config", "test_bug_2" );
	r.commit();
	QCOMPARE( ad.count(), 1 );
	QCOMPARE( up.count(), 0 );
	r.commit();
	QCOMPARE( ad.count(), 1 );
	QCOMPARE( up.count(), 0 );
	r.setValue( "value", "test_bug_2" );
	r.commit();
	QCOMPARE( ad.count(), 1 );
	QCOMPARE( up.count(), 1 );
	r.commit();
	QCOMPARE( ad.count(), 1 );
	QCOMPARE( up.count(), 1 );
	r.setValue( "value", "test_bug" );
	r.commit();
	QCOMPARE( ad.count(), 1 );
	QCOMPARE( up.count(), 2 );
}

void StoneTest::changeSetBug3()
{
	Table * deliveryTable = schema->tableByName("Delivery")->table();
	Record d = deliveryTable->record(207789);
	if( !d.isRecord() ) {
		d.key(true);
		d.commit();
	}

	QUndoStack * undoStack = new QUndoStack();
	QCOMPARE(undoStack->count(), 0);
	ChangeSet cs = ChangeSet::create("Parent", undoStack);
	ChangeSet cs2 = cs.createChild("Child", undoStack);
	{
		ChangeSetEnabler cse(cs2);
		// No Changes
		d.commit();
	}
	
	cs2.commit();
	QCOMPARE(undoStack->count(), 0);
}

void StoneTest::changeSetBug4()
{
	Table * config = Database::current()->tableByName( "Config" );

	QUndoStack * us1 = new QUndoStack();
	ChangeSet cs = ChangeSet::create( "Parent", us1 );
	ChangeSetNotifier csn(cs,config);
	QSignalSpy ad(&csn, SIGNAL(added(RecordList)));
	QSignalSpy up(&csn, SIGNAL(updated(Record,Record)));


	Record r = config->load();
	r.setValue( "config", "changeSetBug4" );
	r.setValue( "value", "value" );
	r.commit();
	{
		ChangeSet child = cs.createChild( "Child1" );
		ChangeSetEnabler cse(child);
		
		QCOMPARE( ad.count(), 1 );
		QCOMPARE( up.count(), 0 );
		
		r.setValue( "value", "value2" );
		r.commit();

		QCOMPARE( ad.count(), 1 );
		QCOMPARE( up.count(), 0 );
		
		child.commit();
		
		QCOMPARE( ad.count(), 1 );
		QCOMPARE( up.count(), 1 );
	}

	{
		ChangeSet child = cs.createChild( "Child2" );
		ChangeSetEnabler cse(child);
		
		QCOMPARE( ad.count(), 1 );
		QCOMPARE( up.count(), 1 );
		
		r.setValue( "value", "value3" );
		r.commit();

		QCOMPARE( ad.count(), 1 );
		QCOMPARE( up.count(), 1 );
		
		r.commit();

		QCOMPARE( ad.count(), 1 );
		QCOMPARE( up.count(), 1 );

		child.commit();
		
		QCOMPARE( ad.count(), 1 );
		QCOMPARE( up.count(), 2 );
	}
}

void StoneTest::changeSetBug5()
{
	Table * config = Database::current()->tableByName( "Config" );
	ChangeSet cs0 = ChangeSet::create("Test");
	ChangeSet cs = cs0.createChild("create");
	
	Record r;
	
	{
		ChangeSetEnabler cse(cs);
		r = config->load();
		r.setValue( "config", "changeSetBug5" );
		r.commit();
	}
	
	cs.commit();
	cs = cs0.createChild("set role");
	
	{
		ChangeSetEnabler cse(cs);
		r.setValue( "value", "value" );
		r.commit();
	}
	
	cs.commit();
	cs0.commit(); // Ensure doesn't crash here
}

void StoneTest::nestedChangeSet()
{
	Table * config = Database::current()->tableByName( "Config" );

	QUndoStack * us1 = new QUndoStack();
	ChangeSet cs1 = ChangeSet::create( "Test1", us1 );
	ChangeSetNotifier csn1(cs1);
	QSignalSpy ad1(&csn1, SIGNAL(added(RecordList)));
	QSignalSpy up1(&csn1, SIGNAL(updated(Record,Record)));

	QUndoStack * us2 = new QUndoStack();
	ChangeSet cs2 = cs1.createChild( "Test2", us2 );
	ChangeSetNotifier csn2(cs2);
	QSignalSpy ad2(&csn2, SIGNAL(added(RecordList)));
	QSignalSpy up2(&csn2, SIGNAL(updated(Record,Record)));

	QUndoStack * us3 = new QUndoStack();
	ChangeSet cs3 = cs2.createChild( "Test3", us3 );
	ChangeSetNotifier csn3(cs3);
	QSignalSpy ad3(&csn3, SIGNAL(added(RecordList)));
	QSignalSpy up3(&csn3, SIGNAL(updated(Record,Record)));

	Record r;
	{
		ChangeSetEnabler cse(cs3);
		r = config->load();
		r.setValue( "config", "nested" );
		r.setValue( "value", "value" );
		r.commit();
		
		QCOMPARE( r.getValue("config").toString(), QString("nested") );
		
		QCOMPARE( ad3.count(), 1 );
		QCOMPARE( ad2.count(), 0 );
		QCOMPARE( ad1.count(), 0 );
		QVERIFY( up1.count() == up2.count() == up3.count() == 0 );
		
		r.setValue( "value", "value2" );
		r.commit();
		
		QCOMPARE( ad3.count(), 1 );
		QCOMPARE( up3.count(), 1 );
		QCOMPARE( ad2.count(), 0 );
		QCOMPARE( ad1.count(), 0 );
		QCOMPARE( up2.count(), 0 );
		QCOMPARE( up1.count(), 0 );
	}
	
	// New record should be invisible to all but cs3
	{
		ChangeSetEnabler cse(cs2);
		QCOMPARE( r.isRecord(), false );
		QCOMPARE( r.getValue( "config" ).isNull(), true );
	}
	
	{
		ChangeSetEnabler cse(cs1);
		QCOMPARE( r.isRecord(), false );
		QCOMPARE( r.getValue( "config" ).isNull(), true );
	}
	
	QCOMPARE( r.isRecord(), false );
	QCOMPARE( r.getValue( "config" ).isNull(), true );
	
	cs3.commit(false);
	QCOMPARE(us3->count(),1);
	
	// New record should be invisible to all but cs3 and cs2
	{
		ChangeSetEnabler cse(cs3);
		QCOMPARE( r.isRecord(), true );
		QCOMPARE( r.getValue( "config" ).toString(), QString("nested") );
	}

	{
		ChangeSetEnabler cse(cs2);
		QCOMPARE( r.isRecord(), true );
		QCOMPARE( r.getValue( "config" ).toString(), QString("nested") );
	}
	
	{
		ChangeSetEnabler cse(cs1);
		QCOMPARE( r.isRecord(), false );
		QCOMPARE( r.getValue( "config" ).isNull(), true );
	}
	
	QCOMPARE( r.isRecord(), false );
	QCOMPARE( r.getValue( "config" ).isNull(), true );
	
	QCOMPARE( ad3.count(), 1 );
	QCOMPARE( up3.count(), 1 );
	QCOMPARE( ad2.count(), 1 );
	QCOMPARE( ad1.count(), 0 );
	QCOMPARE( up1.count(), 0 );
	QCOMPARE( up2.count(), 0 );
	
	us3->undo();
	
	// Undone new record should be invisible to all but cs3
	{
		ChangeSetEnabler cse(cs3);
		QCOMPARE( r.isRecord(), true );
		QCOMPARE( r.getValue( "config" ).toString(), QString("nested") );
	}

	{
		ChangeSetEnabler cse(cs2);
		QCOMPARE( r.isRecord(), false );
		QCOMPARE( r.getValue( "config" ).isNull(), true );
	}
	
	{
		ChangeSetEnabler cse(cs1);
		QCOMPARE( r.isRecord(), false );
		QCOMPARE( r.getValue( "config" ).isNull(), true );
	}
	
	QCOMPARE( r.isRecord(), false );
	QCOMPARE( r.getValue( "config" ).isNull(), true );
	
	QCOMPARE( ad3.count(), 1 );
	QCOMPARE( up3.count(), 1 );
	QCOMPARE( ad2.count(), 1 );
	QCOMPARE( ad1.count(), 0 );
	QCOMPARE( up1.count(), 0 );
	QCOMPARE( up2.count(), 0 );
	
	us3->redo();
	
	// New record should be invisible to all but cs3 and cs2
	{
		ChangeSetEnabler cse(cs3);
		QCOMPARE( r.isRecord(), true );
		QCOMPARE( r.getValue( "config" ).toString(), QString("nested") );
	}

	{
		ChangeSetEnabler cse(cs2);
		QCOMPARE( r.isRecord(), true );
		QCOMPARE( r.getValue( "config" ).toString(), QString("nested") );
	}
	
	{
		ChangeSetEnabler cse(cs1);
		QCOMPARE( r.isRecord(), false );
		QCOMPARE( r.getValue( "config" ).isNull(), true );
	}
	
	QCOMPARE( r.isRecord(), false );
	QCOMPARE( r.getValue( "config" ).isNull(), true );
	
	QCOMPARE( ad3.count(), 2 );
	QCOMPARE( up3.count(), 1 );
	QCOMPARE( ad2.count(), 2 );
	QCOMPARE( ad1.count(), 0 );
	QCOMPARE( up1.count(), 0 );
	QCOMPARE( up2.count(), 0 );

	
	cs2.commit(false);
	
	// New record should be visible to cs3, cs2, cs1, invisible globally
	{
		ChangeSetEnabler cse(cs3);
		QCOMPARE( r.isRecord(), true );
		QCOMPARE( r.getValue( "config" ).toString(), QString("nested") );
	}

	{
		ChangeSetEnabler cse(cs2);
		QCOMPARE( r.isRecord(), true );
		QCOMPARE( r.getValue( "config" ).toString(), QString("nested") );
	}
	
	{
		ChangeSetEnabler cse(cs1);
		QCOMPARE( r.isRecord(), true );
		QCOMPARE( r.getValue( "config" ).toString(), QString("nested") );
	}
	
	QCOMPARE( r.isRecord(), false );
	QCOMPARE( r.getValue( "config" ).isNull(), true );

	QCOMPARE( ad3.count(), 2 );
	QCOMPARE( up3.count(), 1 );
	QCOMPARE( ad2.count(), 2 );
	QCOMPARE( ad1.count(), 1 );
	QCOMPARE( up1.count(), 0 );
	QCOMPARE( up2.count(), 0 );
}

void StoneTest::nestedChangeSetIndexes()
{
}

#include "expression.h"
#include "interval.h"

void StoneTest::expressions()
{
	// Simple expression evaluation
	QCOMPARE( (ev_(3) & ev_(2) == ev_(2)).matches(Record()), true );
	QCOMPARE( (ev_(3) | ev_(2) == ev_(3)).matches(Record()), true );
	QCOMPARE( (ev_(3) ^ ev_(2) == ev_(1)).matches(Record()), true );
	QCOMPARE( ev_("test").like(ev_("test")).matches(Record()), true );
	QCOMPARE( ev_("test").like(ev_("TeSt")).matches(Record()), false );
	QCOMPARE( ev_("test").ilike(ev_("TeSt")).matches(Record()), true );
	QCOMPARE( ev_("test").ilike(ev_("T__t")).matches(Record()), true );
	QCOMPARE( ev_("test").ilike(ev_("T%t")).matches(Record()), true );
	
	Table * configTable = Database::current()->tableByName( "Config" );
	TableSchema * configSchema = configTable->schema();
	
	// Emulate a generated class with StaticFieldExpression members
	struct _Config {
		StaticFieldExpression Key, Config, Value;
	} Config = { configSchema->field("keyconfig"), configSchema->field("config"), configSchema->field("value") };
	
	Expression e = (Config.Config == "test") && (Config.Value == 2) || (Config.Config=="test2");
	Record c = configTable->load();
	c.setValue( Config.Config, "test" );
	c.setValue( Config.Value, 2 );
	QCOMPARE( e.matches( c ), true );
	c.setValue( Config.Value, 3 );
	QCOMPARE( e.matches( c ), false );
	c.setValue( Config.Config, "test2" );
	QCOMPARE( e.matches( c ), true );
	
	e = Config.Config.in( ExpressionList() << ev_(2) << ev_(3) << ev_(4) << ev_("test2") << (Config.Value == 1) );
	QCOMPARE( e.matches(c), true );
	c.setValue( Config.Config, true );
	QCOMPARE( e.matches(c), false );
	c.setValue( Config.Value, 1 );
	QCOMPARE( e.matches(c), true );
	
	e = (Config.Config + " test string ").orderBy( Config.Value ).orderBy( Config.Config );
	qDebug() << e.toString();
	
	e = ev_(QDate::fromString("2012-01-01",Qt::ISODate)) + _evt(Interval::fromString("1 day")) == ev_(QDate::fromString("2012-01-02",Qt::ISODate));
	qDebug() << e.toString();
	QCOMPARE( e.matches(Record()), true );

	e = Config.Config == "test2";
	qDebug() << e.toString();
	QVERIFY( e.select().size() > 0 );
	
	Table * calendar = Database::current()->tableByName( "Calendar" );
	TableSchema * calendarSchema = calendar->schema();
	
	struct _Calendar {
		StaticFieldExpression CalendarCategory, Repeat, Notification, Date;
	} Calendar = { calendarSchema->field("fkeycalendarcategory"), calendarSchema->field("repeat"), calendarSchema->field("fkeynotification"), calendarSchema->field("date") };
	
	Table * calendarCategory = Database::current()->tableByName( "CalendarCategory" );
	Record cc = calendarCategory->load();
	cc.key(true);
	//"fkeycalendarcategory = %i AND "
	//		"(repeat IS NULL OR repeat IN (0, 1)) AND "
	//		"fkeynotification IS NULL AND "
	//		"date >= '%s'"
	//		% (cat.key(), QDate.currentDate().toString(Qt.ISODate))
	
	e = Calendar.CalendarCategory == cc
		&& (Calendar.Repeat.isNull() || Calendar.Repeat.in(ExpressionList() << ev_(0) << ev_(1)))
		&& Calendar.Notification.isNull()
		&& Calendar.Date >= ev_(QDate::currentDate());
	qDebug() << e.toString();
	
	Expression sub = Expression::createCombination( Expression::Union_Expression, ExpressionList() << Query(ExpressionList() << ev_("test"),Expression()) << Query(Config.Config, configTable, (Config.Config != "test").orderBy(Config.Config).limit(2)));
	qDebug() << Config.Config.in(sub).toString();
	
	//QCOMPARE( sub.toString(), sub.copy().toString() );
	//TableAlias a(configTable, "c2");
	//Expression q = Query(ExpressionList() << a(Config.Config) << Config.Config, ExpressionList() << configTable << a, (Config.Config == a(Config.Config)));
	//qDebug() << q.toString();
	//q.select();
}

void StoneTest::changeSetExpressions()
{
	Table * calendar = Database::current()->tableByName( "Calendar" );
	TableSchema * calendarSchema = calendar->schema();
	
	struct _Calendar {
		StaticFieldExpression CalendarCategory, Repeat, Notification, Date;
	} Calendar = { calendarSchema->field("fkeycalendarcategory"), calendarSchema->field("repeat"), calendarSchema->field("fkeynotification"), calendarSchema->field("date") };
	
	Table * calendarCategory = Database::current()->tableByName( "CalendarCategory" );
	TableSchema * ccSchema = calendarCategory->schema();
	struct _CalendarCategory {
		StaticFieldExpression Key, Name;
	} CalendarCategory = { ccSchema->field(ccSchema->primaryKeyIndex()), ccSchema->field("calendarcategory") };

	Record cc = calendarCategory->load();
	cc.setValue(CalendarCategory.Name,"Test");
	cc.commit();

	Record cal = calendar->load();
	cal.setForeignKey(Calendar.CalendarCategory,cc);
	cal.commit();
	
	ChangeSet cs(ChangeSet::create());
	ChangeSetEnabler cse(cs);
	
	cc.setValue(CalendarCategory.Name,"Test2");
	cc.commit();
	
	Record cal2 = calendar->load();
	cal2.setForeignKey(Calendar.CalendarCategory,cc);
	cal2.commit();
	
	Expression subQuery = Query( CalendarCategory.Key, calendarCategory, CalendarCategory.Name==ev_("Test2") );
	QVERIFY( calendar->select( Calendar.CalendarCategory.in( subQuery ) ).size() == 2 );
}

void StoneTest::expressionInheritanceSelect()
{
	Database::current()->tableByName( "Job" )->select( Expression().limit(2) );
}

void StoneTest::schemaDelete()
{
	Schema * schema = new Schema();
	delete schema;
}

QTEST_MAIN(StoneTest)
#include "test.moc"
