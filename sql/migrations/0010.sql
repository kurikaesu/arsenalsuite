CREATE OR REPLACE FUNCTION cancel_job_assignment(_keyjobassignment integer)
  RETURNS void AS
$BODY$
DECLARE
    cancelledStatusKey int;
    _packettype text;
BEGIN
    SELECT INTO cancelledStatusKey keyjobassignmentstatus FROM jobassignmentstatus WHERE status='cancelled';
    SELECT INTO _packettype packettype FROM job WHERE keyjob=assignment.fkeyjob;

    IF _packettype = 'preassigned' THEN
        UPDATE JobTask SET status='new' 
            WHERE fkeyjobtaskassignment IN (SELECT keyjobtaskassignment FROM jobtaskassignment WHERE fkeyjobassignment=_keyjobassignment) 
            AND status IN ('assigned','busy');
    ELSE
        UPDATE JobTask SET status='new', fkeyhost=NULL WHERE fkeyjobtaskassignment IN (SELECT keyjobtaskassignment FROM jobtaskassignment WHERE fkeyjobassignment=_keyjobassignment) AND status IN ('assigned','busy');
    END IF;

    UPDATE JobTaskAssignment SET fkeyjobassignmentstatus=cancelledStatusKey WHERE fkeyjobassignment=_keyjobassignment AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus from jobassignmentstatus WHERE status IN ('ready','copy','busy'));

    -- Dont set it to cancelled if it already cancelled or error
    UPDATE JobAssignment SET fkeyjobassignmentstatus=cancelledStatusKey WHERE keyjobassignment=_keyjobassignment AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus from jobassignmentstatus WHERE status IN ('ready','copy','busy'));
END;
$BODY$
  LANGUAGE plpgsql VOLATILE
  COST 100;
