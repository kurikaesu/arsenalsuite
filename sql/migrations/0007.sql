CREATE OR REPLACE FUNCTION job_update()
  RETURNS trigger AS
$BODY$
DECLARE
    stat RECORD;
BEGIN
    -- Set Job.taskscount and Job.tasksunassigned
    IF NEW.status IN ('verify','verify-suspended') THEN
        PERFORM update_job_task_counts(NEW.keyjob);
    END IF;

    IF NEW.status IN ('verify', 'suspended', 'deleted') AND OLD.status != 'submit' THEN
        PERFORM cancel_job_assignments(NEW.keyjob, 'cancelled', 'new');
    END IF;

    IF (NEW.fkeyjobstat IS NOT NULL) THEN
        IF (NEW.status='started' AND OLD.status='ready') THEN
            UPDATE jobstat SET started=NOW() WHERE keyjobstat=NEW.fkeyjobstat AND started IS NULL;
        END IF;
        IF (NEW.status='ready' AND OLD.status='done') THEN
            SELECT INTO stat * from jobstat where keyjobstat=NEW.fkeyjobstat;
            INSERT INTO jobstat (fkeyelement, fkeyproject, taskCount, fkeyusr, started, pass, fkeyjobtype) VALUES (stat.fkeyelement, stat.fkeyproject, stat.taskCount, stat.fkeyusr, NOW(), stat.pass, NEW.fkeyjobtype);
            SELECT INTO NEW.fkeyjobstat currval('jobstat_keyjobstat_seq');
        END IF;
        IF (NEW.status IN ('done','deleted') AND OLD.status IN ('ready','started','suspended') ) THEN
            UPDATE jobstat
            SET
                fkeyjobtype=NEW.fkeyjobtype,
                ended=NOW(),
                errorcount=iq.errorcount,
                totaltasktime=iq.totaltasktime, mintasktime=iq.mintasktime, maxtasktime=iq.maxtasktime, avgtasktime=iq.avgtasktime,
                totalcputime=iq.totalcputime, mincputime=iq.mincputime, maxcputime=iq.maxcputime, avgcputime=iq.avgcputime,
                totalloadtime=iq.totalloadtime,
                totalerrortime=iq.totalerrortime,
                totalcanceltime=iq.totalcanceltime,
                totalcopytime=iq.totalcopytime,
                avgefficiency = (iq.totalcputime / iq.totaltasktime) / NEW.slots
            FROM (select * FROM Job_GatherStats_2(NEW.keyjob)) as iq
            WHERE keyjobstat=NEW.fkeyjobstat;
            UPDATE jobstat
            SET
                taskcount=iq.taskscount,
                taskscompleted=iq.tasksdone
            FROM (select jobstatus.taskscount, jobstatus.tasksdone FROM jobstatus WHERE jobstatus.fkeyjob=NEW.keyjob) as iq
            WHERE keyjobstat=NEW.fkeyjobstat;
        END IF;
    END IF;

    RETURN NEW;
END;
$BODY$
  LANGUAGE plpgsql VOLATILE
  COST 100;
