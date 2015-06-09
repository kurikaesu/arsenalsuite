
CREATE OR REPLACE FUNCTION update_single_job_health( j job ) RETURNS VOID AS $$
DECLARE
	history hosthistory_status_summary_return;
	successtime interval;
	errortime interval;
	totaltime interval;
	job_health float;
BEGIN
	--RAISE NOTICE 'Updating health of job % %', j.job, j.keyjob;
	successtime := 0;
	errortime := 0;
	FOR history IN SELECT * FROM hosthistory_status_summary( 'hosthistory WHERE fkeyjob=' || quote_literal(j.keyjob) ) LOOP
		IF history.wasted = true THEN
			errortime := errortime + history.total_time;
		ELSE
			IF history.status = 'busy' AND history.loading = false THEN
				successtime := history.total_time;
			END IF;
		END IF;
	END LOOP;
	totaltime := 0;
	IF successtime IS NOT NULL THEN
		totaltime := successtime;
	ELSE
		successtime := 0;
	END IF;
	IF errortime IS NOT NULL THEN
		totaltime := totaltime + errortime;
	END IF;
	IF extract( epoch from totaltime ) > 0 THEN
		job_health := successtime / totaltime;
	ELSE
		job_health := 1.0;
	END IF;
	--RAISE NOTICE 'Got Success Time %, Error Time %, Total Time %, Health %', successtime, errortime, totaltime, job_health;
	UPDATE jobstatus SET health = job_health where jobstatus.fkeyjob=j.keyjob;
	RETURN;
END;
$$ LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION update_job_health_by_key( jobkey int ) RETURNS VOID AS $$
DECLARE
	j job;
BEGIN
	SELECT INTO j * FROM job WHERE keyjob=jobkey;
	PERFORM update_single_job_health( j );
	RETURN;
END;
$$ LANGUAGE 'PLPGSQL';

CREATE OR REPLACE FUNCTION update_job_health() RETURNS VOID AS $$
DECLARE
	j job;
BEGIN
	FOR j IN SELECT * FROM job WHERE status='started' ORDER BY keyjob ASC LOOP
		PERFORM update_single_job_health( j );
	END LOOP;
	RETURN;
END;
$$ LANGUAGE 'PLPGSQL';
