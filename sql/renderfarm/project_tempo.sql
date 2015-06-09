
CREATE OR REPLACE FUNCTION update_project_tempo() RETURNS void AS $$
	DECLARE
		cur RECORD;
		hosts RECORD;
		newtempo float8;
	BEGIN
		SELECT INTO hosts count(*) from hoststatus where slavestatus in ('ready','assigned','busy','copy');
		FOR cur IN
			SELECT projecttempo.fkeyproject, max(projecttempo.tempo) as tempo, sum(jobstatus.hostsonjob) as hostcount, count(job.keyjob) as jobs
			FROM projecttempo LEFT JOIN job ON projecttempo.fkeyproject=job.fkeyproject LEFT JOIN jobstatus ON jobstatus.fkeyjob=job.keyjob
			AND status IN('started','ready')
			GROUP BY projecttempo.fkeyproject
			ORDER BY hostcount ASC
		LOOP
			IF cur.hostcount IS NULL OR cur.hostcount=0 THEN
				DELETE FROM projecttempo WHERE fkeyproject=cur.fkeyproject;
			ELSE
				UPDATE projecttempo SET tempo=(cur.hostcount::float8/hosts.count)::numeric(4,3)::float8 where fkeyproject=cur.fkeyproject;
			END IF;
		END LOOP;
		FOR cur IN
			SELECT job.fkeyproject, sum(jobstatus.hostsonjob) as hostcount
			FROM job INNER JOIN jobstatus ON job.keyjob=jobstatus.fkeyjob
			WHERE status IN('started','ready') AND fkeyproject NOT IN (SELECT fkeyproject FROM projecttempo) AND fkeyproject IS NOT NULL
			GROUP BY job.fkeyproject
		LOOP
			newtempo := (cur.hostcount::float8/hosts.count)::numeric(4,3)::float8;
			IF newtempo > .001 THEN
				INSERT INTO projecttempo (fkeyproject, tempo) values (cur.fkeyproject,newtempo);
			END IF;
		END LOOP;
		RETURN;
	END;
$$ LANGUAGE plpgsql;
