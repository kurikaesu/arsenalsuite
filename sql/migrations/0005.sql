CREATE OR REPLACE FUNCTION after_update_jobtask()
  RETURNS trigger AS
$BODY$
DECLARE
BEGIN
    -- Update Job Counters when a tasks status changes
    IF (NEW.status != coalesce(OLD.status,'')) THEN
        PERFORM update_job_task_counts(NEW.fkeyjob);
    END IF;
RETURN new;
END;
$BODY$
  LANGUAGE plpgsql VOLATILE
  COST 100;
