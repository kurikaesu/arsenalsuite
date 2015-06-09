CREATE OR REPLACE FUNCTION return_slave_tasks(_keyhost int4, commithoststatus bool, preassigned bool)
  RETURNS void AS
$BODY$
BEGIN
	IF commitHostStatus THEN
		UPDATE hoststatus SET slaveframes='', fkeyjob=NULL WHERE fkeyhost=_keyhost;
	END IF;
	IF preassigned THEN
		UPDATE jobtask SET status = 'new', started=NULL, memory=NULL, fkeyjobcommandhistory=NULL WHERE fkeyhost=_keyhost AND status IN ('assigned','busy');
	ELSE
		UPDATE jobtask SET status = 'new', fkeyhost=NULL, started=NULL, memory=NULL, fkeyjobcommandhistory=NULL WHERE fkeyhost=_keyhost AND status IN ('assigned','busy');
	END IF;
END;
$BODY$
  LANGUAGE 'plpgsql' VOLATILE;

CREATE OR REPLACE FUNCTION return_slave_tasks_2(_keyhost int4)
  RETURNS void AS
$BODY$
DECLARE
	j job;
BEGIN
	FOR j IN SELECT * from Job WHERE keyjob IN (SELECT fkeyjob FROM JobTask WHERE fkeyhost=_keyhost AND status IN ('assigned','busy')) LOOP
		IF j.packettype = 'preassigned' THEN
			UPDATE jobtask SET status = 'new' WHERE fkeyhost=_keyhost AND fkeyjob=j.keyjob AND status IN ('assigned','busy');
		ELSE
			UPDATE jobtask SET status = 'new', fkeyhost=NULL WHERE fkeyhost=_keyhost AND fkeyjob=j.keyjob AND status IN ('assigned','busy');
		END IF;
	END LOOP;
END;
$BODY$
  LANGUAGE 'plpgsql' VOLATILE;

CREATE OR REPLACE FUNCTION return_slave_tasks_3(_keyhost int4)
  RETURNS void AS
$BODY$
DECLARE
	ja jobassignment;
	cancelledStatusKey int;
BEGIN
	PERFORM return_slave_tasks_2(_keyhost);
	
	SELECT INTO cancelledStatusKey keyjobassignmentstatus FROM jobassignmentstatus WHERE status='cancelled';
	UPDATE JobAssignment SET fkeyjobassignmentstatus=cancelledStatusKey WHERE fkeyhost=_keyhost AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus FROM jobassignmentstatus WHERE status IN ('ready','copy','busy'));
	UPDATE JobTaskAssignment SET fkeyjobassignmentstatus=cancelledStatusKey WHERE fkeyjobassignment IN (SELECT keyjobassignment FROM jobassignment WHERE fkeyhost=_keyhost) AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus FROM jobassignmentstatus WHERE status IN ('ready','copy','busy'));
END;
$BODY$
  LANGUAGE 'plpgsql' VOLATILE;
