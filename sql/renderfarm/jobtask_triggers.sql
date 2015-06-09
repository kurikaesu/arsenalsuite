
-- Sets host.fkeyjobtask when a host begins working on a jobtask
-- Sets hosthistory.success when a task is completed
CREATE OR REPLACE FUNCTION after_update_JobTask() RETURNS trigger AS $$
DECLARE
BEGIN
	-- Update Job Counters when a tasks status changes
	IF (NEW.status != coalesce(OLD.status,'')) THEN
		PERFORM update_job_task_counts(NEW.fkeyjob);
	END IF;
RETURN new;
END;
$$ LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION update_job_task_counts(_keyjob int4) RETURNS void AS
$BODY$
DECLARE
	cur RECORD;
	unassigned integer := 0;
	busy integer := 0;
	assigned integer := 0;
	done integer := 0;
	cancelled integer := 0;
	suspended integer := 0;
	holding integer := 0;
BEGIN
	FOR cur IN
		SELECT status, count(*) as c FROM jobtask WHERE fkeyjob=_keyjob
		GROUP BY status
	LOOP
		IF( cur.status = 'new' ) THEN
			unassigned := cur.c;
		ELSIF( cur.status = 'assigned' ) THEN
			assigned := cur.c;
		ELSIF( cur.status = 'busy' ) THEN
			busy := cur.c;
		ELSIF( cur.status = 'done' ) THEN
			done := cur.c;
		ELSIF( cur.status = 'cancelled' ) THEN
			cancelled := cur.c;
		ELSIF( cur.status = 'suspended' ) THEN
			suspended := cur.c;
		ELSIF( cur.status = 'holding' ) THEN
			holding := cur.c;
		END IF;
	END LOOP;
	
	UPDATE JobStatus SET
		taskscount = unassigned + assigned + busy + done + cancelled + suspended + holding,
		tasksUnassigned = unassigned,
		tasksAssigned = assigned,
		tasksBusy = busy,
		tasksDone = done,
		tasksCancelled = cancelled,
		tasksSuspended = suspended
	WHERE fkeyjob = _keyjob;
	RETURN;
END;
$BODY$
LANGUAGE 'plpgsql' VOLATILE;

DROP TRIGGER IF EXISTS after_update_JobTask ON jobtask;
CREATE TRIGGER after_update_JobTask
AFTER UPDATE
ON jobtask
FOR EACH ROW
EXECUTE PROCEDURE after_update_JobTask();

