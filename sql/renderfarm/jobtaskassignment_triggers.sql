
CREATE OR REPLACE FUNCTION jobtaskassignment_update() RETURNS trigger AS $$
DECLARE
	cancelledStatusKey int;
	busyStatusKey int;
BEGIN
	SELECT INTO cancelledStatusKey keyjobassignmentstatus FROM jobassignmentstatus WHERE status='cancelled';
	SELECT INTO busyStatusKey keyjobassignmentstatus FROM jobassignmentstatus WHERE status='busy';
	IF NEW.fkeyjobassignmentstatus = cancelledStatusKey AND OLD.fkeyjobassignmentstatus = busyStatusKey THEN
		UPDATE JobAssignment SET fkeyjobassignmentstatus=cancelledStatusKey WHERE keyjobassignment=NEW.fkeyjobassignment;
	END IF;
	RETURN NEW;
END;
$$ LANGUAGE 'plpgsql';

DROP TRIGGER IF EXISTS jobtaskassignment_update_trigger ON jobtaskassignment;
CREATE TRIGGER jobtaskassignment_update_trigger
BEFORE UPDATE
ON jobtaskassignment
FOR EACH ROW
EXECUTE PROCEDURE jobtaskassignment_update();
