CREATE OR REPLACE FUNCTION update_job_stats(_keyjob integer)
  RETURNS void AS
$BODY$
DECLARE
        _totaltime bigint := 0;
        _cputime bigint := 0;
        _byteswrite bigint := 0;
        _bytesread bigint := 0;
        _opswrite bigint := 0;
        _opsread bigint := 0;
        _avgtasktime bigint := 0;
        _avgmem bigint := 0;
        _efficiency float := 0.0;

BEGIN

SELECT INTO _avgtasktime, _avgmem GREATEST(0, 
                AVG( GREATEST(0, 
                              EXTRACT(epoch FROM (coalesce(jobtaskassignment.ended,now())-jobtaskassignment.started))
                             )
                   )
                )::int,
                AVG( jobtaskassignment.memory )::int
        FROM JobTaskAssignment 
        INNER JOIN JobTask ON JobTask.fkeyjobtaskassignment=jobtaskassignment.keyjobtaskassignment 
        WHERE fkeyjob=_keyjob
        AND jobtaskassignment.started IS NOT NULL;

SELECT INTO _totaltime, _cputime, _byteswrite, _bytesread, _opswrite, _opsread
        extract(epoch from sum((coalesce(ended,now())-started)))::bigint,
        sum(usertime+systime),
        sum(byteswrite),
        sum(bytesread),
        sum(opswrite),
        sum(opsread)
        FROM jobassignment ja
        WHERE fkeyjob = _keyjob AND fkeyjobassignmentstatus < 5;

        UPDATE jobstatus SET
        averagememory = _avgmem,
        tasksaveragetime = _avgtasktime,
        totaltime = _totaltime,
        cputime = _cputime,
        byteswrite = _byteswrite,
        bytesread = _bytesread,
        opswrite = _opswrite,
        opsread = _opsread,
        efficiency = _cputime::double precision / GREATEST(_totaltime*1000,1)::double precision
    WHERE fkeyjob = _keyjob;

    RETURN;
END;
$BODY$
  LANGUAGE plpgsql VOLATILE
  COST 100;
