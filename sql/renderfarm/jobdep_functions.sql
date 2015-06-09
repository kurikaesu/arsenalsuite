
CREATE OR REPLACE FUNCTION update_job_deps(_keyjob int4) RETURNS void AS
$BODY$
DECLARE
    childDepRec RECORD;
    jobDepRec RECORD;
    newstatus text;
BEGIN
    FOR jobDepRec IN
        SELECT job.keyjob as childKey
        FROM jobdep
        JOIN job ON jobdep.fkeyjob = job.keyjob
        WHERE jobdep.deptype = 1 AND fkeydep = _keyjob
    LOOP
        newstatus := 'ready';
        FOR childDepRec IN
            SELECT status
            FROM jobdep
            JOIN job ON jobdep.fkeydep = job.keyjob
            WHERE keyjob != _keyjob AND fkeyjob = jobdep.keyjob
        LOOP
            IF childDepRec.status != 'done' AND childDepRec.status != 'deleted' THEN
                newstatus := 'holding';
            END IF;
        END LOOP;
        UPDATE JobStatus SET status = newstatus WHERE fkeyjob = jobDepRec.childKey;
    END LOOP;
    RETURN;
END;
$BODY$
LANGUAGE 'plpgsql' VOLATILE;

CREATE OR REPLACE FUNCTION update_job_links(_keyjob int4) RETURNS void AS
$BODY$
BEGIN
UPDATE jobtask SET status = 'new' WHERE keyjobtask IN (
        SELECT jobtask.keyjobtask as childDep
        FROM jobdep
        JOIN jobtask on jobtask.fkeyjob = jobdep.fkeyjob
        WHERE jobdep.deptype = 2 AND jobtask.status = 'holding' AND fkeydep = _keyjob
);
END;
$BODY$
LANGUAGE 'plpgsql' VOLATILE;

