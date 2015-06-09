
DROP FUNCTION IF EXISTS Job_GatherStats(int);
DROP TYPE IF EXISTS Job_GatherStats_Result;

CREATE TYPE Job_GatherStats_Result AS (mintasktime interval, avgtasktime interval, maxtasktime interval, totaltasktime interval, taskcount int,
	minloadtime interval, avgloadtime interval, maxloadtime interval, totalloadtime interval, loadcount int,
	minerrortime interval, avgerrortime interval, maxerrortime interval, totalerrortime interval, errorcount int,
	mincopytime interval, avgcopytime interval, maxcopytime interval, totalcopytime interval, copycount int, totaltime interval);

CREATE OR REPLACE FUNCTION Job_GatherStats(_keyjob int) RETURNS Job_GatherStats_Result AS $$
DECLARE
	ret Job_GatherStats_Result;
BEGIN
	SELECT INTO ret.mintasktime, ret.avgtasktime, ret.maxtasktime, ret.totaltasktime, ret.taskcount
				min(ja.ended-ja.started) as minloadtime,
				avg(ja.ended-ja.started) as avgloadtime,
				max(ja.ended-ja.started) as maxloadtime,
				sum(ja.ended-ja.started) as totalloadtime,
				count(*) as loadcount
			FROM jobtaskassignment jta
			JOIN jobassignmentstatus jas ON jas.keyjobassignmentstatus=jta.fkeyjobassignmentstatus
			JOIN jobassignment ja ON ja.keyjobassignment=jta.fkeyjobassignment
			WHERE
				ja.fkeyjob=_keyjob
				AND jas.status='done'
			GROUP BY jas.status;
	-- Load Times
	SELECT INTO ret.minloadtime, ret.avgloadtime, ret.maxloadtime, ret.totalloadtime, ret.loadcount
				min(ja.ended-ja.started) as minloadtime,
				avg(ja.ended-ja.started) as avgloadtime,
				max(ja.ended-ja.started) as maxloadtime,
				sum(ja.ended-ja.started) as totalloadtime,
				count(*) as loadcount
			FROM jobtaskassignment jta
			JOIN jobassignmentstatus jas ON jas.keyjobassignmentstatus=jta.fkeyjobassignmentstatus
			JOIN jobassignment ja ON ja.keyjobassignment=jta.fkeyjobassignment
			WHERE
				ja.fkeyjob=_keyjob
				AND jas.status='copy'
			GROUP BY jas.status;
	-- Error Times
	SELECT INTO ret.minerrortime, ret.avgerrortime, ret.maxerrortime, ret.totalerrortime, ret.errorcount
				min(ja.ended-ja.started) as minloadtime,
				avg(ja.ended-ja.started) as avgloadtime,
				max(ja.ended-ja.started) as maxloadtime,
				sum(ja.ended-ja.started) as totalloadtime,
				count(*) as loadcount
			FROM jobtaskassignment jta
			JOIN jobassignmentstatus jas ON jas.keyjobassignmentstatus=jta.fkeyjobassignmentstatus
			JOIN jobassignment ja ON ja.keyjobassignment=jta.fkeyjobassignment
			WHERE
				ja.fkeyjob=_keyjob
				AND jas.status='error'
			GROUP BY jas.status;
	SELECT INTO ret.mincopytime, ret.avgcopytime, ret.maxcopytime, ret.totalcopytime, ret.copycount
				min(ja.ended-ja.started) as minloadtime,
				avg(ja.ended-ja.started) as avgloadtime,
				max(ja.ended-ja.started) as maxloadtime,
				sum(ja.ended-ja.started) as totalloadtime,
				count(*) as loadcount
			FROM jobtaskassignment jta
			JOIN jobassignmentstatus jas ON jas.keyjobassignmentstatus=jta.fkeyjobassignmentstatus
			JOIN jobassignment ja ON ja.keyjobassignment=jta.fkeyjobassignment
			WHERE
				ja.fkeyjob=_keyjob
				AND jas.status='copy'
			GROUP BY jas.status;
	ret.totaltime := '0 minutes'::interval;
	IF ret.totaltasktime IS NOT NULL THEN
		ret.totaltime := ret.totaltime + ret.totaltasktime;
	END IF;
	IF ret.totalloadtime IS NOT NULL THEN
		ret.totaltime := ret.totaltime + ret.totalloadtime;
	END IF;
	IF ret.totalerrortime IS NOT NULL THEN
		ret.totaltime := ret.totaltime + ret.totalerrortime;
	END IF;
	RETURN ret;
END;
$$ LANGUAGE 'plpgsql';

