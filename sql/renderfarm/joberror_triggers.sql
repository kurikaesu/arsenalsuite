
-- Increment JobError.count for every occurrence until the error is "cleared"
-- Remove count from jobstatus when error gets cleared
CREATE OR REPLACE FUNCTION job_error_increment() returns TRIGGER AS $$
BEGIN
	IF (NEW.cleared IS NULL OR NEW.cleared=false) AND (NEW.count > coalesce(OLD.count,0)) THEN
		UPDATE jobstatus SET errorcount=errorcount+1 where fkeyjob=NEW.fkeyjob;
	END IF;

	IF (NEW.cleared = true) AND (coalesce(OLD.cleared,false)=false) THEN
		UPDATE jobstatus SET errorcount=errorcount-NEW.count WHERE fkeyjob=NEW.fkeyjob;
	END IF;
	RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION job_error_insert() returns TRIGGER AS $$
BEGIN
	UPDATE jobstatus SET errorcount=errorcount+1 WHERE fkeyjob=NEW.fkeyjob;
	RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS job_error_update_trigger ON joberror;
DROP TRIGGER IF EXISTS job_error_insert_trigger ON joberror;

CREATE TRIGGER job_error_update_trigger AFTER UPDATE ON joberror FOR EACH ROW EXECUTE PROCEDURE job_error_increment();
CREATE TRIGGER job_error_insert_trigger AFTER INSERT ON joberror FOR EACH ROW EXECUTE PROCEDURE job_error_insert();

