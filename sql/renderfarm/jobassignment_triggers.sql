

CREATE OR REPLACE FUNCTION cancel_job_assignments( _keyjob int ) RETURNS VOID AS $$
DECLARE
	cancelledStatusKey int;
	_job job;
BEGIN
	SELECT INTO cancelledStatusKey keyjobassignmentstatus FROM jobassignmentstatus WHERE status='cancelled';
	SELECT INTO _job * FROM job WHERE keyjob=_keyjob;
		
	IF _job.packettype = 'preassigned' THEN
		UPDATE JobTask SET status='new' WHERE fkeyjob=_keyjob AND status IN ('assigned','busy');
	ELSE
		UPDATE JobTask SET status='new', fkeyhost=NULL WHERE fkeyjob=_keyjob AND status IN ('assigned','busy');
	END IF;

	UPDATE JobTaskAssignment SET fkeyjobassignmentstatus=cancelledStatusKey WHERE fkeyjobassignment IN(SELECT keyjobassignment FROM JobAssignment WHERE fkeyjob=_keyjob) AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus from jobassignmentstatus WHERE status IN ('ready','copy','busy'));

	UPDATE JobAssignment SET fkeyjobassignmentstatus=cancelledStatusKey WHERE fkeyjob=_keyjob AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus from jobassignmentstatus WHERE status IN ('ready','copy','busy'));
END;
$$ LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION cancel_job_assignment( _keyjobassignment int ) RETURNS VOID AS $$
DECLARE
	cancelledStatusKey int;
	assignment jobassignment;
	_job job;
BEGIN
	SELECT INTO cancelledStatusKey keyjobassignmentstatus FROM jobassignmentstatus WHERE status='cancelled';
	SELECT INTO assignment * FROM jobassignment WHERE keyjobassignment=_keyjobassignment;
	SELECT INTO _job * FROM job WHERE keyjob=assignment.fkeyjob;

	IF _job.packettype = 'preassigned' THEN
		UPDATE JobTask SET status='new' WHERE fkeyjobtaskassignment IN (SELECT keyjobtaskassignment FROM jobtaskassignment WHERE fkeyjobassignment=_keyjobassignment) AND status IN ('assigned','busy');
	ELSE
		UPDATE JobTask SET status='new', fkeyhost=NULL WHERE fkeyjobtaskassignment IN (SELECT keyjobtaskassignment FROM jobtaskassignment WHERE fkeyjobassignment=_keyjobassignment) AND status IN ('assigned','busy');
	END IF;

	UPDATE JobTaskAssignment SET fkeyjobassignmentstatus=cancelledStatusKey WHERE fkeyjobassignment=_keyjobassignment AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus from jobassignmentstatus WHERE status IN ('ready','copy','busy'));

	-- Dont set it to cancelled if it already cancelled or error
	UPDATE JobAssignment SET fkeyjobassignmentstatus=cancelledStatusKey WHERE keyjobassignment=_keyjobassignment AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus from jobassignmentstatus WHERE status IN ('ready','copy','busy'));
END;
$$ LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION updateJobLicenseCounts( fkeyjob int, countChange int ) RETURNS VOID AS $$
DECLARE
	serv service;
BEGIN
	FOR serv IN SELECT * FROM Service s JOIN JobService js ON (s.keyService = js.fkeyService AND js.fkeyjob=fkeyjob) WHERE fkeyLicense IS NOT NULL LOOP
		UPDATE License SET available=available - countChange WHERE keylicense=serv.fkeylicense;
	END LOOP;
	RETURN;
END;
$$ LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION assignment_status_count( fkeyjobassignmentstatus int ) RETURNS INT AS $$
DECLARE
	status jobassignmentstatus;
BEGIN
	SELECT INTO status * FROM jobassignmentstatus WHERE keyjobassignmentstatus=fkeyjobassignmentstatus;
	IF status.status IN ('done','cancelled','error') THEN
		RETURN 0;
	END IF;
	IF status.status IN ('ready','copy','busy') THEN
		return 1;
	END IF;
	RETURN 0;
END;
$$ LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION jobassignment_insert() RETURNS trigger AS $$
DECLARE
	newstatusval int := 0;
BEGIN
	newstatusval := assignment_status_count(NEW.fkeyjobassignmentstatus);
	IF newstatusval > 0 THEN
		UPDATE HostStatus SET activeassignmentCount=coalesce(activeassignmentCount,0)+newstatusval,lastAssignmentChange=NOW() WHERE HostStatus.fkeyhost=NEW.fkeyhost;
	END IF;
	-- If we are starting at 'new','ready', or 'busy', we need to update license counts for the services
	IF newstatusval > 0 THEN
		PERFORM updateJobLicenseCounts( NEW.fkeyjob, 1 );
	END IF;
	RETURN NEW;
END;
$$ LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION jobassignment_update() RETURNS trigger AS $$
DECLARE
	currentjob job;
	oldstatusval int := 0;
	newstatusval int := 0;
	assignmentcountchange int :=0;
BEGIN
	IF NEW.fkeyhost != OLD.fkeyhost THEN
		-- TODO: Raise some kind of error
		RAISE NOTICE 'JobAssignment.fkeyhost is immutable';
	END IF;

	oldstatusval := assignment_status_count(OLD.fkeyjobassignmentstatus);
	newstatusval := assignment_status_count(NEW.fkeyjobassignmentstatus);

	-- Update license count if status is changing from one that requires a license to one that doesnt
	IF oldstatusval > newstatusval THEN
		PERFORM updateJobLicenseCounts( NEW.fkeyjob, -1 );
		IF NEW.started IS NOT NULL THEN
			NEW.ended := NOW();
		END IF;
	END IF;
	IF newstatusval > oldstatusval THEN
		PERFORM updateJobLicenseCounts( NEW.fkeyjob, 1 );
	END IF;

	-- Update the number of active assignments for the hsot
	IF newstatusval != oldstatusval THEN
		UPDATE HostStatus SET activeAssignmentCount=coalesce(activeAssignmentCount,0)+(newstatusval-oldstatusval), lastAssignmentChange=NOW() WHERE HostStatus.fkeyhost=NEW.fkeyhost;
	END IF;
	RETURN NEW;
END;
$$ LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION jobassignment_delete() RETURNS trigger AS $$
DECLARE
	oldstatusval int := 0;
BEGIN
	oldstatusval := assignment_status_count(OLD.fkeyjobassignmentstatus);
	IF oldstatusval > 0 THEN
		PERFORM updateJobLicenseCounts( OLD.fkeyjob, -1 );
	END IF;
	IF oldstatusval > 0 THEN
		UPDATE HostStatus SET activeAssignmentCount=coalesce(activeAssignmentCount,0)-oldstatusval, lastAssignmentChange=NOW() WHERE HostStatus.fkeyhost=OLD.fkeyhost;
	END IF;
	RETURN OLD;
END;
$$ LANGUAGE 'plpgsql';

DROP TRIGGER IF EXISTS jobassignment_insert_trigger ON jobassignment;
CREATE TRIGGER jobassignment_insert_trigger
BEFORE INSERT
ON jobassignment
FOR EACH ROW
EXECUTE PROCEDURE jobassignment_insert();

DROP TRIGGER IF EXISTS jobassignment_update_trigger ON jobassignment;
CREATE TRIGGER jobassignment_update_trigger
BEFORE UPDATE
ON jobassignment
FOR EACH ROW
EXECUTE PROCEDURE jobassignment_update();

DROP TRIGGER IF EXISTS jobassignment_delete_trigger ON jobassignment;
CREATE TRIGGER jobassignment_delete_trigger
BEFORE DELETE
ON jobassignment
FOR EACH ROW
EXECUTE PROCEDURE jobassignment_delete();
