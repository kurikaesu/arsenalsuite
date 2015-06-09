CREATE OR REPLACE FUNCTION cancel_job_assignments(_keyjob integer, _reason text, _nextstate text)
  RETURNS void AS
$BODY$
DECLARE
    _statusKey int;
    _job job;
BEGIN
    SELECT INTO _statusKey keyjobassignmentstatus FROM jobassignmentstatus WHERE status=_reason;
    SELECT INTO _job * FROM job WHERE keyjob=_keyjob;
        
    IF _job.packettype = 'preassigned' THEN
        UPDATE JobTask SET status=_nextstate WHERE fkeyjob=_keyjob AND status IN ('assigned','busy');
    ELSE
        UPDATE JobTask SET status=_nextstate, fkeyhost=NULL WHERE fkeyjob=_keyjob AND status IN ('assigned','busy');
    END IF;

    UPDATE JobTaskAssignment SET fkeyjobassignmentstatus=_statusKey, ended=now()
        WHERE fkeyjobassignment IN(SELECT keyjobassignment FROM JobAssignment WHERE fkeyjob=_keyjob) 
        AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus from jobassignmentstatus WHERE status IN ('ready','copy','busy'));

    UPDATE JobAssignment SET fkeyjobassignmentstatus=_statusKey, ended=now()
        WHERE fkeyjob=_keyjob 
        AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus from jobassignmentstatus WHERE status IN ('ready','copy','busy'));
END;
$BODY$
  LANGUAGE plpgsql VOLATILE
  COST 100;
