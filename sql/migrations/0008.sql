CREATE OR REPLACE FUNCTION jobassignment_update()
  RETURNS trigger AS
$BODY$
DECLARE
    oldstatusval int := 0;
    newstatusval int := 0;
BEGIN
    IF NEW.fkeyhost != OLD.fkeyhost THEN
        -- TODO: Raise some kind of error
        RAISE NOTICE 'JobAssignment.fkeyhost is immutable';
    END IF;

    IF OLD.fkeyjobassignmentstatus != NEW.fkeyjobassignmentstatus THEN
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

        IF NEW.fkeyjobassignmentstatus > 4 THEN
            -- somethere terrible has happened so trickle down to all active JobAssignmentTasks
            --RAISE NOTICE 'JA bugging out, let JTAs know';

            UPDATE JobTaskAssignment 
            SET fkeyjoberror = NEW.fkeyjoberror
            WHERE fkeyjobassignment=NEW.keyjobassignment;
        END IF;
        IF NEW.fkeyjobassignmentstatus > 3 THEN
            NEW.ended = now();
        END IF;
    END IF;

    RETURN NEW;
END;
$BODY$
  LANGUAGE plpgsql VOLATILE
  COST 100;
