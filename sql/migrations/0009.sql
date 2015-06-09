CREATE OR REPLACE FUNCTION jobtaskassignment_update()
  RETURNS trigger AS
$BODY$
DECLARE
    cancelledStatusKey int;
    busyStatusKey int;
BEGIN
    IF OLD.memory IS NOT NULL AND OLD.memory > NEW.memory THEN
        NEW.memory := OLD.memory;
    END IF;
    RETURN NEW;
END;
$BODY$
  LANGUAGE plpgsql VOLATILE
  COST 100;
