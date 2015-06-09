--
-- PostgreSQL database dump
--

SET statement_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = off;
SET check_function_bodies = false;
SET client_min_messages = warning;
SET escape_string_warning = off;

SET search_path = public, pg_catalog;

--
-- Name: hosthistory_status_percentages_return; Type: TYPE; Schema: public; Owner: farmer
--

CREATE TYPE hosthistory_status_percentages_return AS (
	status text,
	loading boolean,
	wasted boolean,
	total_time interval,
	total_perc double precision,
	non_idle_perc double precision,
	non_idle_non_wasted_perc double precision
);


ALTER TYPE public.hosthistory_status_percentages_return OWNER TO farmer;

--
-- Name: hosthistory_status_summary_return; Type: TYPE; Schema: public; Owner: farmer
--

CREATE TYPE hosthistory_status_summary_return AS (
	status text,
	loading boolean,
	wasted boolean,
	count integer,
	total_time interval,
	avg_time interval,
	max_time interval,
	min_time interval
);


ALTER TYPE public.hosthistory_status_summary_return OWNER TO farmer;

--
-- Name: hosthistory_user_slave_summary_return; Type: TYPE; Schema: public; Owner: farmer
--

CREATE TYPE hosthistory_user_slave_summary_return AS (
	usr text,
	host text,
	hours numeric
);


ALTER TYPE public.hosthistory_user_slave_summary_return OWNER TO farmer;

--
-- Name: job_gatherstats_result; Type: TYPE; Schema: public; Owner: farmer
--

CREATE TYPE job_gatherstats_result AS (
	mintasktime interval,
	avgtasktime interval,
	maxtasktime interval,
	totaltasktime interval,
	taskcount integer,
	minloadtime interval,
	avgloadtime interval,
	maxloadtime interval,
	totalloadtime interval,
	loadcount integer,
	minerrortime interval,
	avgerrortime interval,
	maxerrortime interval,
	totalerrortime interval,
	errorcount integer,
	mincopytime interval,
	avgcopytime interval,
	maxcopytime interval,
	totalcopytime interval,
	copycount integer,
	totaltime interval
);


ALTER TYPE public.job_gatherstats_result OWNER TO farmer;

--
-- Name: job_gatherstats_result_2; Type: TYPE; Schema: public; Owner: farmer
--

CREATE TYPE job_gatherstats_result_2 AS (
	mintasktime interval,
	avgtasktime interval,
	maxtasktime interval,
	totaltasktime interval,
	mincputime interval,
	avgcputime interval,
	maxcputime interval,
	totalcputime interval,
	taskcount integer,
	totalloadtime interval,
	loadcount integer,
	totalerrortime interval,
	errorcount integer,
	totalcopytime interval,
	copycount integer,
	totalcanceltime interval,
	cancelcount integer,
	totaltime interval
);


ALTER TYPE public.job_gatherstats_result_2 OWNER TO farmer;

--
-- Name: passpreseedret; Type: TYPE; Schema: public; Owner: postgres
--

CREATE TYPE passpreseedret AS (
	slots integer,
	averagememory integer,
	maxmemory integer
);


ALTER TYPE public.passpreseedret OWNER TO postgres;

--
-- Name: waywardhostret; Type: TYPE; Schema: public; Owner: farmer
--

CREATE TYPE waywardhostret AS (
	keyhost integer,
	reason integer
);


ALTER TYPE public.waywardhostret OWNER TO farmer;

--
-- Name: after_update_jobtask(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION after_update_jobtask() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
DECLARE
BEGIN
    -- Update Job Counters when a tasks status changes
    IF (NEW.status != coalesce(OLD.status,'')) THEN
        PERFORM update_job_task_counts(NEW.fkeyjob);
    END IF;
RETURN new;
END;
$$;


ALTER FUNCTION public.after_update_jobtask() OWNER TO farmer;

--
-- Name: are_ontens_complete(integer); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION are_ontens_complete(_fkeyjob integer) RETURNS boolean
    LANGUAGE plpgsql COST 10
    AS $$
DECLARE
	task RECORD;
	stripe int := 10;
	cur_pos int := 0;
	is_complete BOOLEAN := TRUE;
BEGIN

	FOR task IN SELECT * FROM JobTask WHERE JobTask.fkeyjob=_fkeyjob ORDER BY jobtask ASC LOOP
		cur_pos := cur_pos + 1;
		IF (cur_pos = 1 OR cur_pos % stripe = 0) AND task.status != 'done' THEN
			--RAISE NOTICE 'RETURN NEXT pos % is task %', cur_pos, task.jobtask;
			-- one of the on-tens is not complete, so NO
			is_complete := FALSE;
		END IF;
		--RAISE NOTICE 'pos % is task % with stripe % and task status %. start_task is %', cur_pos, task.jobtask, (cur_pos % stripe), task.status, start_task;
	END LOOP;

	RETURN is_complete;
END;$$;


ALTER FUNCTION public.are_ontens_complete(_fkeyjob integer) OWNER TO postgres;

--
-- Name: are_ontens_dispatched(integer); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION are_ontens_dispatched(_fkeyjob integer) RETURNS boolean
    LANGUAGE plpgsql COST 10
    AS $$
DECLARE
	task RECORD;
	stripe int := 10;
	cur_pos int := 0;
	is_running BOOLEAN := TRUE;
BEGIN

	FOR task IN SELECT * FROM JobTask WHERE JobTask.fkeyjob=_fkeyjob ORDER BY jobtask ASC LOOP
		cur_pos := cur_pos + 1;
		IF (cur_pos = 1 OR cur_pos % stripe = 0) AND task.status = 'new' THEN
			--RAISE NOTICE 'RETURN NEXT pos % is task %', cur_pos, task.jobtask;
			-- one of the on-tens is not complete, so NO
			is_running := FALSE;
		END IF;
		--RAISE NOTICE 'pos % is task % with stripe % and task status %. start_task is %', cur_pos, task.jobtask, (cur_pos % stripe), task.status, start_task;
	END LOOP;

	RETURN is_running;
END;$$;


ALTER FUNCTION public.are_ontens_dispatched(_fkeyjob integer) OWNER TO postgres;

--
-- Name: assign_single_host(integer, integer, integer[]); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION assign_single_host(_keyjob integer, _keyhost integer, _tasks integer[]) RETURNS integer
    LANGUAGE plpgsql
    AS $$
DECLARE
    _hostStatus hoststatus;
    _readyStatus integer;
    _jobstatus jobstatus;
    _job job;
    _minMem integer;
    _keyjobassignment integer;
    _keyjobtaskassignment integer;
    _keytask integer;
    _assignedTasks integer;
BEGIN

_assignedTasks := 0;
SELECT INTO _hostStatus * FROM hoststatus WHERE fkeyHost = _keyhost;

IF _hostStatus.slaveStatus != 'ready' THEN
    ROLLBACK;
    RAISE NOTICE 'Host is no longer ready for frames(status is ) returning';
    return 0;
END IF;

SELECT INTO _job * FROM job WHERE keyjob = _keyjob;

_minMem := _job.minmemory;
IF _minMem = 0 THEN
    SELECT INTO _jobstatus * FROM jobstatus WHERE fkeyjob = _keyjob;
    _minMem := _jobstatus.averagememory;
END IF;
IF _hostStatus.availablememory*1024 - _minMem <= 0 THEN
    RAISE NOTICE 'Host does not have %, memory required %', _hostStatus.availablememory, _minMem;
    ROLLBACK;
    return 0;
END IF;
UPDATE hoststatus SET availablememory = ((availablememory * 1024) - _minMem)/1024 WHERE fkeyhost = _keyhost;

SELECT INTO _readyStatus keyjobassignmentstatus FROM jobassignmentstatus WHERE status = 'ready';
INSERT INTO jobassignment (fkeyjob, fkeyhost, fkeyjobassignmentstatus)
    VALUES (_keyjob, _keyhost, _readyStatus)
    RETURNING keyjobassignment INTO _keyjobassignment;

FOR i IN 1..array_upper(_tasks,1) LOOP
    _keytask := _tasks[i];
    _assignedTasks := _assignedTasks + 1;
    INSERT INTO jobtaskassignment (fkeyjobtask, fkeyjobassignment, fkeyjobassignmentstatus)
        VALUES (_keytask, _keyjobassignment, _readyStatus)
        RETURNING keyjobtaskassignment INTO _keyjobtaskassignment;
    UPDATE jobtask SET fkeyjobtaskassignment = _keyjobtaskassignment, status = 'assigned', fkeyhost = _keyhost 
        WHERE keyjobtask = _keytask;
END LOOP;

IF _job.maxhosts > 0 THEN
    UPDATE jobstatus SET hostsonjob=hostsonjob+1 WHERE fkeyjob = _keyjob;
END IF;

return _assignedTasks;

END;
$$;


ALTER FUNCTION public.assign_single_host(_keyjob integer, _keyhost integer, _tasks integer[]) OWNER TO farmer;

--
-- Name: assign_single_host(integer, integer, integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION assign_single_host(_keyjob integer, _keyhost integer, _packetsize integer) RETURNS integer
    LANGUAGE plpgsql
    AS $$
DECLARE
    _hostStatus hoststatus;
    _readyStatus integer;
    _jobstatus jobstatus;
    _job job;
    _minMem integer;
    _keyjobassignment integer;
    _keyjobtaskassignment integer;
    _keytask integer;
    _assignedTasks integer;
    _task jobtask;
    _tasksql text;
BEGIN

_assignedTasks := 0;
SELECT INTO _hostStatus * FROM hoststatus WHERE fkeyHost = _keyhost;

IF _hostStatus.slaveStatus != 'ready' THEN
    ROLLBACK;
    RAISE EXCEPTION 'Host is no longer ready for frames(status is ) returning';
    return 0;
END IF;

SELECT INTO _job * FROM job WHERE keyjob = _keyjob;

_minMem := _job.minmemory;
IF _minMem = 0 THEN
    SELECT INTO _jobstatus * FROM jobstatus WHERE fkeyjob = _keyjob;
    _minMem := _jobstatus.averagememory;
END IF;
IF _hostStatus.availablememory*1024 - _minMem <= 0 THEN
    RAISE EXCEPTION 'Host does not have %, memory required %', _hostStatus.availablememory, _minMem;
    ROLLBACK;
    return 0;
END IF;
UPDATE hoststatus SET availablememory = ((availablememory * 1024) - _minMem)/1024 WHERE fkeyhost = _keyhost;

SELECT INTO _readyStatus keyjobassignmentstatus FROM jobassignmentstatus WHERE status = 'ready';
INSERT INTO jobassignment (fkeyjob, fkeyhost, fkeyjobassignmentstatus)
    VALUES (_keyjob, _keyhost, _readyStatus)
    RETURNING keyjobassignment INTO _keyjobassignment;


IF _job.packetType = 'random' THEN
	_tasksql := 'SELECT keyjobtask FROM jobtask WHERE fkeyjob = ' || _keyjob || ' AND status=''new'' ORDER BY random() ASC LIMIT ' || _packetSize;
ELSIF _job.packetType = 'preassigned' THEN
	_tasksql := 'SELECT keyjobtask FROM jobtask WHERE fkeyjob = ' || _keyjob || ' AND fkeyhost=' || _keyhost || ' AND status=''new''';
ELSIF _job.packetType = 'continuous' THEN
	_tasksql := 'SELECT * FROM get_continuous_tasks(' || _keyjob || ', ' || _packetSize || ')';
ELSE
	_tasksql := 'SELECT keyjobtask FROM jobtask WHERE fkeyjob = ' || _keyjob || ' AND status=''new'' ORDER BY jobtask ASC LIMIT ' || _packetSize;
END IF;

FOR _keytask IN EXECUTE(_tasksql) LOOP  
    _assignedTasks := _assignedTasks + 1;
    INSERT INTO jobtaskassignment (fkeyjobtask, fkeyjobassignment, fkeyjobassignmentstatus)
        VALUES (_keytask, _keyjobassignment, _readyStatus)
        RETURNING keyjobtaskassignment INTO _keyjobtaskassignment;
    UPDATE jobtask SET fkeyjobtaskassignment = _keyjobtaskassignment, status = 'assigned', fkeyhost = _keyhost 
        WHERE keyjobtask = _keytask;
END LOOP;

IF _job.maxhosts > 0 THEN
    UPDATE jobstatus SET hostsonjob=hostsonjob+1 WHERE fkeyjob = _keyjob;
END IF;

return _assignedTasks;

END;
$$;


ALTER FUNCTION public.assign_single_host(_keyjob integer, _keyhost integer, _packetsize integer) OWNER TO farmer;

--
-- Name: assign_single_host_2(integer, integer, integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION assign_single_host_2(_keyjob integer, _keyhost integer, _packetsize integer) RETURNS SETOF integer
    LANGUAGE plpgsql
    AS $$
DECLARE
    --_hostStatus hoststatus;
    --_readyStatus integer;
    _jobstatus jobstatus;
    _job job;
    _minMem integer;
    _keyjobassignment integer;
    _keyjobtaskassignment integer;
    _keytask integer;
    _task jobtask;
    _tasksql text;
    _assigned integer;
BEGIN

--SELECT INTO _hostStatus availableMemory FROM hoststatus WHERE fkeyHost = _keyhost;

--IF _hostStatus.slaveStatus != 'ready' THEN
--    ROLLBACK;
--    RAISE EXCEPTION 'Host is no longer ready for frames(status is ) returning';
--    return 0;
--END IF;

_assigned := 0;
SELECT INTO _job * FROM job WHERE keyjob = _keyjob;

_minMem := _job.minmemory;

UPDATE hoststatus SET availablememory = ((availablememory * 1024) - _minMem)/1024 WHERE fkeyhost = _keyhost;

--SELECT INTO _readyStatus keyjobassignmentstatus FROM jobassignmentstatus WHERE status = 'ready';
INSERT INTO jobassignment (fkeyjob, fkeyhost, fkeyjobassignmentstatus)
    VALUES (_keyjob, _keyhost, 1)
    RETURNING keyjobassignment INTO _keyjobassignment;


IF _job.packetType = 'random' THEN
	_tasksql := 'SELECT keyjobtask FROM jobtask WHERE fkeyjob = ' || _keyjob || ' AND status=''new'' ORDER BY random() ASC LIMIT ' || _packetSize;
ELSIF _job.packetType = 'preassigned' THEN
	_tasksql := 'SELECT keyjobtask FROM jobtask WHERE fkeyjob = ' || _keyjob || ' AND fkeyhost=' || _keyhost || ' AND status=''new''';
ELSIF _job.packetType = 'continuous' THEN
	_tasksql := 'SELECT * FROM get_continuous_tasks(' || _keyjob || ', ' || _packetSize || ')';
ELSIF _job.packetType = 'iterative' THEN
	_tasksql := 'SELECT * FROM get_iterative_tasks(' || _keyjob || ', ' || _packetSize || ',10)';
ELSE
	_tasksql := 'SELECT keyjobtask FROM jobtask WHERE fkeyjob = ' || _keyjob || ' AND status=''new'' ORDER BY jobtask ASC LIMIT ' || _packetSize;
END IF;

IF _job.maxhosts > 0 THEN
    UPDATE jobstatus SET hostsonjob=hostsonjob+1 WHERE fkeyjob = _keyjob;
END IF;

FOR _keytask IN EXECUTE(_tasksql) LOOP  
    INSERT INTO jobtaskassignment (fkeyjobtask, fkeyjobassignment, fkeyjobassignmentstatus)
        VALUES (_keytask, _keyjobassignment, 1)
        RETURNING keyjobtaskassignment INTO _keyjobtaskassignment;
    UPDATE jobtask SET fkeyjobtaskassignment = _keyjobtaskassignment, status = 'assigned', fkeyhost = _keyhost 
        WHERE keyjobtask = _keytask;

    _assigned := _assigned + 1;
        
    RETURN NEXT _keytask;
END LOOP;

IF _assigned = 0 THEN
	RAISE EXCEPTION 'no tasks to assign';
END IF;

END;
$$;


ALTER FUNCTION public.assign_single_host_2(_keyjob integer, _keyhost integer, _packetsize integer) OWNER TO farmer;

--
-- Name: assignment_status_count(integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION assignment_status_count(fkeyjobassignmentstatus integer) RETURNS integer
    LANGUAGE plpgsql
    AS $$
DECLARE
	status jobassignmentstatus;
BEGIN
	SELECT INTO status * FROM jobassignmentstatus WHERE keyjobassignmentstatus=fkeyjobassignmentstatus;
	IF status.status IN ('done','cancelled','error') THEN
		RETURN 0;
	END IF;
	IF status.status IN ('ready','copy','busy') THEN
		return 1;
	END IF;
	RETURN 0;
END;
$$;


ALTER FUNCTION public.assignment_status_count(fkeyjobassignmentstatus integer) OWNER TO farmer;

--
-- Name: cancel_job_assignment(integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION cancel_job_assignment(_keyjobassignment integer) RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
	cancelledStatusKey int;
	assignment jobassignment;
	_job job;
BEGIN
	SELECT INTO cancelledStatusKey keyjobassignmentstatus FROM jobassignmentstatus WHERE status='cancelled';
	SELECT INTO assignment * FROM jobassignment WHERE keyjobassignment=_keyjobassignment;
	SELECT INTO _job * FROM job WHERE keyjob=assignment.fkeyjob;

	IF _job.packettype = 'preassigned' THEN
		UPDATE JobTask SET status='new' WHERE fkeyjobtaskassignment IN (SELECT keyjobtaskassignment FROM jobtaskassignment WHERE fkeyjobassignment=_keyjobassignment) AND status IN ('assigned','busy');
	ELSE
		UPDATE JobTask SET status='new', fkeyhost=NULL WHERE fkeyjobtaskassignment IN (SELECT keyjobtaskassignment FROM jobtaskassignment WHERE fkeyjobassignment=_keyjobassignment) AND status IN ('assigned','busy');
	END IF;

	UPDATE JobTaskAssignment SET fkeyjobassignmentstatus=cancelledStatusKey WHERE fkeyjobassignment=_keyjobassignment AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus from jobassignmentstatus WHERE status IN ('ready','copy','busy'));

	-- Dont set it to cancelled if it already cancelled or error
	UPDATE JobAssignment SET fkeyjobassignmentstatus=cancelledStatusKey WHERE keyjobassignment=_keyjobassignment AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus from jobassignmentstatus WHERE status IN ('ready','copy','busy'));
END;
$$;


ALTER FUNCTION public.cancel_job_assignment(_keyjobassignment integer) OWNER TO farmer;

--
-- Name: cancel_job_assignment(integer, text, text); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION cancel_job_assignment(_keyjobassignment integer, _reason text, _nextstate text) RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
	_statusKey int;
	_packettype text;
BEGIN
	SELECT INTO _statusKey keyjobassignmentstatus FROM jobassignmentstatus WHERE status=_reason;

	UPDATE JobTask SET status=_nextstate, fkeyhost=NULL 
		WHERE fkeyjobtaskassignment IN (SELECT keyjobtaskassignment FROM jobtaskassignment WHERE fkeyjobassignment=_keyjobassignment) 
		AND status IN ('assigned','busy');

	UPDATE JobTaskAssignment SET fkeyjobassignmentstatus=_statusKey, ended=now()
		WHERE fkeyjobassignment=_keyjobassignment 
		AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus from jobassignmentstatus WHERE status IN ('ready','copy','busy'));

	-- Dont set it to cancelled if it already cancelled or error
	UPDATE JobAssignment SET fkeyjobassignmentstatus=_statusKey, ended=now()
		WHERE keyjobassignment=_keyjobassignment 
		AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus from jobassignmentstatus WHERE status IN ('ready','copy','busy'));
END;
$$;


ALTER FUNCTION public.cancel_job_assignment(_keyjobassignment integer, _reason text, _nextstate text) OWNER TO farmer;

--
-- Name: cancel_job_assignments(integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION cancel_job_assignments(_keyjob integer) RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
	cancelledStatusKey int;
	_job job;
BEGIN
	SELECT INTO cancelledStatusKey keyjobassignmentstatus FROM jobassignmentstatus WHERE status='cancelled';
	SELECT INTO _job * FROM job WHERE keyjob=_keyjob;
		
	IF _job.packettype = 'preassigned' THEN
		UPDATE JobTask SET status='new' WHERE fkeyjob=_keyjob AND status IN ('assigned','busy');
	ELSE
		UPDATE JobTask SET status='new', fkeyhost=NULL WHERE fkeyjob=_keyjob AND status IN ('assigned','busy');
	END IF;

	UPDATE JobTaskAssignment SET fkeyjobassignmentstatus=cancelledStatusKey WHERE fkeyjobassignment IN(SELECT keyjobassignment FROM JobAssignment WHERE fkeyjob=_keyjob) AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus from jobassignmentstatus WHERE status IN ('ready','copy','busy'));

	UPDATE JobAssignment SET fkeyjobassignmentstatus=cancelledStatusKey WHERE fkeyjob=_keyjob AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus from jobassignmentstatus WHERE status IN ('ready','copy','busy'));
END;
$$;


ALTER FUNCTION public.cancel_job_assignments(_keyjob integer) OWNER TO farmer;

--
-- Name: cancel_job_assignments(integer, text, text); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION cancel_job_assignments(_keyjob integer, _reason text, _nextstate text) RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
    _statusKey int;
    _job job;
BEGIN
    SELECT INTO _statusKey keyjobassignmentstatus FROM jobassignmentstatus WHERE status=_reason;
    SELECT INTO _job * FROM job WHERE keyjob=_keyjob;
        
    IF _job.packettype = 'preassigned' THEN
        UPDATE JobTask SET status=_nextstate WHERE fkeyjob=_keyjob AND status IN ('assigned','busy');
    ELSE
        UPDATE JobTask SET status=_nextstate, fkeyhost=NULL WHERE fkeyjob=_keyjob AND status IN ('assigned','busy');
    END IF;

    UPDATE JobTaskAssignment SET fkeyjobassignmentstatus=_statusKey, ended=now()
        WHERE fkeyjobassignment IN(SELECT keyjobassignment FROM JobAssignment WHERE fkeyjob=_keyjob) 
        AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus from jobassignmentstatus WHERE status IN ('ready','copy','busy'));

    UPDATE JobAssignment SET fkeyjobassignmentstatus=_statusKey, ended=now()
        WHERE fkeyjob=_keyjob 
        AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus from jobassignmentstatus WHERE status IN ('ready','copy','busy'));
END;
$$;


ALTER FUNCTION public.cancel_job_assignments(_keyjob integer, _reason text, _nextstate text) OWNER TO farmer;

--
-- Name: epoch_to_timestamp(double precision); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION epoch_to_timestamp(double precision) RETURNS timestamp without time zone
    LANGUAGE sql IMMUTABLE STRICT
    AS $_$
SELECT ('epoch'::timestamp + $1 * '1 second'::interval)::timestamp;
$_$;


ALTER FUNCTION public.epoch_to_timestamp(double precision) OWNER TO farmer;

--
-- Name: fix_jobstatus(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION fix_jobstatus() RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
	j job;
BEGIN
	FOR j IN SELECT * FROM job LOOP
		INSERT INTO jobstatus (fkeyjob) VALUES (j.keyjob);
	END LOOP;

END;
$$;


ALTER FUNCTION public.fix_jobstatus() OWNER TO farmer;

--
-- Name: get_continuous_tasks(integer, integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION get_continuous_tasks(_fkeyjob integer, max_tasks integer) RETURNS SETOF integer
    LANGUAGE plpgsql
    AS $$
DECLARE
	task RECORD;
	start_task int;
	last_task int;
	cont_len int := -1;
	sav_start_task int;
	sav_end_task int;
	sav_cont_len int := 0;
BEGIN
	FOR task IN SELECT count(*) as cnt, jobtask FROM JobTask WHERE JobTask.fkeyjob=_fkeyjob AND status='new' GROUP BY jobtask ORDER BY jobtask ASC LOOP
		IF cont_len < 0 OR (last_task != task.jobTask AND last_task + 1 != task.jobTask) THEN
			start_task := task.jobTask;
			last_task := task.jobTask;
			cont_len := 0;
		END IF;
		cont_len := cont_len + task.cnt;
		last_task := task.jobTask;

		-- Save this sequence, if its better than the last(or if it's the first one)
		IF cont_len > sav_cont_len THEN
			sav_start_task := start_task;
			sav_end_task := task.jobTask;
			sav_cont_len := cont_len;
			-- If we already found enough continous frames, then break out of the loop and return them
			IF cont_len >= max_tasks THEN
				EXIT;
			END IF;
		END IF;

		-- If we already found enough continous frames, then break out of the loop and return them
		IF cont_len >= max_tasks THEN
			EXIT;
		END IF;
	END LOOP;

	IF sav_cont_len > 0 THEN
		FOR task IN SELECT * FROM JobTask WHERE JobTask.fkeyjob=_fkeyjob AND status='new' AND jobtask >= sav_start_task AND jobtask <= sav_end_task ORDER BY jobtask ASC LOOP
			RETURN NEXT task.keyjobtask;
		END LOOP;
	END IF;

	RETURN;
END;
$$;


ALTER FUNCTION public.get_continuous_tasks(_fkeyjob integer, max_tasks integer) OWNER TO farmer;

--
-- Name: get_iterative_tasks(integer, integer, integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION get_iterative_tasks(_fkeyjob integer, max_tasks integer, stripe integer) RETURNS SETOF integer
    LANGUAGE plpgsql
    AS $$DECLARE
	task RECORD;
	start_task int := 0;
	cur_pos int := 0;
	found_tasks int := 0;
	cur_loop int := 0;
	found_start_task BOOLEAN;
BEGIN
	--WHILE found_tasks < max_tasks AND cur_loop <= stripe LOOP
	WHILE found_tasks < max_tasks AND cur_loop <= stripe LOOP
	--RAISE NOTICE 'starting loop % of %', cur_loop, stripe;
	cur_pos := 0;
	cur_loop := cur_loop + 1;
	found_start_task := FALSE;
	
	FOR task IN SELECT * FROM JobTask WHERE JobTask.fkeyjob=_fkeyjob AND jobtask  >= start_task ORDER BY jobtask ASC LOOP
	
		IF start_task < task.jobTask AND found_start_task = FALSE THEN
			start_task := task.jobTask;
			found_start_task := TRUE;
		END IF;

		cur_pos := cur_pos + 1;

		IF (cur_pos = 1 OR cur_pos % stripe = 0) AND task.status = 'new' THEN
			--RAISE NOTICE 'RETURN NEXT pos % is task %', cur_pos, task.jobtask;
			found_tasks := found_tasks + 1;
			RETURN NEXT task.keyjobtask;
		END IF;
		--RAISE NOTICE 'pos % is task % with stripe % and task status %. start_task is %', cur_pos, task.jobtask, (cur_pos % stripe), task.status, start_task;
	
		-- If we already found enough continous frames, then break out of the loop and return them
		IF found_tasks >= max_tasks THEN	
			EXIT;
		END IF;
	END LOOP;
	
	END LOOP;
	RETURN;
END;$$;


ALTER FUNCTION public.get_iterative_tasks(_fkeyjob integer, max_tasks integer, stripe integer) OWNER TO farmer;

--
-- Name: get_iterative_tasks_2(integer, integer, integer, boolean); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION get_iterative_tasks_2(_fkeyjob integer, max_tasks integer, stripe integer, strict_stripe boolean) RETURNS SETOF integer
    LANGUAGE plpgsql COST 200 ROWS 100
    AS $$DECLARE
	task RECORD;
	start_task int := 0;
	cur_pos int := 0;
	found_tasks int := 0;
	cur_loop int := 0;
	found_start_task BOOLEAN;
BEGIN
	--WHILE found_tasks < max_tasks AND cur_loop <= stripe LOOP
	WHILE found_tasks < max_tasks AND cur_loop <= stripe LOOP
	--RAISE NOTICE 'starting loop % of %', cur_loop, stripe;
	cur_pos := 0;
	cur_loop := cur_loop + 1;
	found_start_task := FALSE;
	
	FOR task IN SELECT * FROM JobTask WHERE JobTask.fkeyjob=_fkeyjob AND jobtask  >= start_task ORDER BY jobtask ASC LOOP
	
		IF start_task < task.jobTask AND found_start_task = FALSE THEN
			start_task := task.jobTask;
			found_start_task := TRUE;
		END IF;

		cur_pos := cur_pos + 1;

		IF ((cur_pos-1) % stripe = 0) AND task.status = 'new' THEN
			--RAISE NOTICE 'RETURN NEXT pos % is task %', cur_pos, task.jobtask;
			found_tasks := found_tasks + 1;
			RETURN NEXT task.keyjobtask;
		END IF;
		--RAISE NOTICE 'pos % is task % with stripe % and task status %. start_task is %', cur_pos, task.jobtask, (cur_pos % stripe), task.status, start_task;
	
		-- If we already found enough continous frames, then break out of the loop and return them
		IF found_tasks >= max_tasks THEN	
			EXIT;
		END IF;
	END LOOP;

	-- if strict_stripe then exit after the first loop attempt
	IF strict_stripe = TRUE THEN
		EXIT;
	END IF;
	
	END LOOP;
	RETURN;
END;$$;


ALTER FUNCTION public.get_iterative_tasks_2(_fkeyjob integer, max_tasks integer, stripe integer, strict_stripe boolean) OWNER TO postgres;

--
-- Name: get_iterative_tasks_debug(integer, integer, integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION get_iterative_tasks_debug(_fkeyjob integer, max_tasks integer, stripe integer) RETURNS SETOF integer
    LANGUAGE plpgsql
    AS $$DECLARE
	task RECORD;
	start_task int := 0;
	cur_pos int := 0;
	found_tasks int := 0;
	cur_loop int := 0;
	found_next_start_task BOOLEAN;
BEGIN
	--WHILE found_tasks < max_tasks AND cur_loop <= stripe LOOP
	WHILE found_tasks < max_tasks AND cur_loop <= stripe LOOP
	--RAISE NOTICE 'starting loop % of %', cur_loop, stripe;
	cur_pos := 0;
	cur_loop := cur_loop + 1;
	found_next_start_task := FALSE;
	
	FOR task IN SELECT * FROM JobTask WHERE JobTask.fkeyjob=_fkeyjob AND jobtask  >= start_task ORDER BY jobtask ASC LOOP
	
		IF start_task < task.jobTask AND found_next_start_task = FALSE THEN
			start_task := task.jobTask;
			found_next_start_task := TRUE;
		END IF;

		--IF (start_task = 0) THEN
		--	start_task := task.jobTask;
		--END IF;

		--IF (start_task < task.jobTask) AND found_next_start_task = FALSE THEN
		--	start_task := task.jobTask;
		--	found_next_start_task := TRUE;
		--END IF;

		cur_pos := cur_pos + 1;

		IF (cur_pos = 1 OR cur_pos % stripe = 0) AND task.status = 'new' THEN
			RAISE NOTICE 'RETURN NEXT pos % is task %', cur_pos, task.jobtask;
			found_tasks := found_tasks + 1;
			RETURN NEXT task.keyjobtask;
		END IF;
		--RAISE NOTICE 'pos % is task % with stripe % and task status %. start_task is %', cur_pos, task.jobtask, (cur_pos % stripe), task.status, start_task;
	
		-- If we already found enough continous frames, then break out of the loop and return them
		IF found_tasks >= max_tasks THEN	
			EXIT;
		END IF;
	END LOOP;

	--start_task := start_task + 1;
	--IF stripe > 1 THEN
	--	stripe := stripe / 2;
	--END IF;
	
	END LOOP;
	RETURN;
END;$$;


ALTER FUNCTION public.get_iterative_tasks_debug(_fkeyjob integer, max_tasks integer, stripe integer) OWNER TO farmer;

--
-- Name: get_job_efficiency(integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION get_job_efficiency(_keyhost integer) RETURNS SETOF double precision
    LANGUAGE plpgsql ROWS 10
    AS $$
DECLARE
	job record;
BEGIN

        FOR job IN SELECT ((sum(usertime+systime)::float8/coalesce(assignslots,8)::float8) / GREATEST(
		extract(epoch from sum((coalesce(ended,now())-started)))::bigint*1000,1
		))::float8 as jobefficiency
		FROM jobassignment ja
		WHERE fkeyjob = _keyhost AND fkeyjobassignmentstatus = 4
		group by assignslots
	LOOP
		RETURN NEXT job.jobefficiency::float8;
	END LOOP;

        
END;
$$;


ALTER FUNCTION public.get_job_efficiency(_keyhost integer) OWNER TO farmer;

--
-- Name: get_pass_preseed(text, text); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION get_pass_preseed(_shot text, _pass text) RETURNS SETOF passpreseedret
    LANGUAGE plpgsql ROWS 1
    AS $$
DECLARE
    _ret RECORD;
BEGIN

-- NB: I have no need of averagemem, am returning average done instead -- could not figure out how to change 'psspresetret'

SELECT INTO _ret job.slots, 
js.averagedonetime,
(select MAX(maxmemory) FROM jobassignment ja WHERE ja.fkeyjob = keyjob and fkeyjobassignmentstatus=4) as maxmemory
FROM job
JOIN jobstatus js on js.fkeyjob = keyjob
WHERE shotName = _shot 
AND job LIKE '%' || _pass
AND status IN ('done','deleted')
order by submittedts desc
limit 1;

RETURN NEXT _ret;

END;
$$;


ALTER FUNCTION public.get_pass_preseed(_shot text, _pass text) OWNER TO postgres;

--
-- Name: get_wayward_hosts(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION get_wayward_hosts() RETURNS SETOF integer
    LANGUAGE plpgsql
    AS $$
DECLARE
	hs hoststatus;
BEGIN
	FOR hs IN SELECT * FROM HostStatus WHERE slavestatus IS NOT NULL AND slavestatus NOT IN ('offline','restart','restarting','sleeping','waking','no-ping','stopping','maintenance') AND now() - slavepulse > '10 minutes'::interval LOOP
		RETURN NEXT hs.fkeyhost;
	END LOOP;

	FOR hs in SELECT * FROM HostStatus WHERE slavestatus='assigned' AND now() - laststatuschange > '5 minutes'::interval LOOP
		RETURN NEXT hs.fkeyhost;
	END LOOP;
END;
$$;


ALTER FUNCTION public.get_wayward_hosts() OWNER TO farmer;

--
-- Name: get_wayward_hosts_2(interval, interval); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION get_wayward_hosts_2(pulse_period interval, loop_time interval) RETURNS SETOF waywardhostret
    LANGUAGE plpgsql
    AS $$
DECLARE
	hs hoststatus;
	ret WaywardHostRet;
BEGIN
	-- Give a host 15 minutes to wake up
	FOR hs IN SELECT * FROM HostStatus WHERE slavestatus='waking' AND now() - laststatuschange > '15 minutes'::interval LOOP
		ret := ROW(hs.fkeyhost,1);
		RETURN NEXT ret;
	END LOOP;

	-- 5 minutes to go offline from stopping or restart
	FOR hs IN SELECT * FROM HostStatus WHERE slavestatus IN ('stopping','starting','restart','reboot','client-update','assigned') AND now() - laststatuschange > '5 minutes'::interval LOOP
		ret := ROW(hs.fkeyhost,1);
		RETURN NEXT ret;
	END LOOP;

	FOR hs IN SELECT * FROM HostStatus WHERE slavestatus IS NOT NULL AND slavestatus IN ('ready','copy','busy','offline','maintenance','restarting') AND (slavepulse IS NULL OR (now() - slavepulse) > (pulse_period*2 + loop_time)) LOOP
		ret := ROW(hs.fkeyhost,2);
		RETURN NEXT ret;
	END LOOP;
END;
$$;


ALTER FUNCTION public.get_wayward_hosts_2(pulse_period interval, loop_time interval) OWNER TO farmer;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: countercache; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE countercache (
    hoststotal integer,
    hostsactive integer,
    hostsready integer,
    jobstotal integer,
    jobsactive integer,
    jobsdone integer,
    lastupdated timestamp without time zone,
    slotstotal integer,
    slotsactive integer,
    jobswaiting integer
);


ALTER TABLE public.countercache OWNER TO farmer;

--
-- Name: getcounterstate(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION getcounterstate() RETURNS countercache
    LANGUAGE plpgsql
    AS $$
	DECLARE
		cache countercache;
	BEGIN
		SELECT INTO cache * FROM countercache LIMIT 1;
		IF NOT FOUND THEN
			INSERT INTO countercache (hoststotal) values (null);
			cache.lastUpdated := now() - interval'1 year';
		END IF;
		IF now() - cache.lastUpdated > interval'200000 seconds' THEN
			DECLARE
				hosts_total int;
				hosts_active int;
				hosts_ready int;
				jobs_total int;
				jobs_active int;
				jobs_done int;
				jobs_waiting int;
				slots_total int;
				slots_active int;
			BEGIN
				SELECT count(*) INTO hosts_total FROM HostService INNER JOIN HostStatus ON HostStatus.fkeyHost=HostService.fkeyHost AND fkeyService=1;

				SELECT count(fkeyHost) INTO hosts_active FROM JobAssignment WHERE fkeyjobassignmentstatus < 4;
				
				SELECT count(fkeyHost) INTO hosts_ready FROM HostStatus WHERE slaveStatus='ready';

				SELECT SUM(maxassignments) INTO slots_total FROM Host
					JOIN HostStatus hs ON keyhost=hs.fkeyhost
					WHERE host.online=1 AND hs.slaveStatus = 'ready';

				SELECT sum(slots) INTO slots_active FROM Job
					JOIN JobAssignment ja ON keyjob=ja.fkeyjob
					WHERE ja.fkeyjobassignmentstatus < 4;

				SELECT count(keyJob) INTO jobs_total FROM Job WHERE status IN('submit','verify','holding','ready', 'busy', 'started','suspended','done');
				SELECT count(keyJob) INTO jobs_active FROM Job WHERE status = 'started';
				SELECT count(keyJob) INTO jobs_waiting FROM Job WHERE status IN ('ready', 'holding');
				SELECT count(keyJob) INTO jobs_done FROM Job WHERE status='done';
				
				UPDATE CounterCache SET hoststotal=hosts_total, hostsactive=hosts_active, hostsReady=hosts_ready,
					jobsTotal=jobs_total, jobsActive=jobs_active, jobsDone=jobs_done, jobsWaiting=jobs_waiting,
					slotsTotal=slots_total, slotsActive=slots_active,
					lastUpdated=now();

				SELECT INTO cache * FROM countercache LIMIT 1;
			END;
		END IF;
		RETURN cache;
	END;
$$;


ALTER FUNCTION public.getcounterstate() OWNER TO farmer;

--
-- Name: hosthistory_keyhosthistory_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE hosthistory_keyhosthistory_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.hosthistory_keyhosthistory_seq OWNER TO farmer;

--
-- Name: hosthistory_keyhosthistory_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('hosthistory_keyhosthistory_seq', 1, false);


--
-- Name: hosthistory; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE hosthistory (
    keyhosthistory integer DEFAULT nextval('hosthistory_keyhosthistory_seq'::regclass) NOT NULL,
    fkeyhost integer,
    fkeyjob integer,
    fkeyjobstat integer,
    status text,
    laststatus text,
    datetime timestamp without time zone,
    duration interval,
    fkeyjobtask integer,
    fkeyjobtype integer,
    nextstatus text,
    success boolean,
    fkeyjoberror integer,
    change_from_ip inet,
    fkeyjobcommandhistory integer
);


ALTER TABLE public.hosthistory OWNER TO farmer;

--
-- Name: hosthistory_dynamic_query(text, text); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION hosthistory_dynamic_query(where_stmt text, order_limit text) RETURNS SETOF hosthistory
    LANGUAGE plpgsql
    AS $$
DECLARE
	rec hosthistory;
	query text;
	where_gen text;
BEGIN
	query := 'SELECT * FROM hosthistory ';
	where_gen := where_stmt;
	IF char_length(where_stmt) > 0 THEN
		IF position('where' in lower(where_stmt)) = 0 THEN
			where_gen := 'WHERE ' || where_gen;
		END IF;
		query := query || where_gen;
	END IF;
	IF char_length(order_limit) > 0 THEN
		query := ' ' || order_limit;
	END IF;
	FOR rec IN EXECUTE query LOOP
		RETURN NEXT rec;
	END LOOP;
	RETURN;
END;
$$;


ALTER FUNCTION public.hosthistory_dynamic_query(where_stmt text, order_limit text) OWNER TO farmer;

--
-- Name: hosthistory_overlaps_timespan(timestamp without time zone, timestamp without time zone, text, text); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION hosthistory_overlaps_timespan(time_start timestamp without time zone, time_end timestamp without time zone, where_stmt text, order_limit text) RETURNS SETOF hosthistory
    LANGUAGE plpgsql
    AS $$
DECLARE
	rec hosthistory;
	where_gen text;
BEGIN
	where_gen := '((	(datetime > ' || quote_literal(time_start) || ')
			AND	(datetime < ' || quote_literal(time_end) || '))
			OR
			-- Ends inside the timespan
			(	(datetime + duration < ' || quote_literal(time_end) || ')
			AND (datetime + duration > ' || quote_literal(time_start) || '))
			OR
			-- Covors the entire timespan
			(	(datetime < ' || quote_literal(time_start) || ')
			AND (
					(datetime + duration > ' || quote_literal(time_end) || ')
				OR 	(duration IS NULL) )
			))';
	IF char_length(where_stmt) > 0 THEN
		where_gen := where_gen || ' AND (' || where_stmt || ')';
	END IF;

	FOR rec IN SELECT * FROM hosthistory_dynamic_query( where_gen, order_limit ) LOOP
		RETURN NEXT rec;
	END LOOP;
	RETURN;
END;
$$;


ALTER FUNCTION public.hosthistory_overlaps_timespan(time_start timestamp without time zone, time_end timestamp without time zone, where_stmt text, order_limit text) OWNER TO farmer;

--
-- Name: hosthistory_status_percentages(text); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION hosthistory_status_percentages(query_input text) RETURNS SETOF hosthistory_status_percentages_return
    LANGUAGE plpgsql
    AS $$
DECLARE
	rec hosthistory_status_summary_return;
	ret hosthistory_status_percentages_return;
	total_time interval := '0 minutes';
	non_idle_time interval := '0 minutes';
	non_idle_non_wasted_time interval := '0 minutes';
BEGIN
	FOR rec IN SELECT * FROM hosthistory_status_summary( query_input ) LOOP
		IF rec.total_time IS NOT NULL THEN
			total_time := total_time + rec.total_time;
			IF rec.status IN ('busy','copy','assigned') THEN
				non_idle_time := non_idle_time + rec.total_time;
				IF rec.wasted=false THEN
					non_idle_non_wasted_time := non_idle_non_wasted_time + rec.total_time;
				END IF;
			END IF;
		END IF;
	END LOOP;
	-- Can I keep the values from the first select and loop through them twice, seems i should be able to...
	FOR rec IN SELECT * FROM hosthistory_status_summary( query_input ) LOOP
		ret.status := rec.status;
		ret.loading := rec.loading;
		ret.wasted := rec.wasted;
		ret.total_time := rec.total_time;
		ret.total_perc := rec.total_time / total_time;
		ret.non_idle_non_wasted_perc := NULL;
		ret.non_idle_perc := NULL;
		IF rec.status IN ('busy','copy','assigned') THEN
			ret.non_idle_perc := rec.total_time / non_idle_time;
			IF rec.wasted=false THEN
				ret.non_idle_non_wasted_perc := rec.total_time / non_idle_non_wasted_time;
			END IF;
		END IF;
		RETURN NEXT ret;
	END LOOP;
	RETURN;
END;
$$;


ALTER FUNCTION public.hosthistory_status_percentages(query_input text) OWNER TO farmer;

--
-- Name: hosthistory_status_percentages_duration_adjusted(timestamp without time zone, timestamp without time zone); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION hosthistory_status_percentages_duration_adjusted(history_start timestamp without time zone, history_end timestamp without time zone) RETURNS SETOF hosthistory_status_percentages_return
    LANGUAGE plpgsql
    AS $$
DECLARE
	rec hosthistory_status_percentages_return;
BEGIN
	FOR rec IN SELECT * FROM hosthistory_status_percentages( 
		'hosthistory_timespan_duration_adjusted( ' || quote_literal(history_start) || ', ' || quote_literal(history_end) || ','''','''')' ) LOOP
		RETURN NEXT rec;
	END LOOP;
END;
$$;


ALTER FUNCTION public.hosthistory_status_percentages_duration_adjusted(history_start timestamp without time zone, history_end timestamp without time zone) OWNER TO farmer;

--
-- Name: hosthistory_status_summary(text); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION hosthistory_status_summary(query_input text) RETURNS SETOF hosthistory_status_summary_return
    LANGUAGE plpgsql
    AS $$
DECLARE
 rec hosthistory_status_summary_return;
 query text;
BEGIN
	query := 'SELECT 
		status,
		(fkeyjobtask is null and status=''busy'') as loading,
		status=''busy'' AND ((fkeyjobtask is null AND nextstatus=''ready'' AND nextstatus IS NOT NULL) OR (fkeyjobtask IS NOT null AND success is null)) as error,
		count(*),
		sum(duration),
		avg(duration),
		max(duration),
		min(duration)
		FROM ' || query_input || 
		' GROUP BY status, loading, error
		ORDER BY status';

	FOR rec IN EXECUTE query LOOP
		RETURN NEXT rec;
	END LOOP;
	RETURN;
END;
$$;


ALTER FUNCTION public.hosthistory_status_summary(query_input text) OWNER TO farmer;

--
-- Name: hosthistory_status_summary_duration_adjusted(timestamp without time zone, timestamp without time zone); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION hosthistory_status_summary_duration_adjusted(history_start timestamp without time zone, history_end timestamp without time zone) RETURNS SETOF hosthistory_status_summary_return
    LANGUAGE plpgsql
    AS $$
DECLARE
	rec hosthistory_status_summary_return;
BEGIN
	FOR rec IN SELECT * FROM hosthistory_status_summary( 
		'hosthistory_timespan_duration_adjusted( ' || quote_literal(history_start) || ', ' || quote_literal(history_end) || ','''','''')' ) LOOP
		RETURN NEXT rec;
	END LOOP;
END;
$$;


ALTER FUNCTION public.hosthistory_status_summary_duration_adjusted(history_start timestamp without time zone, history_end timestamp without time zone) OWNER TO farmer;

--
-- Name: hosthistory_timespan_duration_adjusted(timestamp without time zone, timestamp without time zone); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION hosthistory_timespan_duration_adjusted(time_start timestamp without time zone, time_end timestamp without time zone) RETURNS SETOF hosthistory
    LANGUAGE plpgsql
    AS $$
DECLARE
	rec hosthistory;
	dur interval;
BEGIN
	FOR rec IN SELECT
		*
		FROM HostHistory
		WHERE
			-- Starts inside the timespan
			(	(datetime > time_start)
			AND	(datetime < time_end))
			OR
			-- Ends inside the timespan
			(	(datetime + duration < time_end)
			AND (datetime + duration > time_start))
			OR
			-- Covors the entire timespan
			(	(datetime < time_start)
			AND (
					(datetime + duration > time_end)
				OR 	(duration IS NULL) )
			)
		LOOP
			dur := rec.duration;

			-- Account for hosthistory records that havent finished
			IF dur IS NULL THEN
				dur := now() - rec.datetime;
			END IF;

			-- Cut off front
			IF rec.datetime < time_start THEN
				dur := dur - (time_start - rec.datetime);
			END IF;
			
			-- Cut off the end
			IF rec.datetime + rec.duration > time_end THEN
				dur := dur - ((rec.datetime + rec.duration) - rec.datetime);
			END IF;

			rec.duration := dur;

			RETURN NEXT rec;
		END LOOP;
	RETURN;
END;
$$;


ALTER FUNCTION public.hosthistory_timespan_duration_adjusted(time_start timestamp without time zone, time_end timestamp without time zone) OWNER TO farmer;

--
-- Name: hosthistory_timespan_duration_adjusted(timestamp without time zone, timestamp without time zone, text, text); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION hosthistory_timespan_duration_adjusted(time_start timestamp without time zone, time_end timestamp without time zone, where_stmt text, order_limit text) RETURNS SETOF hosthistory
    LANGUAGE plpgsql
    AS $$
DECLARE
	rec hosthistory;
	dur interval;
	end_ts timestamp;
BEGIN
	FOR rec IN SELECT * FROM hosthistory_overlaps_timespan( time_start, time_end, where_stmt, order_limit ) LOOP
			dur := rec.duration;
			-- Account for hosthistory records that havent finished
			IF dur IS NULL THEN
				dur := now() - rec.datetime;
			END IF;

			end_ts := rec.datetime + dur;
			-- Cut off front
			IF rec.datetime < time_start THEN
				dur := dur - (time_start - rec.datetime);
			END IF;
			
			-- Cut off the end
			IF end_ts > time_end THEN
				dur := dur - (end_ts - time_end);
			END IF;

			rec.duration := justify_hours(dur);

			RETURN NEXT rec;
		END LOOP;
	RETURN;
END;
$$;


ALTER FUNCTION public.hosthistory_timespan_duration_adjusted(time_start timestamp without time zone, time_end timestamp without time zone, where_stmt text, order_limit text) OWNER TO farmer;

--
-- Name: hosthistory_user_slave_summary(timestamp without time zone, timestamp without time zone); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION hosthistory_user_slave_summary(history_start timestamp without time zone, history_end timestamp without time zone) RETURNS SETOF hosthistory_user_slave_summary_return
    LANGUAGE plpgsql
    AS $$
DECLARE
	ret hosthistory_user_slave_summary_return;
BEGIN
	FOR ret IN
		SELECT
			usr.name,
			host.host,
			(extract(epoch from sum) / 3600)::numeric(8,2) as totalHours
			FROM
				(SELECT
					sum(duration),
					fkeyhost
				FROM
					hosthistory_timespan_duration_adjusted(history_start,history_end, '', '') as hosthistory
				WHERE
					hosthistory.status in ('ready','busy','copy','assigned')
				GROUP BY fkeyhost)
			AS 	iq,
				host,
				usr
			WHERE
				host.keyhost=iq.fkeyhost
			AND usr.fkeyhost=host.keyhost
			ORDER BY sum desc
	LOOP
		RETURN NEXT ret;
	END LOOP;
	RETURN;
END;
$$;


ALTER FUNCTION public.hosthistory_user_slave_summary(history_start timestamp without time zone, history_end timestamp without time zone) OWNER TO farmer;

--
-- Name: hoststatus_update(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION hoststatus_update() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
BEGIN
    IF (NEW.slavestatus IS NOT NULL)
    AND (NEW.slavestatus != OLD.slavestatus) THEN
        NEW.lastStatusChange = NOW();
    END IF;

    IF (OLD.slavestatus != NEW.slavestatus) AND (NEW.slavestatus IN ('starting','stopping','restart')) THEN
        PERFORM return_slave_tasks_3(NEW.fkeyhost);
    END IF;

    RETURN NEW;
END;
$$;


ALTER FUNCTION public.hoststatus_update() OWNER TO farmer;

--
-- Name: increment_loadavgadjust(integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION increment_loadavgadjust(_fkeyhost integer) RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
	loadavgadjust_inc float;
BEGIN
	SELECT INTO loadavgadjust_inc value::float FROM config WHERE config='assburnerLoadAvgAdjustInc';
	UPDATE HostLoad SET loadavgadjust=loadavgadjust_inc+loadavgadjust WHERE fkeyhost=_fkeyhost;
END;
$$;


ALTER FUNCTION public.increment_loadavgadjust(_fkeyhost integer) OWNER TO farmer;

--
-- Name: insert_jobtask(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION insert_jobtask() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
BEGIN
UPDATE JobStatus SET tasksCount = tasksCount + 1, tasksUnassigned = tasksUnassigned + 1 WHERE fkeyJob = NEW.fkeyjob;
RETURN NEW;
END;
$$;


ALTER FUNCTION public.insert_jobtask() OWNER TO farmer;

--
-- Name: interval_divide(interval, interval); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION interval_divide(numerator interval, denominator interval) RETURNS double precision
    LANGUAGE plpgsql
    AS $$
BEGIN
	RETURN extract(epoch from numerator) / extract(epoch from denominator)::float;
END;
$$;


ALTER FUNCTION public.interval_divide(numerator interval, denominator interval) OWNER TO farmer;

--
-- Name: job_delete(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION job_delete() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
DECLARE
    stat RECORD;
BEGIN
-- this is too goddamn slow
    --DELETE FROM jobassignment WHERE fkeyjob = OLD.keyjob;
    --DELETE FROM jobtask     WHERE fkeyjob = OLD.keyjob;
    --DELETE FROM joberror    WHERE fkeyjob = OLD.keyjob;
    --DELETE FROM jobhistory  WHERE fkeyjob = OLD.keyjob;
    --DELETE FROM jobstatus   WHERE fkeyjob = OLD.keyjob;
    --DELETE FROM jobservice  WHERE fkeyjob = OLD.keyjob;
    --DELETE FROM jobdep      WHERE fkeyjob = OLD.keyjob;
    --DELETE FROM joboutput   WHERE fkeyjob = OLD.keyjob;

    RETURN OLD;
END;
$$;


ALTER FUNCTION public.job_delete() OWNER TO farmer;

--
-- Name: job_error_increment(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION job_error_increment() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
BEGIN
	IF (NEW.cleared IS NULL OR NEW.cleared=false) AND (NEW.count > coalesce(OLD.count,0)) THEN
		UPDATE jobstatus SET errorcount=errorcount+1 where fkeyjob=NEW.fkeyjob;
	END IF;

	IF (NEW.cleared = true) AND (coalesce(OLD.cleared,false)=false) THEN
		UPDATE jobstatus SET errorcount=errorcount-NEW.count WHERE fkeyjob=NEW.fkeyjob;
	END IF;
	RETURN NEW;
END;
$$;


ALTER FUNCTION public.job_error_increment() OWNER TO farmer;

--
-- Name: job_error_insert(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION job_error_insert() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
BEGIN
	UPDATE jobstatus SET errorcount=errorcount+1 WHERE fkeyjob=NEW.fkeyjob;
	RETURN NEW;
END;
$$;


ALTER FUNCTION public.job_error_insert() OWNER TO farmer;

--
-- Name: job_gatherstats(integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION job_gatherstats(_keyjob integer) RETURNS job_gatherstats_result
    LANGUAGE plpgsql
    AS $$
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
                                AND (jas.status='error' OR jas.status='cancelled')
                        GROUP BY ja.fkeyjob;
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
$$;


ALTER FUNCTION public.job_gatherstats(_keyjob integer) OWNER TO farmer;

--
-- Name: job_gatherstats_2(integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION job_gatherstats_2(_keyjob integer) RETURNS job_gatherstats_result_2
    LANGUAGE plpgsql
    AS $$
DECLARE
        ret Job_GatherStats_Result_2;
BEGIN
	-- Task (frame) times - only this stat is in wallclock time
	-- cputime is amount of actual processor time spent
        SELECT INTO ret.mintasktime, ret.avgtasktime, ret.maxtasktime, ret.totaltasktime,
                    ret.mincputime, ret.avgcputime, ret.maxcputime, ret.totalcputime,
                    ret.taskcount
		min(ja.ended-ja.started) as mintime,
		avg(ja.ended-ja.started) as avgtime,
		max(ja.ended-ja.started) as maxtime,
		sum(ja.ended-ja.started) as totaltime,
		min(usertime+systime) * '1 msecs'::interval as mincputime,
		avg(usertime+systime) * '1 msecs'::interval as avgcputime,
		max(usertime+systime) * '1 msecs'::interval as maxcputime,
		sum(usertime+systime) * '1 msecs'::interval as totalcputime,
                count(*) as loadcount
        FROM jobtaskassignment jta
        JOIN jobassignmentstatus jas ON jas.keyjobassignmentstatus=jta.fkeyjobassignmentstatus
        JOIN jobassignment ja ON ja.keyjobassignment=jta.fkeyjobassignment
        WHERE ja.fkeyjob=_keyjob
              AND jas.status='done'
        GROUP BY jas.status;
        
	-- Load Times
        SELECT INTO ret.totalloadtime, ret.loadcount
		sum(ja.ended-ja.started) as totaltime,
                count(*) as loadcount
        FROM jobtaskassignment jta
        JOIN jobassignmentstatus jas ON jas.keyjobassignmentstatus=jta.fkeyjobassignmentstatus
        JOIN jobassignment ja ON ja.keyjobassignment=jta.fkeyjobassignment
        WHERE ja.fkeyjob=_keyjob
              AND jas.status='copy'
        GROUP BY jas.status;
        
        -- Error Times
        SELECT INTO ret.totalerrortime, ret.errorcount
		sum(ja.ended-ja.started) as totaltime,
		count(*) as loadcount
        FROM jobtaskassignment jta
        JOIN jobassignmentstatus jas ON jas.keyjobassignmentstatus=jta.fkeyjobassignmentstatus
        JOIN jobassignment ja ON ja.keyjobassignment=jta.fkeyjobassignment
        WHERE ja.fkeyjob=_keyjob
              AND jas.status='error'
        GROUP BY ja.fkeyjob;

        -- Cancel Times
        SELECT INTO ret.totalcanceltime, ret.cancelcount
		sum(ja.ended-ja.started) as totaltime,
		count(*) as loadcount
        FROM jobtaskassignment jta
        JOIN jobassignmentstatus jas ON jas.keyjobassignmentstatus=jta.fkeyjobassignmentstatus
        JOIN jobassignment ja ON ja.keyjobassignment=jta.fkeyjobassignment
        WHERE ja.fkeyjob=_keyjob
              AND jas.status='cancelled'
        GROUP BY ja.fkeyjob;

        -- Copy times
        SELECT INTO ret.totalcopytime, ret.copycount
		sum(ja.ended-ja.started) as totaltime,
                count(*) as loadcount
        FROM jobtaskassignment jta
        JOIN jobassignmentstatus jas ON jas.keyjobassignmentstatus=jta.fkeyjobassignmentstatus
        JOIN jobassignment ja ON ja.keyjobassignment=jta.fkeyjobassignment
        WHERE ja.fkeyjob=_keyjob
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
        IF ret.totalcanceltime IS NOT NULL THEN
                ret.totaltime := ret.totaltime + ret.totalcanceltime;
        END IF;
        IF ret.totalcopytime IS NOT NULL THEN
                ret.totaltime := ret.totaltime + ret.totalcopytime;
        END IF;
        RETURN ret;
END;
$$;


ALTER FUNCTION public.job_gatherstats_2(_keyjob integer) OWNER TO farmer;

--
-- Name: job_insert(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION job_insert() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
DECLARE
    _envkey integer;
BEGIN
	INSERT INTO jobstatus (fkeyjob) VALUES (NEW.keyjob);
	RETURN NEW;
END;
$$;


ALTER FUNCTION public.job_insert() OWNER TO farmer;

--
-- Name: job_update(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION job_update() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
DECLARE
    stat RECORD;
BEGIN
    -- Set Job.taskscount and Job.tasksunassigned
    IF NEW.status IN ('verify','verify-suspended') THEN
        PERFORM update_job_task_counts(NEW.keyjob);
    END IF;

    IF NEW.status IN ('verify', 'suspended', 'deleted') AND OLD.status NOT IN ('submit','done','suspended') THEN
        PERFORM cancel_job_assignments(NEW.keyjob, 'cancelled', 'new');
    END IF;

    IF (NEW.fkeyjobstat IS NOT NULL) THEN
        IF (NEW.status='started' AND OLD.status='ready') THEN
            UPDATE jobstat SET started=NOW() WHERE keyjobstat=NEW.fkeyjobstat AND started IS NULL;
        END IF;
        IF (NEW.status='ready' AND OLD.status='done') THEN
            SELECT INTO stat * from jobstat where keyjobstat=NEW.fkeyjobstat;
            INSERT INTO jobstat (fkeyelement, fkeyproject, taskCount, fkeyusr, started, pass, fkeyjobtype, fkeyjob) VALUES (stat.fkeyelement, stat.fkeyproject, stat.taskCount, stat.fkeyusr, NOW(), stat.pass, NEW.fkeyjobtype, NEW.keyjob);
            SELECT INTO NEW.fkeyjobstat currval('jobstat_keyjobstat_seq');
        END IF;
        IF (NEW.status IN ('done','deleted') AND OLD.status IN ('ready','started','suspended') ) THEN
            UPDATE jobstat
            SET
                fkeyjobtype=NEW.fkeyjobtype,
                fkeyjob=NEW.keyjob,
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
            -- Clear the wait reason on done and deleted jobs.
            UPDATE jobstatus
            SET
                fkeyjobstatusskipreason=0
            WHERE jobstatus.fkeyjob=NEW.keyjob;
        END IF;
    END IF;


    --IF NEW.fkeyjobenvironment IS NOT NULL AND NEW.environment IS NULL THEN
	--SELECT INTO NEW.environment environment from jobenvironment where keyjobenvironment = NEW.fkeyjobenvironment;
    --END IF;

    IF NEW.fkeyjobenvironment IS NULL AND (OLD.environment IS NULL OR NEW.environment != OLD.environment) THEN
	INSERT INTO jobenvironment (keyjobenvironment, environment) VALUES (DEFAULT, NEW.environment) RETURNING keyjobenvironment INTO NEW.fkeyjobenvironment;
    END IF;

    -- Set the suspended timestamp if job gets suspended. Clear the wait reason as well.
    IF NEW.status IN ('suspended') THEN
        NEW.suspendedts = NOW();
        UPDATE jobstatus
        SET
            fkeyjobstatusskipreason=0
        WHERE jobstatus.fkeyjob=NEW.keyjob;
    END IF;

    -- Clear the suspended timestamp if the job is removed from suspension
    IF (NEW.status != OLD.status AND OLD.status='suspended') THEN
        NEW.suspendedts = NULL;
    END IF;

    RETURN NEW;
END;
$$;


ALTER FUNCTION public.job_update() OWNER TO farmer;

--
-- Name: jobassignment_after_update(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION jobassignment_after_update() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
BEGIN
	PERFORM update_host_assignment_count(NEW.fkeyhost);
	--IF TG_OP = 'UPDATE' AND OLD.fkeyjobassignmentstatus != NEW.fkeyjobassignmentstatus THEN
	--	PERFORM update_job_task_counts_2(NEW.fkeyjob);
	--END IF;
	return NEW;
END;
$$;


ALTER FUNCTION public.jobassignment_after_update() OWNER TO farmer;

--
-- Name: jobassignment_delete(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION jobassignment_delete() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
DECLARE
	oldstatusval int := 0;
BEGIN
	oldstatusval := assignment_status_count(OLD.fkeyjobassignmentstatus);
	IF oldstatusval > 0 THEN
		PERFORM updateJobLicenseCounts( OLD.fkeyjob, -1 );
	END IF;
	IF oldstatusval > 0 THEN
		PERFORM update_host_assignment_count(OLD.fkeyhost);
	END IF;
	RETURN OLD;
END;
$$;


ALTER FUNCTION public.jobassignment_delete() OWNER TO farmer;

--
-- Name: jobassignment_insert(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION jobassignment_insert() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
BEGIN
	IF NEW.assignslots IS NULL THEN
		SELECT INTO NEW.assignslots slots FROM job WHERE keyjob = NEW.fkeyjob;
	END IF;
	
	PERFORM updateJobLicenseCounts( NEW.fkeyjob, 1 );

	RETURN NEW;
END;
$$;


ALTER FUNCTION public.jobassignment_insert() OWNER TO farmer;

--
-- Name: jobassignment_update(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION jobassignment_update() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
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
$$;


ALTER FUNCTION public.jobassignment_update() OWNER TO farmer;

--
-- Name: jobdep_delete(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION jobdep_delete() RETURNS trigger
    LANGUAGE plpgsql
    AS $$DECLARE
    dependentJob RECORD;
    curStatus text;
BEGIN
    FOR dependentJob IN
        SELECT job.keyjob as dependentKey
        FROM jobdep
        JOIN job ON jobdep.fkeyjob = job.keyjob
        WHERE fkeydep = OLD.fkeyjob
    LOOP
	SELECT INTO curStatus status
	FROM job
	WHERE keyjob = dependentJob.dependentKey;

	IF curStatus = 'holding' THEN
		RAISE WARNING 'Updating job status for %', dependentJob.dependentKey;

		UPDATE job SET status = 'verify' WHERE keyjob = dependentJob.dependentKey;
		INSERT INTO jobhistory (fkeyjob, message) VALUES (dependentJob.dependentKey, 'Job dependency has been removed by parent job. Setting job status to verify.');
	END IF;
    END LOOP;
    RETURN OLD;
END;$$;


ALTER FUNCTION public.jobdep_delete() OWNER TO farmer;

--
-- Name: jobdep_keyjobdep_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobdep_keyjobdep_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobdep_keyjobdep_seq OWNER TO farmer;

--
-- Name: jobdep_keyjobdep_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobdep_keyjobdep_seq', 1, false);


--
-- Name: jobdep; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobdep (
    keyjobdep integer DEFAULT nextval('jobdep_keyjobdep_seq'::regclass) NOT NULL,
    fkeyjob integer,
    fkeydep integer,
    deptype integer DEFAULT 1 NOT NULL
);


ALTER TABLE public.jobdep OWNER TO farmer;

--
-- Name: jobdep_recursive(text); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION jobdep_recursive(keylist text) RETURNS SETOF jobdep
    LANGUAGE plpgsql
    AS $$
BEGIN
       RETURN QUERY EXECUTE
               'WITH RECURSIVE job_dep_rec AS (
                       SELECT jobdep.* from jobdep WHERE fkeyjob IN (' || keylist || ')
                       UNION
                       SELECT jd.* FROM job_dep_rec jdr, jobdep jd WHERE jd.fkeyjob=jdr.fkeydep
               ) SELECT * FROM job_dep_rec';
END;
$$;


ALTER FUNCTION public.jobdep_recursive(keylist text) OWNER TO postgres;

--
-- Name: joberror_inc(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION joberror_inc() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
DECLARE
	j RECORD;
BEGIN
	SELECT INTO j * FROM job WHERE keyjob=NEW.fkeyjob;
	IF (j.fkeyjobstat IS NOT NULL) THEN
		UPDATE jobstat SET errorcount=errorcount+1 WHERE keyjobstat=j.fkeyjobstat;
	END IF;
	RETURN NULL;
END;
$$;


ALTER FUNCTION public.joberror_inc() OWNER TO farmer;

--
-- Name: jobtaskassignment_update(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION jobtaskassignment_update() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
BEGIN
    IF OLD.memory IS NOT NULL AND OLD.memory > NEW.memory THEN
        NEW.memory := OLD.memory;
    END IF;
    RETURN NEW;
END;
$$;


ALTER FUNCTION public.jobtaskassignment_update() OWNER TO farmer;

--
-- Name: pg_stat_statements(); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION pg_stat_statements(OUT userid oid, OUT dbid oid, OUT query text, OUT calls bigint, OUT total_time double precision, OUT rows bigint, OUT shared_blks_hit bigint, OUT shared_blks_read bigint, OUT shared_blks_written bigint, OUT local_blks_hit bigint, OUT local_blks_read bigint, OUT local_blks_written bigint, OUT temp_blks_read bigint, OUT temp_blks_written bigint) RETURNS SETOF record
    LANGUAGE c
    AS '$libdir/pg_stat_statements', 'pg_stat_statements';


ALTER FUNCTION public.pg_stat_statements(OUT userid oid, OUT dbid oid, OUT query text, OUT calls bigint, OUT total_time double precision, OUT rows bigint, OUT shared_blks_hit bigint, OUT shared_blks_read bigint, OUT shared_blks_written bigint, OUT local_blks_hit bigint, OUT local_blks_read bigint, OUT local_blks_written bigint, OUT temp_blks_read bigint, OUT temp_blks_written bigint) OWNER TO postgres;

--
-- Name: pg_stat_statements_reset(); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION pg_stat_statements_reset() RETURNS void
    LANGUAGE c
    AS '$libdir/pg_stat_statements', 'pg_stat_statements_reset';


ALTER FUNCTION public.pg_stat_statements_reset() OWNER TO postgres;

--
-- Name: return_slave_tasks(integer, boolean, boolean); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION return_slave_tasks(_keyhost integer, commithoststatus boolean, preassigned boolean) RETURNS void
    LANGUAGE plpgsql
    AS $$
BEGIN
	IF commitHostStatus THEN
		UPDATE hoststatus SET slaveframes='', fkeyjob=NULL WHERE fkeyhost=_keyhost;
	END IF;
	IF preassigned THEN
		UPDATE jobtask SET status = 'new', started=NULL, memory=NULL, fkeyjobcommandhistory=NULL WHERE fkeyhost=_keyhost AND status IN ('assigned','busy');
	ELSE
		UPDATE jobtask SET status = 'new', fkeyhost=NULL, started=NULL, memory=NULL, fkeyjobcommandhistory=NULL WHERE fkeyhost=_keyhost AND status IN ('assigned','busy');
	END IF;
END;
$$;


ALTER FUNCTION public.return_slave_tasks(_keyhost integer, commithoststatus boolean, preassigned boolean) OWNER TO farmer;

--
-- Name: return_slave_tasks_2(integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION return_slave_tasks_2(_keyhost integer) RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
	j job;
BEGIN
	FOR j IN SELECT * from job WHERE keyjob IN (
		SELECT fkeyjob FROM JobTask WHERE fkeyhost=_keyhost AND status IN ('assigned','busy') GROUP BY fkeyjob
		)
	LOOP
		IF j.packettype = 'preassigned' THEN
			UPDATE jobtask SET status = 'new' 
				       WHERE fkeyhost=_keyhost 
					 AND fkeyjob=j.keyjob 
					 AND status IN ('assigned','busy');
		ELSE
			UPDATE jobtask SET status = 'new', 
					   fkeyhost=NULL 
				       WHERE fkeyhost=_keyhost 
				         AND fkeyjob=j.keyjob 
				         AND status IN ('assigned','busy');
		END IF;
	END LOOP;
END;
$$;


ALTER FUNCTION public.return_slave_tasks_2(_keyhost integer) OWNER TO farmer;

--
-- Name: return_slave_tasks_3(integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION return_slave_tasks_3(_keyhost integer) RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
	cancelledStatusKey int;
BEGIN
	PERFORM return_slave_tasks_2(_keyhost);
	
	SELECT INTO cancelledStatusKey keyjobassignmentstatus FROM jobassignmentstatus WHERE status='cancelled';
	UPDATE JobAssignment SET fkeyjobassignmentstatus=cancelledStatusKey 
	                     WHERE fkeyhost=_keyhost 
	                       AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus 
								FROM jobassignmentstatus 
								WHERE status IN ('ready','copy','busy'));
	UPDATE JobTaskAssignment SET fkeyjobassignmentstatus=cancelledStatusKey 
				 WHERE fkeyjobassignment IN (SELECT keyjobassignment 
								FROM jobassignment 
								WHERE fkeyhost=_keyhost) 
				   AND fkeyjobassignmentstatus IN (SELECT keyjobassignmentstatus 
								      FROM jobassignmentstatus 
								      WHERE status IN ('ready','copy','busy'));
END;
$$;


ALTER FUNCTION public.return_slave_tasks_3(_keyhost integer) OWNER TO farmer;

--
-- Name: sync_host_to_hoststatus(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION sync_host_to_hoststatus() RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
BEGIN
	DELETE FROM HostStatus WHERE fkeyhost NOT IN (SELECT keyhost FROM Host);
	INSERT INTO HostStatus (fkeyhost) SELECT keyhost FROM Host WHERE keyhost NOT IN (SELECT fkeyhost FROM HostStatus WHERE fkeyhost IS NOT NULL);
END;
$$;


ALTER FUNCTION public.sync_host_to_hoststatus() OWNER TO farmer;

--
-- Name: sync_host_to_hoststatus_trigger(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION sync_host_to_hoststatus_trigger() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
BEGIN
	PERFORM sync_host_to_hoststatus();
	RETURN NEW;
END
$$;


ALTER FUNCTION public.sync_host_to_hoststatus_trigger() OWNER TO farmer;

--
-- Name: update_host(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION update_host() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
DECLARE
currentjob job;
BEGIN
IF (NEW.slavestatus IS NOT NULL)
 AND (NEW.slavestatus != OLD.slavestatus) THEN
	NEW.lastStatusChange = NOW();
	IF (NEW.slavestatus != 'busy') THEN
		NEW.fkeyjobtask = NULL;
	END IF;
END IF;

-- Increment Job.hostsOnJob
--IF (NEW.fkeyjob IS NOT NULL AND (NEW.fkeyjob != OLD.fkeyjob OR OLD.fkeyjob IS NULL) ) THEN
--	UPDATE Job SET hostsOnJob=hostsOnJob+1 WHERE keyjob=NEW.fkeyjob;
--END IF;

-- Decrement Job.hostsOnJob
--IF (OLD.fkeyjob IS NOT NULL AND (OLD.fkeyjob != NEW.fkeyjob OR NEW.fkeyjob IS NULL) ) THEN
--	UPDATE Job SET hostsOnJob=hostsOnJob-1 WHERE keyjob=OLD.fkeyjob;
--END IF;

IF (NEW.slavestatus IS NOT NULL)
 AND ((NEW.slavestatus != OLD.slavestatus) OR (NEW.fkeyjobtask != OLD.fkeyjobtask) OR ((OLD.fkeyjobtask IS NULL) != (NEW.fkeyjobtask IS NULL))) THEN
	SELECT INTO currentjob * from job where keyjob=NEW.fkeyjob;
	UPDATE hosthistory SET duration = now() - datetime, nextstatus=NEW.slavestatus WHERE duration is null and fkeyhost=NEW.keyhost;
	INSERT INTO hosthistory (datetime,fkeyhost,fkeyjob,fkeyjobstat,status,laststatus,fkeyjobtype,fkeyjobtask) values (now(),NEW.keyhost,NEW.fkeyjob,currentjob.fkeyjobstat,NEW.slavestatus,OLD.slavestatus,currentjob.fkeyjobtype,NEW.fkeyjobtask);
END IF;
RETURN NEW;
END;
$$;


ALTER FUNCTION public.update_host() OWNER TO farmer;

--
-- Name: update_host_assignment_count(integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION update_host_assignment_count(_keyhost integer) RETURNS void
    LANGUAGE plpgsql
    AS $$ 
BEGIN
UPDATE HostStatus hs
SET activeAssignmentCount = coalesce((
	SELECT sum(assignslots)  
	FROM jobassignment ja
        WHERE ja.fkeyjobassignmentstatus < 4 AND ja.fkeyhost = _keyhost),0),
    lastAssignmentChange=NOW()
WHERE hs.fkeyhost = _keyhost;
END;
$$;


ALTER FUNCTION public.update_host_assignment_count(_keyhost integer) OWNER TO farmer;

--
-- Name: update_hostload(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION update_hostload() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
DECLARE
	seconds float;
BEGIN
	IF NEW.loadavg != OLD.loadavg THEN
		IF OLD.loadavgadjusttimestamp IS NULL THEN
			seconds := 1;
		ELSE
			seconds := extract(epoch FROM (NOW() - OLD.loadavgadjusttimestamp)::interval);
		END IF;
		IF seconds > 0 THEN
			-- 20 Second Half-Life
			NEW.loadavgadjust = OLD.loadavgadjust / ( 1.0 + (seconds * .05) );
			IF NEW.loadavgadjust < .01 THEN
				NEW.loadavgadjust = 0.0;
			END IF;
			NEW.loadavgadjusttimestamp = NOW();
		END IF; 

		-- Remove this once all the clients are updated
		UPDATE Host SET loadavg=NEW.loadavg + NEW.loadavgadjust WHERE keyhost=NEW.fkeyhost;

	END IF;
	RETURN NEW;
END;
$$;


ALTER FUNCTION public.update_hostload() OWNER TO farmer;

--
-- Name: update_hostservice(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION update_hostservice() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
DECLARE
BEGIN
IF (NEW.pulse != OLD.pulse) OR (NEW.pulse IS NOT NULL AND OLD.pulse IS NULL) THEN
	UPDATE HostStatus Set slavepulse=NEW.pulse WHERE HostStatus.fkeyhost=NEW.fkeyhost;
END IF;
RETURN NEW;
END;
$$;


ALTER FUNCTION public.update_hostservice() OWNER TO farmer;

--
-- Name: update_job_deps(integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION update_job_deps(_keyjob integer) RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
    childDepRec RECORD;
    jobDepRec RECORD;
    newstatus text;
BEGIN
    FOR jobDepRec IN
        SELECT job.keyjob as childKey
        FROM jobdep
        JOIN job ON jobdep.fkeyjob = job.keyjob
        WHERE fkeydep = _keyjob
    LOOP
        newstatus := 'ready';
        FOR childDepRec IN
            SELECT status
            FROM jobdep
            JOIN job ON jobdep.fkeydep = job.keyjob
            WHERE keyjob != _keyjob AND fkeyjob = jobDepRec.childKey
        LOOP
            IF childDepRec.status != 'done' AND childDepRec.status != 'deleted' THEN
                newstatus := 'holding';
            END IF;
        END LOOP;
        UPDATE job SET status = newstatus WHERE keyjob = jobDepRec.childKey;
        INSERT INTO jobhistory (fkeyjob, message) VALUES (jobDepRec.childKey, 'Job status set to: ' || newstatus || ' by parent job'); 
    END LOOP;
    RETURN;
END;    
$$;


ALTER FUNCTION public.update_job_deps(_keyjob integer) OWNER TO farmer;

--
-- Name: update_job_hard_deps(integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION update_job_hard_deps(_keyjob integer) RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
    childDepRec RECORD;
    jobDepRec RECORD;
    newstatus text;
BEGIN
    -- iterate through all a jobs children when its status has changed
    FOR jobDepRec IN
        SELECT fkeyjob as childKey 
        FROM jobdep
        WHERE fkeydep = _keyjob AND deptype = 1
    LOOP
        newstatus := 'ready';

        -- if it's a hard dependency, check all possible parents to make sure they are
        -- done as well
	FOR childDepRec IN
		SELECT status
                FROM jobdep
                JOIN job ON jobdep.fkeydep = job.keyjob
                WHERE keyjob != _keyjob AND fkeyjob = jobDepRec.childKey AND deptype = 1
        LOOP
                -- if a parent is not done or deleted then the child should stay on holding
                IF childDepRec.status != 'done' AND childDepRec.status != 'deleted' THEN
                    newstatus := 'holding';
                END IF;
        END LOOP;

	-- for suspend dependencies, we want to fire regardless of other parents
	--IF (_oldstatus = 'started' OR _oldstatus = 'ready') AND _newstatus = 'suspended' AND jobDepRec.childDepType = 3 THEN
	--END IF;

        -- this update should be a null op if the newstatus doesn't change
        UPDATE job SET status = newstatus WHERE keyjob = jobDepRec.childKey;
        INSERT INTO jobhistory (fkeyjob, message) VALUES (jobDepRec.childKey, 'Job status set to: ' || newstatus || ' by hard parent job');
    END LOOP;
    RETURN;
END;    
$$;


ALTER FUNCTION public.update_job_hard_deps(_keyjob integer) OWNER TO farmer;

--
-- Name: update_job_hard_deps_2(integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION update_job_hard_deps_2(_keyjob integer) RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
    childDepRec RECORD;
    jobDepRec RECORD;
    newstatus text;
BEGIN
    -- iterate through all a jobs children when its status has changed
    FOR jobDepRec IN
        SELECT fkeyjob as childKey 
        FROM jobdep
        WHERE fkeydep = _keyjob AND deptype = 1
    LOOP
        newstatus := 'ready';

        -- if it's a hard dependency, check all possible parents to make sure they are
        -- done as well
	FOR childDepRec IN
		SELECT status
                FROM jobdep
                JOIN job ON jobdep.fkeydep = job.keyjob
                WHERE keyjob != _keyjob AND fkeyjob = jobDepRec.childKey AND deptype = 1
        LOOP
                -- if a parent is not done or deleted then the child should stay on holding
                IF childDepRec.status != 'done' AND childDepRec.status != 'deleted' THEN
                    newstatus := 'holding';
                END IF;
		EXIT WHEN newStatus = 'holding';
        END LOOP;

	-- for suspend dependencies, we want to fire regardless of other parents
	--IF (_oldstatus = 'started' OR _oldstatus = 'ready') AND _newstatus = 'suspended' AND jobDepRec.childDepType = 3 THEN
	--END IF;

        -- this update should be a null op if the newstatus doesn't change
        UPDATE job SET status = newstatus WHERE keyjob = jobDepRec.childKey;
        INSERT INTO jobhistory (fkeyjob, message) VALUES (jobDepRec.childKey, 'Job status set to: ' || newstatus || ' by hard parent job');
    END LOOP;
    RETURN;
END;    
$$;


ALTER FUNCTION public.update_job_hard_deps_2(_keyjob integer) OWNER TO farmer;

--
-- Name: update_job_health(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION update_job_health() RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
	j job;
BEGIN
	FOR j IN SELECT * FROM job WHERE status='started' ORDER BY keyjob ASC LOOP
		PERFORM update_single_job_health( j );
	END LOOP;
	RETURN;
END;
$$;


ALTER FUNCTION public.update_job_health() OWNER TO farmer;

--
-- Name: update_job_health_by_key(integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION update_job_health_by_key(_jobkey integer) RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
	j job;
BEGIN
	SELECT INTO j * FROM job WHERE keyjob=_jobkey;
	PERFORM update_single_job_health( j );
	RETURN;
END;
$$;


ALTER FUNCTION public.update_job_health_by_key(_jobkey integer) OWNER TO farmer;

--
-- Name: update_job_links(integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION update_job_links(_keyjob integer) RETURNS void
    LANGUAGE plpgsql
    AS $$ 
BEGIN
UPDATE jobtask SET status = 'new' WHERE keyjobtask IN (
        SELECT jt1.keyjobtask
        FROM jobdep 
        JOIN jobtask AS jt1 ON jt1.fkeyjob = jobdep.fkeyjob AND jt1.status = 'holding'
        JOIN jobtask AS jt2 ON jt2.fkeyjob = jobdep.fkeydep AND jt2.status = 'done'
        WHERE jobdep.deptype = 2 AND jobdep.fkeydep = _keyjob AND jt1.jobtask = jt2.jobtask
);  
END;
$$;


ALTER FUNCTION public.update_job_links(_keyjob integer) OWNER TO farmer;

--
-- Name: update_job_other_deps(integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION update_job_other_deps(_keyjob integer) RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
    jobDepRec RECORD;
BEGIN
    -- iterate through all a jobs children when its status has changed
    FOR jobDepRec IN
        SELECT fkeyjob as childKey 
        FROM jobdep
        WHERE fkeydep = _keyjob AND (deptype = 3 OR deptype = 4)
    LOOP
        -- this update should be a null op if the newstatus doesn't change
        UPDATE job SET status = 'ready' WHERE keyjob = jobDepRec.childKey AND status = 'holding';
        INSERT INTO jobhistory (fkeyjob, message) VALUES (jobDepRec.childKey, 'Job status set to: ' || newstatus || ' by hard parent job');
    END LOOP;
    RETURN;
END;    
$$;


ALTER FUNCTION public.update_job_other_deps(_keyjob integer) OWNER TO farmer;

--
-- Name: update_job_soft_deps(integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION update_job_soft_deps(_keyjob integer) RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
    childDepRec RECORD;
    jobDepRec RECORD;
    newstatus text;
BEGIN
    -- iterate through all a jobs children, find tasks that can be now be run
    FOR jobDepRec IN
        SELECT jt1.keyjobtask as _taskKey, jobdep.fkeyjob as _depKey
        FROM jobdep 
        JOIN jobtask AS jt1 ON jt1.fkeyjob = jobdep.fkeyjob AND jt1.status = 'holding'
        JOIN jobtask AS jt2 ON jt2.fkeyjob = jobdep.fkeydep AND jt2.status = 'done'
        WHERE jobdep.deptype = 2 AND jobdep.fkeydep = _keyjob AND jt1.jobtask = jt2.jobtask
    LOOP
        newstatus := 'new';

        -- check all possible parents to make sure they are done as well
	FOR childDepRec IN
		SELECT jt2.status
                FROM jobdep
                JOIN jobtask AS jt1 ON jt1.fkeyjob = jobdep.fkeyjob AND jt1.status = 'holding'
		JOIN jobtask AS jt2 ON jt2.fkeyjob = jobdep.fkeydep AND jt2.status != 'done'
		WHERE jobdep.deptype = 2 AND jobdep.fkeyjob = jobDepRec._depKey AND jt1.jobtask = jt2.jobtask AND jt1.keyjobtask = jobDepRec._taskKey
        LOOP
                -- if a parent is not done or canceled then the child should stay on holding
                IF childDepRec.status != 'done' AND childDepRec.status != 'canceled' THEN
                    newstatus := 'holding';
                END IF;
        END LOOP;


	RAISE NOTICE 'jobtask % change to status %', jobDepRec._taskKey, newstatus;
        -- this update should be a null op if the newstatus doesn't change
        UPDATE jobtask SET status = newstatus WHERE keyjobtask = jobDepRec._taskKey;
    END LOOP;
    RETURN;
END;    
$$;


ALTER FUNCTION public.update_job_soft_deps(_keyjob integer) OWNER TO farmer;

--
-- Name: update_job_stats(integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION update_job_stats(_keyjob integer) RETURNS void
    LANGUAGE plpgsql COST 1000
    AS $$
DECLARE
        _totaltime bigint := 0;
        _cputime bigint := 0;
        _byteswrite bigint := 0;
        _bytesread bigint := 0;
        _opswrite bigint := 0;
        _opsread bigint := 0;
        _avgtasktime bigint := 0;
        _avgdonetime int := 0;
        _avgmem bigint := 0;
        _efficiency float := 0.0;

BEGIN

SELECT INTO _avgtasktime, _avgmem GREATEST(0, 
                AVG( GREATEST(0, 
                              EXTRACT(epoch FROM (coalesce(jobtaskassignment.ended,now())-jobtaskassignment.started))*job.slots
                             )
                   )
                )::int,
                AVG( jobtaskassignment.memory )::int
        FROM JobTaskAssignment 
        INNER JOIN JobTask ON JobTask.fkeyjobtaskassignment=jobtaskassignment.keyjobtaskassignment 
        JOIN job ON jobtask.fkeyjob=job.keyjob
        WHERE fkeyjob=_keyjob
        AND jobtaskassignment.started IS NOT NULL;

SELECT INTO _avgdonetime AVG(date_part('epoch'::text, jobassignment.ended::timestamp with time zone - jobassignment.started::timestamp with time zone) * jobassignment.assignslots )::integer AS avgtime
   FROM jobassignment
   JOIN job ON jobassignment.fkeyjob = job.keyjob AND (job.status = ANY (ARRAY['ready'::text, 'started'::text]))
  WHERE fkeyjob=_keyjob AND jobassignment.ended IS NOT NULL
  GROUP BY job.keyjob;

SELECT INTO _totaltime, _cputime, _byteswrite, _bytesread, _opswrite, _opsread
        extract(epoch from sum((coalesce(ended,now())-started)))::bigint,
        sum(usertime+systime),
        avg(byteswrite)::bigint,
        avg(bytesread)::bigint,
        avg(opswrite)::bigint,
        avg(opsread)::bigint
        FROM jobassignment ja
        WHERE fkeyjob = _keyjob AND fkeyjobassignmentstatus < 5;

        UPDATE jobstatus SET
        averagememory = _avgmem,
        tasksaveragetime = _avgtasktime,
        averagedonetime = _avgdonetime,
        totaltime = _totaltime,
        cputime = _cputime,
        byteswrite = _byteswrite,
        bytesread = _bytesread,
        opswrite = _opswrite,
        opsread = _opsread,
        efficiency = (
select avg(get_job_efficiency) from get_job_efficiency(_keyjob)
        )::double precision
    WHERE fkeyjob = _keyjob;

    RETURN;
END;
$$;


ALTER FUNCTION public.update_job_stats(_keyjob integer) OWNER TO farmer;

--
-- Name: update_job_tallies(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION update_job_tallies() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
DECLARE
        cur RECORD;
BEGIN
        BEGIN
                FOR cur IN SELECT * FROM job_tally_dirty LOOP
                        PERFORM update_job_task_counts( cur.fkeyjob );
                END LOOP;
                DELETE FROM job_tally_dirty;
        EXCEPTION
                WHEN undefined_table THEN
                        -- Do nothing
        END;
RETURN NEW;
END;
$$;


ALTER FUNCTION public.update_job_tallies() OWNER TO farmer;

--
-- Name: update_job_task_counts(integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION update_job_task_counts(_keyjob integer) RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
        cur RECORD;
        unassigned integer := 0;
        busy integer := 0;
        assigned integer := 0;
        done integer := 0;
        cancelled integer := 0;
        suspended integer := 0;
        holding integer := 0;
BEGIN
        FOR cur IN
                SELECT status, count(*) as c FROM jobtask WHERE fkeyjob=_keyjob
                GROUP BY status
        LOOP
                IF( cur.status = 'new' ) THEN
                        unassigned := cur.c;
                ELSIF( cur.status = 'assigned' ) THEN
                        assigned := cur.c;
                ELSIF( cur.status = 'busy' ) THEN
                        busy := cur.c;
                ELSIF( cur.status = 'done' ) THEN
                        done := cur.c;
                ELSIF( cur.status = 'cancelled' ) THEN
                        cancelled := cur.c;
                ELSIF( cur.status = 'suspended' ) THEN
                        suspended := cur.c;
                ELSIF( cur.status = 'holding' ) THEN
                        holding := cur.c;
                END IF;
        END LOOP;

        UPDATE JobStatus SET
                taskscount = unassigned + assigned + busy + done + cancelled + suspended + holding,
                tasksUnassigned = unassigned,
                tasksAssigned = assigned,
                tasksBusy = busy,
                tasksDone = done,
                tasksCancelled = cancelled,
                tasksSuspended = suspended
        WHERE fkeyjob = _keyjob;
        RETURN;
END;
$$;


ALTER FUNCTION public.update_job_task_counts(_keyjob integer) OWNER TO farmer;

--
-- Name: update_job_task_counts_2(integer); Type: FUNCTION; Schema: public; Owner: postgres
--

CREATE FUNCTION update_job_task_counts_2(_keyjob integer) RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
        cur RECORD;
        unassigned integer := 0;
        busy integer := 0;
        assigned integer := 0;
        done integer := 0;
        cancelled integer := 0;
        suspended integer := 0;
        holding integer := 0;
        bitmap text := '';
BEGIN
        FOR cur IN
                SELECT * FROM jobtask WHERE fkeyjob=_keyjob ORDER BY jobtask
        LOOP
		bitmap := bitmap || substring(cur.status from 1 for 1);
                IF( cur.status = 'new' ) THEN
                        unassigned := unassigned + 1;
                ELSIF( cur.status = 'assigned' ) THEN
                        assigned := assigned + 1;
                ELSIF( cur.status = 'busy' ) THEN
                        busy := busy + 1;
                ELSIF( cur.status = 'done' ) THEN
                        done := done + 1;
                ELSIF( cur.status = 'cancelled' ) THEN
                        cancelled := cancelled + 1;
                ELSIF( cur.status = 'suspended' ) THEN
                        suspended := suspended + 1;
                ELSIF( cur.status = 'holding' ) THEN
                        holding := holding + 1;
                END IF;
        END LOOP;

        UPDATE JobStatus SET
                taskscount = unassigned + assigned + busy + done + cancelled + suspended + holding,
                tasksUnassigned = unassigned,
                tasksAssigned = assigned,
                tasksBusy = busy,
                tasksDone = done,
                tasksCancelled = cancelled,
                tasksSuspended = suspended,
                taskBitmap = bitmap
        WHERE fkeyjob = _keyjob;
        RETURN;
END;
$$;


ALTER FUNCTION public.update_job_task_counts_2(_keyjob integer) OWNER TO postgres;

--
-- Name: update_jobtask_counts(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION update_jobtask_counts() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
DECLARE
BEGIN
        -- Update Job Counters when a tasks status changes
        IF (NEW.status != OLD.status) THEN
		PERFORM update_job_task_counts(NEW.fkeyjob);
        END IF;
RETURN new;
END;
$$;


ALTER FUNCTION public.update_jobtask_counts() OWNER TO farmer;

--
-- Name: update_laststatuschange(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION update_laststatuschange() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
DECLARE
currentjob job;
BEGIN
IF (NEW.slavestatus IS NOT NULL)
 AND (NEW.slavestatus != OLD.slavestatus) THEN
	NEW.lastStatusChange = NOW();
	IF (NEW.slavestatus != 'busy') THEN
		NEW.fkeyjobtask = NULL;
	END IF;
END IF;

-- Increment Job.hostsOnJob
IF (NEW.fkeyjob IS NOT NULL AND (NEW.fkeyjob != OLD.fkeyjob OR OLD.fkeyjob IS NULL) ) THEN
	UPDATE Job SET hostsOnJob=hostsOnJob+1 WHERE keyjob=NEW.fkeyjob;
END IF;

-- Decrement Job.hostsOnJob
IF (OLD.fkeyjob IS NOT NULL AND (OLD.fkeyjob != NEW.fkeyjob OR NEW.fkeyjob IS NULL) ) THEN
	UPDATE Job SET hostsOnJob=hostsOnJob-1 WHERE keyjob=OLD.fkeyjob;
END IF;

IF (NEW.slavestatus IS NOT NULL)
 AND ((NEW.slavestatus != OLD.slavestatus) OR (NEW.fkeyjobtask != OLD.fkeyjobtask) OR ((OLD.fkeyjobtask IS NULL) != (NEW.fkeyjobtask IS NULL))) THEN
	SELECT INTO currentjob * from job where keyjob=NEW.fkeyjob;
	UPDATE hosthistory SET duration = now() - datetime, nextstatus=NEW.slavestatus WHERE duration is null and fkeyhost=NEW.keyhost;
	INSERT INTO hosthistory (datetime,fkeyhost,fkeyjob,fkeyjobstat,status,laststatus,fkeyjobtype,fkeyjobtask) values (now(),NEW.keyhost,NEW.fkeyjob,currentjob.fkeyjobstat,NEW.slavestatus,OLD.slavestatus,currentjob.fkeyjobtype,NEW.fkeyjobtask);
END IF;

RETURN new;
END;
$$;


ALTER FUNCTION public.update_laststatuschange() OWNER TO farmer;

--
-- Name: update_project_tempo(); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION update_project_tempo() RETURNS void
    LANGUAGE plpgsql
    AS $$
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
$$;


ALTER FUNCTION public.update_project_tempo() OWNER TO farmer;

--
-- Name: job_keyjob_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE job_keyjob_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.job_keyjob_seq OWNER TO farmer;

--
-- Name: job_keyjob_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('job_keyjob_seq', 1, false);


--
-- Name: job; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE job (
    keyjob integer DEFAULT nextval('job_keyjob_seq'::regclass) NOT NULL,
    fkeyelement integer,
    fkeyhost integer,
    fkeyjobtype integer,
    fkeyproject integer,
    fkeyusr integer,
    hostlist text,
    job text,
    jobtime text,
    outputpath text,
    status text DEFAULT 'new'::text,
    submitted integer,
    started integer,
    ended integer,
    expires integer,
    deleteoncomplete integer,
    hostsonjob integer DEFAULT 0,
    taskscount integer DEFAULT 0,
    tasksunassigned integer DEFAULT 0,
    tasksdone integer DEFAULT 0,
    tasksaveragetime integer,
    priority integer DEFAULT 50,
    errorcount integer,
    queueorder integer,
    packettype text DEFAULT 'random'::text,
    packetsize integer,
    queueeta integer,
    notifyonerror text,
    notifyoncomplete text,
    maxtasktime integer DEFAULT 3600,
    cleaned integer,
    filesize integer,
    btinfohash text,
    rendertime integer,
    abversion text,
    deplist text,
    args text,
    filename text,
    filemd5sum text,
    fkeyjobstat integer,
    username text,
    domain text,
    password text,
    stats text,
    currentmapserverweight double precision,
    loadtimeaverage integer,
    tasksassigned integer DEFAULT 0,
    tasksbusy integer DEFAULT 0,
    prioritizeoutertasks boolean,
    outertasksassigned boolean,
    lastnotifiederrorcount integer,
    taskscancelled integer DEFAULT 0,
    taskssuspended integer DEFAULT 0,
    health real,
    maxloadtime integer,
    license text,
    maxmemory integer,
    fkeyjobparent integer,
    endedts timestamp without time zone,
    startedts timestamp without time zone,
    submittedts timestamp without time zone,
    maxhosts integer,
    personalpriority integer DEFAULT 50,
    loggingenabled boolean DEFAULT true NOT NULL,
    environment text,
    runassubmitter boolean,
    checkfilemd5 boolean,
    uploadedfile boolean,
    framenth integer,
    framenthmode integer,
    exclusiveassignment boolean,
    hastaskprogress boolean,
    minmemory integer,
    scenename text,
    shotname text,
    slots integer DEFAULT 4 NOT NULL,
    fkeyjobfilterset integer,
    maxerrors integer DEFAULT 27 NOT NULL,
    notifycompletemessage text,
    notifyerrormessage text,
    fkeywrangler integer,
    maxquiettime integer DEFAULT 3600 NOT NULL,
    autoadaptslots integer DEFAULT (-1) NOT NULL,
    fkeyjobenvironment integer,
    suspendedts timestamp without time zone,
    toggleflags integer
);


ALTER TABLE public.job OWNER TO farmer;

--
-- Name: update_single_job_health(job); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION update_single_job_health(j job) RETURNS void
    LANGUAGE plpgsql
    AS $$
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
$$;


ALTER FUNCTION public.update_single_job_health(j job) OWNER TO farmer;

--
-- Name: updatejoblicensecounts(integer, integer); Type: FUNCTION; Schema: public; Owner: farmer
--

CREATE FUNCTION updatejoblicensecounts(_fkeyjob integer, countchange integer) RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
    serv service;
BEGIN
    --FOR serv IN SELECT * FROM Service s JOIN JobService js ON (s.keyService = js.fkeyService AND js.fkeyjob=_fkeyjob) WHERE fkeyLicense IS NOT NULL LOOP
    --    UPDATE License SET inUse=greatest(0,coalesce(inUse,0) + countChange) WHERE keylicense=serv.fkeylicense;
    --END LOOP;
    RETURN;
END;
$$;


ALTER FUNCTION public.updatejoblicensecounts(_fkeyjob integer, countchange integer) OWNER TO farmer;

--
-- Name: /; Type: OPERATOR; Schema: public; Owner: farmer
--

CREATE OPERATOR / (
    PROCEDURE = interval_divide,
    LEFTARG = interval,
    RIGHTARG = interval
);


ALTER OPERATOR public./ (interval, interval) OWNER TO farmer;

--
-- Name: host_keyhost_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE host_keyhost_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.host_keyhost_seq OWNER TO farmer;

--
-- Name: host_keyhost_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('host_keyhost_seq', 1, false);


--
-- Name: host; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE host (
    keyhost integer DEFAULT nextval('host_keyhost_seq'::regclass) NOT NULL,
    backupbytes text,
    cpus integer,
    description text,
    diskusage text,
    fkeyjob integer,
    host text NOT NULL,
    manufacturer text,
    model text,
    os text,
    rendertime text,
    slavepluginlist text,
    sn text,
    version text,
    renderrate double precision,
    dutycycle double precision,
    memory integer,
    mhtz integer,
    online integer,
    uid integer,
    slavepacketweight double precision,
    framecount integer,
    viruscount integer,
    virustimestamp date,
    errortempo integer,
    fkeyhost_backup integer,
    oldkey integer,
    abversion text,
    laststatuschange timestamp without time zone,
    loadavg double precision,
    allowmapping boolean DEFAULT false,
    allowsleep boolean DEFAULT false,
    fkeyjobtask integer,
    wakeonlan boolean DEFAULT false,
    architecture text,
    loc_x real,
    loc_y real,
    loc_z real,
    ostext text,
    bootaction text,
    fkeydiskimage integer,
    syncname text,
    fkeylocation integer,
    cpuname text,
    osversion text,
    slavepulse timestamp without time zone,
    puppetpulse timestamp without time zone,
    maxassignments integer DEFAULT 2 NOT NULL,
    fkeyuser integer,
    maxmemory integer,
    userisloggedin boolean
);


ALTER TABLE public.host OWNER TO farmer;

--
-- Name: hostinterface_keyhostinterface_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE hostinterface_keyhostinterface_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.hostinterface_keyhostinterface_seq OWNER TO farmer;

--
-- Name: hostinterface_keyhostinterface_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('hostinterface_keyhostinterface_seq', 1, false);


--
-- Name: hostinterface; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE hostinterface (
    keyhostinterface integer DEFAULT nextval('hostinterface_keyhostinterface_seq'::regclass) NOT NULL,
    fkeyhost integer NOT NULL,
    mac text,
    ip text,
    fkeyhostinterfacetype integer,
    switchport integer,
    fkeyswitch integer,
    inst text
);


ALTER TABLE public.hostinterface OWNER TO farmer;

--
-- Name: HostInterfacesVerbose; Type: VIEW; Schema: public; Owner: farmer
--

CREATE VIEW "HostInterfacesVerbose" AS
    SELECT host.keyhost, host.backupbytes, host.cpus, host.description, host.diskusage, host.fkeyjob, host.host, host.manufacturer, host.model, host.os, host.rendertime, host.slavepluginlist, host.sn, host.version, host.renderrate, host.dutycycle, host.memory, host.mhtz, host.online, host.uid, host.slavepacketweight, host.framecount, host.viruscount, host.virustimestamp, host.errortempo, host.fkeyhost_backup, host.oldkey, host.abversion, host.laststatuschange, host.loadavg, host.allowmapping, host.allowsleep, host.fkeyjobtask, host.wakeonlan, host.architecture, host.loc_x, host.loc_y, host.loc_z, host.ostext, host.bootaction, host.fkeydiskimage, host.syncname, host.fkeylocation, host.cpuname, host.osversion, host.slavepulse, hostinterface.keyhostinterface, hostinterface.fkeyhost, hostinterface.mac, hostinterface.ip, hostinterface.fkeyhostinterfacetype, hostinterface.switchport, hostinterface.fkeyswitch, hostinterface.inst FROM (host JOIN hostinterface ON ((host.keyhost = hostinterface.fkeyhost)));


ALTER TABLE public."HostInterfacesVerbose" OWNER TO farmer;

--
-- Name: jobtask_keyjobtask_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobtask_keyjobtask_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobtask_keyjobtask_seq OWNER TO farmer;

--
-- Name: jobtask_keyjobtask_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobtask_keyjobtask_seq', 1, false);


--
-- Name: jobtask; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobtask (
    keyjobtask integer DEFAULT nextval('jobtask_keyjobtask_seq'::regclass) NOT NULL,
    fkeyhost integer,
    fkeyjob integer NOT NULL,
    status text DEFAULT 'new'::text,
    jobtask integer,
    label text,
    fkeyjoboutput integer,
    progress integer,
    fkeyjobtaskassignment integer,
    schedulepolicy integer
);


ALTER TABLE public.jobtask OWNER TO farmer;

--
-- Name: jobtype_keyjobtype_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobtype_keyjobtype_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobtype_keyjobtype_seq OWNER TO farmer;

--
-- Name: jobtype_keyjobtype_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobtype_keyjobtype_seq', 1, false);


--
-- Name: jobtype; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobtype (
    keyjobtype integer DEFAULT nextval('jobtype_keyjobtype_seq'::regclass) NOT NULL,
    jobtype text,
    fkeyservice integer,
    icon bytea
);


ALTER TABLE public.jobtype OWNER TO farmer;

--
-- Name: StatsByType; Type: VIEW; Schema: public; Owner: farmer
--

CREATE VIEW "StatsByType" AS
    SELECT ''::text AS day, jobtype.jobtype, count(*) AS frames FROM ((jobtask LEFT JOIN job ON ((jobtask.fkeyjob = job.keyjob))) LEFT JOIN jobtype ON ((jobtype.keyjobtype = job.fkeyjobtype))) WHERE (jobtask.status = 'done'::text) GROUP BY jobtype.jobtype;


ALTER TABLE public."StatsByType" OWNER TO farmer;

--
-- Name: abdownloadstat_keyabdownloadstat_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE abdownloadstat_keyabdownloadstat_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.abdownloadstat_keyabdownloadstat_seq OWNER TO farmer;

--
-- Name: abdownloadstat_keyabdownloadstat_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('abdownloadstat_keyabdownloadstat_seq', 1, false);


--
-- Name: abdownloadstat; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE abdownloadstat (
    keyabdownloadstat integer DEFAULT nextval('abdownloadstat_keyabdownloadstat_seq'::regclass) NOT NULL,
    type text,
    size integer,
    fkeyhost integer,
    "time" integer,
    abrev integer,
    finished timestamp without time zone,
    fkeyjob integer
);


ALTER TABLE public.abdownloadstat OWNER TO farmer;

--
-- Name: annotation_keyannotation_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE annotation_keyannotation_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.annotation_keyannotation_seq OWNER TO farmer;

--
-- Name: annotation_keyannotation_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('annotation_keyannotation_seq', 1, false);


--
-- Name: annotation; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE annotation (
    keyannotation integer DEFAULT nextval('annotation_keyannotation_seq'::regclass) NOT NULL,
    notes text,
    sequence text,
    framestart integer,
    frameend integer,
    markupdata text
);


ALTER TABLE public.annotation OWNER TO farmer;

--
-- Name: element_keyelement_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE element_keyelement_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.element_keyelement_seq OWNER TO farmer;

--
-- Name: element_keyelement_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('element_keyelement_seq', 1, false);


--
-- Name: element; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE element (
    keyelement integer DEFAULT nextval('element_keyelement_seq'::regclass) NOT NULL,
    daysbid double precision,
    description text,
    fkeyelement integer,
    fkeyelementstatus integer,
    fkeyelementtype integer,
    fkeyproject integer,
    fkeythumbnail integer,
    name text,
    daysscheduled double precision,
    daysestimated double precision,
    status text,
    filepath text,
    fkeyassettype integer,
    fkeypathtemplate integer,
    fkeystatusset integer,
    allowtime boolean,
    datestart date,
    datecomplete date,
    fkeyassettemplate integer,
    icon bytea,
    arsenalslotlimit integer DEFAULT (-1) NOT NULL,
    arsenalslotreserve integer DEFAULT 0 NOT NULL
);


ALTER TABLE public.element OWNER TO farmer;

--
-- Name: asset; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE asset (
    fkeyassettype integer,
    icon bytea,
    arsenalslotlimit integer,
    arsenalslotreserve integer,
    fkeystatus integer,
    keyasset integer,
    version integer
)
INHERITS (element);


ALTER TABLE public.asset OWNER TO farmer;

--
-- Name: assetdep; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE assetdep (
    keyassetdep integer NOT NULL,
    path text,
    fkeypackage integer,
    fkeyasset integer
);


ALTER TABLE public.assetdep OWNER TO farmer;

--
-- Name: assetdep_keyassetdep_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE assetdep_keyassetdep_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.assetdep_keyassetdep_seq OWNER TO farmer;

--
-- Name: assetdep_keyassetdep_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE assetdep_keyassetdep_seq OWNED BY assetdep.keyassetdep;


--
-- Name: assetdep_keyassetdep_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('assetdep_keyassetdep_seq', 1, false);


--
-- Name: assetgroup; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE assetgroup (
    fkeyassettype integer,
    arsenalslotlimit integer,
    arsenalslotreserve integer
)
INHERITS (element);


ALTER TABLE public.assetgroup OWNER TO farmer;

--
-- Name: assetprop; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE assetprop (
    keyassetprop integer NOT NULL,
    fkeyassetproptype integer,
    fkeyasset integer
);


ALTER TABLE public.assetprop OWNER TO farmer;

--
-- Name: assetprop_keyassetprop_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE assetprop_keyassetprop_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.assetprop_keyassetprop_seq OWNER TO farmer;

--
-- Name: assetprop_keyassetprop_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE assetprop_keyassetprop_seq OWNED BY assetprop.keyassetprop;


--
-- Name: assetprop_keyassetprop_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('assetprop_keyassetprop_seq', 1, false);


--
-- Name: assetproperty_keyassetproperty_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE assetproperty_keyassetproperty_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.assetproperty_keyassetproperty_seq OWNER TO farmer;

--
-- Name: assetproperty_keyassetproperty_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('assetproperty_keyassetproperty_seq', 1, false);


--
-- Name: assetproperty; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE assetproperty (
    keyassetproperty integer DEFAULT nextval('assetproperty_keyassetproperty_seq'::regclass) NOT NULL,
    name text,
    type integer,
    value text,
    fkeyelement integer
);


ALTER TABLE public.assetproperty OWNER TO farmer;

--
-- Name: assetproptype; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE assetproptype (
    keyassetproptype integer NOT NULL,
    name text,
    depth integer
);


ALTER TABLE public.assetproptype OWNER TO farmer;

--
-- Name: assetproptype_keyassetproptype_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE assetproptype_keyassetproptype_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.assetproptype_keyassetproptype_seq OWNER TO farmer;

--
-- Name: assetproptype_keyassetproptype_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE assetproptype_keyassetproptype_seq OWNED BY assetproptype.keyassetproptype;


--
-- Name: assetproptype_keyassetproptype_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('assetproptype_keyassetproptype_seq', 1, false);


--
-- Name: assetset_keyassetset_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE assetset_keyassetset_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.assetset_keyassetset_seq OWNER TO farmer;

--
-- Name: assetset_keyassetset_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('assetset_keyassetset_seq', 1, false);


--
-- Name: assetset; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE assetset (
    keyassetset integer DEFAULT nextval('assetset_keyassetset_seq'::regclass) NOT NULL,
    fkeyproject integer,
    fkeyelementtype integer,
    fkeyassettype integer,
    name text
);


ALTER TABLE public.assetset OWNER TO farmer;

--
-- Name: assetsetitem_keyassetsetitem_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE assetsetitem_keyassetsetitem_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.assetsetitem_keyassetsetitem_seq OWNER TO farmer;

--
-- Name: assetsetitem_keyassetsetitem_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('assetsetitem_keyassetsetitem_seq', 1, false);


--
-- Name: assetsetitem; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE assetsetitem (
    keyassetsetitem integer DEFAULT nextval('assetsetitem_keyassetsetitem_seq'::regclass) NOT NULL,
    fkeyassetset integer,
    fkeyassettype integer,
    fkeyelementtype integer,
    fkeytasktype integer
);


ALTER TABLE public.assetsetitem OWNER TO farmer;

--
-- Name: assettemplate_keyassettemplate_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE assettemplate_keyassettemplate_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.assettemplate_keyassettemplate_seq OWNER TO farmer;

--
-- Name: assettemplate_keyassettemplate_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('assettemplate_keyassettemplate_seq', 1, false);


--
-- Name: assettemplate; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE assettemplate (
    keyassettemplate integer DEFAULT nextval('assettemplate_keyassettemplate_seq'::regclass) NOT NULL,
    fkeyassettype integer,
    fkeyelement integer,
    fkeyproject integer,
    name text
);


ALTER TABLE public.assettemplate OWNER TO farmer;

--
-- Name: assettype_keyassettype_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE assettype_keyassettype_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.assettype_keyassettype_seq OWNER TO farmer;

--
-- Name: assettype_keyassettype_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('assettype_keyassettype_seq', 1, false);


--
-- Name: assettype; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE assettype (
    keyassettype integer DEFAULT nextval('assettype_keyassettype_seq'::regclass) NOT NULL,
    assettype text,
    deleted boolean
);


ALTER TABLE public.assettype OWNER TO farmer;

--
-- Name: attachment_keyattachment_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE attachment_keyattachment_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.attachment_keyattachment_seq OWNER TO farmer;

--
-- Name: attachment_keyattachment_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('attachment_keyattachment_seq', 1, false);


--
-- Name: attachment; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE attachment (
    keyattachment integer DEFAULT nextval('attachment_keyattachment_seq'::regclass) NOT NULL,
    caption text,
    created timestamp without time zone,
    filename text,
    fkeyelement integer,
    fkeyuser integer,
    origpath text,
    attachment text,
    url text,
    description text,
    fkeyauthor integer,
    fkeyattachmenttype integer
);


ALTER TABLE public.attachment OWNER TO farmer;

--
-- Name: attachmenttype_keyattachmenttype_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE attachmenttype_keyattachmenttype_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.attachmenttype_keyattachmenttype_seq OWNER TO farmer;

--
-- Name: attachmenttype_keyattachmenttype_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('attachmenttype_keyattachmenttype_seq', 1, false);


--
-- Name: attachmenttype; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE attachmenttype (
    keyattachmenttype integer DEFAULT nextval('attachmenttype_keyattachmenttype_seq'::regclass) NOT NULL,
    attachmenttype text
);


ALTER TABLE public.attachmenttype OWNER TO farmer;

--
-- Name: calendar_keycalendar_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE calendar_keycalendar_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.calendar_keycalendar_seq OWNER TO farmer;

--
-- Name: calendar_keycalendar_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('calendar_keycalendar_seq', 1, false);


--
-- Name: calendar; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE calendar (
    keycalendar integer DEFAULT nextval('calendar_keycalendar_seq'::regclass) NOT NULL,
    repeat integer,
    fkeycalendarcategory integer,
    url text,
    fkeyauthor integer,
    fieldname text,
    notifylist text,
    notifybatch text,
    leadtime integer,
    notifymask integer,
    fkeyusr integer,
    private integer,
    date timestamp without time zone,
    calendar text,
    fkeyproject integer
);


ALTER TABLE public.calendar OWNER TO farmer;

--
-- Name: calendarcategory_keycalendarcategory_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE calendarcategory_keycalendarcategory_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.calendarcategory_keycalendarcategory_seq OWNER TO farmer;

--
-- Name: calendarcategory_keycalendarcategory_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('calendarcategory_keycalendarcategory_seq', 1, false);


--
-- Name: calendarcategory; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE calendarcategory (
    keycalendarcategory integer DEFAULT nextval('calendarcategory_keycalendarcategory_seq'::regclass) NOT NULL,
    calendarcategory text
);


ALTER TABLE public.calendarcategory OWNER TO farmer;

--
-- Name: checklistitem_keychecklistitem_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE checklistitem_keychecklistitem_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.checklistitem_keychecklistitem_seq OWNER TO farmer;

--
-- Name: checklistitem_keychecklistitem_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('checklistitem_keychecklistitem_seq', 1, false);


--
-- Name: checklistitem; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE checklistitem (
    keychecklistitem integer DEFAULT nextval('checklistitem_keychecklistitem_seq'::regclass) NOT NULL,
    body text,
    checklistitem text,
    fkeyproject integer,
    fkeythumbnail integer,
    fkeytimesheetcategory integer,
    type text,
    fkeystatusset integer
);


ALTER TABLE public.checklistitem OWNER TO farmer;

--
-- Name: checkliststatus_keycheckliststatus_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE checkliststatus_keycheckliststatus_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.checkliststatus_keycheckliststatus_seq OWNER TO farmer;

--
-- Name: checkliststatus_keycheckliststatus_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('checkliststatus_keycheckliststatus_seq', 1, false);


--
-- Name: checkliststatus; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE checkliststatus (
    keycheckliststatus integer DEFAULT nextval('checkliststatus_keycheckliststatus_seq'::regclass) NOT NULL,
    fkeychecklistitem integer,
    fkeyelement integer,
    state integer,
    fkeyelementstatus integer
);


ALTER TABLE public.checkliststatus OWNER TO farmer;

--
-- Name: client_keyclient_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE client_keyclient_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.client_keyclient_seq OWNER TO farmer;

--
-- Name: client_keyclient_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('client_keyclient_seq', 1, false);


--
-- Name: client; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE client (
    keyclient integer DEFAULT nextval('client_keyclient_seq'::regclass) NOT NULL,
    client text,
    textcard text
);


ALTER TABLE public.client OWNER TO farmer;

--
-- Name: config_keyconfig_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE config_keyconfig_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.config_keyconfig_seq OWNER TO farmer;

--
-- Name: config_keyconfig_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('config_keyconfig_seq', 1, false);


--
-- Name: config; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE config (
    keyconfig integer DEFAULT nextval('config_keyconfig_seq'::regclass) NOT NULL,
    config text,
    value text
);


ALTER TABLE public.config OWNER TO farmer;

--
-- Name: darwinweight; Type: TABLE; Schema: public; Owner: farmers; Tablespace: 
--

CREATE TABLE darwinweight (
    keydarwinscore integer NOT NULL,
    shotname text NOT NULL,
    projectname text NOT NULL,
    weight integer DEFAULT 1 NOT NULL
);


ALTER TABLE public.darwinweight OWNER TO farmers;

--
-- Name: darwinweight_keydarwinscore_seq; Type: SEQUENCE; Schema: public; Owner: farmers
--

CREATE SEQUENCE darwinweight_keydarwinscore_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.darwinweight_keydarwinscore_seq OWNER TO farmers;

--
-- Name: darwinweight_keydarwinscore_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmers
--

ALTER SEQUENCE darwinweight_keydarwinscore_seq OWNED BY darwinweight.keydarwinscore;


--
-- Name: darwinweight_keydarwinscore_seq; Type: SEQUENCE SET; Schema: public; Owner: farmers
--

SELECT pg_catalog.setval('darwinweight_keydarwinscore_seq', 1, false);


--
-- Name: delivery; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE delivery (
    icon bytea,
    arsenalslotlimit integer,
    arsenalslotreserve integer
)
INHERITS (element);


ALTER TABLE public.delivery OWNER TO farmer;

--
-- Name: deliveryelement_keydeliveryshot_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE deliveryelement_keydeliveryshot_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.deliveryelement_keydeliveryshot_seq OWNER TO farmer;

--
-- Name: deliveryelement_keydeliveryshot_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('deliveryelement_keydeliveryshot_seq', 1, false);


--
-- Name: deliveryelement; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE deliveryelement (
    keydeliveryshot integer DEFAULT nextval('deliveryelement_keydeliveryshot_seq'::regclass) NOT NULL,
    fkeydelivery integer,
    fkeyelement integer,
    framestart integer,
    frameend integer
);


ALTER TABLE public.deliveryelement OWNER TO farmer;

--
-- Name: demoreel_keydemoreel_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE demoreel_keydemoreel_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.demoreel_keydemoreel_seq OWNER TO farmer;

--
-- Name: demoreel_keydemoreel_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('demoreel_keydemoreel_seq', 1, false);


--
-- Name: demoreel; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE demoreel (
    keydemoreel integer DEFAULT nextval('demoreel_keydemoreel_seq'::regclass) NOT NULL,
    demoreel text,
    datesent date,
    projectlist text,
    contactinfo text,
    notes text,
    playlist text,
    shippingtype integer
);


ALTER TABLE public.demoreel OWNER TO farmer;

--
-- Name: diskimage_keydiskimage_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE diskimage_keydiskimage_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.diskimage_keydiskimage_seq OWNER TO farmer;

--
-- Name: diskimage_keydiskimage_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('diskimage_keydiskimage_seq', 1, false);


--
-- Name: diskimage; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE diskimage (
    keydiskimage integer DEFAULT nextval('diskimage_keydiskimage_seq'::regclass) NOT NULL,
    diskimage text,
    path text,
    created timestamp without time zone
);


ALTER TABLE public.diskimage OWNER TO farmer;

--
-- Name: dynamichostgroup_keydynamichostgroup_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE dynamichostgroup_keydynamichostgroup_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.dynamichostgroup_keydynamichostgroup_seq OWNER TO farmer;

--
-- Name: dynamichostgroup_keydynamichostgroup_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('dynamichostgroup_keydynamichostgroup_seq', 1, false);


--
-- Name: hostgroup_keyhostgroup_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE hostgroup_keyhostgroup_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.hostgroup_keyhostgroup_seq OWNER TO farmer;

--
-- Name: hostgroup_keyhostgroup_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('hostgroup_keyhostgroup_seq', 1, false);


--
-- Name: hostgroup; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE hostgroup (
    keyhostgroup integer DEFAULT nextval('hostgroup_keyhostgroup_seq'::regclass) NOT NULL,
    hostgroup text,
    fkeyusr integer,
    private boolean
);


ALTER TABLE public.hostgroup OWNER TO farmer;

--
-- Name: dynamichostgroup; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE dynamichostgroup (
    keydynamichostgroup integer DEFAULT nextval('dynamichostgroup_keydynamichostgroup_seq'::regclass) NOT NULL,
    hostwhereclause text
)
INHERITS (hostgroup);


ALTER TABLE public.dynamichostgroup OWNER TO farmer;

--
-- Name: elementdep_keyelementdep_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE elementdep_keyelementdep_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.elementdep_keyelementdep_seq OWNER TO farmer;

--
-- Name: elementdep_keyelementdep_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('elementdep_keyelementdep_seq', 1, false);


--
-- Name: elementdep; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE elementdep (
    keyelementdep integer DEFAULT nextval('elementdep_keyelementdep_seq'::regclass) NOT NULL,
    fkeyelement integer,
    fkeyelementdep integer,
    relationtype text
);


ALTER TABLE public.elementdep OWNER TO farmer;

--
-- Name: elementstatus_keyelementstatus_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE elementstatus_keyelementstatus_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.elementstatus_keyelementstatus_seq OWNER TO farmer;

--
-- Name: elementstatus_keyelementstatus_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('elementstatus_keyelementstatus_seq', 1, false);


--
-- Name: elementstatus; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE elementstatus (
    keyelementstatus integer DEFAULT nextval('elementstatus_keyelementstatus_seq'::regclass) NOT NULL,
    name text,
    color text,
    fkeystatusset integer,
    "order" integer
);


ALTER TABLE public.elementstatus OWNER TO farmer;

--
-- Name: elementthread_keyelementthread_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE elementthread_keyelementthread_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.elementthread_keyelementthread_seq OWNER TO farmer;

--
-- Name: elementthread_keyelementthread_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('elementthread_keyelementthread_seq', 1, false);


--
-- Name: elementthread; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE elementthread (
    keyelementthread integer DEFAULT nextval('elementthread_keyelementthread_seq'::regclass) NOT NULL,
    datetime timestamp without time zone,
    elementthread text,
    fkeyelement integer,
    fkeyusr integer,
    skeyreply integer,
    topic text,
    todostatus integer,
    hasattachments integer,
    fkeyjob integer
);


ALTER TABLE public.elementthread OWNER TO farmer;

--
-- Name: elementtype_keyelementtype_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE elementtype_keyelementtype_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.elementtype_keyelementtype_seq OWNER TO farmer;

--
-- Name: elementtype_keyelementtype_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('elementtype_keyelementtype_seq', 1, false);


--
-- Name: elementtype; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE elementtype (
    keyelementtype integer DEFAULT nextval('elementtype_keyelementtype_seq'::regclass) NOT NULL,
    elementtype text,
    sortprefix text
);


ALTER TABLE public.elementtype OWNER TO farmer;

--
-- Name: elementtypetasktype_keyelementtypetasktype_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE elementtypetasktype_keyelementtypetasktype_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.elementtypetasktype_keyelementtypetasktype_seq OWNER TO farmer;

--
-- Name: elementtypetasktype_keyelementtypetasktype_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('elementtypetasktype_keyelementtypetasktype_seq', 1, false);


--
-- Name: elementtypetasktype; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE elementtypetasktype (
    keyelementtypetasktype integer DEFAULT nextval('elementtypetasktype_keyelementtypetasktype_seq'::regclass) NOT NULL,
    fkeyelementtype integer,
    fkeytasktype integer,
    fkeyassettype integer
);


ALTER TABLE public.elementtypetasktype OWNER TO farmer;

--
-- Name: elementuser_keyelementuser_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE elementuser_keyelementuser_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.elementuser_keyelementuser_seq OWNER TO farmer;

--
-- Name: elementuser_keyelementuser_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('elementuser_keyelementuser_seq', 1, false);


--
-- Name: elementuser; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE elementuser (
    keyelementuser integer DEFAULT nextval('elementuser_keyelementuser_seq'::regclass) NOT NULL,
    fkeyelement integer,
    fkeyuser integer,
    active boolean,
    fkeyassettype integer
);


ALTER TABLE public.elementuser OWNER TO farmer;

--
-- Name: usr; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE usr (
    arsenalslotlimit integer,
    arsenalslotreserve integer,
    dateoflastlogon date,
    email text,
    fkeyhost integer,
    gpgkey text,
    jid text,
    pager text,
    password text,
    remoteips text,
    schedule text,
    shell text,
    uid integer,
    threadnotifybyjabber integer,
    threadnotifybyemail integer,
    fkeyclient integer,
    intranet integer,
    homedir text,
    disabled integer DEFAULT 0,
    gid integer,
    usr text,
    keyusr integer,
    rolemask text,
    usrlevel integer,
    remoteok integer,
    requestcount integer,
    sessiontimeout integer,
    logoncount integer,
    useradded integer,
    oldkeyusr integer,
    sid text,
    lastlogontype text
)
INHERITS (element);


ALTER TABLE public.usr OWNER TO farmer;

--
-- Name: employee; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE employee (
    arsenalslotlimit integer,
    namefirst text,
    namelast text,
    dateofhire date,
    dateoftermination date,
    dateofbirth date,
    logon text,
    lockedout integer,
    bebackat timestamp without time zone,
    comment text,
    userlevel integer,
    nopostdays integer,
    initials text,
    missingtimesheetcount integer,
    namemiddle text
)
INHERITS (usr);


ALTER TABLE public.employee OWNER TO farmer;

--
-- Name: eventalert_keyEventAlert_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE "eventalert_keyEventAlert_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public."eventalert_keyEventAlert_seq" OWNER TO farmer;

--
-- Name: eventalert_keyEventAlert_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('"eventalert_keyEventAlert_seq"', 1, false);


--
-- Name: eventalert; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE eventalert (
    "keyEventAlert" integer DEFAULT nextval('"eventalert_keyEventAlert_seq"'::regclass) NOT NULL,
    "fkeyHost" integer NOT NULL,
    graphds text NOT NULL,
    "sampleType" character varying(32) NOT NULL,
    "samplePeriod" integer NOT NULL,
    severity smallint DEFAULT 1 NOT NULL,
    "sampleDirection" character varying(32),
    varname text,
    "sampleValue" real
);


ALTER TABLE public.eventalert OWNER TO farmer;

--
-- Name: filetemplate_keyfiletemplate_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE filetemplate_keyfiletemplate_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.filetemplate_keyfiletemplate_seq OWNER TO farmer;

--
-- Name: filetemplate_keyfiletemplate_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('filetemplate_keyfiletemplate_seq', 1, false);


--
-- Name: filetemplate; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE filetemplate (
    keyfiletemplate integer DEFAULT nextval('filetemplate_keyfiletemplate_seq'::regclass) NOT NULL,
    fkeyelementtype integer,
    fkeyproject integer,
    fkeytasktype integer,
    name text,
    sourcefile text,
    templatefilename text,
    trackertable text
);


ALTER TABLE public.filetemplate OWNER TO farmer;

--
-- Name: filetracker_keyfiletracker_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE filetracker_keyfiletracker_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.filetracker_keyfiletracker_seq OWNER TO farmer;

--
-- Name: filetracker_keyfiletracker_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('filetracker_keyfiletracker_seq', 1, false);


--
-- Name: filetracker; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE filetracker (
    keyfiletracker integer DEFAULT nextval('filetracker_keyfiletracker_seq'::regclass) NOT NULL,
    fkeyelement integer,
    name text,
    path text,
    filename text,
    fkeypathtemplate integer,
    fkeyprojectstorage integer,
    storagename text
);


ALTER TABLE public.filetracker OWNER TO farmer;

--
-- Name: filetrackerdep_keyfiletrackerdep_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE filetrackerdep_keyfiletrackerdep_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.filetrackerdep_keyfiletrackerdep_seq OWNER TO farmer;

--
-- Name: filetrackerdep_keyfiletrackerdep_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('filetrackerdep_keyfiletrackerdep_seq', 1, false);


--
-- Name: filetrackerdep; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE filetrackerdep (
    keyfiletrackerdep integer DEFAULT nextval('filetrackerdep_keyfiletrackerdep_seq'::regclass) NOT NULL,
    fkeyinput integer,
    fkeyoutput integer
);


ALTER TABLE public.filetrackerdep OWNER TO farmer;

--
-- Name: fileversion_keyfileversion_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE fileversion_keyfileversion_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.fileversion_keyfileversion_seq OWNER TO farmer;

--
-- Name: fileversion_keyfileversion_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('fileversion_keyfileversion_seq', 1, false);


--
-- Name: fileversion; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE fileversion (
    keyfileversion integer DEFAULT nextval('fileversion_keyfileversion_seq'::regclass) NOT NULL,
    version integer,
    iteration integer,
    path text,
    oldfilenames text,
    filename text,
    filenametemplate text,
    automaster integer,
    fkeyelement integer,
    fkeyfileversion integer
);


ALTER TABLE public.fileversion OWNER TO farmer;

--
-- Name: folder_keyfolder_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE folder_keyfolder_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.folder_keyfolder_seq OWNER TO farmer;

--
-- Name: folder_keyfolder_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('folder_keyfolder_seq', 1, false);


--
-- Name: folder; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE folder (
    keyfolder integer DEFAULT nextval('folder_keyfolder_seq'::regclass) NOT NULL,
    folder text,
    mount text,
    tablename text,
    fkey integer,
    online integer,
    alias text,
    host text,
    link text
);


ALTER TABLE public.folder OWNER TO farmer;

--
-- Name: graph_keygraph_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE graph_keygraph_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.graph_keygraph_seq OWNER TO farmer;

--
-- Name: graph_keygraph_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('graph_keygraph_seq', 1, false);


--
-- Name: graph; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE graph (
    keygraph integer DEFAULT nextval('graph_keygraph_seq'::regclass) NOT NULL,
    height integer,
    width integer,
    vlabel character varying(64),
    period character varying(32),
    fkeygraphpage integer,
    upperlimit real,
    lowerlimit real,
    stack smallint DEFAULT 0 NOT NULL,
    graphmax smallint DEFAULT 0 NOT NULL,
    sortorder smallint,
    graph character varying(64)
);


ALTER TABLE public.graph OWNER TO farmer;

--
-- Name: graphds_keygraphds_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE graphds_keygraphds_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.graphds_keygraphds_seq OWNER TO farmer;

--
-- Name: graphds_keygraphds_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('graphds_keygraphds_seq', 1, false);


--
-- Name: graphds; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE graphds (
    keygraphds integer DEFAULT nextval('graphds_keygraphds_seq'::regclass) NOT NULL,
    varname character varying(64),
    dstype character varying(16),
    fkeyhost integer,
    cdef text,
    graphds text,
    fieldname text,
    filename text,
    negative boolean
);


ALTER TABLE public.graphds OWNER TO farmer;

--
-- Name: TABLE graphds; Type: COMMENT; Schema: public; Owner: farmer
--

COMMENT ON TABLE graphds IS 'Graph Datasource';


--
-- Name: graphpage_keygraphpage_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE graphpage_keygraphpage_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.graphpage_keygraphpage_seq OWNER TO farmer;

--
-- Name: graphpage_keygraphpage_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('graphpage_keygraphpage_seq', 1, false);


--
-- Name: graphpage; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE graphpage (
    keygraphpage integer DEFAULT nextval('graphpage_keygraphpage_seq'::regclass) NOT NULL,
    fkeygraphpage integer,
    name character varying(32)
);


ALTER TABLE public.graphpage OWNER TO farmer;

--
-- Name: graphrelationship_keygraphrelationship_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE graphrelationship_keygraphrelationship_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.graphrelationship_keygraphrelationship_seq OWNER TO farmer;

--
-- Name: graphrelationship_keygraphrelationship_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('graphrelationship_keygraphrelationship_seq', 1, false);


--
-- Name: graphrelationship; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE graphrelationship (
    keygraphrelationship integer DEFAULT nextval('graphrelationship_keygraphrelationship_seq'::regclass) NOT NULL,
    fkeygraphds integer NOT NULL,
    fkeygraph integer NOT NULL,
    negative smallint DEFAULT 0 NOT NULL
);


ALTER TABLE public.graphrelationship OWNER TO farmer;

--
-- Name: gridtemplate_keygridtemplate_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE gridtemplate_keygridtemplate_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.gridtemplate_keygridtemplate_seq OWNER TO farmer;

--
-- Name: gridtemplate_keygridtemplate_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('gridtemplate_keygridtemplate_seq', 1, false);


--
-- Name: gridtemplate; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE gridtemplate (
    keygridtemplate integer DEFAULT nextval('gridtemplate_keygridtemplate_seq'::regclass) NOT NULL,
    fkeyproject integer,
    gridtemplate text
);


ALTER TABLE public.gridtemplate OWNER TO farmer;

--
-- Name: gridtemplateitem_keygridtemplateitem_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE gridtemplateitem_keygridtemplateitem_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.gridtemplateitem_keygridtemplateitem_seq OWNER TO farmer;

--
-- Name: gridtemplateitem_keygridtemplateitem_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('gridtemplateitem_keygridtemplateitem_seq', 1, false);


--
-- Name: gridtemplateitem; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE gridtemplateitem (
    keygridtemplateitem integer DEFAULT nextval('gridtemplateitem_keygridtemplateitem_seq'::regclass) NOT NULL,
    fkeygridtemplate integer,
    fkeytasktype integer,
    checklistitems text,
    columntype integer,
    headername text,
    "position" integer
);


ALTER TABLE public.gridtemplateitem OWNER TO farmer;

--
-- Name: groupmapping_keygroupmapping_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE groupmapping_keygroupmapping_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.groupmapping_keygroupmapping_seq OWNER TO farmer;

--
-- Name: groupmapping_keygroupmapping_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('groupmapping_keygroupmapping_seq', 1, false);


--
-- Name: groupmapping; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE groupmapping (
    keygroupmapping integer DEFAULT nextval('groupmapping_keygroupmapping_seq'::regclass) NOT NULL,
    fkeygrp integer,
    fkeymapping integer
);


ALTER TABLE public.groupmapping OWNER TO farmer;

--
-- Name: grp_keygrp_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE grp_keygrp_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.grp_keygrp_seq OWNER TO farmer;

--
-- Name: grp_keygrp_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('grp_keygrp_seq', 1, false);


--
-- Name: grp; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE grp (
    keygrp integer DEFAULT nextval('grp_keygrp_seq'::regclass) NOT NULL,
    grp text,
    alias text
);


ALTER TABLE public.grp OWNER TO farmer;

--
-- Name: gruntscript_keygruntscript_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE gruntscript_keygruntscript_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.gruntscript_keygruntscript_seq OWNER TO farmer;

--
-- Name: gruntscript_keygruntscript_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('gruntscript_keygruntscript_seq', 1, false);


--
-- Name: gruntscript; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE gruntscript (
    keygruntscript integer DEFAULT nextval('gruntscript_keygruntscript_seq'::regclass) NOT NULL,
    runcount integer,
    lastrun date,
    scriptname text
);


ALTER TABLE public.gruntscript OWNER TO farmer;

--
-- Name: history_keyhistory_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE history_keyhistory_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.history_keyhistory_seq OWNER TO farmer;

--
-- Name: history_keyhistory_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('history_keyhistory_seq', 1, false);


--
-- Name: history; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE history (
    keyhistory integer DEFAULT nextval('history_keyhistory_seq'::regclass) NOT NULL,
    date timestamp without time zone,
    fkeyelement integer,
    fkeyusr integer,
    history text
);


ALTER TABLE public.history OWNER TO farmer;

--
-- Name: hostdailystat_keyhostdailystat_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE hostdailystat_keyhostdailystat_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.hostdailystat_keyhostdailystat_seq OWNER TO farmer;

--
-- Name: hostdailystat_keyhostdailystat_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('hostdailystat_keyhostdailystat_seq', 1, false);


--
-- Name: hostdailystat; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE hostdailystat (
    keyhostdailystat integer DEFAULT nextval('hostdailystat_keyhostdailystat_seq'::regclass) NOT NULL,
    fkeyhost integer,
    readytime text,
    assignedtime text,
    copytime text,
    loadtime text,
    busytime text,
    offlinetime text,
    date date,
    tasksdone integer,
    loaderrors integer,
    taskerrors integer,
    loaderrortime text,
    busyerrortime text
);


ALTER TABLE public.hostdailystat OWNER TO farmer;

--
-- Name: hostgroupitem_keyhostgroupitem_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE hostgroupitem_keyhostgroupitem_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.hostgroupitem_keyhostgroupitem_seq OWNER TO farmer;

--
-- Name: hostgroupitem_keyhostgroupitem_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('hostgroupitem_keyhostgroupitem_seq', 1, false);


--
-- Name: hostgroupitem; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE hostgroupitem (
    keyhostgroupitem integer DEFAULT nextval('hostgroupitem_keyhostgroupitem_seq'::regclass) NOT NULL,
    fkeyhostgroup integer,
    fkeyhost integer
);


ALTER TABLE public.hostgroupitem OWNER TO farmer;

--
-- Name: hostinterfacetype_keyhostinterfacetype_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE hostinterfacetype_keyhostinterfacetype_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.hostinterfacetype_keyhostinterfacetype_seq OWNER TO farmer;

--
-- Name: hostinterfacetype_keyhostinterfacetype_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('hostinterfacetype_keyhostinterfacetype_seq', 1, false);


--
-- Name: hostinterfacetype; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE hostinterfacetype (
    keyhostinterfacetype integer DEFAULT nextval('hostinterfacetype_keyhostinterfacetype_seq'::regclass) NOT NULL,
    hostinterfacetype text
);


ALTER TABLE public.hostinterfacetype OWNER TO farmer;

--
-- Name: hostload_keyhostload_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE hostload_keyhostload_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.hostload_keyhostload_seq OWNER TO farmer;

--
-- Name: hostload_keyhostload_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('hostload_keyhostload_seq', 1, false);


--
-- Name: hostload; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE hostload (
    keyhostload integer DEFAULT nextval('hostload_keyhostload_seq'::regclass) NOT NULL,
    fkeyhost integer,
    loadavg double precision,
    loadavgadjust double precision DEFAULT 0.0,
    loadavgadjusttimestamp timestamp without time zone
);


ALTER TABLE public.hostload OWNER TO farmer;

--
-- Name: hostmapping_keyhostmapping_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE hostmapping_keyhostmapping_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.hostmapping_keyhostmapping_seq OWNER TO farmer;

--
-- Name: hostmapping_keyhostmapping_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('hostmapping_keyhostmapping_seq', 1, false);


--
-- Name: hostmapping; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE hostmapping (
    keyhostmapping integer DEFAULT nextval('hostmapping_keyhostmapping_seq'::regclass) NOT NULL,
    fkeyhost integer NOT NULL,
    fkeymapping integer NOT NULL
);


ALTER TABLE public.hostmapping OWNER TO farmer;

--
-- Name: hostport_keyhostport_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE hostport_keyhostport_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.hostport_keyhostport_seq OWNER TO farmer;

--
-- Name: hostport_keyhostport_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('hostport_keyhostport_seq', 1, false);


--
-- Name: hostport; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE hostport (
    keyhostport integer DEFAULT nextval('hostport_keyhostport_seq'::regclass) NOT NULL,
    fkeyhost integer NOT NULL,
    port integer NOT NULL,
    monitor smallint DEFAULT 0 NOT NULL,
    monitorstatus character varying(16)
);


ALTER TABLE public.hostport OWNER TO farmer;

--
-- Name: hostresource_keyhostresource_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE hostresource_keyhostresource_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.hostresource_keyhostresource_seq OWNER TO farmer;

--
-- Name: hostresource_keyhostresource_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('hostresource_keyhostresource_seq', 1, false);


--
-- Name: hostresource; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE hostresource (
    keyhostresource integer DEFAULT nextval('hostresource_keyhostresource_seq'::regclass) NOT NULL,
    fkeyhost integer,
    hostresource text
);


ALTER TABLE public.hostresource OWNER TO farmer;

--
-- Name: hosts_active; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE hosts_active (
    count bigint
);


ALTER TABLE public.hosts_active OWNER TO farmer;

--
-- Name: hosts_ready; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE hosts_ready (
    count bigint
);


ALTER TABLE public.hosts_ready OWNER TO farmer;

--
-- Name: hosts_total; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE hosts_total (
    count bigint
);


ALTER TABLE public.hosts_total OWNER TO farmer;

--
-- Name: hostservice_keyhostservice_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE hostservice_keyhostservice_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.hostservice_keyhostservice_seq OWNER TO farmer;

--
-- Name: hostservice_keyhostservice_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('hostservice_keyhostservice_seq', 1, false);


--
-- Name: hostservice; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE hostservice (
    keyhostservice integer DEFAULT nextval('hostservice_keyhostservice_seq'::regclass) NOT NULL,
    fkeyhost integer,
    fkeyservice integer,
    hostserviceinfo text,
    hostservice text,
    fkeysyslog integer,
    enabled boolean,
    pulse timestamp without time zone,
    remotelogport integer
);


ALTER TABLE public.hostservice OWNER TO farmer;

--
-- Name: hostsoftware_keyhostsoftware_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE hostsoftware_keyhostsoftware_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.hostsoftware_keyhostsoftware_seq OWNER TO farmer;

--
-- Name: hostsoftware_keyhostsoftware_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('hostsoftware_keyhostsoftware_seq', 1, false);


--
-- Name: hostsoftware; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE hostsoftware (
    keyhostsoftware integer DEFAULT nextval('hostsoftware_keyhostsoftware_seq'::regclass) NOT NULL,
    fkeyhost integer,
    fkeysoftware integer
);


ALTER TABLE public.hostsoftware OWNER TO farmer;

--
-- Name: hoststatus_keyhoststatus_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE hoststatus_keyhoststatus_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.hoststatus_keyhoststatus_seq OWNER TO farmer;

--
-- Name: hoststatus_keyhoststatus_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('hoststatus_keyhoststatus_seq', 1, false);


--
-- Name: hoststatus; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE hoststatus (
    keyhoststatus integer DEFAULT nextval('hoststatus_keyhoststatus_seq'::regclass) NOT NULL,
    fkeyhost integer NOT NULL,
    slavestatus character varying(32),
    laststatuschange timestamp without time zone,
    slavepulse timestamp without time zone,
    fkeyjobtask integer,
    online integer,
    activeassignmentcount integer DEFAULT 0,
    availablememory integer,
    lastassignmentchange timestamp without time zone
);


ALTER TABLE public.hoststatus OWNER TO farmer;

--
-- Name: job3delight; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE job3delight (
    framenth integer,
    framenthmode integer,
    exclusiveassignment boolean,
    hastaskprogress boolean,
    minmemory integer,
    scenename text,
    shotname text,
    toggleflags integer,
    width integer,
    height integer,
    framestart integer,
    frameend integer,
    threads integer DEFAULT 4,
    processes integer,
    jobscript text,
    jobscriptparam text,
    renderdlcmd text
)
INHERITS (job);


ALTER TABLE public.job3delight OWNER TO farmer;

--
-- Name: jobassignment_old; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobassignment_old (
    keyjobassignment integer NOT NULL,
    fkeyjob integer,
    fkeyjobassignmentstatus integer,
    fkeyhost integer,
    stdout text,
    stderr text,
    command text,
    maxmemory integer,
    started timestamp without time zone,
    ended timestamp without time zone,
    fkeyjoberror integer,
    realtime bigint DEFAULT 0 NOT NULL,
    usertime bigint DEFAULT 0 NOT NULL,
    systime bigint DEFAULT 0 NOT NULL,
    iowait bigint,
    bytesread bigint,
    byteswrite bigint,
    efficiency double precision,
    opsread integer,
    opswrite integer
);


ALTER TABLE public.jobassignment_old OWNER TO farmer;

--
-- Name: jobassignment_keyjobassignment_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobassignment_keyjobassignment_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 10;


ALTER TABLE public.jobassignment_keyjobassignment_seq OWNER TO farmer;

--
-- Name: jobassignment_keyjobassignment_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE jobassignment_keyjobassignment_seq OWNED BY jobassignment_old.keyjobassignment;


--
-- Name: jobassignment_keyjobassignment_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobassignment_keyjobassignment_seq', 1, false);


--
-- Name: jobassignment; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobassignment (
    keyjobassignment integer DEFAULT nextval('jobassignment_keyjobassignment_seq'::regclass) NOT NULL,
    fkeyjob integer,
    fkeyjobassignmentstatus integer,
    fkeyhost integer,
    stdout text,
    stderr text,
    command text,
    maxmemory integer,
    started timestamp without time zone,
    ended timestamp without time zone,
    fkeyjoberror integer,
    realtime bigint DEFAULT 0 NOT NULL,
    usertime bigint DEFAULT 0 NOT NULL,
    systime bigint DEFAULT 0 NOT NULL,
    iowait bigint,
    bytesread bigint,
    byteswrite bigint,
    efficiency double precision,
    opsread integer,
    opswrite integer,
    assignslots integer,
    assignmaxmemory integer,
    assignminmemory integer
);


ALTER TABLE public.jobassignment OWNER TO farmer;

--
-- Name: jobassignmentstatus; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobassignmentstatus (
    keyjobassignmentstatus integer NOT NULL,
    status text
);


ALTER TABLE public.jobassignmentstatus OWNER TO farmer;

--
-- Name: jobassignmentstatus_keyjobassignmentstatus_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobassignmentstatus_keyjobassignmentstatus_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobassignmentstatus_keyjobassignmentstatus_seq OWNER TO farmer;

--
-- Name: jobassignmentstatus_keyjobassignmentstatus_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE jobassignmentstatus_keyjobassignmentstatus_seq OWNED BY jobassignmentstatus.keyjobassignmentstatus;


--
-- Name: jobassignmentstatus_keyjobassignmentstatus_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobassignmentstatus_keyjobassignmentstatus_seq', 6, true);


--
-- Name: jobbatch; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobbatch (
    license text,
    fkeyjobparent integer,
    endedts timestamp without time zone,
    startedts timestamp without time zone,
    submittedts timestamp without time zone,
    maxhosts integer,
    environment text,
    runassubmitter boolean,
    framenth integer,
    framenthmode integer,
    exclusiveassignment boolean,
    hastaskprogress boolean,
    minmemory integer,
    fkeyjobfilterset integer,
    maxerrors integer,
    fkeyjobenvironment integer,
    toggleflags integer,
    cmd text,
    restartafterfinish boolean DEFAULT false,
    restartaftershutdown boolean,
    passslaveframesasparam boolean,
    disablewow64fsredirect boolean
)
INHERITS (job);


ALTER TABLE public.jobbatch OWNER TO farmer;

--
-- Name: jobcannedbatch_keyjobcannedbatch_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobcannedbatch_keyjobcannedbatch_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobcannedbatch_keyjobcannedbatch_seq OWNER TO farmer;

--
-- Name: jobcannedbatch_keyjobcannedbatch_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobcannedbatch_keyjobcannedbatch_seq', 1, false);


--
-- Name: jobcannedbatch; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobcannedbatch (
    keyjobcannedbatch integer DEFAULT nextval('jobcannedbatch_keyjobcannedbatch_seq'::regclass) NOT NULL,
    name text,
    "group" text,
    cmd text,
    disablewow64fsredirect boolean
);


ALTER TABLE public.jobcannedbatch OWNER TO farmer;

--
-- Name: jobcommandhistory_keyjobcommandhistory_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobcommandhistory_keyjobcommandhistory_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobcommandhistory_keyjobcommandhistory_seq OWNER TO farmer;

--
-- Name: jobcommandhistory_keyjobcommandhistory_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobcommandhistory_keyjobcommandhistory_seq', 1, false);


--
-- Name: jobcommandhistory; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobcommandhistory (
    keyjobcommandhistory integer DEFAULT nextval('jobcommandhistory_keyjobcommandhistory_seq'::regclass) NOT NULL,
    stderr text DEFAULT ''::text,
    stdout text DEFAULT ''::text,
    command text,
    memory integer,
    fkeyjob integer,
    fkeyhost integer,
    fkeyhosthistory integer,
    iowait integer,
    realtime double precision,
    systime double precision,
    usertime double precision
);


ALTER TABLE public.jobcommandhistory OWNER TO farmer;

--
-- Name: jobenvironment; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobenvironment (
    keyjobenvironment integer NOT NULL,
    environment text
);


ALTER TABLE public.jobenvironment OWNER TO farmer;

--
-- Name: jobenvironment_keyjobenvironment_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobenvironment_keyjobenvironment_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobenvironment_keyjobenvironment_seq OWNER TO farmer;

--
-- Name: jobenvironment_keyjobenvironment_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE jobenvironment_keyjobenvironment_seq OWNED BY jobenvironment.keyjobenvironment;


--
-- Name: jobenvironment_keyjobenvironment_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobenvironment_keyjobenvironment_seq', 1, false);


--
-- Name: joberror_keyjoberror_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE joberror_keyjoberror_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.joberror_keyjoberror_seq OWNER TO farmer;

--
-- Name: joberror_keyjoberror_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('joberror_keyjoberror_seq', 1, false);


--
-- Name: joberror; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE joberror (
    keyjoberror integer DEFAULT nextval('joberror_keyjoberror_seq'::regclass) NOT NULL,
    fkeyhost integer,
    fkeyjob integer,
    frames text,
    message text,
    errortime integer,
    count integer DEFAULT 1,
    cleared boolean DEFAULT false,
    lastoccurrence timestamp without time zone,
    timeout boolean
);


ALTER TABLE public.joberror OWNER TO farmer;

--
-- Name: joberrorhandler_keyjoberrorhandler_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE joberrorhandler_keyjoberrorhandler_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.joberrorhandler_keyjoberrorhandler_seq OWNER TO farmer;

--
-- Name: joberrorhandler_keyjoberrorhandler_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('joberrorhandler_keyjoberrorhandler_seq', 1, false);


--
-- Name: joberrorhandler; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE joberrorhandler (
    keyjoberrorhandler integer DEFAULT nextval('joberrorhandler_keyjoberrorhandler_seq'::regclass) NOT NULL,
    fkeyjobtype integer,
    errorregex text,
    fkeyjoberrorhandlerscript integer
);


ALTER TABLE public.joberrorhandler OWNER TO farmer;

--
-- Name: joberrorhandlerscript_keyjoberrorhandlerscript_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE joberrorhandlerscript_keyjoberrorhandlerscript_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.joberrorhandlerscript_keyjoberrorhandlerscript_seq OWNER TO farmer;

--
-- Name: joberrorhandlerscript_keyjoberrorhandlerscript_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('joberrorhandlerscript_keyjoberrorhandlerscript_seq', 1, false);


--
-- Name: joberrorhandlerscript; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE joberrorhandlerscript (
    keyjoberrorhandlerscript integer DEFAULT nextval('joberrorhandlerscript_keyjoberrorhandlerscript_seq'::regclass) NOT NULL,
    script text
);


ALTER TABLE public.joberrorhandlerscript OWNER TO farmer;

--
-- Name: jobfiltermessage; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobfiltermessage (
    keyjobfiltermessage integer NOT NULL,
    fkeyjobfiltertype integer,
    fkeyjobfilterset integer,
    regex text,
    comment text,
    enabled boolean
);


ALTER TABLE public.jobfiltermessage OWNER TO farmer;

--
-- Name: jobfiltermessage_keyjobfiltermessage_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobfiltermessage_keyjobfiltermessage_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobfiltermessage_keyjobfiltermessage_seq OWNER TO farmer;

--
-- Name: jobfiltermessage_keyjobfiltermessage_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE jobfiltermessage_keyjobfiltermessage_seq OWNED BY jobfiltermessage.keyjobfiltermessage;


--
-- Name: jobfiltermessage_keyjobfiltermessage_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobfiltermessage_keyjobfiltermessage_seq', 181, true);


--
-- Name: jobfilterset; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobfilterset (
    keyjobfilterset integer NOT NULL,
    name text
);


ALTER TABLE public.jobfilterset OWNER TO farmer;

--
-- Name: jobfilterset_keyjobfilterset_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobfilterset_keyjobfilterset_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobfilterset_keyjobfilterset_seq OWNER TO farmer;

--
-- Name: jobfilterset_keyjobfilterset_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE jobfilterset_keyjobfilterset_seq OWNED BY jobfilterset.keyjobfilterset;


--
-- Name: jobfilterset_keyjobfilterset_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobfilterset_keyjobfilterset_seq', 6, true);


--
-- Name: jobfiltertype; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobfiltertype (
    keyjobfiltertype integer NOT NULL,
    name text
);


ALTER TABLE public.jobfiltertype OWNER TO farmer;

--
-- Name: jobfiltertype_keyjobfiltertype_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobfiltertype_keyjobfiltertype_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobfiltertype_keyjobfiltertype_seq OWNER TO farmer;

--
-- Name: jobfiltertype_keyjobfiltertype_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE jobfiltertype_keyjobfiltertype_seq OWNED BY jobfiltertype.keyjobfiltertype;


--
-- Name: jobfiltertype_keyjobfiltertype_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobfiltertype_keyjobfiltertype_seq', 5, true);


--
-- Name: jobhistory_keyjobhistory_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobhistory_keyjobhistory_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobhistory_keyjobhistory_seq OWNER TO farmer;

--
-- Name: jobhistory_keyjobhistory_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobhistory_keyjobhistory_seq', 1, false);


--
-- Name: jobhistory; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobhistory (
    keyjobhistory integer DEFAULT nextval('jobhistory_keyjobhistory_seq'::regclass) NOT NULL,
    fkeyjobhistorytype integer,
    fkeyjob integer,
    fkeyhost integer,
    fkeyuser integer,
    message text,
    created timestamp without time zone DEFAULT now() NOT NULL
);


ALTER TABLE public.jobhistory OWNER TO farmer;

--
-- Name: jobhistorytype_keyjobhistorytype_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobhistorytype_keyjobhistorytype_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobhistorytype_keyjobhistorytype_seq OWNER TO farmer;

--
-- Name: jobhistorytype_keyjobhistorytype_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobhistorytype_keyjobhistorytype_seq', 1, false);


--
-- Name: jobhistorytype; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobhistorytype (
    keyjobhistorytype integer DEFAULT nextval('jobhistorytype_keyjobhistorytype_seq'::regclass) NOT NULL,
    type text
);


ALTER TABLE public.jobhistorytype OWNER TO farmer;

--
-- Name: jobmantra100; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobmantra100 (
    fkeyjobfilterset integer,
    maxerrors integer,
    toggleflags integer,
    keyjobmantra100 integer NOT NULL,
    forceraytrace boolean,
    geocachesize integer,
    height integer,
    qualityflag text,
    renderquality integer,
    threads integer DEFAULT 4 NOT NULL,
    width integer
)
INHERITS (job);


ALTER TABLE public.jobmantra100 OWNER TO farmer;

--
-- Name: jobmantra100_keyjobmantra100_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobmantra100_keyjobmantra100_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobmantra100_keyjobmantra100_seq OWNER TO farmer;

--
-- Name: jobmantra100_keyjobmantra100_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE jobmantra100_keyjobmantra100_seq OWNED BY jobmantra100.keyjobmantra100;


--
-- Name: jobmantra100_keyjobmantra100_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobmantra100_keyjobmantra100_seq', 1, false);


--
-- Name: jobmantra95; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobmantra95 (
    keyjobmantra95 integer NOT NULL,
    forceraytrace boolean,
    geocachesize integer,
    height integer,
    qualityflag text,
    renderquality integer,
    threads integer,
    width integer
)
INHERITS (job);


ALTER TABLE public.jobmantra95 OWNER TO farmer;

--
-- Name: jobmantra95_keyjobmantra95_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobmantra95_keyjobmantra95_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobmantra95_keyjobmantra95_seq OWNER TO farmer;

--
-- Name: jobmantra95_keyjobmantra95_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE jobmantra95_keyjobmantra95_seq OWNED BY jobmantra95.keyjobmantra95;


--
-- Name: jobmantra95_keyjobmantra95_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobmantra95_keyjobmantra95_seq', 1, false);


--
-- Name: jobmax; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE jobmax (
    camera text,
    elementfile text,
    exrattributes text,
    exrchannels text,
    exrcompression integer,
    exrsavebitdepth integer,
    exrsaveregion boolean,
    exrsavescanline boolean,
    exrtilesize integer,
    exrversion integer,
    fileoriginal text,
    flag_h integer,
    flag_v integer,
    flag_w integer,
    flag_x2 integer,
    flag_xa integer,
    flag_xc integer,
    flag_xd integer,
    flag_xe integer,
    flag_xf integer,
    flag_xh integer,
    flag_xk integer,
    flag_xn integer,
    flag_xo integer,
    flag_xp integer,
    flag_xv integer,
    frameend integer,
    framelist text,
    framestart integer,
    plugininipath text,
    startupscript text
)
INHERITS (job);


ALTER TABLE public.jobmax OWNER TO postgres;

--
-- Name: jobmax10; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobmax10 (
)
INHERITS (jobmax);


ALTER TABLE public.jobmax10 OWNER TO farmer;

--
-- Name: jobmax2009; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobmax2009 (
)
INHERITS (jobmax);


ALTER TABLE public.jobmax2009 OWNER TO farmer;

--
-- Name: jobmax2010; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobmax2010 (
)
INHERITS (jobmax);


ALTER TABLE public.jobmax2010 OWNER TO farmer;

--
-- Name: jobmaxscript; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobmaxscript (
    script text,
    maxtime integer,
    outputfiles text,
    silent boolean,
    maxversion text,
    runmax64 boolean,
    runpythonscript boolean,
    use3dsmaxcmd boolean
)
INHERITS (job);


ALTER TABLE public.jobmaxscript OWNER TO farmer;

--
-- Name: jobmaya; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobmaya (
    fkeyjobparent integer,
    endedts timestamp without time zone,
    startedts timestamp without time zone,
    submittedts timestamp without time zone,
    loggingenabled boolean,
    environment text,
    checkfilemd5 boolean,
    uploadedfile boolean,
    framenth integer,
    framenthmode integer,
    exclusiveassignment boolean,
    hastaskprogress boolean,
    minmemory integer,
    scenename text,
    shotname text,
    fkeyjobfilterset integer,
    maxerrors integer,
    fkeyjobenvironment integer,
    toggleflags integer,
    framestart integer,
    frameend integer,
    camera text,
    renderer text,
    projectpath text,
    width integer,
    height integer,
    append text
)
INHERITS (job);


ALTER TABLE public.jobmaya OWNER TO farmer;

--
-- Name: jobmaya2008; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobmaya2008 (
    exclusiveassignment boolean,
    hastaskprogress boolean,
    minmemory integer,
    fkeyjobenvironment integer,
    suspendedts timestamp without time zone
)
INHERITS (jobmaya);


ALTER TABLE public.jobmaya2008 OWNER TO farmer;

--
-- Name: jobmaya2009; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobmaya2009 (
    framenthmode integer,
    exclusiveassignment boolean,
    hastaskprogress boolean,
    minmemory integer,
    notifycompletemessage text,
    notifyerrormessage text
)
INHERITS (jobmaya);


ALTER TABLE public.jobmaya2009 OWNER TO farmer;

--
-- Name: jobmaya2011; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobmaya2011 (
    notifycompletemessage text,
    notifyerrormessage text
)
INHERITS (jobmaya);


ALTER TABLE public.jobmaya2011 OWNER TO farmer;

--
-- Name: jobmaya7; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobmaya7 (
    maxhosts integer,
    framenthmode integer,
    exclusiveassignment boolean,
    hastaskprogress boolean,
    minmemory integer
)
INHERITS (jobmaya);


ALTER TABLE public.jobmaya7 OWNER TO farmer;

--
-- Name: jobmaya8; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobmaya8 (
    endedts timestamp without time zone,
    startedts timestamp without time zone,
    submittedts timestamp without time zone,
    maxhosts integer,
    exclusiveassignment boolean,
    hastaskprogress boolean,
    minmemory integer,
    notifycompletemessage text,
    notifyerrormessage text
)
INHERITS (jobmaya);


ALTER TABLE public.jobmaya8 OWNER TO farmer;

--
-- Name: jobmaya85; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobmaya85 (
    endedts timestamp without time zone,
    startedts timestamp without time zone,
    submittedts timestamp without time zone,
    maxhosts integer,
    framenthmode integer,
    exclusiveassignment boolean,
    hastaskprogress boolean,
    minmemory integer,
    scenename text,
    shotname text,
    notifycompletemessage text,
    notifyerrormessage text
)
INHERITS (jobmaya);


ALTER TABLE public.jobmaya85 OWNER TO farmer;

--
-- Name: jobmentalray2009; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobmentalray2009 (
    notifycompletemessage text,
    notifyerrormessage text,
    fkeyjobenvironment integer
)
INHERITS (jobmaya);


ALTER TABLE public.jobmentalray2009 OWNER TO farmer;

--
-- Name: jobmentalray2011; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobmentalray2011 (
    notifycompletemessage text,
    notifyerrormessage text,
    fkeyjobenvironment integer,
    suspendedts timestamp without time zone
)
INHERITS (jobmaya);


ALTER TABLE public.jobmentalray2011 OWNER TO farmer;

--
-- Name: jobmentalray7; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobmentalray7 (
    maxmemory integer,
    fkeyjobparent integer,
    maxhosts integer,
    exclusiveassignment boolean,
    hastaskprogress boolean,
    minmemory integer,
    scenename text,
    shotname text,
    notifycompletemessage text,
    notifyerrormessage text
)
INHERITS (jobmaya);


ALTER TABLE public.jobmentalray7 OWNER TO farmer;

--
-- Name: jobmentalray8; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobmentalray8 (
    fkeyjobparent integer,
    maxhosts integer,
    framenthmode integer,
    scenename text,
    shotname text
)
INHERITS (jobmaya);


ALTER TABLE public.jobmentalray8 OWNER TO farmer;

--
-- Name: jobmentalray85; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobmentalray85 (
    maxhosts integer,
    loggingenabled boolean,
    framenthmode integer,
    exclusiveassignment boolean,
    hastaskprogress boolean,
    minmemory integer
)
INHERITS (jobmaya);


ALTER TABLE public.jobmentalray85 OWNER TO farmer;

--
-- Name: jobnaiad; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobnaiad (
    notifycompletemessage text,
    notifyerrormessage text,
    fkeyjobenvironment integer,
    toggleflags integer,
    framestart integer,
    frameend integer,
    threads integer,
    append text,
    restartcache text,
    fullframe integer,
    forcerestart boolean DEFAULT false NOT NULL
)
INHERITS (job);


ALTER TABLE public.jobnaiad OWNER TO farmer;

--
-- Name: jobnuke; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobnuke (
    fkeyjobenvironment integer,
    toggleflags integer,
    framestart integer,
    frameend integer,
    outputcount integer,
    viewname text,
    nodes text,
    terminalonly boolean
)
INHERITS (job);


ALTER TABLE public.jobnuke OWNER TO farmer;

--
-- Name: jobnuke51; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobnuke51 (
    framenth integer,
    framenthmode integer,
    exclusiveassignment boolean,
    hastaskprogress boolean,
    minmemory integer,
    fkeyjobfilterset integer,
    framestart integer,
    frameend integer,
    outputcount integer
)
INHERITS (job);


ALTER TABLE public.jobnuke51 OWNER TO farmer;

--
-- Name: jobnuke52; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobnuke52 (
    scenename text,
    shotname text,
    fkeyjobfilterset integer,
    maxerrors integer,
    framestart integer,
    frameend integer,
    outputcount integer
)
INHERITS (job);


ALTER TABLE public.jobnuke52 OWNER TO farmer;

--
-- Name: joboutput_keyjoboutput_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE joboutput_keyjoboutput_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.joboutput_keyjoboutput_seq OWNER TO farmer;

--
-- Name: joboutput_keyjoboutput_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('joboutput_keyjoboutput_seq', 1, false);


--
-- Name: joboutput; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE joboutput (
    keyjoboutput integer DEFAULT nextval('joboutput_keyjoboutput_seq'::regclass) NOT NULL,
    fkeyjob integer,
    name text,
    fkeyfiletracker integer
);


ALTER TABLE public.joboutput OWNER TO farmer;

--
-- Name: jobrenderman_keyjob_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobrenderman_keyjob_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobrenderman_keyjob_seq OWNER TO farmer;

--
-- Name: jobrenderman_keyjob_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobrenderman_keyjob_seq', 1, false);


--
-- Name: jobribgen_keyjob_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobribgen_keyjob_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobribgen_keyjob_seq OWNER TO farmer;

--
-- Name: jobribgen_keyjob_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobribgen_keyjob_seq', 1, false);


--
-- Name: jobservice_keyjobservice_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobservice_keyjobservice_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobservice_keyjobservice_seq OWNER TO farmer;

--
-- Name: jobservice_keyjobservice_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobservice_keyjobservice_seq', 1, false);


--
-- Name: jobservice; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobservice (
    keyjobservice integer DEFAULT nextval('jobservice_keyjobservice_seq'::regclass) NOT NULL,
    fkeyjob integer,
    fkeyservice integer
);


ALTER TABLE public.jobservice OWNER TO farmer;

--
-- Name: jobstat_keyjobstat_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobstat_keyjobstat_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobstat_keyjobstat_seq OWNER TO farmer;

--
-- Name: jobstat_keyjobstat_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobstat_keyjobstat_seq', 1, false);


--
-- Name: jobstat; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobstat (
    keyjobstat integer DEFAULT nextval('jobstat_keyjobstat_seq'::regclass) NOT NULL,
    fkeyelement integer,
    fkeyproject integer,
    fkeyusr integer,
    pass text,
    taskcount integer,
    taskscompleted integer DEFAULT 0,
    tasktime integer DEFAULT 0,
    started timestamp without time zone,
    ended timestamp without time zone,
    name text,
    errorcount integer DEFAULT 0,
    mintasktime interval,
    maxtasktime interval,
    avgtasktime interval,
    totaltasktime interval,
    minerrortime interval,
    maxerrortime interval,
    avgerrortime interval,
    totalerrortime interval,
    mincopytime interval,
    maxcopytime interval,
    avgcopytime interval,
    totalcopytime interval,
    copycount integer,
    minloadtime interval,
    maxloadtime interval,
    avgloadtime interval,
    totalloadtime interval,
    loadcount integer,
    submitted timestamp without time zone DEFAULT now(),
    minmemory integer,
    maxmemory integer,
    avgmemory integer,
    minefficiency double precision,
    maxefficiency double precision,
    avgefficiency double precision,
    totalbytesread integer,
    minbytesread integer,
    maxbytesread integer,
    avgbytesread integer,
    totalopsread integer,
    minopsread integer,
    maxopsread integer,
    avgopsread integer,
    totalbyteswrite integer,
    minbyteswrite integer,
    maxbyteswrite integer,
    avgbyteswrite integer,
    totalopswrite integer,
    minopswrite integer,
    maxopswrite integer,
    avgopswrite integer,
    totaliowait integer,
    miniowait integer,
    maxiowait integer,
    avgiowait integer,
    avgcputime interval,
    maxcputime interval,
    mincputime interval,
    totalcanceltime interval,
    totalcputime interval,
    fkeyjobtype integer,
    fkeyjob integer
);


ALTER TABLE public.jobstat OWNER TO farmer;

--
-- Name: jobstateaction; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobstateaction (
    keyjobstateaction integer NOT NULL,
    fkeyjob integer,
    oldstatus text,
    newstatus text,
    modified timestamp without time zone
);


ALTER TABLE public.jobstateaction OWNER TO farmer;

--
-- Name: jobstateaction_keyjobstateaction_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobstateaction_keyjobstateaction_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobstateaction_keyjobstateaction_seq OWNER TO farmer;

--
-- Name: jobstateaction_keyjobstateaction_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE jobstateaction_keyjobstateaction_seq OWNED BY jobstateaction.keyjobstateaction;


--
-- Name: jobstateaction_keyjobstateaction_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobstateaction_keyjobstateaction_seq', 1, false);


--
-- Name: jobstatus_keyjobstatus_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobstatus_keyjobstatus_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobstatus_keyjobstatus_seq OWNER TO farmer;

--
-- Name: jobstatus_keyjobstatus_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobstatus_keyjobstatus_seq', 1, false);


--
-- Name: jobstatus; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobstatus (
    keyjobstatus integer DEFAULT nextval('jobstatus_keyjobstatus_seq'::regclass) NOT NULL,
    hostsonjob integer DEFAULT 0,
    fkeyjob integer,
    tasksunassigned integer DEFAULT 0,
    taskscount integer DEFAULT 0,
    tasksdone integer DEFAULT 0,
    taskscancelled integer DEFAULT 0,
    taskssuspended integer DEFAULT 0,
    tasksassigned integer DEFAULT 0,
    tasksbusy integer DEFAULT 0,
    tasksaveragetime integer,
    health double precision,
    joblastupdated timestamp without time zone,
    errorcount integer DEFAULT 0,
    lastnotifiederrorcount integer,
    averagememory integer,
    bytesread bigint,
    byteswrite bigint,
    cputime bigint,
    efficiency double precision,
    opsread bigint,
    opswrite bigint,
    totaltime bigint,
    queueorder smallint DEFAULT 0 NOT NULL,
    taskbitmap text,
    averagedonetime integer DEFAULT 0,
    fkeyjobstatusskipreason integer
);


ALTER TABLE public.jobstatus OWNER TO farmer;

--
-- Name: jobstatus_old; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobstatus_old (
    keyjobstatus integer DEFAULT 0 NOT NULL,
    hostsonjob integer DEFAULT 0,
    fkeyjob integer,
    tasksunassigned integer DEFAULT 0,
    taskscount integer DEFAULT 0,
    tasksdone integer DEFAULT 0,
    taskscancelled integer DEFAULT 0,
    taskssuspended integer DEFAULT 0,
    tasksassigned integer DEFAULT 0,
    tasksbusy integer DEFAULT 0,
    tasksaveragetime integer,
    health double precision,
    joblastupdated timestamp without time zone,
    errorcount integer DEFAULT 0,
    lastnotifiederrorcount integer,
    averagememory integer,
    bytesread bigint,
    byteswrite bigint,
    cputime bigint,
    efficiency double precision,
    opsread bigint,
    opswrite bigint,
    totaltime bigint
);


ALTER TABLE public.jobstatus_old OWNER TO farmer;

--
-- Name: jobstatusskipreason; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobstatusskipreason (
    keyjobstatusskipreason integer NOT NULL,
    name text
);


ALTER TABLE public.jobstatusskipreason OWNER TO farmer;

--
-- Name: jobstatusskipreason_keyjobstatusskipreason_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobstatusskipreason_keyjobstatusskipreason_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobstatusskipreason_keyjobstatusskipreason_seq OWNER TO farmer;

--
-- Name: jobstatusskipreason_keyjobstatusskipreason_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE jobstatusskipreason_keyjobstatusskipreason_seq OWNED BY jobstatusskipreason.keyjobstatusskipreason;


--
-- Name: jobstatusskipreason_keyjobstatusskipreason_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobstatusskipreason_keyjobstatusskipreason_seq', 1, false);


--
-- Name: jobtaskassignment_old; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobtaskassignment_old (
    keyjobtaskassignment integer,
    fkeyjobassignment integer,
    memory integer,
    started timestamp without time zone,
    ended timestamp without time zone,
    fkeyjobassignmentstatus integer,
    fkeyjobtask integer,
    fkeyjoberror integer
);


ALTER TABLE public.jobtaskassignment_old OWNER TO farmer;

--
-- Name: jobtaskassignment_keyjobtaskassignment_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobtaskassignment_keyjobtaskassignment_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 10;


ALTER TABLE public.jobtaskassignment_keyjobtaskassignment_seq OWNER TO farmer;

--
-- Name: jobtaskassignment_keyjobtaskassignment_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE jobtaskassignment_keyjobtaskassignment_seq OWNED BY jobtaskassignment_old.keyjobtaskassignment;


--
-- Name: jobtaskassignment_keyjobtaskassignment_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobtaskassignment_keyjobtaskassignment_seq', 1, false);


--
-- Name: jobtaskassignment; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobtaskassignment (
    keyjobtaskassignment integer DEFAULT nextval('jobtaskassignment_keyjobtaskassignment_seq'::regclass) NOT NULL,
    fkeyjobassignment integer,
    memory integer,
    started timestamp without time zone,
    ended timestamp without time zone,
    fkeyjobassignmentstatus integer,
    fkeyjobtask integer,
    fkeyjoberror integer
);


ALTER TABLE public.jobtaskassignment OWNER TO farmer;

--
-- Name: jobtypemapping_keyjobtypemapping_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE jobtypemapping_keyjobtypemapping_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobtypemapping_keyjobtypemapping_seq OWNER TO farmer;

--
-- Name: jobtypemapping_keyjobtypemapping_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('jobtypemapping_keyjobtypemapping_seq', 1, false);


--
-- Name: jobtypemapping; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE jobtypemapping (
    keyjobtypemapping integer DEFAULT nextval('jobtypemapping_keyjobtypemapping_seq'::regclass) NOT NULL,
    fkeyjobtype integer,
    fkeymapping integer
);


ALTER TABLE public.jobtypemapping OWNER TO farmer;

--
-- Name: license_keylicense_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE license_keylicense_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.license_keylicense_seq OWNER TO farmer;

--
-- Name: license_keylicense_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('license_keylicense_seq', 1, false);


--
-- Name: license; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE license (
    keylicense integer DEFAULT nextval('license_keylicense_seq'::regclass) NOT NULL,
    license text,
    fkeyhost integer,
    fkeysoftware integer,
    total integer,
    reserved integer,
    inuse integer,
    CONSTRAINT check_licenses_inuse CHECK ((inuse <= total))
);


ALTER TABLE public.license OWNER TO farmer;

--
-- Name: service_keyservice_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE service_keyservice_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.service_keyservice_seq OWNER TO farmer;

--
-- Name: service_keyservice_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('service_keyservice_seq', 1, false);


--
-- Name: service; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE service (
    keyservice integer DEFAULT nextval('service_keyservice_seq'::regclass) NOT NULL,
    service text,
    description text,
    fkeylicense integer,
    enabled boolean,
    forbiddenprocesses text,
    active boolean,
    "unique" boolean,
    fkeysoftware integer,
    fkeyjobfilterset integer
);


ALTER TABLE public.service OWNER TO farmer;

--
-- Name: license_usage; Type: VIEW; Schema: public; Owner: farmer
--

CREATE VIEW license_usage AS
    SELECT service.service, license.license, (license.total - license.reserved), count(*) AS count FROM ((((jobassignment ja JOIN job ON ((ja.fkeyjob = job.keyjob))) JOIN jobservice js ON ((js.fkeyjob = job.keyjob))) JOIN service ON ((js.fkeyservice = service.keyservice))) JOIN license ON ((service.fkeylicense = license.keylicense))) WHERE (ja.fkeyjobassignmentstatus < 4) GROUP BY service.service, license.license, license.total, license.reserved;


ALTER TABLE public.license_usage OWNER TO farmer;

--
-- Name: license_usage_2; Type: VIEW; Schema: public; Owner: farmer
--

CREATE VIEW license_usage_2 AS
    SELECT service.service, (license.total - license.reserved), count(ja.keyjobassignment) AS count FROM (((jobservice js JOIN service ON ((js.fkeyservice = service.keyservice))) LEFT JOIN jobassignment ja ON (((ja.fkeyjobassignmentstatus < 4) AND (ja.fkeyjob = js.fkeyjob)))) JOIN license ON ((service.fkeylicense = license.keylicense))) GROUP BY service.service, license.total, license.reserved;


ALTER TABLE public.license_usage_2 OWNER TO farmer;

--
-- Name: location_keylocation_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE location_keylocation_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.location_keylocation_seq OWNER TO farmer;

--
-- Name: location_keylocation_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('location_keylocation_seq', 1, false);


--
-- Name: location; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE location (
    keylocation integer DEFAULT nextval('location_keylocation_seq'::regclass) NOT NULL,
    name text
);


ALTER TABLE public.location OWNER TO farmer;

--
-- Name: mapping_keymapping_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE mapping_keymapping_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.mapping_keymapping_seq OWNER TO farmer;

--
-- Name: mapping_keymapping_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('mapping_keymapping_seq', 1, false);


--
-- Name: mapping; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE mapping (
    keymapping integer DEFAULT nextval('mapping_keymapping_seq'::regclass) NOT NULL,
    fkeyhost integer,
    share text,
    mount text,
    fkeymappingtype integer,
    description text
);


ALTER TABLE public.mapping OWNER TO farmer;

--
-- Name: mappingtype_keymappingtype_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE mappingtype_keymappingtype_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.mappingtype_keymappingtype_seq OWNER TO farmer;

--
-- Name: mappingtype_keymappingtype_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('mappingtype_keymappingtype_seq', 1, false);


--
-- Name: mappingtype; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE mappingtype (
    keymappingtype integer DEFAULT nextval('mappingtype_keymappingtype_seq'::regclass) NOT NULL,
    name text
);


ALTER TABLE public.mappingtype OWNER TO farmer;

--
-- Name: methodperms_keymethodperms_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE methodperms_keymethodperms_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.methodperms_keymethodperms_seq OWNER TO farmer;

--
-- Name: methodperms_keymethodperms_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('methodperms_keymethodperms_seq', 1, false);


--
-- Name: methodperms; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE methodperms (
    keymethodperms integer DEFAULT nextval('methodperms_keymethodperms_seq'::regclass) NOT NULL,
    method text,
    users text,
    groups text,
    fkeyproject integer
);


ALTER TABLE public.methodperms OWNER TO farmer;

--
-- Name: notification_keynotification_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE notification_keynotification_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.notification_keynotification_seq OWNER TO farmer;

--
-- Name: notification_keynotification_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('notification_keynotification_seq', 1, false);


--
-- Name: notification; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE notification (
    keynotification integer DEFAULT nextval('notification_keynotification_seq'::regclass) NOT NULL,
    created timestamp without time zone,
    subject text,
    message text,
    component text,
    event text,
    routed timestamp without time zone,
    fkeyelement integer
);


ALTER TABLE public.notification OWNER TO farmer;

--
-- Name: notificationdestination_keynotificationdestination_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE notificationdestination_keynotificationdestination_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.notificationdestination_keynotificationdestination_seq OWNER TO farmer;

--
-- Name: notificationdestination_keynotificationdestination_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('notificationdestination_keynotificationdestination_seq', 1, false);


--
-- Name: notificationdestination; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE notificationdestination (
    keynotificationdestination integer DEFAULT nextval('notificationdestination_keynotificationdestination_seq'::regclass) NOT NULL,
    fkeynotification integer,
    fkeynotificationmethod integer,
    delivered timestamp without time zone,
    destination text,
    fkeyuser integer,
    routed timestamp without time zone
);


ALTER TABLE public.notificationdestination OWNER TO farmer;

--
-- Name: notificationmethod_keynotificationmethod_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE notificationmethod_keynotificationmethod_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.notificationmethod_keynotificationmethod_seq OWNER TO farmer;

--
-- Name: notificationmethod_keynotificationmethod_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('notificationmethod_keynotificationmethod_seq', 1, false);


--
-- Name: notificationmethod; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE notificationmethod (
    keynotificationmethod integer DEFAULT nextval('notificationmethod_keynotificationmethod_seq'::regclass) NOT NULL,
    name text
);


ALTER TABLE public.notificationmethod OWNER TO farmer;

--
-- Name: notificationroute_keynotificationuserroute_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE notificationroute_keynotificationuserroute_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.notificationroute_keynotificationuserroute_seq OWNER TO farmer;

--
-- Name: notificationroute_keynotificationuserroute_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('notificationroute_keynotificationuserroute_seq', 1, false);


--
-- Name: notificationroute; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE notificationroute (
    keynotificationuserroute integer DEFAULT nextval('notificationroute_keynotificationuserroute_seq'::regclass) NOT NULL,
    eventmatch text,
    componentmatch text,
    fkeyuser integer,
    subjectmatch text,
    messagematch text,
    actions text,
    priority integer,
    fkeyelement integer,
    routeassetdescendants boolean
);


ALTER TABLE public.notificationroute OWNER TO farmer;

--
-- Name: notify_keynotify_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE notify_keynotify_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.notify_keynotify_seq OWNER TO farmer;

--
-- Name: notify_keynotify_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('notify_keynotify_seq', 1, false);


--
-- Name: notify; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE notify (
    keynotify integer DEFAULT nextval('notify_keynotify_seq'::regclass) NOT NULL,
    notify text,
    fkeyusr integer,
    fkeysyslogrealm integer,
    severitymask text,
    starttime timestamp without time zone,
    endtime timestamp without time zone,
    threshhold integer,
    notifyclass text,
    notifymethod text,
    fkeynotifymethod integer,
    threshold integer
);


ALTER TABLE public.notify OWNER TO farmer;

--
-- Name: notifymethod_keynotifymethod_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE notifymethod_keynotifymethod_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.notifymethod_keynotifymethod_seq OWNER TO farmer;

--
-- Name: notifymethod_keynotifymethod_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('notifymethod_keynotifymethod_seq', 1, false);


--
-- Name: notifymethod; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE notifymethod (
    keynotifymethod integer DEFAULT nextval('notifymethod_keynotifymethod_seq'::regclass) NOT NULL,
    notifymethod text
);


ALTER TABLE public.notifymethod OWNER TO farmer;

--
-- Name: notifysent_keynotifysent_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE notifysent_keynotifysent_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.notifysent_keynotifysent_seq OWNER TO farmer;

--
-- Name: notifysent_keynotifysent_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('notifysent_keynotifysent_seq', 1, false);


--
-- Name: notifysent; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE notifysent (
    keynotifysent integer DEFAULT nextval('notifysent_keynotifysent_seq'::regclass) NOT NULL,
    fkeynotify integer,
    fkeysyslog integer
);


ALTER TABLE public.notifysent OWNER TO farmer;

--
-- Name: notifywho_keynotifywho_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE notifywho_keynotifywho_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.notifywho_keynotifywho_seq OWNER TO farmer;

--
-- Name: notifywho_keynotifywho_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('notifywho_keynotifywho_seq', 1, false);


--
-- Name: notifywho; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE notifywho (
    keynotifywho integer DEFAULT nextval('notifywho_keynotifywho_seq'::regclass) NOT NULL,
    class text,
    fkeynotify integer,
    fkeyusr integer,
    fkey integer
);


ALTER TABLE public.notifywho OWNER TO farmer;

--
-- Name: package; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE package (
    keypackage integer NOT NULL,
    version integer,
    fkeystatus integer
);


ALTER TABLE public.package OWNER TO farmer;

--
-- Name: package_keypackage_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE package_keypackage_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.package_keypackage_seq OWNER TO farmer;

--
-- Name: package_keypackage_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE package_keypackage_seq OWNED BY package.keypackage;


--
-- Name: package_keypackage_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('package_keypackage_seq', 1, false);


--
-- Name: packageoutput; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE packageoutput (
    keypackageoutput integer NOT NULL,
    fkeyasset integer
);


ALTER TABLE public.packageoutput OWNER TO farmer;

--
-- Name: packageoutput_keypackageoutput_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE packageoutput_keypackageoutput_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.packageoutput_keypackageoutput_seq OWNER TO farmer;

--
-- Name: packageoutput_keypackageoutput_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE packageoutput_keypackageoutput_seq OWNED BY packageoutput.keypackageoutput;


--
-- Name: packageoutput_keypackageoutput_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('packageoutput_keypackageoutput_seq', 1, false);


--
-- Name: pathsynctarget_keypathsynctarget_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE pathsynctarget_keypathsynctarget_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.pathsynctarget_keypathsynctarget_seq OWNER TO farmer;

--
-- Name: pathsynctarget_keypathsynctarget_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('pathsynctarget_keypathsynctarget_seq', 1, false);


--
-- Name: pathsynctarget; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE pathsynctarget (
    keypathsynctarget integer DEFAULT nextval('pathsynctarget_keypathsynctarget_seq'::regclass) NOT NULL,
    fkeypathtracker integer,
    fkeyprojectstorage integer
);


ALTER TABLE public.pathsynctarget OWNER TO farmer;

--
-- Name: pathtemplate_keypathtemplate_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE pathtemplate_keypathtemplate_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.pathtemplate_keypathtemplate_seq OWNER TO farmer;

--
-- Name: pathtemplate_keypathtemplate_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('pathtemplate_keypathtemplate_seq', 1, false);


--
-- Name: pathtemplate; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE pathtemplate (
    keypathtemplate integer DEFAULT nextval('pathtemplate_keypathtemplate_seq'::regclass) NOT NULL,
    name text,
    pathtemplate text,
    pathre text,
    filenametemplate text,
    filenamere text,
    version integer,
    pythoncode text
);


ALTER TABLE public.pathtemplate OWNER TO farmer;

--
-- Name: pathtracker_keypathtracker_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE pathtracker_keypathtracker_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.pathtracker_keypathtracker_seq OWNER TO farmer;

--
-- Name: pathtracker_keypathtracker_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('pathtracker_keypathtracker_seq', 1, false);


--
-- Name: pathtracker; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE pathtracker (
    keypathtracker integer DEFAULT nextval('pathtracker_keypathtracker_seq'::regclass) NOT NULL,
    fkeyelement integer,
    path text,
    fkeypathtemplate integer,
    fkeyprojectstorage integer,
    storagename text
);


ALTER TABLE public.pathtracker OWNER TO farmer;

--
-- Name: permission_keypermission_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE permission_keypermission_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.permission_keypermission_seq OWNER TO farmer;

--
-- Name: permission_keypermission_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('permission_keypermission_seq', 1, false);


--
-- Name: permission; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE permission (
    keypermission integer DEFAULT nextval('permission_keypermission_seq'::regclass) NOT NULL,
    methodpattern text,
    fkeyusr integer,
    permission text,
    fkeygrp integer,
    class text
);


ALTER TABLE public.permission OWNER TO farmer;

--
-- Name: pg_stat_statements; Type: VIEW; Schema: public; Owner: postgres
--

CREATE VIEW pg_stat_statements AS
    SELECT pg_stat_statements.userid, pg_stat_statements.dbid, pg_stat_statements.query, pg_stat_statements.calls, pg_stat_statements.total_time, pg_stat_statements.rows, pg_stat_statements.shared_blks_hit, pg_stat_statements.shared_blks_read, pg_stat_statements.shared_blks_written, pg_stat_statements.local_blks_hit, pg_stat_statements.local_blks_read, pg_stat_statements.local_blks_written, pg_stat_statements.temp_blks_read, pg_stat_statements.temp_blks_written FROM pg_stat_statements() pg_stat_statements(userid, dbid, query, calls, total_time, rows, shared_blks_hit, shared_blks_read, shared_blks_written, local_blks_hit, local_blks_read, local_blks_written, temp_blks_read, temp_blks_written);


ALTER TABLE public.pg_stat_statements OWNER TO postgres;

--
-- Name: phoneno_keyphoneno_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE phoneno_keyphoneno_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.phoneno_keyphoneno_seq OWNER TO farmer;

--
-- Name: phoneno_keyphoneno_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('phoneno_keyphoneno_seq', 1, false);


--
-- Name: phoneno; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE phoneno (
    keyphoneno integer DEFAULT nextval('phoneno_keyphoneno_seq'::regclass) NOT NULL,
    phoneno text,
    fkeyphonetype integer,
    fkeyemployee integer,
    domain text
);


ALTER TABLE public.phoneno OWNER TO farmer;

--
-- Name: phonetype_keyphonetype_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE phonetype_keyphonetype_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.phonetype_keyphonetype_seq OWNER TO farmer;

--
-- Name: phonetype_keyphonetype_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('phonetype_keyphonetype_seq', 1, false);


--
-- Name: phonetype; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE phonetype (
    keyphonetype integer DEFAULT nextval('phonetype_keyphonetype_seq'::regclass) NOT NULL,
    phonetype text
);


ALTER TABLE public.phonetype OWNER TO farmer;

--
-- Name: project; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE project (
    datestart date,
    icon bytea,
    arsenalslotlimit integer,
    arsenalslotreserve integer,
    compoutputdrive text,
    datedue date,
    filetype text,
    fkeyclient integer,
    notes text,
    renderoutputdrive text,
    script text,
    shortname text,
    wipdrive text,
    projectnumber integer,
    frames integer,
    nda integer,
    dayrate double precision,
    usefilecreation integer,
    dailydrive text,
    lastscanned timestamp without time zone,
    fkeyprojectstatus integer,
    assburnerweight double precision,
    project text,
    fps integer,
    resolution text,
    resolutionwidth integer,
    resolutionheight integer,
    archived integer,
    deliverymedium text,
    renderpixelaspect text
)
INHERITS (element);


ALTER TABLE public.project OWNER TO farmer;

--
-- Name: project_slots_current; Type: VIEW; Schema: public; Owner: farmer
--

CREATE VIEW project_slots_current AS
    SELECT project.name, COALESCE(sum(jobstatus.hostsonjob), (0)::bigint) AS sum, project.arsenalslotreserve, project.arsenalslotlimit FROM ((project LEFT JOIN job ON (((project.keyelement = job.fkeyproject) AND (job.status = 'started'::text)))) LEFT JOIN jobstatus jobstatus ON ((job.keyjob = jobstatus.fkeyjob))) GROUP BY project.name, project.arsenalslotreserve, project.arsenalslotlimit;


ALTER TABLE public.project_slots_current OWNER TO farmer;

--
-- Name: project_slots_limits; Type: VIEW; Schema: public; Owner: farmer
--

CREATE VIEW project_slots_limits AS
    SELECT project.name, project.arsenalslotlimit, project.arsenalslotreserve FROM project WHERE ((project.arsenalslotlimit > (-1)) OR (project.arsenalslotreserve > 0));


ALTER TABLE public.project_slots_limits OWNER TO farmer;

--
-- Name: projectresolution_keyprojectresolution_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE projectresolution_keyprojectresolution_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.projectresolution_keyprojectresolution_seq OWNER TO farmer;

--
-- Name: projectresolution_keyprojectresolution_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('projectresolution_keyprojectresolution_seq', 1, false);


--
-- Name: projectresolution; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE projectresolution (
    keyprojectresolution integer DEFAULT nextval('projectresolution_keyprojectresolution_seq'::regclass) NOT NULL,
    deliveryformat text,
    fkeyproject integer,
    height integer,
    outputformat text,
    projectresolution text,
    width integer,
    pixelaspect double precision,
    fps integer
);


ALTER TABLE public.projectresolution OWNER TO farmer;

--
-- Name: projectstatus_keyprojectstatus_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE projectstatus_keyprojectstatus_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.projectstatus_keyprojectstatus_seq OWNER TO farmer;

--
-- Name: projectstatus_keyprojectstatus_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('projectstatus_keyprojectstatus_seq', 1, false);


--
-- Name: projectstatus; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE projectstatus (
    keyprojectstatus integer DEFAULT nextval('projectstatus_keyprojectstatus_seq'::regclass) NOT NULL,
    projectstatus text,
    chronology integer
);


ALTER TABLE public.projectstatus OWNER TO farmer;

--
-- Name: projectstorage_keyprojectstorage_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE projectstorage_keyprojectstorage_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.projectstorage_keyprojectstorage_seq OWNER TO farmer;

--
-- Name: projectstorage_keyprojectstorage_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('projectstorage_keyprojectstorage_seq', 1, false);


--
-- Name: projectstorage; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE projectstorage (
    keyprojectstorage integer DEFAULT nextval('projectstorage_keyprojectstorage_seq'::regclass) NOT NULL,
    fkeyproject integer NOT NULL,
    name text NOT NULL,
    location text NOT NULL,
    storagename text,
    "default" boolean,
    fkeyhost integer
);


ALTER TABLE public.projectstorage OWNER TO farmer;

--
-- Name: projecttempo; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE projecttempo (
    fkeyproject integer,
    tempo double precision
);


ALTER TABLE public.projecttempo OWNER TO farmer;

--
-- Name: queueorder; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE queueorder (
    queueorder integer NOT NULL,
    fkeyjob integer NOT NULL
);


ALTER TABLE public.queueorder OWNER TO farmer;

--
-- Name: rangefiletracker; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE rangefiletracker (
    filenametemplate text NOT NULL,
    framestart integer,
    frameend integer,
    fkeyresolution integer,
    renderelement text
)
INHERITS (filetracker);


ALTER TABLE public.rangefiletracker OWNER TO farmer;

--
-- Name: renderframe_keyrenderframe_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE renderframe_keyrenderframe_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.renderframe_keyrenderframe_seq OWNER TO farmer;

--
-- Name: renderframe_keyrenderframe_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('renderframe_keyrenderframe_seq', 1, false);


--
-- Name: renderframe; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE renderframe (
    keyrenderframe integer DEFAULT nextval('renderframe_keyrenderframe_seq'::regclass) NOT NULL,
    fkeyshot integer,
    frame integer,
    fkeyresolution integer,
    status text
);


ALTER TABLE public.renderframe OWNER TO farmer;

--
-- Name: running_shots_averagetime; Type: VIEW; Schema: public; Owner: farmer
--

CREATE VIEW running_shots_averagetime AS
    SELECT job.shotname, avg(jobstatus.tasksaveragetime) AS avgtime FROM (job JOIN jobstatus jobstatus ON ((jobstatus.fkeyjob = job.keyjob))) WHERE (job.status = ANY (ARRAY['ready'::text, 'started'::text])) GROUP BY job.shotname;


ALTER TABLE public.running_shots_averagetime OWNER TO farmer;

--
-- Name: running_shots_averagetime_2; Type: VIEW; Schema: public; Owner: farmer
--

CREATE VIEW running_shots_averagetime_2 AS
    SELECT job.shotname, project.name, (GREATEST((0)::double precision, avg(GREATEST((0)::double precision, (date_part('epoch'::text, ((jobtaskassignment.ended)::timestamp with time zone - (jobtaskassignment.started)::timestamp with time zone)) * (job.slots)::double precision)))))::numeric AS avgtime FROM ((((jobtaskassignment JOIN jobtask ON ((jobtask.fkeyjobtaskassignment = jobtaskassignment.keyjobtaskassignment))) JOIN job ON ((jobtask.fkeyjob = job.keyjob))) JOIN jobstatus jobstatus ON ((jobstatus.fkeyjob = job.keyjob))) JOIN project ON ((job.fkeyproject = project.keyelement))) WHERE ((jobtaskassignment.ended IS NOT NULL) AND (job.status = ANY (ARRAY['ready'::text, 'started'::text]))) GROUP BY job.shotname, project.name;


ALTER TABLE public.running_shots_averagetime_2 OWNER TO farmer;

--
-- Name: running_shots_averagetime_3; Type: VIEW; Schema: public; Owner: farmer
--

CREATE VIEW running_shots_averagetime_3 AS
    SELECT job.shotname, project.name, (GREATEST((0)::double precision, avg(GREATEST((0)::double precision, (date_part('epoch'::text, ((jobassignment.ended)::timestamp with time zone - (jobassignment.started)::timestamp with time zone)) * (jobassignment.assignslots)::double precision)))))::integer AS avgtime FROM ((jobassignment JOIN job ON (((jobassignment.fkeyjob = job.keyjob) AND (job.status = ANY (ARRAY['ready'::text, 'started'::text]))))) JOIN project ON ((job.fkeyproject = project.keyelement))) WHERE (jobassignment.ended IS NOT NULL) GROUP BY job.shotname, project.name;


ALTER TABLE public.running_shots_averagetime_3 OWNER TO farmer;

--
-- Name: running_shots_averagetime_4; Type: VIEW; Schema: public; Owner: postgres
--

CREATE VIEW running_shots_averagetime_4 AS
    SELECT job.shotname, project.name, ((sum(jobstatus.averagedonetime))::integer / COALESCE(dw.weight, 1)) AS avgtime FROM (((job JOIN jobstatus ON ((jobstatus.fkeyjob = job.keyjob))) JOIN project ON ((job.fkeyproject = project.keyelement))) LEFT JOIN darwinweight dw ON (((job.shotname = dw.shotname) AND (project.name = dw.projectname)))) WHERE (job.status = ANY (ARRAY['ready'::text, 'started'::text])) GROUP BY job.shotname, project.name, dw.weight ORDER BY job.shotname;


ALTER TABLE public.running_shots_averagetime_4 OWNER TO postgres;

--
-- Name: schedule_keyschedule_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE schedule_keyschedule_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.schedule_keyschedule_seq OWNER TO farmer;

--
-- Name: schedule_keyschedule_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('schedule_keyschedule_seq', 1, false);


--
-- Name: schedule; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE schedule (
    keyschedule integer DEFAULT nextval('schedule_keyschedule_seq'::regclass) NOT NULL,
    fkeyuser integer,
    date date NOT NULL,
    starthour integer,
    hours integer,
    fkeyelement integer,
    fkeyassettype integer,
    fkeycreatedbyuser integer,
    duration interval,
    starttime time without time zone
);


ALTER TABLE public.schedule OWNER TO farmer;

--
-- Name: serverfileaction; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE serverfileaction (
    keyserverfileaction integer NOT NULL,
    fkeyserverfileactionstatus integer,
    fkeyserverfileactiontype integer,
    fkeyhost integer,
    destpath text,
    errormessage text,
    sourcepath text
);


ALTER TABLE public.serverfileaction OWNER TO farmer;

--
-- Name: serverfileaction_keyserverfileaction_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE serverfileaction_keyserverfileaction_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.serverfileaction_keyserverfileaction_seq OWNER TO farmer;

--
-- Name: serverfileaction_keyserverfileaction_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE serverfileaction_keyserverfileaction_seq OWNED BY serverfileaction.keyserverfileaction;


--
-- Name: serverfileaction_keyserverfileaction_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('serverfileaction_keyserverfileaction_seq', 1, false);


--
-- Name: serverfileactionstatus; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE serverfileactionstatus (
    keyserverfileactionstatus integer NOT NULL,
    status text,
    name text
);


ALTER TABLE public.serverfileactionstatus OWNER TO farmer;

--
-- Name: serverfileactionstatus_keyserverfileactionstatus_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE serverfileactionstatus_keyserverfileactionstatus_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.serverfileactionstatus_keyserverfileactionstatus_seq OWNER TO farmer;

--
-- Name: serverfileactionstatus_keyserverfileactionstatus_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE serverfileactionstatus_keyserverfileactionstatus_seq OWNED BY serverfileactionstatus.keyserverfileactionstatus;


--
-- Name: serverfileactionstatus_keyserverfileactionstatus_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('serverfileactionstatus_keyserverfileactionstatus_seq', 1, false);


--
-- Name: serverfileactiontype; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE serverfileactiontype (
    keyserverfileactiontype integer NOT NULL,
    type text
);


ALTER TABLE public.serverfileactiontype OWNER TO farmer;

--
-- Name: serverfileactiontype_keyserverfileactiontype_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE serverfileactiontype_keyserverfileactiontype_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.serverfileactiontype_keyserverfileactiontype_seq OWNER TO farmer;

--
-- Name: serverfileactiontype_keyserverfileactiontype_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE serverfileactiontype_keyserverfileactiontype_seq OWNED BY serverfileactiontype.keyserverfileactiontype;


--
-- Name: serverfileactiontype_keyserverfileactiontype_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('serverfileactiontype_keyserverfileactiontype_seq', 1, false);


--
-- Name: sessions; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE sessions (
    id text,
    length integer,
    a_session text,
    "time" timestamp without time zone
);


ALTER TABLE public.sessions OWNER TO farmer;

--
-- Name: shot; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE shot (
    arsenalslotlimit integer,
    arsenalslotreserve integer,
    dialog text,
    frameend integer,
    framestart integer,
    shot double precision,
    framestartedl integer,
    frameendedl integer,
    camerainfo text,
    scriptpage integer
)
INHERITS (element);


ALTER TABLE public.shot OWNER TO farmer;

--
-- Name: shotgroup; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE shotgroup (
    arsenalslotlimit integer,
    arsenalslotreserve integer,
    shotgroup text
)
INHERITS (element);


ALTER TABLE public.shotgroup OWNER TO farmer;

--
-- Name: slots_total; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE slots_total (
    sum bigint
);


ALTER TABLE public.slots_total OWNER TO farmer;

--
-- Name: software_keysoftware_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE software_keysoftware_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.software_keysoftware_seq OWNER TO farmer;

--
-- Name: software_keysoftware_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('software_keysoftware_seq', 1, false);


--
-- Name: software; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE software (
    keysoftware integer DEFAULT nextval('software_keysoftware_seq'::regclass) NOT NULL,
    software character varying(64),
    icon text,
    vendor character varying(64),
    vendorcontact text,
    active boolean,
    executable text,
    installedpath text,
    sixtyfourbit boolean,
    version double precision
);


ALTER TABLE public.software OWNER TO farmer;

--
-- Name: status; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE status (
    keystatus integer NOT NULL,
    name text
);


ALTER TABLE public.status OWNER TO farmer;

--
-- Name: status_keystatus_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE status_keystatus_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.status_keystatus_seq OWNER TO farmer;

--
-- Name: status_keystatus_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE status_keystatus_seq OWNED BY status.keystatus;


--
-- Name: status_keystatus_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('status_keystatus_seq', 1, false);


--
-- Name: statusset_keystatusset_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE statusset_keystatusset_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.statusset_keystatusset_seq OWNER TO farmer;

--
-- Name: statusset_keystatusset_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('statusset_keystatusset_seq', 1, false);


--
-- Name: statusset; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE statusset (
    keystatusset integer DEFAULT nextval('statusset_keystatusset_seq'::regclass) NOT NULL,
    name text NOT NULL
);


ALTER TABLE public.statusset OWNER TO farmer;

--
-- Name: syslog_keysyslog_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE syslog_keysyslog_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.syslog_keysyslog_seq OWNER TO farmer;

--
-- Name: syslog_keysyslog_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('syslog_keysyslog_seq', 1, false);


--
-- Name: syslog; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE syslog (
    keysyslog integer DEFAULT nextval('syslog_keysyslog_seq'::regclass) NOT NULL,
    fkeyhost integer,
    fkeysyslogrealm integer,
    fkeysyslogseverity integer,
    message text,
    count integer DEFAULT nextval(('syslog_count_seq'::text)::regclass),
    lastoccurrence timestamp without time zone DEFAULT now(),
    created timestamp without time zone DEFAULT now(),
    class text,
    method text,
    ack smallint DEFAULT 0 NOT NULL,
    firstoccurence timestamp without time zone,
    hostname text,
    username text
);


ALTER TABLE public.syslog OWNER TO farmer;

--
-- Name: syslog_count_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE syslog_count_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.syslog_count_seq OWNER TO farmer;

--
-- Name: syslog_count_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('syslog_count_seq', 1, false);


--
-- Name: syslogrealm_keysyslogrealm_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE syslogrealm_keysyslogrealm_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.syslogrealm_keysyslogrealm_seq OWNER TO farmer;

--
-- Name: syslogrealm_keysyslogrealm_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('syslogrealm_keysyslogrealm_seq', 1, false);


--
-- Name: syslogrealm; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE syslogrealm (
    keysyslogrealm integer DEFAULT nextval('syslogrealm_keysyslogrealm_seq'::regclass) NOT NULL,
    syslogrealm text
);


ALTER TABLE public.syslogrealm OWNER TO farmer;

--
-- Name: syslogseverity_keysyslogseverity_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE syslogseverity_keysyslogseverity_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.syslogseverity_keysyslogseverity_seq OWNER TO farmer;

--
-- Name: syslogseverity_keysyslogseverity_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('syslogseverity_keysyslogseverity_seq', 1, false);


--
-- Name: syslogseverity; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE syslogseverity (
    keysyslogseverity integer DEFAULT nextval('syslogseverity_keysyslogseverity_seq'::regclass) NOT NULL,
    syslogseverity text,
    severity text
);


ALTER TABLE public.syslogseverity OWNER TO farmer;

--
-- Name: task; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE task (
    icon bytea,
    arsenalslotlimit integer,
    fkeytasktype integer,
    shotgroup integer
)
INHERITS (element);


ALTER TABLE public.task OWNER TO farmer;

--
-- Name: tasktype_keytasktype_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE tasktype_keytasktype_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.tasktype_keytasktype_seq OWNER TO farmer;

--
-- Name: tasktype_keytasktype_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('tasktype_keytasktype_seq', 1, false);


--
-- Name: tasktype; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE tasktype (
    keytasktype integer DEFAULT nextval('tasktype_keytasktype_seq'::regclass) NOT NULL,
    tasktype text,
    iconcolor text
);


ALTER TABLE public.tasktype OWNER TO farmer;

--
-- Name: taskuser_keytaskuser_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE taskuser_keytaskuser_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.taskuser_keytaskuser_seq OWNER TO farmer;

--
-- Name: taskuser_keytaskuser_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('taskuser_keytaskuser_seq', 1, false);


--
-- Name: taskuser; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE taskuser (
    keytaskuser integer DEFAULT nextval('taskuser_keytaskuser_seq'::regclass) NOT NULL,
    fkeytask integer,
    fkeyuser integer,
    active integer
);


ALTER TABLE public.taskuser OWNER TO farmer;

--
-- Name: thread_keythread_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE thread_keythread_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.thread_keythread_seq OWNER TO farmer;

--
-- Name: thread_keythread_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('thread_keythread_seq', 1, false);


--
-- Name: thread; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE thread (
    keythread integer DEFAULT nextval('thread_keythread_seq'::regclass) NOT NULL,
    thread text,
    topic text,
    tablename text,
    fkey integer,
    datetime timestamp without time zone,
    fkeyauthor integer,
    skeyreply integer,
    fkeyusr integer,
    fkeythreadcategory integer
);


ALTER TABLE public.thread OWNER TO farmer;

--
-- Name: threadcategory_keythreadcategory_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE threadcategory_keythreadcategory_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.threadcategory_keythreadcategory_seq OWNER TO farmer;

--
-- Name: threadcategory_keythreadcategory_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('threadcategory_keythreadcategory_seq', 1, false);


--
-- Name: threadcategory; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE threadcategory (
    keythreadcategory integer DEFAULT nextval('threadcategory_keythreadcategory_seq'::regclass) NOT NULL,
    threadcategory text
);


ALTER TABLE public.threadcategory OWNER TO farmer;

--
-- Name: threadnotify_keythreadnotify_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE threadnotify_keythreadnotify_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.threadnotify_keythreadnotify_seq OWNER TO farmer;

--
-- Name: threadnotify_keythreadnotify_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('threadnotify_keythreadnotify_seq', 1, false);


--
-- Name: threadnotify; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE threadnotify (
    keythreadnotify integer DEFAULT nextval('threadnotify_keythreadnotify_seq'::regclass) NOT NULL,
    fkeythread integer,
    fkeyuser integer,
    options integer
);


ALTER TABLE public.threadnotify OWNER TO farmer;

--
-- Name: thumbnail_keythumbnail_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE thumbnail_keythumbnail_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.thumbnail_keythumbnail_seq OWNER TO farmer;

--
-- Name: thumbnail_keythumbnail_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('thumbnail_keythumbnail_seq', 1, false);


--
-- Name: thumbnail; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE thumbnail (
    keythumbnail integer DEFAULT nextval('thumbnail_keythumbnail_seq'::regclass) NOT NULL,
    cliprect text,
    date timestamp without time zone,
    fkeyelement integer,
    fkeyuser integer,
    originalfile text,
    image bytea
);


ALTER TABLE public.thumbnail OWNER TO farmer;

--
-- Name: timesheet_keytimesheet_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE timesheet_keytimesheet_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.timesheet_keytimesheet_seq OWNER TO farmer;

--
-- Name: timesheet_keytimesheet_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('timesheet_keytimesheet_seq', 1, false);


--
-- Name: timesheet; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE timesheet (
    keytimesheet integer DEFAULT nextval('timesheet_keytimesheet_seq'::regclass) NOT NULL,
    datetime timestamp without time zone,
    fkeyelement integer,
    fkeyemployee integer,
    fkeyproject integer,
    fkeytimesheetcategory integer,
    scheduledhour double precision,
    datetimesubmitted timestamp without time zone,
    unscheduledhour double precision,
    comment text
);


ALTER TABLE public.timesheet OWNER TO farmer;

--
-- Name: timesheetcategory_keytimesheetcategory_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE timesheetcategory_keytimesheetcategory_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.timesheetcategory_keytimesheetcategory_seq OWNER TO farmer;

--
-- Name: timesheetcategory_keytimesheetcategory_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('timesheetcategory_keytimesheetcategory_seq', 1, false);


--
-- Name: timesheetcategory; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE timesheetcategory (
    keytimesheetcategory integer DEFAULT nextval('timesheetcategory_keytimesheetcategory_seq'::regclass) NOT NULL,
    timesheetcategory text,
    iconcolor integer,
    hasdaily integer,
    chronology text,
    disabled integer,
    istask boolean,
    fkeypathtemplate integer,
    fkeyelementtype integer DEFAULT 4,
    nameregexp text,
    allowtime boolean,
    color text,
    description text,
    sortcolumn text DEFAULT 'displayName'::text,
    tags text,
    sortnumber integer
);


ALTER TABLE public.timesheetcategory OWNER TO farmer;

--
-- Name: tracker_keytracker_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE tracker_keytracker_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.tracker_keytracker_seq OWNER TO farmer;

--
-- Name: tracker_keytracker_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('tracker_keytracker_seq', 1, false);


--
-- Name: tracker; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE tracker (
    keytracker integer DEFAULT nextval('tracker_keytracker_seq'::regclass) NOT NULL,
    tracker text,
    fkeysubmitter integer,
    fkeyassigned integer,
    fkeycategory integer,
    fkeyseverity integer,
    fkeystatus integer,
    datetarget date,
    datechanged timestamp without time zone,
    datesubmitted timestamp without time zone,
    description text,
    timeestimate integer,
    fkeytrackerqueue integer
);


ALTER TABLE public.tracker OWNER TO farmer;

--
-- Name: trackercategory_keytrackercategory_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE trackercategory_keytrackercategory_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.trackercategory_keytrackercategory_seq OWNER TO farmer;

--
-- Name: trackercategory_keytrackercategory_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('trackercategory_keytrackercategory_seq', 1, false);


--
-- Name: trackercategory; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE trackercategory (
    keytrackercategory integer DEFAULT nextval('trackercategory_keytrackercategory_seq'::regclass) NOT NULL,
    trackercategory text
);


ALTER TABLE public.trackercategory OWNER TO farmer;

--
-- Name: trackerlog_keytrackerlog_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE trackerlog_keytrackerlog_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.trackerlog_keytrackerlog_seq OWNER TO farmer;

--
-- Name: trackerlog_keytrackerlog_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('trackerlog_keytrackerlog_seq', 1, false);


--
-- Name: trackerlog; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE trackerlog (
    keytrackerlog integer DEFAULT nextval('trackerlog_keytrackerlog_seq'::regclass) NOT NULL,
    fkeytracker integer,
    fkeyusr integer,
    datelogged integer,
    message text
);


ALTER TABLE public.trackerlog OWNER TO farmer;

--
-- Name: trackerqueue_keytrackerqueue_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE trackerqueue_keytrackerqueue_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.trackerqueue_keytrackerqueue_seq OWNER TO farmer;

--
-- Name: trackerqueue_keytrackerqueue_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('trackerqueue_keytrackerqueue_seq', 1, false);


--
-- Name: trackerqueue; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE trackerqueue (
    keytrackerqueue integer DEFAULT nextval('trackerqueue_keytrackerqueue_seq'::regclass) NOT NULL,
    trackerqueue text
);


ALTER TABLE public.trackerqueue OWNER TO farmer;

--
-- Name: trackerseverity_keytrackerseverity_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE trackerseverity_keytrackerseverity_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.trackerseverity_keytrackerseverity_seq OWNER TO farmer;

--
-- Name: trackerseverity_keytrackerseverity_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('trackerseverity_keytrackerseverity_seq', 1, false);


--
-- Name: trackerseverity; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE trackerseverity (
    keytrackerseverity integer DEFAULT nextval('trackerseverity_keytrackerseverity_seq'::regclass) NOT NULL,
    trackerseverity text
);


ALTER TABLE public.trackerseverity OWNER TO farmer;

--
-- Name: trackerstatus_keytrackerstatus_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE trackerstatus_keytrackerstatus_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.trackerstatus_keytrackerstatus_seq OWNER TO farmer;

--
-- Name: trackerstatus_keytrackerstatus_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('trackerstatus_keytrackerstatus_seq', 1, false);


--
-- Name: trackerstatus; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE trackerstatus (
    keytrackerstatus integer DEFAULT nextval('trackerstatus_keytrackerstatus_seq'::regclass) NOT NULL,
    trackerstatus text
);


ALTER TABLE public.trackerstatus OWNER TO farmer;

--
-- Name: user_service_current; Type: VIEW; Schema: public; Owner: farmer
--

CREATE VIEW user_service_current AS
    SELECT ((usr.name || ':'::text) || service.service), sum(ja.assignslots) AS sum FROM ((((usr JOIN job ON (((usr.keyelement = job.fkeyusr) AND (job.status = ANY (ARRAY['ready'::text, 'started'::text]))))) JOIN jobassignment ja ON (((job.keyjob = ja.fkeyjob) AND (ja.fkeyjobassignmentstatus < 4)))) JOIN jobservice js ON ((job.keyjob = js.fkeyjob))) JOIN service ON ((service.keyservice = js.fkeyservice))) GROUP BY usr.name, service.service;


ALTER TABLE public.user_service_current OWNER TO farmer;

--
-- Name: userservice; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE userservice (
    keyuserservice integer NOT NULL,
    "user" integer,
    service integer,
    "limit" integer
);


ALTER TABLE public.userservice OWNER TO farmer;

--
-- Name: user_service_limits; Type: VIEW; Schema: public; Owner: farmer
--

CREATE VIEW user_service_limits AS
    SELECT ((usr.name || ':'::text) || service.service), sum(us."limit") AS nottoexceed FROM ((usr JOIN userservice us ON ((usr.keyelement = us."user"))) JOIN service ON ((service.keyservice = us.service))) GROUP BY usr.name, service.service;


ALTER TABLE public.user_service_limits OWNER TO farmer;

--
-- Name: user_slots_current; Type: VIEW; Schema: public; Owner: farmer
--

CREATE VIEW user_slots_current AS
    SELECT usr.name, sum(jobstatus.hostsonjob) AS sum FROM ((usr JOIN job ON (((usr.keyelement = job.fkeyusr) AND (job.status = 'started'::text)))) JOIN jobstatus jobstatus ON ((job.keyjob = jobstatus.fkeyjob))) GROUP BY usr.name;


ALTER TABLE public.user_slots_current OWNER TO farmer;

--
-- Name: user_slots_limits; Type: VIEW; Schema: public; Owner: farmer
--

CREATE VIEW user_slots_limits AS
    SELECT usr.name, usr.arsenalslotlimit, usr.arsenalslotreserve FROM usr WHERE ((usr.arsenalslotlimit > (-1)) OR (usr.arsenalslotreserve > 0));


ALTER TABLE public.user_slots_limits OWNER TO farmer;

--
-- Name: userelement_keyuserelement_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE userelement_keyuserelement_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.userelement_keyuserelement_seq OWNER TO farmer;

--
-- Name: userelement_keyuserelement_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('userelement_keyuserelement_seq', 1, false);


--
-- Name: userelement; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE userelement (
    keyuserelement integer DEFAULT nextval('userelement_keyuserelement_seq'::regclass) NOT NULL,
    fkeyelement integer,
    fkeyusr integer,
    fkeyuser integer
);


ALTER TABLE public.userelement OWNER TO farmer;

--
-- Name: usermapping_keyusermapping_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE usermapping_keyusermapping_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.usermapping_keyusermapping_seq OWNER TO farmer;

--
-- Name: usermapping_keyusermapping_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('usermapping_keyusermapping_seq', 1, false);


--
-- Name: usermapping; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE usermapping (
    keyusermapping integer DEFAULT nextval('usermapping_keyusermapping_seq'::regclass) NOT NULL,
    fkeyusr integer,
    fkeymapping integer
);


ALTER TABLE public.usermapping OWNER TO farmer;

--
-- Name: userrole_keyuserrole_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE userrole_keyuserrole_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.userrole_keyuserrole_seq OWNER TO farmer;

--
-- Name: userrole_keyuserrole_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('userrole_keyuserrole_seq', 1, false);


--
-- Name: userrole; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE userrole (
    keyuserrole integer DEFAULT nextval('userrole_keyuserrole_seq'::regclass) NOT NULL,
    fkeytasktype integer,
    fkeyusr integer
);


ALTER TABLE public.userrole OWNER TO farmer;

--
-- Name: userservice_keyuserservice_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE userservice_keyuserservice_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.userservice_keyuserservice_seq OWNER TO farmer;

--
-- Name: userservice_keyuserservice_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE userservice_keyuserservice_seq OWNED BY userservice.keyuserservice;


--
-- Name: userservice_keyuserservice_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('userservice_keyuserservice_seq', 1, false);


--
-- Name: usrgrp_keyusrgrp_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE usrgrp_keyusrgrp_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.usrgrp_keyusrgrp_seq OWNER TO farmer;

--
-- Name: usrgrp_keyusrgrp_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('usrgrp_keyusrgrp_seq', 1, false);


--
-- Name: usrgrp; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE usrgrp (
    keyusrgrp integer DEFAULT nextval('usrgrp_keyusrgrp_seq'::regclass) NOT NULL,
    fkeyusr integer,
    fkeygrp integer,
    usrgrp text
);


ALTER TABLE public.usrgrp OWNER TO farmer;

--
-- Name: version; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE version (
    keyversion integer NOT NULL
);


ALTER TABLE public.version OWNER TO farmer;

--
-- Name: version_keyversion_seq; Type: SEQUENCE; Schema: public; Owner: farmer
--

CREATE SEQUENCE version_keyversion_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.version_keyversion_seq OWNER TO farmer;

--
-- Name: version_keyversion_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: farmer
--

ALTER SEQUENCE version_keyversion_seq OWNED BY version.keyversion;


--
-- Name: version_keyversion_seq; Type: SEQUENCE SET; Schema: public; Owner: farmer
--

SELECT pg_catalog.setval('version_keyversion_seq', 1, false);


--
-- Name: versionfiletracker; Type: TABLE; Schema: public; Owner: farmer; Tablespace: 
--

CREATE TABLE versionfiletracker (
    filenametemplate text,
    fkeyversionfiletracker integer,
    oldfilenames text,
    version integer,
    iteration integer,
    automaster integer
)
INHERITS (filetracker);


ALTER TABLE public.versionfiletracker OWNER TO farmer;

--
-- Name: keyassetdep; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE assetdep ALTER COLUMN keyassetdep SET DEFAULT nextval('assetdep_keyassetdep_seq'::regclass);


--
-- Name: keyassetprop; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE assetprop ALTER COLUMN keyassetprop SET DEFAULT nextval('assetprop_keyassetprop_seq'::regclass);


--
-- Name: keyassetproptype; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE assetproptype ALTER COLUMN keyassetproptype SET DEFAULT nextval('assetproptype_keyassetproptype_seq'::regclass);


--
-- Name: keydarwinscore; Type: DEFAULT; Schema: public; Owner: farmers
--

ALTER TABLE darwinweight ALTER COLUMN keydarwinscore SET DEFAULT nextval('darwinweight_keydarwinscore_seq'::regclass);


--
-- Name: keyjobassignment; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE jobassignment_old ALTER COLUMN keyjobassignment SET DEFAULT nextval('jobassignment_keyjobassignment_seq'::regclass);


--
-- Name: keyjobassignmentstatus; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE jobassignmentstatus ALTER COLUMN keyjobassignmentstatus SET DEFAULT nextval('jobassignmentstatus_keyjobassignmentstatus_seq'::regclass);


--
-- Name: keyjobenvironment; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE jobenvironment ALTER COLUMN keyjobenvironment SET DEFAULT nextval('jobenvironment_keyjobenvironment_seq'::regclass);


--
-- Name: keyjobfiltermessage; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE jobfiltermessage ALTER COLUMN keyjobfiltermessage SET DEFAULT nextval('jobfiltermessage_keyjobfiltermessage_seq'::regclass);


--
-- Name: keyjobfilterset; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE jobfilterset ALTER COLUMN keyjobfilterset SET DEFAULT nextval('jobfilterset_keyjobfilterset_seq'::regclass);


--
-- Name: keyjobfiltertype; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE jobfiltertype ALTER COLUMN keyjobfiltertype SET DEFAULT nextval('jobfiltertype_keyjobfiltertype_seq'::regclass);


--
-- Name: keyjobmantra100; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE jobmantra100 ALTER COLUMN keyjobmantra100 SET DEFAULT nextval('jobmantra100_keyjobmantra100_seq'::regclass);


--
-- Name: keyjobmantra95; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE jobmantra95 ALTER COLUMN keyjobmantra95 SET DEFAULT nextval('jobmantra95_keyjobmantra95_seq'::regclass);


--
-- Name: keyjobstateaction; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE jobstateaction ALTER COLUMN keyjobstateaction SET DEFAULT nextval('jobstateaction_keyjobstateaction_seq'::regclass);


--
-- Name: keyjobstatusskipreason; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE jobstatusskipreason ALTER COLUMN keyjobstatusskipreason SET DEFAULT nextval('jobstatusskipreason_keyjobstatusskipreason_seq'::regclass);


--
-- Name: keypackage; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE package ALTER COLUMN keypackage SET DEFAULT nextval('package_keypackage_seq'::regclass);


--
-- Name: keypackageoutput; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE packageoutput ALTER COLUMN keypackageoutput SET DEFAULT nextval('packageoutput_keypackageoutput_seq'::regclass);


--
-- Name: keyserverfileaction; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE serverfileaction ALTER COLUMN keyserverfileaction SET DEFAULT nextval('serverfileaction_keyserverfileaction_seq'::regclass);


--
-- Name: keyserverfileactionstatus; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE serverfileactionstatus ALTER COLUMN keyserverfileactionstatus SET DEFAULT nextval('serverfileactionstatus_keyserverfileactionstatus_seq'::regclass);


--
-- Name: keyserverfileactiontype; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE serverfileactiontype ALTER COLUMN keyserverfileactiontype SET DEFAULT nextval('serverfileactiontype_keyserverfileactiontype_seq'::regclass);


--
-- Name: keystatus; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE status ALTER COLUMN keystatus SET DEFAULT nextval('status_keystatus_seq'::regclass);


--
-- Name: keyuserservice; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE userservice ALTER COLUMN keyuserservice SET DEFAULT nextval('userservice_keyuserservice_seq'::regclass);


--
-- Name: keyversion; Type: DEFAULT; Schema: public; Owner: farmer
--

ALTER TABLE version ALTER COLUMN keyversion SET DEFAULT nextval('version_keyversion_seq'::regclass);


--
-- Data for Name: abdownloadstat; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY abdownloadstat (keyabdownloadstat, type, size, fkeyhost, "time", abrev, finished, fkeyjob) FROM stdin;
\.


--
-- Data for Name: annotation; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY annotation (keyannotation, notes, sequence, framestart, frameend, markupdata) FROM stdin;
\.


--
-- Data for Name: asset; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY asset (keyelement, daysbid, description, fkeyelement, fkeyelementstatus, fkeyelementtype, fkeyproject, fkeythumbnail, name, daysscheduled, daysestimated, status, filepath, fkeyassettype, fkeypathtemplate, fkeystatusset, allowtime, datestart, datecomplete, fkeyassettemplate, icon, arsenalslotlimit, arsenalslotreserve, fkeystatus, keyasset, version) FROM stdin;
\.


--
-- Data for Name: assetdep; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY assetdep (keyassetdep, path, fkeypackage, fkeyasset) FROM stdin;
\.


--
-- Data for Name: assetgroup; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY assetgroup (keyelement, daysbid, description, fkeyelement, fkeyelementstatus, fkeyelementtype, fkeyproject, fkeythumbnail, name, daysscheduled, daysestimated, status, filepath, fkeyassettype, fkeypathtemplate, fkeystatusset, allowtime, datestart, datecomplete, fkeyassettemplate, icon, arsenalslotlimit, arsenalslotreserve) FROM stdin;
\.


--
-- Data for Name: assetprop; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY assetprop (keyassetprop, fkeyassetproptype, fkeyasset) FROM stdin;
\.


--
-- Data for Name: assetproperty; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY assetproperty (keyassetproperty, name, type, value, fkeyelement) FROM stdin;
\.


--
-- Data for Name: assetproptype; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY assetproptype (keyassetproptype, name, depth) FROM stdin;
\.


--
-- Data for Name: assetset; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY assetset (keyassetset, fkeyproject, fkeyelementtype, fkeyassettype, name) FROM stdin;
\.


--
-- Data for Name: assetsetitem; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY assetsetitem (keyassetsetitem, fkeyassetset, fkeyassettype, fkeyelementtype, fkeytasktype) FROM stdin;
\.


--
-- Data for Name: assettemplate; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY assettemplate (keyassettemplate, fkeyassettype, fkeyelement, fkeyproject, name) FROM stdin;
\.


--
-- Data for Name: assettype; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY assettype (keyassettype, assettype, deleted) FROM stdin;
1	Character	f
2	Environment	f
3	Prop	f
\.


--
-- Data for Name: attachment; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY attachment (keyattachment, caption, created, filename, fkeyelement, fkeyuser, origpath, attachment, url, description, fkeyauthor, fkeyattachmenttype) FROM stdin;
\.


--
-- Data for Name: attachmenttype; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY attachmenttype (keyattachmenttype, attachmenttype) FROM stdin;
\.


--
-- Data for Name: calendar; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY calendar (keycalendar, repeat, fkeycalendarcategory, url, fkeyauthor, fieldname, notifylist, notifybatch, leadtime, notifymask, fkeyusr, private, date, calendar, fkeyproject) FROM stdin;
\.


--
-- Data for Name: calendarcategory; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY calendarcategory (keycalendarcategory, calendarcategory) FROM stdin;
\.


--
-- Data for Name: checklistitem; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY checklistitem (keychecklistitem, body, checklistitem, fkeyproject, fkeythumbnail, fkeytimesheetcategory, type, fkeystatusset) FROM stdin;
\.


--
-- Data for Name: checkliststatus; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY checkliststatus (keycheckliststatus, fkeychecklistitem, fkeyelement, state, fkeyelementstatus) FROM stdin;
\.


--
-- Data for Name: client; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY client (keyclient, client, textcard) FROM stdin;
\.


--
-- Data for Name: config; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY config (keyconfig, config, value) FROM stdin;
1	compOutputDrives	T:
2	wipOutputDrives	G:
3	renderOutputDrives	S:
6	assburnerFtpEnabled	false
7	assburnerDownloadMethods	nfs
9	assburnerPrioSort	100
10	assburnerErrorSort	10
20	assburnerForkJobVerify	false
11	assburnerSubmissionSort	1
12	assburnerHostsSort	5
17	jabberSystemResource	Autobot
18	assburnerAssignRate	15000
21	assburnerAutoPacketTarget	420
22	assburnerAutoPacketDefault	150
4	managerDriveLetter	/drd/jobs
5	emailDomain	@drdstudios.com
14	jabberDomain	im.drd.dmz
15	jabberSystemUser	farm
16	jabberSystemPassword	autologin
23	emailServer	smtp.drd.roam
24	assburnerLogRootDir	/farm/logs/
26	attachmentPathUnix	/farm/logs/.attachments/
27	attachmentPathWin	T:/farm/logs/.attachments/
25	emailSender	farm@drdstudios.com
30	assburnerErrorStep	7
8	assburnerTotalFailureErrorThreshold	9
19	assburnerHostErrorLimit	1
29	assburnerPulsePeriod	600
28	assburnerLoopTime	3000
33	arsenalMaxAutoAdapts	1
13	arsenalSortMethod	key_darwin
32	arsenalAssignMaxHosts	0.4
31	arsenalAssignSloppiness	20.0
\.


--
-- Data for Name: countercache; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY countercache (hoststotal, hostsactive, hostsready, jobstotal, jobsactive, jobsdone, lastupdated, slotstotal, slotsactive, jobswaiting) FROM stdin;
\.


--
-- Data for Name: darwinweight; Type: TABLE DATA; Schema: public; Owner: farmers
--

COPY darwinweight (keydarwinscore, shotname, projectname, weight) FROM stdin;
\.


--
-- Data for Name: delivery; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY delivery (keyelement, daysbid, description, fkeyelement, fkeyelementstatus, fkeyelementtype, fkeyproject, fkeythumbnail, name, daysscheduled, daysestimated, status, filepath, fkeyassettype, fkeypathtemplate, fkeystatusset, allowtime, datestart, datecomplete, fkeyassettemplate, icon, arsenalslotlimit, arsenalslotreserve) FROM stdin;
\.


--
-- Data for Name: deliveryelement; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY deliveryelement (keydeliveryshot, fkeydelivery, fkeyelement, framestart, frameend) FROM stdin;
\.


--
-- Data for Name: demoreel; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY demoreel (keydemoreel, demoreel, datesent, projectlist, contactinfo, notes, playlist, shippingtype) FROM stdin;
\.


--
-- Data for Name: diskimage; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY diskimage (keydiskimage, diskimage, path, created) FROM stdin;
\.


--
-- Data for Name: dynamichostgroup; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY dynamichostgroup (keyhostgroup, hostgroup, fkeyusr, private, keydynamichostgroup, hostwhereclause) FROM stdin;
\.


--
-- Data for Name: element; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY element (keyelement, daysbid, description, fkeyelement, fkeyelementstatus, fkeyelementtype, fkeyproject, fkeythumbnail, name, daysscheduled, daysestimated, status, filepath, fkeyassettype, fkeypathtemplate, fkeystatusset, allowtime, datestart, datecomplete, fkeyassettemplate, icon, arsenalslotlimit, arsenalslotreserve) FROM stdin;
\.


--
-- Data for Name: elementdep; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY elementdep (keyelementdep, fkeyelement, fkeyelementdep, relationtype) FROM stdin;
\.


--
-- Data for Name: elementstatus; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY elementstatus (keyelementstatus, name, color, fkeystatusset, "order") FROM stdin;
1	New	#ffffff	\N	1
2	Active	#00ff55	\N	3
3	Poss. Shot	#ff0000	\N	4
4	Omit	#00ff00	\N	5
5	On Hold	\N	\N	2
7	cbb-sv	\N	\N	6
8	cbb-efx	\N	\N	7
6	Final	\N	\N	8
\.


--
-- Data for Name: elementthread; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY elementthread (keyelementthread, datetime, elementthread, fkeyelement, fkeyusr, skeyreply, topic, todostatus, hasattachments, fkeyjob) FROM stdin;
\.


--
-- Data for Name: elementtype; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY elementtype (keyelementtype, elementtype, sortprefix) FROM stdin;
2	Asset	\N
4	Task	\N
1	Project	\N
3	Shot	\N
6	AssetGroup	\N
7	User	\N
5	ShotGroup	\N
8	Delivery	\N
\.


--
-- Data for Name: elementtypetasktype; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY elementtypetasktype (keyelementtypetasktype, fkeyelementtype, fkeytasktype, fkeyassettype) FROM stdin;
\.


--
-- Data for Name: elementuser; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY elementuser (keyelementuser, fkeyelement, fkeyuser, active, fkeyassettype) FROM stdin;
\.


--
-- Data for Name: employee; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY employee (keyelement, daysbid, description, fkeyelement, fkeyelementstatus, fkeyelementtype, fkeyproject, fkeythumbnail, name, daysscheduled, daysestimated, status, filepath, fkeyassettype, fkeypathtemplate, fkeystatusset, allowtime, datestart, datecomplete, fkeyassettemplate, icon, arsenalslotlimit, arsenalslotreserve, dateoflastlogon, email, fkeyhost, gpgkey, jid, pager, password, remoteips, schedule, shell, uid, threadnotifybyjabber, threadnotifybyemail, fkeyclient, intranet, homedir, disabled, gid, usr, keyusr, rolemask, usrlevel, remoteok, requestcount, sessiontimeout, logoncount, useradded, oldkeyusr, sid, lastlogontype, namefirst, namelast, dateofhire, dateoftermination, dateofbirth, logon, lockedout, bebackat, comment, userlevel, nopostdays, initials, missingtimesheetcount, namemiddle) FROM stdin;
\.


--
-- Data for Name: eventalert; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY eventalert ("keyEventAlert", "fkeyHost", graphds, "sampleType", "samplePeriod", severity, "sampleDirection", varname, "sampleValue") FROM stdin;
\.


--
-- Data for Name: filetemplate; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY filetemplate (keyfiletemplate, fkeyelementtype, fkeyproject, fkeytasktype, name, sourcefile, templatefilename, trackertable) FROM stdin;
\.


--
-- Data for Name: filetracker; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY filetracker (keyfiletracker, fkeyelement, name, path, filename, fkeypathtemplate, fkeyprojectstorage, storagename) FROM stdin;
\.


--
-- Data for Name: filetrackerdep; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY filetrackerdep (keyfiletrackerdep, fkeyinput, fkeyoutput) FROM stdin;
\.


--
-- Data for Name: fileversion; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY fileversion (keyfileversion, version, iteration, path, oldfilenames, filename, filenametemplate, automaster, fkeyelement, fkeyfileversion) FROM stdin;
\.


--
-- Data for Name: folder; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY folder (keyfolder, folder, mount, tablename, fkey, online, alias, host, link) FROM stdin;
\.


--
-- Data for Name: graph; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY graph (keygraph, height, width, vlabel, period, fkeygraphpage, upperlimit, lowerlimit, stack, graphmax, sortorder, graph) FROM stdin;
\.


--
-- Data for Name: graphds; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY graphds (keygraphds, varname, dstype, fkeyhost, cdef, graphds, fieldname, filename, negative) FROM stdin;
\.


--
-- Data for Name: graphpage; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY graphpage (keygraphpage, fkeygraphpage, name) FROM stdin;
\.


--
-- Data for Name: graphrelationship; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY graphrelationship (keygraphrelationship, fkeygraphds, fkeygraph, negative) FROM stdin;
\.


--
-- Data for Name: gridtemplate; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY gridtemplate (keygridtemplate, fkeyproject, gridtemplate) FROM stdin;
\.


--
-- Data for Name: gridtemplateitem; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY gridtemplateitem (keygridtemplateitem, fkeygridtemplate, fkeytasktype, checklistitems, columntype, headername, "position") FROM stdin;
\.


--
-- Data for Name: groupmapping; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY groupmapping (keygroupmapping, fkeygrp, fkeymapping) FROM stdin;
\.


--
-- Data for Name: grp; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY grp (keygrp, grp, alias) FROM stdin;
1	Admin	\N
2	2D	\N
3	RenderOps	\N
4	RenderOps-Notify	\N
\.


--
-- Data for Name: gruntscript; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY gruntscript (keygruntscript, runcount, lastrun, scriptname) FROM stdin;
\.


--
-- Data for Name: history; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY history (keyhistory, date, fkeyelement, fkeyusr, history) FROM stdin;
\.


--
-- Data for Name: host; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY host (keyhost, backupbytes, cpus, description, diskusage, fkeyjob, host, manufacturer, model, os, rendertime, slavepluginlist, sn, version, renderrate, dutycycle, memory, mhtz, online, uid, slavepacketweight, framecount, viruscount, virustimestamp, errortempo, fkeyhost_backup, oldkey, abversion, laststatuschange, loadavg, allowmapping, allowsleep, fkeyjobtask, wakeonlan, architecture, loc_x, loc_y, loc_z, ostext, bootaction, fkeydiskimage, syncname, fkeylocation, cpuname, osversion, slavepulse, puppetpulse, maxassignments, fkeyuser, maxmemory, userisloggedin) FROM stdin;
\.


--
-- Data for Name: hostdailystat; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY hostdailystat (keyhostdailystat, fkeyhost, readytime, assignedtime, copytime, loadtime, busytime, offlinetime, date, tasksdone, loaderrors, taskerrors, loaderrortime, busyerrortime) FROM stdin;
\.


--
-- Data for Name: hostgroup; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY hostgroup (keyhostgroup, hostgroup, fkeyusr, private) FROM stdin;
\.


--
-- Data for Name: hostgroupitem; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY hostgroupitem (keyhostgroupitem, fkeyhostgroup, fkeyhost) FROM stdin;
\.


--
-- Data for Name: hosthistory; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY hosthistory (keyhosthistory, fkeyhost, fkeyjob, fkeyjobstat, status, laststatus, datetime, duration, fkeyjobtask, fkeyjobtype, nextstatus, success, fkeyjoberror, change_from_ip, fkeyjobcommandhistory) FROM stdin;
\.


--
-- Data for Name: hostinterface; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY hostinterface (keyhostinterface, fkeyhost, mac, ip, fkeyhostinterfacetype, switchport, fkeyswitch, inst) FROM stdin;
\.


--
-- Data for Name: hostinterfacetype; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY hostinterfacetype (keyhostinterfacetype, hostinterfacetype) FROM stdin;
1	Primary
2	Alias
3	CNAME
\.


--
-- Data for Name: hostload; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY hostload (keyhostload, fkeyhost, loadavg, loadavgadjust, loadavgadjusttimestamp) FROM stdin;
\.


--
-- Data for Name: hostmapping; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY hostmapping (keyhostmapping, fkeyhost, fkeymapping) FROM stdin;
\.


--
-- Data for Name: hostport; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY hostport (keyhostport, fkeyhost, port, monitor, monitorstatus) FROM stdin;
\.


--
-- Data for Name: hostresource; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY hostresource (keyhostresource, fkeyhost, hostresource) FROM stdin;
\.


--
-- Data for Name: hosts_active; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY hosts_active (count) FROM stdin;
\.


--
-- Data for Name: hosts_ready; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY hosts_ready (count) FROM stdin;
\.


--
-- Data for Name: hosts_total; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY hosts_total (count) FROM stdin;
\.


--
-- Data for Name: hostservice; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY hostservice (keyhostservice, fkeyhost, fkeyservice, hostserviceinfo, hostservice, fkeysyslog, enabled, pulse, remotelogport) FROM stdin;
\.


--
-- Data for Name: hostsoftware; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY hostsoftware (keyhostsoftware, fkeyhost, fkeysoftware) FROM stdin;
\.


--
-- Data for Name: hoststatus; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY hoststatus (keyhoststatus, fkeyhost, slavestatus, laststatuschange, slavepulse, fkeyjobtask, online, activeassignmentcount, availablememory, lastassignmentchange) FROM stdin;
\.


--
-- Data for Name: job; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY job (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags) FROM stdin;
\.


--
-- Data for Name: job3delight; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY job3delight (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, width, height, framestart, frameend, threads, processes, jobscript, jobscriptparam, renderdlcmd) FROM stdin;
\.


--
-- Data for Name: jobassignment; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobassignment (keyjobassignment, fkeyjob, fkeyjobassignmentstatus, fkeyhost, stdout, stderr, command, maxmemory, started, ended, fkeyjoberror, realtime, usertime, systime, iowait, bytesread, byteswrite, efficiency, opsread, opswrite, assignslots, assignmaxmemory, assignminmemory) FROM stdin;
\.


--
-- Data for Name: jobassignment_old; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobassignment_old (keyjobassignment, fkeyjob, fkeyjobassignmentstatus, fkeyhost, stdout, stderr, command, maxmemory, started, ended, fkeyjoberror, realtime, usertime, systime, iowait, bytesread, byteswrite, efficiency, opsread, opswrite) FROM stdin;
\.


--
-- Data for Name: jobassignmentstatus; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobassignmentstatus (keyjobassignmentstatus, status) FROM stdin;
1	ready
2	copy
3	busy
4	done
5	error
6	cancelled
\.


--
-- Data for Name: jobbatch; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobbatch (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, cmd, restartafterfinish, restartaftershutdown, passslaveframesasparam, disablewow64fsredirect) FROM stdin;
\.


--
-- Data for Name: jobcannedbatch; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobcannedbatch (keyjobcannedbatch, name, "group", cmd, disablewow64fsredirect) FROM stdin;
\.


--
-- Data for Name: jobcommandhistory; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobcommandhistory (keyjobcommandhistory, stderr, stdout, command, memory, fkeyjob, fkeyhost, fkeyhosthistory, iowait, realtime, systime, usertime) FROM stdin;
\.


--
-- Data for Name: jobdep; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobdep (keyjobdep, fkeyjob, fkeydep, deptype) FROM stdin;
\.


--
-- Data for Name: jobenvironment; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobenvironment (keyjobenvironment, environment) FROM stdin;
\.


--
-- Data for Name: joberror; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY joberror (keyjoberror, fkeyhost, fkeyjob, frames, message, errortime, count, cleared, lastoccurrence, timeout) FROM stdin;
\.


--
-- Data for Name: joberrorhandler; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY joberrorhandler (keyjoberrorhandler, fkeyjobtype, errorregex, fkeyjoberrorhandlerscript) FROM stdin;
\.


--
-- Data for Name: joberrorhandlerscript; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY joberrorhandlerscript (keyjoberrorhandlerscript, script) FROM stdin;
\.


--
-- Data for Name: jobfiltermessage; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobfiltermessage (keyjobfiltermessage, fkeyjobfiltertype, fkeyjobfilterset, regex, comment, enabled) FROM stdin;
9	2	3	^Error:	\N	t
26	5	1	Internal 3Delight Message code: 45 -> "Can't load requested shader", message severity: 2	 	t
122	2	1	Got Error: dspy_tiff: cannot open output file '/tmp/null' (system\nerror: No such file or directory)	\N	t
14	2	4	^Unable to access file	\N	t
15	2	4	CAUGHT SIGNAL SIGABRT	\N	t
8	3	1	3DL ERROR R5030:	\N	t
101	3	6	bash: .* Segmentation fault	\N	t
127	2	6	ImportError:	\N	f
104	2	6	AttributeError: 'NoneType' object has no attribute 'acquire'	\N	t
13	2	4	^Error:	\N	f
131	1	2	OSError: [Errno 13] Permission denied	\N	t
126	3	6	AttributeError: .* '[^(set|acquire)]'	\N	f
2	2	1	3DL SEVERE ERROR L2033	\N	t
128	3	2	Shuffle_Input_Channels.in: .* is not a layer or channel name	\N	t
181	5	1	CrowdProcedural::doRender median split	\N	t
123	3	6	ERROR: unrecognized arguments:	\N	f
5	2	1	^3DL INFO L2374: no license available	\N	t
121	3	6	bash: .* command not found	\N	f
6	2	1	Could not find file:	\N	t
102	3	6	tank.common.errors	\N	f
3	3	1	Command exited with non-zero status (?!255)\\d+	\N	t
129	3	2	Write failed .* Read-only file system.	\N	t
125	5	2	ERROR: .* Missing input channel	\N	t
4	4	6	^renderdl: cannot open input file	\N	t
135	3	6	DRD_ERROR: A rib file cache	\N	f
171	2	6	created required directory -> **** ERROR: path does not exist	\N	t
136	2	6	socket.error: (60, 'Operation timed out')	\N	t
137	4	2	Interprocess lock failed: /Local/nuke_temp/tilecache/DiskSize.lock	\N	t
21	2	1	Transport endpoint is not connected:	\N	t
175	2	1	mesh internal path .* is invalid	\N	t
176	3	6	Fatal Error. Attempting to save in	\N	t
138	5	1	access attribute user:primType	\N	t
22	1	1	3DL ERROR T2373: cannot read 3D texture file	\N	t
63	1	6	3DL ERROR T2087	\N	t
1	1	1	cannot open output file	\N	t
28	2	1	message contents: T2040	\N	t
173	3	1	ReferenceManager::getAnimDb: load failed	\N	f
172	3	6	RuntimeError: ${VERSION} or ${FROZEN_VERSION} are uninitialized 	\N	f
130	1	1	.rat.zfile	\N	f
103	2	6	Please check for a valid license server host	\N	t
105	2	6	psycopg2.ProgrammingError	\N	t
106	3	6	IOError:	\N	t
107	4	6	No space left on device	\N	t
108	2	6	AssertionError:	\N	t
30	1	1	TextureLoader: file doesn't exist /drd/jobs/hf2/tank/	\N	t
18	2	4	CAUGHT SIGNAL SIGSEGV	\N	t
19	3	2	No NI file specified. Stop teasing Naiad	\N	t
20	2	2	Error checking out license: LM-X Error	\N	t
25	2	2	Command exited with non-zero status	\N	t
27	2	3	Command exited with non-zero status	\N	t
31	1	3	R2086: incomplete (or invalid) parameters set for subsurface scattering	\N	t
55	2	3	FLEXnet Licensing error	\N	t
35	2	1	DRD_ERROR_RETRY	\N	t
53	3	1	code: 13 -> "Arbitrary program limit"	\N	f
54	2	6	Hbatch job wouldn't run due to 'No License Found' error	\N	t
56	1	1	T2028: netcache: lock file has disappeared	\N	t
174	2	1	3DL INFO L2034: all licenses are taken, waiting for a free license	\N	f
62	2	3	'No License Found'	\N	t
16	3	4	dammit	\N	f
17	3	4	exist	\N	f
11	2	2	FOUNDRY LICENSE ERROR	\N	t
10	1	1	Read-only file system	\N	t
57	1	1	error message: R5022	\N	t
59	1	1	message contents: P1165	\N	f
60	1	1	message contents: P1111	\N	t
178	2	1	renderdl: cannot open input file	\N	t
177	2	1	system error: No such file or directory	\N	t
58	3	1	Cannot find method "get_pointcloudname"	\N	t
134	1	2	CrowdProcedural.ERROR: NO LODs DEFINED	\N	t
64	3	6	Batch script exited with result: 1	\N	t
99	3	6	Expression recursion too deep	\N	t
100	3	6	Error rendering child	\N	t
179	2	1	3DL ERROR P1051	\N	f
124	5	6	IndexError: list index out of range	\N	f
7	3	1	Received signal	\N	t
23	3	1	R5030: error reading from point cloud file	\N	t
61	5	1	input in flex scanner failed	\N	t
180	3	6	,False	\N	f
40	1	1	3DL ERROR: .* Invalid char:	\N	t
96	3	1	Received signal 11	\N	t
38	2	1	DRD ERROR: couldn't find file .*\\.exr.*	\N	t
90	1	1	Exception occurred in python burner: Please contact IT	\N	t
32	3	1	"Internal 3Delight Message code: 24 -> "Bad begin-end nesting", message severity: 2	\N	t
33	2	1	^DRD ERROR: EXR is not complete	\N	t
49	5	1	No space left on device.	\N	f
50	5	6	Fatal error: Segmentation fault	\N	f
34	2	6	^DRD_ERROR: A rib file	\N	t
36	1	1	DRD_ERROR_FATAL	\N	t
117	3	6	Cook error in input:	\N	t
51	3	3	Command exited with non-zero status 1	\N	t
79	3	4	AttributeError: 'NoneType' object has no attribute 'set'	\N	t
88	2	3	No valid license available!	\N	t
89	3	3	Foundry::Cache::CacheDiskSizeLockFailed	\N	t
92	2	3	attempting to kill process	\N	t
43	3	5	NAIAD ERROR	\N	t
52	5	1	netcache: could not copy file from	\N	f
37	1	1	DRD_RIB_FATAL	\N	t
39	2	1	^DRD ERROR: post_renderdl_frame command failed on file	\N	t
41	3	1	imdisplay-bin: Unable to open the mplay application	\N	t
44	3	1	ERROR: File is empty	\N	t
45	3	1	!!! CAUGHT SIGNAL	\N	t
46	3	6	output: 'error: Cannot create folder	\N	t
47	3	6	ERROR: Stopping on error	\N	t
48	1	1	guide verts don't seem to match mesh	\N	t
67	3	1	Transport endpoint is not connected	\N	t
68	3	1	'drd::VacuumError'	\N	t
69	3	1	'drd::VacuumHDF5Error'	\N	t
71	2	1	message contents: R5031	\N	t
72	3	1	message contents: T2087	\N	t
73	4	1	No space left on device	\N	t
74	2	6	post_renderdl_frame command	\N	t
75	3	1	message contents: P1124	\N	f
76	2	1	message contents: A2066	\N	f
77	3	1	message contents: A2057	\N	t
78	3	1	CrowdCharacterProcedural.ERROR:	\N	t
80	2	1	DRD_ERROR: bad block	\N	t
83	3	1	S2069: the interface of shader	\N	t
84	3	1	S2050: cannot find shader	\N	t
85	3	1	Cannot read image file .* File is not an image file	\N	t
86	3	1	Cannot access the OTL source	\N	t
87	3	6	NameError: name 'left' is not defined	\N	t
91	4	1	No space left on device	\N	t
109	3	6	TypeError:	\N	f
110	3	6	Unable to initialize rendering module	\N	t
112	2	6	DtexOpenFile error	\N	t
115	3	6	maya_pipe.api.maya_roles.MayaRoleError:	\N	t
111	3	3	IOError:	\N	t
114	3	6	Read error: No such file or directory	\N	f
98	2	1	3DL ERROR S2050	\N	f
120	3	6	yaml.parser.ParserError:	\N	t
119	3	6	meme.core.exceptions	\N	t
93	1	6	Error: maximum recursion depth exceeded	\N	t
118	3	6	Asked for too-large .* input image	\N	t
113	3	6	RuntimeError	\N	t
94	5	1	TESTING: VTimeVariable::getIndex	\N	t
95	2	1	LZWDecode : Not enough data at scanline	\N	t
97	2	6	Batch script exited with result: 3	\N	t
116	3	6	Invalid pickle location value. This argument must be used to store a file	\N	t
\.


--
-- Data for Name: jobfilterset; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobfilterset (keyjobfilterset, name) FROM stdin;
1	3Delight
3	Mantra100
4	filterTest
5	Naiad
2	Nuke
6	Batch
\.


--
-- Data for Name: jobfiltertype; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobfiltertype (keyjobfiltertype, name) FROM stdin;
1	Error-TaskCancel
2	Error-TaskRetry
3	Error-JobSuspend
4	Error-HostOffline
5	Ignore
\.


--
-- Data for Name: jobhistory; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobhistory (keyjobhistory, fkeyjobhistorytype, fkeyjob, fkeyhost, fkeyuser, message, created) FROM stdin;
\.


--
-- Data for Name: jobhistorytype; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobhistorytype (keyjobhistorytype, type) FROM stdin;
\.


--
-- Data for Name: jobmantra100; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobmantra100 (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, keyjobmantra100, forceraytrace, geocachesize, height, qualityflag, renderquality, threads, width) FROM stdin;
\.


--
-- Data for Name: jobmantra95; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobmantra95 (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, keyjobmantra95, forceraytrace, geocachesize, height, qualityflag, renderquality, threads, width) FROM stdin;
\.


--
-- Data for Name: jobmax; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY jobmax (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, camera, elementfile, exrattributes, exrchannels, exrcompression, exrsavebitdepth, exrsaveregion, exrsavescanline, exrtilesize, exrversion, fileoriginal, flag_h, flag_v, flag_w, flag_x2, flag_xa, flag_xc, flag_xd, flag_xe, flag_xf, flag_xh, flag_xk, flag_xn, flag_xo, flag_xp, flag_xv, frameend, framelist, framestart, plugininipath, startupscript) FROM stdin;
\.


--
-- Data for Name: jobmax10; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobmax10 (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, camera, elementfile, exrattributes, exrchannels, exrcompression, exrsavebitdepth, exrsaveregion, exrsavescanline, exrtilesize, exrversion, fileoriginal, flag_h, flag_v, flag_w, flag_x2, flag_xa, flag_xc, flag_xd, flag_xe, flag_xf, flag_xh, flag_xk, flag_xn, flag_xo, flag_xp, flag_xv, frameend, framelist, framestart, plugininipath, startupscript) FROM stdin;
\.


--
-- Data for Name: jobmax2009; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobmax2009 (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, camera, elementfile, exrattributes, exrchannels, exrcompression, exrsavebitdepth, exrsaveregion, exrsavescanline, exrtilesize, exrversion, fileoriginal, flag_h, flag_v, flag_w, flag_x2, flag_xa, flag_xc, flag_xd, flag_xe, flag_xf, flag_xh, flag_xk, flag_xn, flag_xo, flag_xp, flag_xv, frameend, framelist, framestart, plugininipath, startupscript) FROM stdin;
\.


--
-- Data for Name: jobmax2010; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobmax2010 (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, camera, elementfile, exrattributes, exrchannels, exrcompression, exrsavebitdepth, exrsaveregion, exrsavescanline, exrtilesize, exrversion, fileoriginal, flag_h, flag_v, flag_w, flag_x2, flag_xa, flag_xc, flag_xd, flag_xe, flag_xf, flag_xh, flag_xk, flag_xn, flag_xo, flag_xp, flag_xv, frameend, framelist, framestart, plugininipath, startupscript) FROM stdin;
\.


--
-- Data for Name: jobmaxscript; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobmaxscript (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, script, maxtime, outputfiles, silent, maxversion, runmax64, runpythonscript, use3dsmaxcmd) FROM stdin;
\.


--
-- Data for Name: jobmaya; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobmaya (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, framestart, frameend, camera, renderer, projectpath, width, height, append) FROM stdin;
\.


--
-- Data for Name: jobmaya2008; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobmaya2008 (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, framestart, frameend, camera, renderer, projectpath, width, height, append) FROM stdin;
\.


--
-- Data for Name: jobmaya2009; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobmaya2009 (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, framestart, frameend, camera, renderer, projectpath, width, height, append) FROM stdin;
\.


--
-- Data for Name: jobmaya2011; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobmaya2011 (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, framestart, frameend, camera, renderer, projectpath, width, height, append) FROM stdin;
\.


--
-- Data for Name: jobmaya7; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobmaya7 (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, framestart, frameend, camera, renderer, projectpath, width, height, append) FROM stdin;
\.


--
-- Data for Name: jobmaya8; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobmaya8 (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, framestart, frameend, camera, renderer, projectpath, width, height, append) FROM stdin;
\.


--
-- Data for Name: jobmaya85; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobmaya85 (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, framestart, frameend, camera, renderer, projectpath, width, height, append) FROM stdin;
\.


--
-- Data for Name: jobmentalray2009; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobmentalray2009 (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, framestart, frameend, camera, renderer, projectpath, width, height, append) FROM stdin;
\.


--
-- Data for Name: jobmentalray2011; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobmentalray2011 (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, framestart, frameend, camera, renderer, projectpath, width, height, append) FROM stdin;
\.


--
-- Data for Name: jobmentalray7; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobmentalray7 (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, framestart, frameend, camera, renderer, projectpath, width, height, append) FROM stdin;
\.


--
-- Data for Name: jobmentalray8; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobmentalray8 (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, framestart, frameend, camera, renderer, projectpath, width, height, append) FROM stdin;
\.


--
-- Data for Name: jobmentalray85; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobmentalray85 (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, framestart, frameend, camera, renderer, projectpath, width, height, append) FROM stdin;
\.


--
-- Data for Name: jobnaiad; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobnaiad (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, framestart, frameend, threads, append, restartcache, fullframe, forcerestart) FROM stdin;
\.


--
-- Data for Name: jobnuke; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobnuke (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, framestart, frameend, outputcount, viewname, nodes, terminalonly) FROM stdin;
\.


--
-- Data for Name: jobnuke51; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobnuke51 (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, framestart, frameend, outputcount) FROM stdin;
\.


--
-- Data for Name: jobnuke52; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobnuke52 (keyjob, fkeyelement, fkeyhost, fkeyjobtype, fkeyproject, fkeyusr, hostlist, job, jobtime, outputpath, status, submitted, started, ended, expires, deleteoncomplete, hostsonjob, taskscount, tasksunassigned, tasksdone, tasksaveragetime, priority, errorcount, queueorder, packettype, packetsize, queueeta, notifyonerror, notifyoncomplete, maxtasktime, cleaned, filesize, btinfohash, rendertime, abversion, deplist, args, filename, filemd5sum, fkeyjobstat, username, domain, password, stats, currentmapserverweight, loadtimeaverage, tasksassigned, tasksbusy, prioritizeoutertasks, outertasksassigned, lastnotifiederrorcount, taskscancelled, taskssuspended, health, maxloadtime, license, maxmemory, fkeyjobparent, endedts, startedts, submittedts, maxhosts, personalpriority, loggingenabled, environment, runassubmitter, checkfilemd5, uploadedfile, framenth, framenthmode, exclusiveassignment, hastaskprogress, minmemory, scenename, shotname, slots, fkeyjobfilterset, maxerrors, notifycompletemessage, notifyerrormessage, fkeywrangler, maxquiettime, autoadaptslots, fkeyjobenvironment, suspendedts, toggleflags, framestart, frameend, outputcount) FROM stdin;
\.


--
-- Data for Name: joboutput; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY joboutput (keyjoboutput, fkeyjob, name, fkeyfiletracker) FROM stdin;
\.


--
-- Data for Name: jobservice; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobservice (keyjobservice, fkeyjob, fkeyservice) FROM stdin;
\.


--
-- Data for Name: jobstat; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobstat (keyjobstat, fkeyelement, fkeyproject, fkeyusr, pass, taskcount, taskscompleted, tasktime, started, ended, name, errorcount, mintasktime, maxtasktime, avgtasktime, totaltasktime, minerrortime, maxerrortime, avgerrortime, totalerrortime, mincopytime, maxcopytime, avgcopytime, totalcopytime, copycount, minloadtime, maxloadtime, avgloadtime, totalloadtime, loadcount, submitted, minmemory, maxmemory, avgmemory, minefficiency, maxefficiency, avgefficiency, totalbytesread, minbytesread, maxbytesread, avgbytesread, totalopsread, minopsread, maxopsread, avgopsread, totalbyteswrite, minbyteswrite, maxbyteswrite, avgbyteswrite, totalopswrite, minopswrite, maxopswrite, avgopswrite, totaliowait, miniowait, maxiowait, avgiowait, avgcputime, maxcputime, mincputime, totalcanceltime, totalcputime, fkeyjobtype, fkeyjob) FROM stdin;
\.


--
-- Data for Name: jobstateaction; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobstateaction (keyjobstateaction, fkeyjob, oldstatus, newstatus, modified) FROM stdin;
\.


--
-- Data for Name: jobstatus; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobstatus (keyjobstatus, hostsonjob, fkeyjob, tasksunassigned, taskscount, tasksdone, taskscancelled, taskssuspended, tasksassigned, tasksbusy, tasksaveragetime, health, joblastupdated, errorcount, lastnotifiederrorcount, averagememory, bytesread, byteswrite, cputime, efficiency, opsread, opswrite, totaltime, queueorder, taskbitmap, averagedonetime, fkeyjobstatusskipreason) FROM stdin;
\.


--
-- Data for Name: jobstatus_old; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobstatus_old (keyjobstatus, hostsonjob, fkeyjob, tasksunassigned, taskscount, tasksdone, taskscancelled, taskssuspended, tasksassigned, tasksbusy, tasksaveragetime, health, joblastupdated, errorcount, lastnotifiederrorcount, averagememory, bytesread, byteswrite, cputime, efficiency, opsread, opswrite, totaltime) FROM stdin;
\.


--
-- Data for Name: jobstatusskipreason; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobstatusskipreason (keyjobstatusskipreason, name) FROM stdin;
\.


--
-- Data for Name: jobtask; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobtask (keyjobtask, fkeyhost, fkeyjob, status, jobtask, label, fkeyjoboutput, progress, fkeyjobtaskassignment, schedulepolicy) FROM stdin;
\.


--
-- Data for Name: jobtaskassignment; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobtaskassignment (keyjobtaskassignment, fkeyjobassignment, memory, started, ended, fkeyjobassignmentstatus, fkeyjobtask, fkeyjoberror) FROM stdin;
\.


--
-- Data for Name: jobtaskassignment_old; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobtaskassignment_old (keyjobtaskassignment, fkeyjobassignment, memory, started, ended, fkeyjobassignmentstatus, fkeyjobtask, fkeyjoberror) FROM stdin;
\.


--
-- Data for Name: jobtype; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobtype (keyjobtype, jobtype, fkeyservice, icon) FROM stdin;
14	Maya7	3	\N
15	Shake	4	\N
13	Maya8	3	\N
21	MentalRay7	9	\N
20	Maya85	3	\N
23	MentalRay85	9	\N
22	MentalRay8	9	\N
16	Batch	10	\N
28	Maya2009	3	\N
29	Maya2008	3	\N
30	Nuke51	32	\N
31	Mantra95	33	\N
32	Mantra100	33	\N
35	Nuke52	32	\N
36	MentalRay2009	9	\N
37	MentalRay2011	9	\N
38	Maya2011	3	\N
39	Naiad	46	\N
40	Nuke	32	\N
33	3Delight	36	\\x89504e470d0a1a0a0000000d49484452000000300000003008060000005702f987000000097048597300000b1300000b1301009a9c180000064f494441546881ed994b6c1cc51686bfaa7eccc3d3f68cc789337e5e27846089c7864584c50209242c58054456b0210b1e6205c2bc8222010b944848b0892205c412b16187c402090852042c500408ac9b183b8909ce5512dbf1f4744f77d55d743c763f26378278465ce5972c79aaab4f9fbfeb9c537f9d162740f30f86ecb6037f17b708741b6d09e4464729ecde0d42644f1082d2bdf75ed77861cf1e0cc7a1776a0a6b70b0ed3c61dbf4dc7d77e6357b7818bb566b7b6f9a80948cbcf20aff3a7c98e19919761d3d8acce7d30f9592d18307af4b60f0c00172e3e3385353e42726dace337b7b197ee9a5cc6b95871fc6d9bbf7c609988e830e02669f7c927f1f3880f67d0a939300188e43657a9acaf434b2548aad4e7e628281c71fc7d9bb1721e366fff3e9a7d47ffe79c3c6238f507ef0418cde5e0a77dc114d12027b6888ea638f51bcf3cec8976a95c2e424c5bbeec22c976f8c40b0bcccf9234790c52203fbf7939f98c0fbfd77ec5a8d5d478f6256ab188ec3f83bef200b85d65b1a7df34d5410e0dc771f23070fc6480c3efd34f99d3bb16b356e3b7e1cb352c11a1c64e7071f30fac61b00e4c6c7d9f1dc7368cf63f8e597e99d9ac2281631fbfa302b158465651230334701b35ca6ef8107d04a216c9bda0b2ff0e7f1e32c7ff92500ab274fb2e7934f00187af145ce1f394278e50acd3ffe60c7f3cf53bce79e962d61182004c333339c3f7c98abdf7f0f8077f62cb5679f8de65816e7de7e1bd568d0bc7891caf4340b870eb17af224c1952b342f5ebc3102c23010a689373fcfe9679e61fb534f31b07f3ff6f030ee6fbfb5e6f98b8b842b2b00588383e4c6c6607c1c80951327089797530fcbefda45fdd4a9d6efc6e9d3a86673e3ff460380d07511197997855408156ebf9d89f7df4798113761db84cbcb5cfdee3bfa1f7d14ae8546effdf763f6f703b0f6e38fac7cfd357f1e3bc6d2471f212d0bffc285d4c32e7ffe39d5279e8872474aca0f3db45120f42641a013e240a9b604522be0cecee2cdcd31f9d967a84603ffc205e667664008460f1de2b663c750ae8bf23c9a4b4b009c7ff75dc6de7a0b6f6e8edcd81897bff802e5ba9123eb7fc0d2871f32f2eaabecfef86394ebe2cdcfa3eaf5c8e7cd4e6abde174924c02a29d16324a258465115cbe1c675c2ea3c390707535758f5dab115cba84f2bccc8755f7eda3fecb2fb8bffe0a4250ddb78fdcd8188befbd775d27ff1281ad803d32c2e8ebaf83d608c3406bcdfc6baf115cbaf4976d7694005c2b12b60d80f67d7418fe2d7b6dcbe856418721da756f9abdff5f31f74fc12d02ddc62d02ddc62d02dd86d05a6b777696fa4f3f21f3799a4b4b7867cf6e4cb8a687bc8505643e1f1d540c23127a8b8be82088f43ea09b4d1022da6d0d2312685a47bbadd6ac7cf3cd4d276036ce9c61f9abaf227d2225c2b2f0cf9d8b4dd2611809b04d509e178dfd0fb5b8d5906ba74eb59cd04a61f4f5a53a1146a9d43a07b46ecce590b95cc71c6d0769384e7c4429ccbebef898d699876aa354da42d76e0cd2dabe3d3610aead6125fa30caf749ce039049f25d80b4b66d8b751054a3916e4229d57e05da35be3a04292c0ba3b77763446b8c62b175266e4d2c1452ad0d611818c56227fc6c0ba91b8d547828dfc71c18888de966136bdbb69481540e751852f97e2a64c2ab5753fd48edfb58d56ada40b70968cfc3709cd6310fa2b76d66386b24ab1360f4f4a4c2ad93905a6b741060562ab10bc2b25aadc3d6986160f4f4a48c74b39c4a00dd686027f3a05ec7dab12336a67d1f33230f64b70928cf8b1279534954ae8b9d2410869979d0cd448e56200810b95cec3b800ec3a8bc262484512ab5c45bcb886d677e43e8045ade69dfcf2e939bf70888f689e49810c88cdce8045a0494eba6cbe9da5aaa9caa7644bb14461b047c1fabbf3f9e079e8795d8d050aad595de0cc37152e1d60948b1eeb0522065fc4d2a852c95d2b2229f8fed1b1095d864d9ed04249b1252795ebaf2349b6959110499d5c8ec4218c5d65cb96e5a17d5ebd9a4b2ca6932b93b801801edfb98e5725c5e7b5e14f39b65f3fa012721a5653edf715921939f4475b399da6d65a1804cc6bc65a56bbf94c80ecb6b690f0dc53626e5ba295911aeada565c5faee9d40ea38bac5105a6badea75fcc54554b389901299cfb3fac30fac57a8f5834ce3cc1908c38d764910d0989b434889f67d200a396f61212aa9eb1d8b6bdfc956befdf6a613f82f03d53be0591e81e40000000049454e44ae426082
\.


--
-- Data for Name: jobtypemapping; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY jobtypemapping (keyjobtypemapping, fkeyjobtype, fkeymapping) FROM stdin;
\.


--
-- Data for Name: license; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY license (keylicense, license, fkeyhost, fkeysoftware, total, reserved, inuse) FROM stdin;
18	vue	\N	\N	13	0	0
20	mentalray	\N	\N	112	8	0
24	rollingshutter	\N	\N	5	0	0
21	rvio	\N	\N	20	0	0
14	hbatch	\N	\N	162	0	113
28	fastlane	\N	\N	300	0	13
25	tank	\N	\N	100	0	4
29	truelight01_transfer	\N	\N	3	0	0
30	gpu	\N	\N	10	0	0
22	naiad	\N	\N	10	0	6
16	nuke_r	\N	\N	600	0	0
15	fah	\N	\N	8	0	0
33	needsTank	\N	\N	100	0	0
37	bl402_transfer	\N	\N	5	0	0
34	bulk_container_creation	\N	\N	1	0	0
35	bl801_transfer	\N	\N	8	0	0
36	bl201_transfer	\N	\N	8	0	0
31	bl401_transfer	\N	\N	5	0	0
32	tankpublish	\N	\N	50	0	0
17	3delight	\N	\N	1400	0	0
23	mantra	\N	\N	1200	0	13
19	ocula	\N	\N	20	0	0
\.


--
-- Data for Name: location; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY location (keylocation, name) FROM stdin;
\.


--
-- Data for Name: mapping; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY mapping (keymapping, fkeyhost, share, mount, fkeymappingtype, description) FROM stdin;
\.


--
-- Data for Name: mappingtype; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY mappingtype (keymappingtype, name) FROM stdin;
\.


--
-- Data for Name: methodperms; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY methodperms (keymethodperms, method, users, groups, fkeyproject) FROM stdin;
\.


--
-- Data for Name: notification; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY notification (keynotification, created, subject, message, component, event, routed, fkeyelement) FROM stdin;
\.


--
-- Data for Name: notificationdestination; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY notificationdestination (keynotificationdestination, fkeynotification, fkeynotificationmethod, delivered, destination, fkeyuser, routed) FROM stdin;
\.


--
-- Data for Name: notificationmethod; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY notificationmethod (keynotificationmethod, name) FROM stdin;
\.


--
-- Data for Name: notificationroute; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY notificationroute (keynotificationuserroute, eventmatch, componentmatch, fkeyuser, subjectmatch, messagematch, actions, priority, fkeyelement, routeassetdescendants) FROM stdin;
\.


--
-- Data for Name: notify; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY notify (keynotify, notify, fkeyusr, fkeysyslogrealm, severitymask, starttime, endtime, threshhold, notifyclass, notifymethod, fkeynotifymethod, threshold) FROM stdin;
\.


--
-- Data for Name: notifymethod; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY notifymethod (keynotifymethod, notifymethod) FROM stdin;
\.


--
-- Data for Name: notifysent; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY notifysent (keynotifysent, fkeynotify, fkeysyslog) FROM stdin;
\.


--
-- Data for Name: notifywho; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY notifywho (keynotifywho, class, fkeynotify, fkeyusr, fkey) FROM stdin;
\.


--
-- Data for Name: package; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY package (keypackage, version, fkeystatus) FROM stdin;
\.


--
-- Data for Name: packageoutput; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY packageoutput (keypackageoutput, fkeyasset) FROM stdin;
\.


--
-- Data for Name: pathsynctarget; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY pathsynctarget (keypathsynctarget, fkeypathtracker, fkeyprojectstorage) FROM stdin;
\.


--
-- Data for Name: pathtemplate; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY pathtemplate (keypathtemplate, name, pathtemplate, pathre, filenametemplate, filenamere, version, pythoncode) FROM stdin;
\.


--
-- Data for Name: pathtracker; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY pathtracker (keypathtracker, fkeyelement, path, fkeypathtemplate, fkeyprojectstorage, storagename) FROM stdin;
\.


--
-- Data for Name: permission; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY permission (keypermission, methodpattern, fkeyusr, permission, fkeygrp, class) FROM stdin;
1	\N	2	2777	\N	Blur::
2	\N	122	0777	2	Blur::
\.


--
-- Data for Name: phoneno; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY phoneno (keyphoneno, phoneno, fkeyphonetype, fkeyemployee, domain) FROM stdin;
\.


--
-- Data for Name: phonetype; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY phonetype (keyphonetype, phonetype) FROM stdin;
\.


--
-- Data for Name: project; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY project (keyelement, daysbid, description, fkeyelement, fkeyelementstatus, fkeyelementtype, fkeyproject, fkeythumbnail, name, daysscheduled, daysestimated, status, filepath, fkeyassettype, fkeypathtemplate, fkeystatusset, allowtime, datestart, datecomplete, fkeyassettemplate, icon, arsenalslotlimit, arsenalslotreserve, compoutputdrive, datedue, filetype, fkeyclient, notes, renderoutputdrive, script, shortname, wipdrive, projectnumber, frames, nda, dayrate, usefilecreation, dailydrive, lastscanned, fkeyprojectstatus, assburnerweight, project, fps, resolution, resolutionwidth, resolutionheight, archived, deliverymedium, renderpixelaspect) FROM stdin;
\.


--
-- Data for Name: projectresolution; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY projectresolution (keyprojectresolution, deliveryformat, fkeyproject, height, outputformat, projectresolution, width, pixelaspect, fps) FROM stdin;
\.


--
-- Data for Name: projectstatus; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY projectstatus (keyprojectstatus, projectstatus, chronology) FROM stdin;
1	New	10
2	Production	20
3	Post-Production	30
4	Completed	40
\.


--
-- Data for Name: projectstorage; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY projectstorage (keyprojectstorage, fkeyproject, name, location, storagename, "default", fkeyhost) FROM stdin;
\.


--
-- Data for Name: projecttempo; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY projecttempo (fkeyproject, tempo) FROM stdin;
\.


--
-- Data for Name: queueorder; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY queueorder (queueorder, fkeyjob) FROM stdin;
\.


--
-- Data for Name: rangefiletracker; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY rangefiletracker (keyfiletracker, fkeyelement, name, path, filename, fkeypathtemplate, fkeyprojectstorage, storagename, filenametemplate, framestart, frameend, fkeyresolution, renderelement) FROM stdin;
\.


--
-- Data for Name: renderframe; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY renderframe (keyrenderframe, fkeyshot, frame, fkeyresolution, status) FROM stdin;
\.


--
-- Data for Name: schedule; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY schedule (keyschedule, fkeyuser, date, starthour, hours, fkeyelement, fkeyassettype, fkeycreatedbyuser, duration, starttime) FROM stdin;
\.


--
-- Data for Name: serverfileaction; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY serverfileaction (keyserverfileaction, fkeyserverfileactionstatus, fkeyserverfileactiontype, fkeyhost, destpath, errormessage, sourcepath) FROM stdin;
\.


--
-- Data for Name: serverfileactionstatus; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY serverfileactionstatus (keyserverfileactionstatus, status, name) FROM stdin;
\.


--
-- Data for Name: serverfileactiontype; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY serverfileactiontype (keyserverfileactiontype, type) FROM stdin;
\.


--
-- Data for Name: service; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY service (keyservice, service, description, fkeylicense, enabled, forbiddenprocesses, active, "unique", fkeysoftware, fkeyjobfilterset) FROM stdin;
51	GPU_testing	\N	30	t	\N	\N	\N	\N	\N
3	Maya	\N	\N	t	maya.bin	\N	\N	\N	\N
48	DataOps	\N	\N	t	\N	\N	\N	\N	\N
16	AB_reclaim_tasks	\N	\N	t	\N	\N	\N	\N	\N
10	Batch	\N	\N	t	\N	\N	\N	\N	\N
2	AB_manager	\N	\N	t	\N	\N	\N	\N	\N
1	Assburner	\N	\N	t	\N	\N	\N	\N	\N
5	AB_Reaper	\N	\N	t	\N	t	\N	\N	\N
32	Nuke	\N	16	t	\N	\N	\N	\N	\N
34	hbatch	\N	14	t	\N	t	\N	\N	\N
38	mocap_bmo_vac	\N	\N	t	\N	\N	\N	\N	\N
39	prores	\N	\N	t	\N	t	\N	\N	\N
42	grind	\N	\N	t	\N	\N	\N	\N	\N
44	ocula	\N	19	t	\N	t	\N	\N	\N
45	rv	\N	21	t	\N	t	\N	\N	\N
47	terrorblock	\N	\N	t	\N	\N	\N	\N	\N
46	naiad	\N	22	t	\N	t	\N	\N	\N
33	Mantra	\N	23	t	\N	\N	\N	\N	\N
50	rollingshutter	\N	24	t	\N	\N	\N	\N	\N
53	fedex	\N	\N	t	\N	\N	\N	\N	\N
56	BatchOSX	\N	\N	t	\N	\N	\N	\N	\N
57	giant	\N	\N	t	\N	\N	\N	\N	\N
59	AB_Verifier	\N	\N	\N	\N	\N	\N	\N	\N
60	KimBatch	\N	\N	t	\N	t	\N	\N	\N
61	WIP-Writer	\N	\N	t	\N	t	\N	\N	\N
62	truelight01_transfer	\N	29	t	\N	t	\N	\N	\N
37	GPU	\N	30	t	\N	t	\N	\N	\N
9	MentalRay	\N	\N	t	\N	\N	\N	\N	\N
35	fah	\N	\N	t	\N	t	\N	\N	\N
43	Vue	\N	\N	t	\N	t	\N	\N	\N
55	fastlane	\N	\N	t	\N	t	\N	\N	\N
253	GPU_offscreen	\N	\N	t	\N	\N	\N	\N	\N
254	diframecache	\N	\N	t	\N	\N	\N	\N	\N
49	tank	\N	25	t	\N	\N	\N	\N	\N
255	bl401_transfer	\N	31	t	\N	\N	\N	\N	\N
256	tankpublish	\N	32	t	\N	\N	\N	\N	\N
257	needsTank	\N	33	t	\N	\N	\N	\N	\N
262	hosts_24GB+	\N	\N	t	\N	\N	\N	\N	\N
261	bulk_container_creation	\N	34	t	\N	\N	\N	\N	\N
258	bl801_transfer	\N	35	t	\N	\N	\N	\N	\N
259	bl201_transfer	\N	36	t	\N	\N	\N	\N	\N
260	bl402_transfer	\N	37	t	\N	\N	\N	\N	\N
36	3Delight	\N	\N	t	\N	t	\N	\N	\N
\.


--
-- Data for Name: sessions; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY sessions (id, length, a_session, "time") FROM stdin;
\.


--
-- Data for Name: shot; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY shot (keyelement, daysbid, description, fkeyelement, fkeyelementstatus, fkeyelementtype, fkeyproject, fkeythumbnail, name, daysscheduled, daysestimated, status, filepath, fkeyassettype, fkeypathtemplate, fkeystatusset, allowtime, datestart, datecomplete, fkeyassettemplate, icon, arsenalslotlimit, arsenalslotreserve, dialog, frameend, framestart, shot, framestartedl, frameendedl, camerainfo, scriptpage) FROM stdin;
\.


--
-- Data for Name: shotgroup; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY shotgroup (keyelement, daysbid, description, fkeyelement, fkeyelementstatus, fkeyelementtype, fkeyproject, fkeythumbnail, name, daysscheduled, daysestimated, status, filepath, fkeyassettype, fkeypathtemplate, fkeystatusset, allowtime, datestart, datecomplete, fkeyassettemplate, icon, arsenalslotlimit, arsenalslotreserve, shotgroup) FROM stdin;
\.


--
-- Data for Name: slots_total; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY slots_total (sum) FROM stdin;
\.


--
-- Data for Name: software; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY software (keysoftware, software, icon, vendor, vendorcontact, active, executable, installedpath, sixtyfourbit, version) FROM stdin;
\.


--
-- Data for Name: status; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY status (keystatus, name) FROM stdin;
\.


--
-- Data for Name: statusset; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY statusset (keystatusset, name) FROM stdin;
\.


--
-- Data for Name: syslog; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY syslog (keysyslog, fkeyhost, fkeysyslogrealm, fkeysyslogseverity, message, count, lastoccurrence, created, class, method, ack, firstoccurence, hostname, username) FROM stdin;
\.


--
-- Data for Name: syslogrealm; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY syslogrealm (keysyslogrealm, syslogrealm) FROM stdin;
1	Servers
2	Slaves
3	Security
4	System
5	Network
6	Rodin
\.


--
-- Data for Name: syslogseverity; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY syslogseverity (keysyslogseverity, syslogseverity, severity) FROM stdin;
1	Warning	Warning
2	Minor	Minor
3	Major	Major
4	Critical	Critical
\.


--
-- Data for Name: task; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY task (keyelement, daysbid, description, fkeyelement, fkeyelementstatus, fkeyelementtype, fkeyproject, fkeythumbnail, name, daysscheduled, daysestimated, status, filepath, fkeyassettype, fkeypathtemplate, fkeystatusset, allowtime, datestart, datecomplete, fkeyassettemplate, icon, arsenalslotlimit, arsenalslotreserve, fkeytasktype, shotgroup) FROM stdin;
\.


--
-- Data for Name: tasktype; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY tasktype (keytasktype, tasktype, iconcolor) FROM stdin;
\.


--
-- Data for Name: taskuser; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY taskuser (keytaskuser, fkeytask, fkeyuser, active) FROM stdin;
\.


--
-- Data for Name: thread; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY thread (keythread, thread, topic, tablename, fkey, datetime, fkeyauthor, skeyreply, fkeyusr, fkeythreadcategory) FROM stdin;
\.


--
-- Data for Name: threadcategory; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY threadcategory (keythreadcategory, threadcategory) FROM stdin;
\.


--
-- Data for Name: threadnotify; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY threadnotify (keythreadnotify, fkeythread, fkeyuser, options) FROM stdin;
\.


--
-- Data for Name: thumbnail; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY thumbnail (keythumbnail, cliprect, date, fkeyelement, fkeyuser, originalfile, image) FROM stdin;
\.


--
-- Data for Name: timesheet; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY timesheet (keytimesheet, datetime, fkeyelement, fkeyemployee, fkeyproject, fkeytimesheetcategory, scheduledhour, datetimesubmitted, unscheduledhour, comment) FROM stdin;
\.


--
-- Data for Name: timesheetcategory; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY timesheetcategory (keytimesheetcategory, timesheetcategory, iconcolor, hasdaily, chronology, disabled, istask, fkeypathtemplate, fkeyelementtype, nameregexp, allowtime, color, description, sortcolumn, tags, sortnumber) FROM stdin;
172	Systems Administration	\N	\N	\N	0	f	\N	4		f	#d4d0c8		displayName	role	\N
168	Modeling Hair	\N	\N	\N	0	t	\N	4		t	#b5c2d6		displayName	schedule,timesheet	\N
36	Compositing	\N	\N	0	0	t	\N	4		t	#dea5a3		displayName	schedule,timesheet	\N
47	Maps	0	\N	0	1	t	\N	4	\N	t	#FFFFFF	\N	displayName	\N	\N
68	Light	\N	\N	\N	\N	f	\N	4	\N	f	#FFFFFF	\N	displayName	\N	\N
49	Animation	0	1	0	0	t	\N	4		t	#89c8b0		displayName	schedule,timesheet	\N
72	Asset Group	\N	\N	\N	0	f	0	6		f	#FFFFFF	\N	displayName	\N	\N
1	Layout	\N	1	40	0	t	\N	4		t	#dcb4c5	Animatic	displayName	schedule,timesheet	\N
42	Scene Assembly Prep	\N	\N	0	1	t	\N	4	\N	t	#FFFFFF	\N	displayName	\N	\N
69	Project	\N	\N	\N	0	f	4	1	[a-zA-Z0-9_]*	f	#c1afc3		displayName		\N
13	Character Set-up	\N	\N	90	1	t	\N	4	\N	t	#FFFFFF	\N	displayName	\N	\N
111	Rigging Face Robot	\N	\N	\N	0	t	1	4		t	#c2baa4		displayName	schedule,timesheet	\N
43	Rigging	\N	\N	0	0	t	\N	4		t	#c2baa4		displayName	schedule,timesheet	\N
20	Facial Clean-up	\N	\N	115	1	t	\N	4	\N	t	#FFFFFF	\N	displayName	\N	\N
78	Rigging Deformation	\N	\N	\N	0	t	\N	5		t	#c2baa4		displayName	schedule,timesheet	\N
53	Sample Still	0	\N	0	1	t	\N	4	\N	t	#FFFFFF	\N	displayName	\N	\N
50	Renders	0	\N	0	1	t	\N	4	\N	t	#FFFFFF	\N	displayName	\N	\N
28	Render farm	\N	\N	410	1	t	\N	4	\N	t	#FFFFFF	\N	displayName	\N	\N
44	Prop Rigging	0	\N	0	1	t	\N	4	\N	t	#FFFFFF	\N	displayName	\N	\N
51	Production	0	\N	0	0	t	0	4		f	#FFFFFF	\N	displayName	\N	\N
46	Pre_Production	0	\N	0	1	t	\N	4	\N	t	#FFFFFF	\N	displayName	\N	\N
63	Misc Rigging	\N	0	0	1	t	\N	4	\N	t	#FFFFFF	\N	displayName	\N	\N
32	Lighting	\N	\N	51	1	t	\N	4	\N	t	#FFFFFF	\N	displayName	\N	\N
79	Rigging Segment	\N	\N	\N	0	t	\N	5		t	#c2baa4		displayName	schedule,timesheet	\N
12	Mocap Body Clean-up	\N	\N	70	0	t	\N	4		t	#acbebc		displayName	schedule,timesheet,role	\N
80	Master Mesh	\N	\N	\N	0	t	0	5		f	#FFFFFF	\N	displayName	\N	\N
81	Master Rig	\N	\N	\N	0	t	0	5		f	#FFFFFF	\N	displayName	\N	\N
11	Mocap Body Session	\N	\N	60	0	t	\N	4		t	#acbebc		displayName	schedule,timesheet	\N
55	Reference	0	\N	0	0	t	0	4		f	#FFFFFF	\N	displayName	\N	\N
54	Materials	0	\N	0	0	t	0	4		f	#FFFFFF	\N	displayName	\N	\N
37	Mocap Direction	\N	\N	0	0	t	\N	4		t	#acbebc		displayName	schedule,timesheet	\N
115	Mocap Facial Clean-up	\N	\N	\N	0	t	\N	4		t	#acbebc		displayName	schedule,timesheet,role	\N
15	Mocap Facial Session	\N	\N	110	0	t	\N	4		t	#acbebc		displayName	schedule,timesheet	\N
25	Tutorials	\N	\N	430	0	t	\N	4		f	#d1c0c2		displayName		\N
10	R&D	\N	\N	30	0	t	\N	4		t	#989898		displayName	schedule,timesheet	\N
45	Information	0	\N	0	1	t	\N	4		t	#989898		displayName		\N
174	Vehicle	\N	\N	\N	0	t	26	4	^[^\\s]*$	t	#b5c2d6		displayName	schedule,timesheet,bid	\N
23	Mocap Prep	\N	\N	55	0	t	\N	4		t	#acbebc		displayName	schedule,timesheet,role	\N
2	Vacation	\N	\N	505	0	t	\N	4		t	#ffbfff	Vacation!!!!!!!!	displayName	schedule,timesheet	\N
56	Development	0	\N	0	0	t	\N	4		t	#dfdfdf		displayName	schedule,timesheet	\N
120	Audio	\N	\N	\N	0	f	1	4	Audio	f	#dfdfdf		displayName		\N
112	Human Resources	\N	\N	\N	0	t	0	4		t	#dfdfdf		displayName	role,schedule,timesheet	\N
9	Storyboards	\N	\N	20	0	t	\N	4		t	#dfdfdf		displayName	schedule,timesheet	\N
57	Bid	0	\N	0	0	t	0	4		f	#dfdfdf		displayName	schedule,timesheet	\N
130	Software Management	\N	\N	\N	0	t	0	4		t	#aaaaff		displayName	timesheet	\N
38	Animatic Modeling	\N	\N	0	1	t	\N	4		t	#dcb4c5		displayName	\N	\N
41	Morph Targets	\N	\N	0	0	t	0	4		f	#b5c2d6		displayName	\N	\N
26	Project prep	\N	\N	5	1	t	\N	4		t	#dfdfdf		displayName	\N	\N
122	Milestone	\N	\N	\N	0	t	\N	4		f	#ff898b		datestart	\N	\N
117	Credit List	\N	\N	\N	0	f	0	4		f	#000000		displayName	\N	\N
19	Scene Assembly	\N	1	150	0	t	\N	4		t	#dea5a3		displayName	timesheet,schedule	\N
126	Lead	\N	\N	\N	0	f	0	4		f	#d4d0c8	Lead and direct his/her team to create 3D animations and characters, using both key-frames and motion capture following the art direction and technical direction, cooperate with art director and technical director.	displayName	role	\N
166	Rigging Lead	\N	\N	\N	0	f	\N	4		f	#d4d0c8		displayName	role	\N
8	Concept Design	\N	\N	10	0	t	\N	4		t	#ecc4af		displayName	schedule,timesheet	\N
17	Cloth	\N	\N	130	0	t	\N	4		t	#d8c8dc		displayName	schedule,timesheet,role	\N
18	FX	\N	\N	140	0	t	\N	4		t	#c8c889		displayName	schedule,timesheet	\N
35	Holiday	\N	\N	508	0	t	\N	4		t	#ffbfff		displayName	schedule,timesheet	\N
175	Matte Painting	\N	\N	\N	0	f	\N	4		f	#ecc4af		displayName	schedule,timesheet	\N
4	Modeling	\N	\N	50	0	t	0	4		t	#b5c2d6		displayName	schedule,timesheet	\N
31	Modeling Texture	\N	\N	141	0	t	\N	4		t	#b5c2d6		displayName	schedule,timesheet	\N
3	Sick	\N	\N	510	0	t	\N	4		t	#ffbfff		displayName	schedule,timesheet	\N
29	Software	\N	\N	420	0	t	\N	4		t	#aaaaff		displayName	schedule,timesheet	\N
6	Supervise	\N	\N	1	0	t	\N	4		t	#dfdfdf		displayName	schedule,timesheet	\N
16	Animation Facial	\N	\N	120	0	t	0	4		t	#89c8b0		displayName	schedule,timesheet	\N
113	Animation Body Mocap	\N	\N	\N	0	t	0	4		t	#89c8b0		displayName	schedule,timesheet	\N
14	Animation Character 	\N	1	100	0	t	0	4		t	#89c8b0		displayName	schedule,timesheet	\N
114	Animation Facial Mocap	\N	\N	\N	0	t	0	4		t	#89c8b0		displayName	schedule,timesheet	\N
27	Scripting	\N	\N	400	0	t	\N	4		t	#989898		displayName	schedule,timesheet	\N
34	Unpaid Leave	\N	\N	507	0	t	\N	4		t	#ffbfff		displayName	schedule,timesheet	\N
40	Tech downtime	\N	\N	0	0	t	0	4		f	#ffbfff		displayName	timesheet	\N
59	Paid Leave	0	0	0	0	t	\N	4		t	#ffbfff		displayName	schedule,timesheet	\N
66	Prop	\N	\N	\N	0	t	18	4	^[^\\s]*$	t	#b5c2d6		displayName	schedule,timesheet	\N
39	Idle Time	\N	\N	0	0	t	0	4		f	#ffbfff		displayName		\N
65	Environment	\N	\N	\N	0	t	\N	4		t	#b5c2d6		displayName	schedule,timesheet	\N
70	Shot	\N	\N	\N	0	f	\N	3		f	#ffffff		shotNumber		\N
177	Layout Shot	\N	\N	\N	0	f	1	4	S(\\d+(?:\\.\\d+))	f	#efefef		displayName		\N
176	Layout Scene	\N	\N	\N	0	f	1	4	Ly(\\d+)	f	#efefef		displayName		\N
95	Point Cache	\N	\N	\N	0	f	1	4		f	#ffffff		displayName		\N
178	QC	\N	\N	55	0	t	\N	4		t	#d4d0c8	QC 	\N		0
24	Shag	\N	\N	135	1	t	\N	4	\N	t	#FFFFFF	\N	displayName	\N	\N
33	Comp Time	\N	\N	506	0	t	\N	4		t	#ffbfff		displayName	schedule,timesheet	\N
21	Editorial	\N	1	160	0	t	\N	4		t	#dfdfdf		displayName	schedule,timesheet	\N
121	Edit	\N	\N	\N	0	f	\N	4	Edit	f	#d4d0c8		displayName	\N	\N
179	Roto	\N	\N	\N	\N	t	\N	4	\N	t	\N	\N	displayName	\N	\N
\.


--
-- Data for Name: tracker; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY tracker (keytracker, tracker, fkeysubmitter, fkeyassigned, fkeycategory, fkeyseverity, fkeystatus, datetarget, datechanged, datesubmitted, description, timeestimate, fkeytrackerqueue) FROM stdin;
\.


--
-- Data for Name: trackercategory; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY trackercategory (keytrackercategory, trackercategory) FROM stdin;
\.


--
-- Data for Name: trackerlog; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY trackerlog (keytrackerlog, fkeytracker, fkeyusr, datelogged, message) FROM stdin;
\.


--
-- Data for Name: trackerqueue; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY trackerqueue (keytrackerqueue, trackerqueue) FROM stdin;
\.


--
-- Data for Name: trackerseverity; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY trackerseverity (keytrackerseverity, trackerseverity) FROM stdin;
\.


--
-- Data for Name: trackerstatus; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY trackerstatus (keytrackerstatus, trackerstatus) FROM stdin;
\.


--
-- Data for Name: userelement; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY userelement (keyuserelement, fkeyelement, fkeyusr, fkeyuser) FROM stdin;
\.


--
-- Data for Name: usermapping; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY usermapping (keyusermapping, fkeyusr, fkeymapping) FROM stdin;
\.


--
-- Data for Name: userrole; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY userrole (keyuserrole, fkeytasktype, fkeyusr) FROM stdin;
\.


--
-- Data for Name: userservice; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY userservice (keyuserservice, "user", service, "limit") FROM stdin;
\.


--
-- Data for Name: usr; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY usr (keyelement, daysbid, description, fkeyelement, fkeyelementstatus, fkeyelementtype, fkeyproject, fkeythumbnail, name, daysscheduled, daysestimated, status, filepath, fkeyassettype, fkeypathtemplate, fkeystatusset, allowtime, datestart, datecomplete, fkeyassettemplate, icon, arsenalslotlimit, arsenalslotreserve, dateoflastlogon, email, fkeyhost, gpgkey, jid, pager, password, remoteips, schedule, shell, uid, threadnotifybyjabber, threadnotifybyemail, fkeyclient, intranet, homedir, disabled, gid, usr, keyusr, rolemask, usrlevel, remoteok, requestcount, sessiontimeout, logoncount, useradded, oldkeyusr, sid, lastlogontype) FROM stdin;
\.


--
-- Data for Name: usrgrp; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY usrgrp (keyusrgrp, fkeyusr, fkeygrp, usrgrp) FROM stdin;
\.


--
-- Data for Name: version; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY version (keyversion) FROM stdin;
\.


--
-- Data for Name: versionfiletracker; Type: TABLE DATA; Schema: public; Owner: farmer
--

COPY versionfiletracker (keyfiletracker, fkeyelement, name, path, filename, fkeypathtemplate, fkeyprojectstorage, storagename, filenametemplate, fkeyversionfiletracker, oldfilenames, version, iteration, automaster) FROM stdin;
\.


--
-- Name: abdownloadstat_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY abdownloadstat
    ADD CONSTRAINT abdownloadstat_pkey PRIMARY KEY (keyabdownloadstat);


--
-- Name: annotation_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY annotation
    ADD CONSTRAINT annotation_pkey PRIMARY KEY (keyannotation);


--
-- Name: assetdep_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY assetdep
    ADD CONSTRAINT assetdep_pkey PRIMARY KEY (keyassetdep);


--
-- Name: assetprop_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY assetprop
    ADD CONSTRAINT assetprop_pkey PRIMARY KEY (keyassetprop);


--
-- Name: assetproperty_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY assetproperty
    ADD CONSTRAINT assetproperty_pkey PRIMARY KEY (keyassetproperty);


--
-- Name: assetproptype_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY assetproptype
    ADD CONSTRAINT assetproptype_pkey PRIMARY KEY (keyassetproptype);


--
-- Name: assetset_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY assetset
    ADD CONSTRAINT assetset_pkey PRIMARY KEY (keyassetset);


--
-- Name: assetsetitem_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY assetsetitem
    ADD CONSTRAINT assetsetitem_pkey PRIMARY KEY (keyassetsetitem);


--
-- Name: assettemplate_fkeyproject_fkeyassettype_name_unique; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY assettemplate
    ADD CONSTRAINT assettemplate_fkeyproject_fkeyassettype_name_unique UNIQUE (fkeyproject, fkeyassettype, name);


--
-- Name: assettemplate_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY assettemplate
    ADD CONSTRAINT assettemplate_pkey PRIMARY KEY (keyassettemplate);


--
-- Name: assettype_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY assettype
    ADD CONSTRAINT assettype_pkey PRIMARY KEY (keyassettype);


--
-- Name: attachment_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY attachment
    ADD CONSTRAINT attachment_pkey PRIMARY KEY (keyattachment);


--
-- Name: attachmenttype_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY attachmenttype
    ADD CONSTRAINT attachmenttype_pkey PRIMARY KEY (keyattachmenttype);


--
-- Name: c_host_ip; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY hostinterface
    ADD CONSTRAINT c_host_ip UNIQUE (fkeyhost, ip);


--
-- Name: calendar_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY calendar
    ADD CONSTRAINT calendar_pkey PRIMARY KEY (keycalendar);


--
-- Name: calendarcategory_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY calendarcategory
    ADD CONSTRAINT calendarcategory_pkey PRIMARY KEY (keycalendarcategory);


--
-- Name: checklistitem_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY checklistitem
    ADD CONSTRAINT checklistitem_pkey PRIMARY KEY (keychecklistitem);


--
-- Name: checkliststatus_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY checkliststatus
    ADD CONSTRAINT checkliststatus_pkey PRIMARY KEY (keycheckliststatus);


--
-- Name: client_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY client
    ADD CONSTRAINT client_pkey PRIMARY KEY (keyclient);


--
-- Name: config_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY config
    ADD CONSTRAINT config_pkey PRIMARY KEY (keyconfig);


--
-- Name: darwinweight_pkey; Type: CONSTRAINT; Schema: public; Owner: farmers; Tablespace: 
--

ALTER TABLE ONLY darwinweight
    ADD CONSTRAINT darwinweight_pkey PRIMARY KEY (keydarwinscore);


--
-- Name: deliveryelement_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY deliveryelement
    ADD CONSTRAINT deliveryelement_pkey PRIMARY KEY (keydeliveryshot);


--
-- Name: demoreel_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY demoreel
    ADD CONSTRAINT demoreel_pkey PRIMARY KEY (keydemoreel);


--
-- Name: diskimage_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY diskimage
    ADD CONSTRAINT diskimage_pkey PRIMARY KEY (keydiskimage);


--
-- Name: dynamichostgroup_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY dynamichostgroup
    ADD CONSTRAINT dynamichostgroup_pkey PRIMARY KEY (keydynamichostgroup);


--
-- Name: element_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY element
    ADD CONSTRAINT element_pkey PRIMARY KEY (keyelement);


--
-- Name: elementdep_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY elementdep
    ADD CONSTRAINT elementdep_pkey PRIMARY KEY (keyelementdep);


--
-- Name: elementstatus_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY elementstatus
    ADD CONSTRAINT elementstatus_pkey PRIMARY KEY (keyelementstatus);


--
-- Name: elementthread_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY elementthread
    ADD CONSTRAINT elementthread_pkey PRIMARY KEY (keyelementthread);


--
-- Name: elementtype_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY elementtype
    ADD CONSTRAINT elementtype_pkey PRIMARY KEY (keyelementtype);


--
-- Name: elementtypetasktype_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY elementtypetasktype
    ADD CONSTRAINT elementtypetasktype_pkey PRIMARY KEY (keyelementtypetasktype);


--
-- Name: elementuser_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY elementuser
    ADD CONSTRAINT elementuser_pkey PRIMARY KEY (keyelementuser);


--
-- Name: eventalert_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY eventalert
    ADD CONSTRAINT eventalert_pkey PRIMARY KEY ("keyEventAlert");


--
-- Name: filetemplate_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY filetemplate
    ADD CONSTRAINT filetemplate_pkey PRIMARY KEY (keyfiletemplate);


--
-- Name: filetracker_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY filetracker
    ADD CONSTRAINT filetracker_pkey PRIMARY KEY (keyfiletracker);


--
-- Name: filetrackerdep_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY filetrackerdep
    ADD CONSTRAINT filetrackerdep_pkey PRIMARY KEY (keyfiletrackerdep);


--
-- Name: fileversion_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY fileversion
    ADD CONSTRAINT fileversion_pkey PRIMARY KEY (keyfileversion);


--
-- Name: folder_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY folder
    ADD CONSTRAINT folder_pkey PRIMARY KEY (keyfolder);


--
-- Name: graph_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY graph
    ADD CONSTRAINT graph_pkey PRIMARY KEY (keygraph);


--
-- Name: graphds_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY graphds
    ADD CONSTRAINT graphds_pkey PRIMARY KEY (keygraphds);


--
-- Name: graphpage_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY graphpage
    ADD CONSTRAINT graphpage_pkey PRIMARY KEY (keygraphpage);


--
-- Name: graphrel_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY graphrelationship
    ADD CONSTRAINT graphrel_pkey PRIMARY KEY (keygraphrelationship);


--
-- Name: gridtemplate_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY gridtemplate
    ADD CONSTRAINT gridtemplate_pkey PRIMARY KEY (keygridtemplate);


--
-- Name: gridtemplateitem_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY gridtemplateitem
    ADD CONSTRAINT gridtemplateitem_pkey PRIMARY KEY (keygridtemplateitem);


--
-- Name: groupmapping_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY groupmapping
    ADD CONSTRAINT groupmapping_pkey PRIMARY KEY (keygroupmapping);


--
-- Name: grp_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY grp
    ADD CONSTRAINT grp_pkey PRIMARY KEY (keygrp);


--
-- Name: gruntscript_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY gruntscript
    ADD CONSTRAINT gruntscript_pkey PRIMARY KEY (keygruntscript);


--
-- Name: history_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY history
    ADD CONSTRAINT history_pkey PRIMARY KEY (keyhistory);


--
-- Name: host_hostname; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY host
    ADD CONSTRAINT host_hostname UNIQUE (host);


--
-- Name: host_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY host
    ADD CONSTRAINT host_pkey PRIMARY KEY (keyhost);


--
-- Name: hostdailystat_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY hostdailystat
    ADD CONSTRAINT hostdailystat_pkey PRIMARY KEY (keyhostdailystat);


--
-- Name: hostgroup_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY hostgroup
    ADD CONSTRAINT hostgroup_pkey PRIMARY KEY (keyhostgroup);


--
-- Name: hostgroupitem_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY hostgroupitem
    ADD CONSTRAINT hostgroupitem_pkey PRIMARY KEY (keyhostgroupitem);


--
-- Name: hosthistory_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY hosthistory
    ADD CONSTRAINT hosthistory_pkey PRIMARY KEY (keyhosthistory);


--
-- Name: hostinterface_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY hostinterface
    ADD CONSTRAINT hostinterface_pkey PRIMARY KEY (keyhostinterface);


--
-- Name: hostinterfacetype_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY hostinterfacetype
    ADD CONSTRAINT hostinterfacetype_pkey PRIMARY KEY (keyhostinterfacetype);


--
-- Name: hostload_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY hostload
    ADD CONSTRAINT hostload_pkey PRIMARY KEY (keyhostload);


--
-- Name: hostmapping_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY hostmapping
    ADD CONSTRAINT hostmapping_pkey PRIMARY KEY (keyhostmapping);


--
-- Name: hostport_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY hostport
    ADD CONSTRAINT hostport_pkey PRIMARY KEY (keyhostport);


--
-- Name: hostresource_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY hostresource
    ADD CONSTRAINT hostresource_pkey PRIMARY KEY (keyhostresource);


--
-- Name: hostservice_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY hostservice
    ADD CONSTRAINT hostservice_pkey PRIMARY KEY (keyhostservice);


--
-- Name: hostsoftware_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY hostsoftware
    ADD CONSTRAINT hostsoftware_pkey PRIMARY KEY (keyhostsoftware);


--
-- Name: hoststatus_fkeyhost; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY hoststatus
    ADD CONSTRAINT hoststatus_fkeyhost UNIQUE (fkeyhost);


--
-- Name: hoststatus_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY hoststatus
    ADD CONSTRAINT hoststatus_pkey PRIMARY KEY (keyhoststatus);


--
-- Name: job_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY job
    ADD CONSTRAINT job_pkey PRIMARY KEY (keyjob);


--
-- Name: jobassignment2_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobassignment
    ADD CONSTRAINT jobassignment2_pkey PRIMARY KEY (keyjobassignment);


--
-- Name: jobassignment_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobassignment_old
    ADD CONSTRAINT jobassignment_pkey PRIMARY KEY (keyjobassignment);


--
-- Name: jobassignmentstatus_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobassignmentstatus
    ADD CONSTRAINT jobassignmentstatus_pkey PRIMARY KEY (keyjobassignmentstatus);


--
-- Name: jobcannedbatch_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobcannedbatch
    ADD CONSTRAINT jobcannedbatch_pkey PRIMARY KEY (keyjobcannedbatch);


--
-- Name: jobcommandhistory_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobcommandhistory
    ADD CONSTRAINT jobcommandhistory_pkey PRIMARY KEY (keyjobcommandhistory);


--
-- Name: jobdep_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobdep
    ADD CONSTRAINT jobdep_pkey PRIMARY KEY (keyjobdep);


--
-- Name: jobenvironment_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobenvironment
    ADD CONSTRAINT jobenvironment_pkey PRIMARY KEY (keyjobenvironment);


--
-- Name: joberror_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY joberror
    ADD CONSTRAINT joberror_pkey PRIMARY KEY (keyjoberror);


--
-- Name: joberrorhandler_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY joberrorhandler
    ADD CONSTRAINT joberrorhandler_pkey PRIMARY KEY (keyjoberrorhandler);


--
-- Name: joberrorhandlerscript_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY joberrorhandlerscript
    ADD CONSTRAINT joberrorhandlerscript_pkey PRIMARY KEY (keyjoberrorhandlerscript);


--
-- Name: jobfiltermessage_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobfiltermessage
    ADD CONSTRAINT jobfiltermessage_pkey PRIMARY KEY (keyjobfiltermessage);


--
-- Name: jobfilterset_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobfilterset
    ADD CONSTRAINT jobfilterset_pkey PRIMARY KEY (keyjobfilterset);


--
-- Name: jobfiltertype_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobfiltertype
    ADD CONSTRAINT jobfiltertype_pkey PRIMARY KEY (keyjobfiltertype);


--
-- Name: jobhistory_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobhistory
    ADD CONSTRAINT jobhistory_pkey PRIMARY KEY (keyjobhistory);


--
-- Name: jobhistorytype_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobhistorytype
    ADD CONSTRAINT jobhistorytype_pkey PRIMARY KEY (keyjobhistorytype);


--
-- Name: jobmantra100_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobmantra100
    ADD CONSTRAINT jobmantra100_pkey PRIMARY KEY (keyjobmantra100);


--
-- Name: jobmantra95_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobmantra95
    ADD CONSTRAINT jobmantra95_pkey PRIMARY KEY (keyjobmantra95);


--
-- Name: joboutput_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY joboutput
    ADD CONSTRAINT joboutput_pkey PRIMARY KEY (keyjoboutput);


--
-- Name: jobservice_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobservice
    ADD CONSTRAINT jobservice_pkey PRIMARY KEY (keyjobservice);


--
-- Name: jobstat_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobstat
    ADD CONSTRAINT jobstat_pkey PRIMARY KEY (keyjobstat);


--
-- Name: jobstateaction_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobstateaction
    ADD CONSTRAINT jobstateaction_pkey PRIMARY KEY (keyjobstateaction);


--
-- Name: jobstatus2_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobstatus
    ADD CONSTRAINT jobstatus2_pkey PRIMARY KEY (keyjobstatus);


--
-- Name: jobstatus_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobstatus_old
    ADD CONSTRAINT jobstatus_pkey PRIMARY KEY (keyjobstatus);


--
-- Name: jobstatusskipreason_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobstatusskipreason
    ADD CONSTRAINT jobstatusskipreason_pkey PRIMARY KEY (keyjobstatusskipreason);


--
-- Name: jobtask_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobtask
    ADD CONSTRAINT jobtask_pkey PRIMARY KEY (keyjobtask);


--
-- Name: jobtaskassignment2_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobtaskassignment
    ADD CONSTRAINT jobtaskassignment2_pkey PRIMARY KEY (keyjobtaskassignment);


--
-- Name: jobtype_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobtype
    ADD CONSTRAINT jobtype_pkey PRIMARY KEY (keyjobtype);


--
-- Name: jobtypemapping_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobtypemapping
    ADD CONSTRAINT jobtypemapping_pkey PRIMARY KEY (keyjobtypemapping);


--
-- Name: license_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY license
    ADD CONSTRAINT license_pkey PRIMARY KEY (keylicense);


--
-- Name: location_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY location
    ADD CONSTRAINT location_pkey PRIMARY KEY (keylocation);


--
-- Name: mapping_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY mapping
    ADD CONSTRAINT mapping_pkey PRIMARY KEY (keymapping);


--
-- Name: mappingtype_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY mappingtype
    ADD CONSTRAINT mappingtype_pkey PRIMARY KEY (keymappingtype);


--
-- Name: methodperms_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY methodperms
    ADD CONSTRAINT methodperms_pkey PRIMARY KEY (keymethodperms);


--
-- Name: notification_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY notification
    ADD CONSTRAINT notification_pkey PRIMARY KEY (keynotification);


--
-- Name: notificationdestination_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY notificationdestination
    ADD CONSTRAINT notificationdestination_pkey PRIMARY KEY (keynotificationdestination);


--
-- Name: notificationmethod_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY notificationmethod
    ADD CONSTRAINT notificationmethod_pkey PRIMARY KEY (keynotificationmethod);


--
-- Name: notificationroute_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY notificationroute
    ADD CONSTRAINT notificationroute_pkey PRIMARY KEY (keynotificationuserroute);


--
-- Name: notify_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY notify
    ADD CONSTRAINT notify_pkey PRIMARY KEY (keynotify);


--
-- Name: notifymethod_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY notifymethod
    ADD CONSTRAINT notifymethod_pkey PRIMARY KEY (keynotifymethod);


--
-- Name: notifysent_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY notifysent
    ADD CONSTRAINT notifysent_pkey PRIMARY KEY (keynotifysent);


--
-- Name: notifywho_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY notifywho
    ADD CONSTRAINT notifywho_pkey PRIMARY KEY (keynotifywho);


--
-- Name: package_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY package
    ADD CONSTRAINT package_pkey PRIMARY KEY (keypackage);


--
-- Name: packageoutput_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY packageoutput
    ADD CONSTRAINT packageoutput_pkey PRIMARY KEY (keypackageoutput);


--
-- Name: pathsynctarget_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY pathsynctarget
    ADD CONSTRAINT pathsynctarget_pkey PRIMARY KEY (keypathsynctarget);


--
-- Name: pathtemplate_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY pathtemplate
    ADD CONSTRAINT pathtemplate_pkey PRIMARY KEY (keypathtemplate);


--
-- Name: pathtracker_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY pathtracker
    ADD CONSTRAINT pathtracker_pkey PRIMARY KEY (keypathtracker);


--
-- Name: permission_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY permission
    ADD CONSTRAINT permission_pkey PRIMARY KEY (keypermission);


--
-- Name: phoneno_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY phoneno
    ADD CONSTRAINT phoneno_pkey PRIMARY KEY (keyphoneno);


--
-- Name: phonetype_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY phonetype
    ADD CONSTRAINT phonetype_pkey PRIMARY KEY (keyphonetype);


--
-- Name: pkey_employee; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY employee
    ADD CONSTRAINT pkey_employee PRIMARY KEY (keyelement);


--
-- Name: pkey_job3delight; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY job3delight
    ADD CONSTRAINT pkey_job3delight PRIMARY KEY (keyjob);


--
-- Name: pkey_job_maya2008; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobmaya2008
    ADD CONSTRAINT pkey_job_maya2008 PRIMARY KEY (keyjob);


--
-- Name: pkey_job_maya2009; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobmaya2009
    ADD CONSTRAINT pkey_job_maya2009 PRIMARY KEY (keyjob);


--
-- Name: pkey_job_maya85; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobmaya85
    ADD CONSTRAINT pkey_job_maya85 PRIMARY KEY (keyjob);


--
-- Name: pkey_jobbatch; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobbatch
    ADD CONSTRAINT pkey_jobbatch PRIMARY KEY (keyjob);


--
-- Name: pkey_jobmentalray85; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobmentalray85
    ADD CONSTRAINT pkey_jobmentalray85 PRIMARY KEY (keyjob);


--
-- Name: pkey_jobnaiad; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobnaiad
    ADD CONSTRAINT pkey_jobnaiad PRIMARY KEY (keyjob);


--
-- Name: pkey_jobnuke; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobnuke
    ADD CONSTRAINT pkey_jobnuke PRIMARY KEY (keyjob);


--
-- Name: pkey_jobnuke51; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY jobnuke51
    ADD CONSTRAINT pkey_jobnuke51 PRIMARY KEY (keyjob);


--
-- Name: pkey_shot; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY shot
    ADD CONSTRAINT pkey_shot PRIMARY KEY (keyelement);


--
-- Name: pkey_shotgroup; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY shotgroup
    ADD CONSTRAINT pkey_shotgroup PRIMARY KEY (keyelement);


--
-- Name: pkey_task; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY task
    ADD CONSTRAINT pkey_task PRIMARY KEY (keyelement);


--
-- Name: pkey_usr; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY usr
    ADD CONSTRAINT pkey_usr PRIMARY KEY (keyelement);


--
-- Name: project_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY project
    ADD CONSTRAINT project_pkey PRIMARY KEY (keyelement);


--
-- Name: projectresolution_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY projectresolution
    ADD CONSTRAINT projectresolution_pkey PRIMARY KEY (keyprojectresolution);


--
-- Name: projectstatus_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY projectstatus
    ADD CONSTRAINT projectstatus_pkey PRIMARY KEY (keyprojectstatus);


--
-- Name: projectstorage_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY projectstorage
    ADD CONSTRAINT projectstorage_pkey PRIMARY KEY (keyprojectstorage);


--
-- Name: renderframe_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY renderframe
    ADD CONSTRAINT renderframe_pkey PRIMARY KEY (keyrenderframe);


--
-- Name: schedule_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY schedule
    ADD CONSTRAINT schedule_pkey PRIMARY KEY (keyschedule);


--
-- Name: serverfileaction_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY serverfileaction
    ADD CONSTRAINT serverfileaction_pkey PRIMARY KEY (keyserverfileaction);


--
-- Name: serverfileactionstatus_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY serverfileactionstatus
    ADD CONSTRAINT serverfileactionstatus_pkey PRIMARY KEY (keyserverfileactionstatus);


--
-- Name: serverfileactiontype_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY serverfileactiontype
    ADD CONSTRAINT serverfileactiontype_pkey PRIMARY KEY (keyserverfileactiontype);


--
-- Name: service_name_unique; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY service
    ADD CONSTRAINT service_name_unique UNIQUE (service);


--
-- Name: service_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY service
    ADD CONSTRAINT service_pkey PRIMARY KEY (keyservice);


--
-- Name: software_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY software
    ADD CONSTRAINT software_pkey PRIMARY KEY (keysoftware);


--
-- Name: status_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY status
    ADD CONSTRAINT status_pkey PRIMARY KEY (keystatus);


--
-- Name: statusset_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY statusset
    ADD CONSTRAINT statusset_pkey PRIMARY KEY (keystatusset);


--
-- Name: syslog_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY syslog
    ADD CONSTRAINT syslog_pkey PRIMARY KEY (keysyslog);


--
-- Name: syslogrealm_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY syslogrealm
    ADD CONSTRAINT syslogrealm_pkey PRIMARY KEY (keysyslogrealm);


--
-- Name: syslogseverity_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY syslogseverity
    ADD CONSTRAINT syslogseverity_pkey PRIMARY KEY (keysyslogseverity);


--
-- Name: tasktype_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY tasktype
    ADD CONSTRAINT tasktype_pkey PRIMARY KEY (keytasktype);


--
-- Name: taskuser_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY taskuser
    ADD CONSTRAINT taskuser_pkey PRIMARY KEY (keytaskuser);


--
-- Name: thread_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY thread
    ADD CONSTRAINT thread_pkey PRIMARY KEY (keythread);


--
-- Name: threadcategory_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY threadcategory
    ADD CONSTRAINT threadcategory_pkey PRIMARY KEY (keythreadcategory);


--
-- Name: threadnotify_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY threadnotify
    ADD CONSTRAINT threadnotify_pkey PRIMARY KEY (keythreadnotify);


--
-- Name: thumbnail_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY thumbnail
    ADD CONSTRAINT thumbnail_pkey PRIMARY KEY (keythumbnail);


--
-- Name: timesheet_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY timesheet
    ADD CONSTRAINT timesheet_pkey PRIMARY KEY (keytimesheet);


--
-- Name: timesheetcategory_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY timesheetcategory
    ADD CONSTRAINT timesheetcategory_pkey PRIMARY KEY (keytimesheetcategory);


--
-- Name: tracker_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY tracker
    ADD CONSTRAINT tracker_pkey PRIMARY KEY (keytracker);


--
-- Name: trackercategory_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY trackercategory
    ADD CONSTRAINT trackercategory_pkey PRIMARY KEY (keytrackercategory);


--
-- Name: trackerlog_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY trackerlog
    ADD CONSTRAINT trackerlog_pkey PRIMARY KEY (keytrackerlog);


--
-- Name: trackerqueue_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY trackerqueue
    ADD CONSTRAINT trackerqueue_pkey PRIMARY KEY (keytrackerqueue);


--
-- Name: trackerseverity_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY trackerseverity
    ADD CONSTRAINT trackerseverity_pkey PRIMARY KEY (keytrackerseverity);


--
-- Name: trackerstatus_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY trackerstatus
    ADD CONSTRAINT trackerstatus_pkey PRIMARY KEY (keytrackerstatus);


--
-- Name: userelement_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY userelement
    ADD CONSTRAINT userelement_pkey PRIMARY KEY (keyuserelement);


--
-- Name: usermapping_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY usermapping
    ADD CONSTRAINT usermapping_pkey PRIMARY KEY (keyusermapping);


--
-- Name: userrole_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY userrole
    ADD CONSTRAINT userrole_pkey PRIMARY KEY (keyuserrole);


--
-- Name: userservice_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY userservice
    ADD CONSTRAINT userservice_pkey PRIMARY KEY (keyuserservice);


--
-- Name: usrgrp_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY usrgrp
    ADD CONSTRAINT usrgrp_pkey PRIMARY KEY (keyusrgrp);


--
-- Name: version_pkey; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY version
    ADD CONSTRAINT version_pkey PRIMARY KEY (keyversion);


--
-- Name: x_hostservice_fkeyhost_fkeyservice; Type: CONSTRAINT; Schema: public; Owner: farmer; Tablespace: 
--

ALTER TABLE ONLY hostservice
    ADD CONSTRAINT x_hostservice_fkeyhost_fkeyservice UNIQUE (fkeyhost, fkeyservice);


--
-- Name: fki_fkey_dep; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX fki_fkey_dep ON jobdep USING btree (fkeydep);


--
-- Name: fki_fkey_elementuser_element; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX fki_fkey_elementuser_element ON elementuser USING btree (fkeyelement);


--
-- Name: fki_fkey_elementuser_user; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX fki_fkey_elementuser_user ON elementuser USING btree (fkeyuser);


--
-- Name: fki_fkey_hgi_hg; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX fki_fkey_hgi_hg ON hostgroupitem USING btree (fkeyhostgroup);


--
-- Name: fki_fkey_hgi_host; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX fki_fkey_hgi_host ON hostgroupitem USING btree (fkeyhost);


--
-- Name: fki_fkey_host; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX fki_fkey_host ON jobcommandhistory USING btree (fkeyhost);


--
-- Name: fki_fkey_job; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX fki_fkey_job ON jobdep USING btree (fkeyjob);


--
-- Name: fki_fkey_jobassignment; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX fki_fkey_jobassignment ON jobtaskassignment USING btree (fkeyjobassignment);


--
-- Name: fki_joboutput_fkey_job; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX fki_joboutput_fkey_job ON joboutput USING btree (fkeyjob);


--
-- Name: fki_jobtask_fkey_host; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX fki_jobtask_fkey_host ON jobtask USING btree (fkeyhost);


--
-- Name: fki_jobtask_fkey_job; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX fki_jobtask_fkey_job ON jobtask USING btree (fkeyjob);


--
-- Name: x_annotation_sequence; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_annotation_sequence ON annotation USING btree (sequence);


--
-- Name: x_config_config; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_config_config ON config USING btree (config);


--
-- Name: x_filetracker_fkeyelement; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_filetracker_fkeyelement ON filetracker USING btree (fkeyelement);


--
-- Name: x_filetracker_name; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_filetracker_name ON filetracker USING btree (name);


--
-- Name: x_filetracker_path; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_filetracker_path ON filetracker USING btree (path);


--
-- Name: x_filetrackerdep_fkeyinput; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_filetrackerdep_fkeyinput ON filetrackerdep USING btree (fkeyinput);


--
-- Name: x_filetrackerdep_fkeyoutput; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_filetrackerdep_fkeyoutput ON filetrackerdep USING btree (fkeyoutput);


--
-- Name: x_hosthistory_datetime; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_hosthistory_datetime ON hosthistory USING btree (datetime);


--
-- Name: x_hosthistory_datetimeplusduration; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_hosthistory_datetimeplusduration ON hosthistory USING btree (((datetime + duration)));


--
-- Name: x_hosthistory_fkeyhost; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_hosthistory_fkeyhost ON hosthistory USING btree (fkeyhost);


--
-- Name: x_hosthistory_fkeyjob; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_hosthistory_fkeyjob ON hosthistory USING btree (fkeyjob);


--
-- Name: x_hostmapping_fkeymapping; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_hostmapping_fkeymapping ON hostmapping USING btree (fkeymapping);


--
-- Name: x_hostservice_fkeyhost; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_hostservice_fkeyhost ON hostservice USING btree (fkeyhost);


--
-- Name: x_hostservice_fkeyservice; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_hostservice_fkeyservice ON hostservice USING btree (fkeyservice);


--
-- Name: x_hoststatus_host; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_hoststatus_host ON hoststatus USING btree (fkeyhost);


--
-- Name: x_hoststatus_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_hoststatus_status ON hoststatus USING btree (slavestatus);


--
-- Name: x_jch_job; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jch_job ON jobcommandhistory USING btree (fkeyjob);


--
-- Name: x_jfm_set; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jfm_set ON jobfiltermessage USING btree (fkeyjobfilterset);


--
-- Name: x_job3delight_project; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_job3delight_project ON job3delight USING btree (fkeyproject);


--
-- Name: x_job3delight_shotname; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_job3delight_shotname ON job3delight USING btree (shotname);


--
-- Name: x_job3delight_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_job3delight_status ON job3delight USING btree (status);


--
-- Name: x_job3delight_user; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_job3delight_user ON job3delight USING btree (fkeyusr);


--
-- Name: x_job_fkeyusr; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_job_fkeyusr ON job USING btree (fkeyusr) WHERE ((((status = 'new'::text) OR (status = 'ready'::text)) OR (status = 'started'::text)) OR (status = 'done'::text));


--
-- Name: x_job_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_job_status ON job USING btree (status);


--
-- Name: x_job_user; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_job_user ON job USING btree (fkeyusr);


--
-- Name: x_jobassignment_fkeyjob; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobassignment_fkeyjob ON jobassignment USING btree (fkeyjob);


--
-- Name: x_jobassignment_host; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobassignment_host ON jobassignment USING btree (fkeyhost);


--
-- Name: x_jobassignment_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobassignment_status ON jobassignment USING btree (fkeyjobassignmentstatus);


--
-- Name: x_jobbatch_project; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobbatch_project ON jobbatch USING btree (fkeyproject);


--
-- Name: x_jobbatch_shotname; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobbatch_shotname ON jobbatch USING btree (shotname);


--
-- Name: x_jobbatch_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobbatch_status ON jobbatch USING btree (status);


--
-- Name: x_jobbatch_user; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobbatch_user ON jobbatch USING btree (fkeyusr);


--
-- Name: x_joberror_cleared; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_joberror_cleared ON joberror USING btree (cleared);


--
-- Name: x_joberror_fkeyhost; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_joberror_fkeyhost ON joberror USING btree (fkeyhost);


--
-- Name: x_joberror_fkeyjob; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_joberror_fkeyjob ON joberror USING btree (fkeyjob);


--
-- Name: x_jobhistory_job; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobhistory_job ON jobhistory USING btree (fkeyjob);


--
-- Name: x_jobmantra_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobmantra_status ON jobmantra100 USING btree (status);


--
-- Name: x_jobmaya2008_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobmaya2008_status ON jobmaya2008 USING btree (status);


--
-- Name: x_jobmaya2009_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobmaya2009_status ON jobmaya2009 USING btree (status);


--
-- Name: x_jobmaya2011_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobmaya2011_status ON jobmaya2011 USING btree (status);


--
-- Name: x_jobmaya7_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobmaya7_status ON jobmaya7 USING btree (status);


--
-- Name: x_jobmaya85_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobmaya85_status ON jobmaya85 USING btree (status);


--
-- Name: x_jobmaya8_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobmaya8_status ON jobmaya8 USING btree (status);


--
-- Name: x_jobmaya_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobmaya_status ON jobmaya USING btree (status);


--
-- Name: x_jobmentalray2009_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobmentalray2009_status ON jobmentalray2009 USING btree (status);


--
-- Name: x_jobmentalray2011_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobmentalray2011_status ON jobmentalray2011 USING btree (status);


--
-- Name: x_jobmentalray7_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobmentalray7_status ON jobmentalray7 USING btree (status);


--
-- Name: x_jobmentalray85_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobmentalray85_status ON jobmentalray85 USING btree (status);


--
-- Name: x_jobmentalray8_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobmentalray8_status ON jobmentalray8 USING btree (status);


--
-- Name: x_jobnaiad_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobnaiad_status ON jobnaiad USING btree (status);


--
-- Name: x_jobnaiad_user; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobnaiad_user ON jobnaiad USING btree (fkeyusr);


--
-- Name: x_jobnuke51_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobnuke51_status ON jobnuke51 USING btree (status);


--
-- Name: x_jobnuke52_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobnuke52_status ON jobnuke52 USING btree (status);


--
-- Name: x_jobnuke_project; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobnuke_project ON jobnuke USING btree (fkeyproject);


--
-- Name: x_jobnuke_shotname; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobnuke_shotname ON jobnuke USING btree (shotname);


--
-- Name: x_jobnuke_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobnuke_status ON jobnuke USING btree (status);


--
-- Name: x_jobnuke_user; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobnuke_user ON jobnuke USING btree (fkeyusr);


--
-- Name: x_jobservice_job; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobservice_job ON jobservice USING btree (fkeyjob);


--
-- Name: x_jobservice_service; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobservice_service ON jobservice USING btree (fkeyservice);


--
-- Name: x_jobstatus_job; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobstatus_job ON jobstatus USING btree (fkeyjob);


--
-- Name: x_jobtask_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobtask_status ON jobtask USING btree (status);


--
-- Name: x_jobtaskassignment_jobtask; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_jobtaskassignment_jobtask ON jobtaskassignment USING btree (fkeyjobtask);


--
-- Name: x_mantra95_status; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_mantra95_status ON jobmantra95 USING btree (status);


--
-- Name: x_rangefiletracker_fkeyelement; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_rangefiletracker_fkeyelement ON rangefiletracker USING btree (fkeyelement);


--
-- Name: x_rangefiletracker_name; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_rangefiletracker_name ON rangefiletracker USING btree (name);


--
-- Name: x_rangefiletracker_path; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_rangefiletracker_path ON rangefiletracker USING btree (path);


--
-- Name: x_schedule_fkeyelement; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_schedule_fkeyelement ON schedule USING btree (fkeyelement);


--
-- Name: x_schedule_fkeyuser; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_schedule_fkeyuser ON schedule USING btree (fkeyuser);


--
-- Name: x_syslog_ack; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_syslog_ack ON syslog USING btree (ack);


--
-- Name: x_timesheet_fkeyemployee; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_timesheet_fkeyemployee ON timesheet USING btree (fkeyemployee);


--
-- Name: x_timesheet_fkeyproject; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_timesheet_fkeyproject ON timesheet USING btree (fkeyproject);


--
-- Name: x_versionfiletracker_fkeyelement; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_versionfiletracker_fkeyelement ON versionfiletracker USING btree (fkeyelement);


--
-- Name: x_versionfiletracker_name; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_versionfiletracker_name ON versionfiletracker USING btree (name);


--
-- Name: x_versionfiletracker_path; Type: INDEX; Schema: public; Owner: farmer; Tablespace: 
--

CREATE INDEX x_versionfiletracker_path ON versionfiletracker USING btree (path);


--
-- Name: hoststatus_update_trigger; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER hoststatus_update_trigger AFTER UPDATE ON hoststatus FOR EACH ROW EXECUTE PROCEDURE hoststatus_update();


--
-- Name: job3delight_delete; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER job3delight_delete AFTER DELETE ON job3delight FOR EACH ROW EXECUTE PROCEDURE job_delete();


--
-- Name: job3delight_insert; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER job3delight_insert AFTER INSERT ON job3delight FOR EACH ROW EXECUTE PROCEDURE job_insert();


--
-- Name: job3delight_update; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER job3delight_update BEFORE UPDATE ON job3delight FOR EACH ROW EXECUTE PROCEDURE job_update();


--
-- Name: job_delete; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER job_delete AFTER DELETE ON job FOR EACH ROW EXECUTE PROCEDURE job_delete();


--
-- Name: job_error_insert_trigger; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER job_error_insert_trigger AFTER INSERT ON joberror FOR EACH ROW EXECUTE PROCEDURE job_error_insert();


--
-- Name: job_error_update_trigger; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER job_error_update_trigger AFTER UPDATE ON joberror FOR EACH ROW EXECUTE PROCEDURE job_error_increment();


--
-- Name: job_insert; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER job_insert AFTER INSERT ON job FOR EACH ROW EXECUTE PROCEDURE job_insert();


--
-- Name: job_update; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER job_update BEFORE UPDATE ON job FOR EACH ROW EXECUTE PROCEDURE job_update();


--
-- Name: jobassignment_after_trigger; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobassignment_after_trigger AFTER INSERT OR UPDATE ON jobassignment_old FOR EACH ROW EXECUTE PROCEDURE jobassignment_after_update();


--
-- Name: jobassignment_after_trigger; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobassignment_after_trigger AFTER INSERT OR UPDATE ON jobassignment FOR EACH ROW EXECUTE PROCEDURE jobassignment_after_update();


--
-- Name: jobassignment_delete_trigger; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobassignment_delete_trigger BEFORE DELETE ON jobassignment_old FOR EACH ROW EXECUTE PROCEDURE jobassignment_delete();


--
-- Name: jobassignment_delete_trigger; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobassignment_delete_trigger BEFORE DELETE ON jobassignment FOR EACH ROW EXECUTE PROCEDURE jobassignment_delete();


--
-- Name: jobassignment_insert_trigger; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobassignment_insert_trigger BEFORE INSERT ON jobassignment_old FOR EACH ROW EXECUTE PROCEDURE jobassignment_insert();


--
-- Name: jobassignment_insert_trigger; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobassignment_insert_trigger BEFORE INSERT ON jobassignment FOR EACH ROW EXECUTE PROCEDURE jobassignment_insert();


--
-- Name: jobassignment_update_trigger; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobassignment_update_trigger BEFORE UPDATE ON jobassignment_old FOR EACH ROW EXECUTE PROCEDURE jobassignment_update();


--
-- Name: jobassignment_update_trigger; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobassignment_update_trigger BEFORE UPDATE ON jobassignment FOR EACH ROW EXECUTE PROCEDURE jobassignment_update();


--
-- Name: jobbatch_delete; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobbatch_delete AFTER DELETE ON jobbatch FOR EACH ROW EXECUTE PROCEDURE job_delete();


--
-- Name: jobbatch_insert; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobbatch_insert AFTER INSERT ON jobbatch FOR EACH ROW EXECUTE PROCEDURE job_insert();


--
-- Name: jobbatch_update; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobbatch_update BEFORE UPDATE ON jobbatch FOR EACH ROW EXECUTE PROCEDURE job_update();


--
-- Name: jobdep_delete; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobdep_delete AFTER DELETE ON jobdep FOR EACH ROW EXECUTE PROCEDURE jobdep_delete();


--
-- Name: joberror_inc; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER joberror_inc AFTER INSERT OR UPDATE ON joberror FOR EACH ROW EXECUTE PROCEDURE joberror_inc();


--
-- Name: jobmantra100_delete; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmantra100_delete AFTER DELETE ON jobmantra100 FOR EACH ROW EXECUTE PROCEDURE job_delete();


--
-- Name: jobmantra100_insert; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmantra100_insert AFTER INSERT ON jobmantra100 FOR EACH ROW EXECUTE PROCEDURE job_insert();


--
-- Name: jobmantra100_update; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmantra100_update BEFORE UPDATE ON jobmantra100 FOR EACH ROW EXECUTE PROCEDURE job_update();


--
-- Name: jobmax2009_insert; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmax2009_insert AFTER INSERT ON jobmaya2009 FOR EACH ROW EXECUTE PROCEDURE job_insert();


--
-- Name: jobmaya2008_delete; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmaya2008_delete AFTER DELETE ON jobmaya2008 FOR EACH ROW EXECUTE PROCEDURE job_delete();


--
-- Name: jobmaya2008_insert; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmaya2008_insert AFTER INSERT ON jobmaya2008 FOR EACH ROW EXECUTE PROCEDURE job_insert();


--
-- Name: jobmaya2008_update; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmaya2008_update BEFORE UPDATE ON jobmaya2008 FOR EACH ROW EXECUTE PROCEDURE job_update();


--
-- Name: jobmaya2009_delete; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmaya2009_delete AFTER DELETE ON jobmaya2009 FOR EACH ROW EXECUTE PROCEDURE job_delete();


--
-- Name: jobmaya2009_insert; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmaya2009_insert AFTER INSERT ON jobmaya2009 FOR EACH ROW EXECUTE PROCEDURE job_insert();


--
-- Name: jobmaya2009_update; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmaya2009_update BEFORE UPDATE ON jobmaya2009 FOR EACH ROW EXECUTE PROCEDURE job_update();


--
-- Name: jobmaya2011_delete; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmaya2011_delete AFTER DELETE ON jobmaya2011 FOR EACH ROW EXECUTE PROCEDURE job_delete();


--
-- Name: jobmaya2011_insert; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmaya2011_insert AFTER INSERT ON jobmaya2011 FOR EACH ROW EXECUTE PROCEDURE job_insert();


--
-- Name: jobmaya2011_update; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmaya2011_update BEFORE UPDATE ON jobmaya2011 FOR EACH ROW EXECUTE PROCEDURE job_update();


--
-- Name: jobmaya7_delete; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmaya7_delete AFTER DELETE ON jobmaya7 FOR EACH ROW EXECUTE PROCEDURE job_delete();


--
-- Name: jobmaya7_insert; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmaya7_insert AFTER INSERT ON jobmaya7 FOR EACH ROW EXECUTE PROCEDURE job_insert();


--
-- Name: jobmaya7_update; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmaya7_update BEFORE UPDATE ON jobmaya7 FOR EACH ROW EXECUTE PROCEDURE job_update();


--
-- Name: jobmaya85_delete; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmaya85_delete AFTER DELETE ON jobmaya85 FOR EACH ROW EXECUTE PROCEDURE job_delete();


--
-- Name: jobmaya85_insert; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmaya85_insert AFTER INSERT ON jobmaya85 FOR EACH ROW EXECUTE PROCEDURE job_insert();


--
-- Name: jobmaya85_update; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmaya85_update BEFORE UPDATE ON jobmaya85 FOR EACH ROW EXECUTE PROCEDURE job_update();


--
-- Name: jobmentalray2009_delete; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmentalray2009_delete AFTER DELETE ON jobmentalray2009 FOR EACH ROW EXECUTE PROCEDURE job_delete();


--
-- Name: jobmentalray2009_insert; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmentalray2009_insert AFTER INSERT ON jobmentalray2009 FOR EACH ROW EXECUTE PROCEDURE job_insert();


--
-- Name: jobmentalray2009_update; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmentalray2009_update BEFORE UPDATE ON jobmentalray2009 FOR EACH ROW EXECUTE PROCEDURE job_update();


--
-- Name: jobmentalray2011_delete; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmentalray2011_delete AFTER DELETE ON jobmentalray2011 FOR EACH ROW EXECUTE PROCEDURE job_delete();


--
-- Name: jobmentalray2011_insert; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmentalray2011_insert AFTER INSERT ON jobmentalray2011 FOR EACH ROW EXECUTE PROCEDURE job_insert();


--
-- Name: jobmentalray2011_update; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmentalray2011_update BEFORE UPDATE ON jobmentalray2011 FOR EACH ROW EXECUTE PROCEDURE job_update();


--
-- Name: jobmentalray85_delete; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmentalray85_delete AFTER DELETE ON jobmentalray85 FOR EACH ROW EXECUTE PROCEDURE job_delete();


--
-- Name: jobmentalray85_insert; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmentalray85_insert AFTER INSERT ON jobmentalray85 FOR EACH ROW EXECUTE PROCEDURE job_insert();


--
-- Name: jobmentalray85_update; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobmentalray85_update BEFORE UPDATE ON jobmentalray85 FOR EACH ROW EXECUTE PROCEDURE job_update();


--
-- Name: jobnaiad_delete; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobnaiad_delete AFTER DELETE ON jobnaiad FOR EACH ROW EXECUTE PROCEDURE job_delete();


--
-- Name: jobnaiad_insert; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobnaiad_insert AFTER INSERT ON jobnaiad FOR EACH ROW EXECUTE PROCEDURE job_insert();


--
-- Name: jobnaiad_update; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobnaiad_update BEFORE UPDATE ON jobnaiad FOR EACH ROW EXECUTE PROCEDURE job_update();


--
-- Name: jobnuke51_delete; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobnuke51_delete AFTER DELETE ON jobnuke51 FOR EACH ROW EXECUTE PROCEDURE job_delete();


--
-- Name: jobnuke51_insert; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobnuke51_insert AFTER INSERT ON jobnuke51 FOR EACH ROW EXECUTE PROCEDURE job_insert();


--
-- Name: jobnuke51_update; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobnuke51_update BEFORE UPDATE ON jobnuke51 FOR EACH ROW EXECUTE PROCEDURE job_update();


--
-- Name: jobnuke52_delete; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobnuke52_delete AFTER DELETE ON jobnuke52 FOR EACH ROW EXECUTE PROCEDURE job_delete();


--
-- Name: jobnuke52_insert; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobnuke52_insert AFTER INSERT ON jobnuke52 FOR EACH ROW EXECUTE PROCEDURE job_insert();


--
-- Name: jobnuke52_update; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobnuke52_update BEFORE UPDATE ON jobnuke52 FOR EACH ROW EXECUTE PROCEDURE job_update();


--
-- Name: jobnuke_delete; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobnuke_delete AFTER DELETE ON jobnuke FOR EACH ROW EXECUTE PROCEDURE job_delete();


--
-- Name: jobnuke_insert; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobnuke_insert AFTER INSERT ON jobnuke FOR EACH ROW EXECUTE PROCEDURE job_insert();


--
-- Name: jobnuke_update; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobnuke_update BEFORE UPDATE ON jobnuke FOR EACH ROW EXECUTE PROCEDURE job_update();


--
-- Name: jobtaskassignment_update_trigger; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobtaskassignment_update_trigger BEFORE UPDATE ON jobtaskassignment_old FOR EACH ROW EXECUTE PROCEDURE jobtaskassignment_update();


--
-- Name: jobtaskassignment_update_trigger; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER jobtaskassignment_update_trigger BEFORE UPDATE ON jobtaskassignment FOR EACH ROW EXECUTE PROCEDURE jobtaskassignment_update();


--
-- Name: sync_host_to_hoststatus_trigger; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER sync_host_to_hoststatus_trigger AFTER INSERT OR DELETE ON host FOR EACH STATEMENT EXECUTE PROCEDURE sync_host_to_hoststatus_trigger();


--
-- Name: update_hostload; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER update_hostload BEFORE UPDATE ON hostload FOR EACH ROW EXECUTE PROCEDURE update_hostload();


--
-- Name: update_hostservice; Type: TRIGGER; Schema: public; Owner: farmer
--

CREATE TRIGGER update_hostservice BEFORE UPDATE ON hostservice FOR EACH ROW EXECUTE PROCEDURE update_hostservice();


--
-- Name: fkey_eventalert_host; Type: FK CONSTRAINT; Schema: public; Owner: farmer
--

ALTER TABLE ONLY eventalert
    ADD CONSTRAINT fkey_eventalert_host FOREIGN KEY ("fkeyHost") REFERENCES host(keyhost);


--
-- Name: fkey_graphrel_graph; Type: FK CONSTRAINT; Schema: public; Owner: farmer
--

ALTER TABLE ONLY graphrelationship
    ADD CONSTRAINT fkey_graphrel_graph FOREIGN KEY (fkeygraph) REFERENCES graph(keygraph) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: fkey_graphrel_graphds; Type: FK CONSTRAINT; Schema: public; Owner: farmer
--

ALTER TABLE ONLY graphrelationship
    ADD CONSTRAINT fkey_graphrel_graphds FOREIGN KEY (fkeygraphds) REFERENCES graphds(keygraphds) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: fkey_hgi_hg; Type: FK CONSTRAINT; Schema: public; Owner: farmer
--

ALTER TABLE ONLY hostgroupitem
    ADD CONSTRAINT fkey_hgi_hg FOREIGN KEY (fkeyhostgroup) REFERENCES hostgroup(keyhostgroup) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: fkey_hgi_host; Type: FK CONSTRAINT; Schema: public; Owner: farmer
--

ALTER TABLE ONLY hostgroupitem
    ADD CONSTRAINT fkey_hgi_host FOREIGN KEY (fkeyhost) REFERENCES host(keyhost) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: fkey_host; Type: FK CONSTRAINT; Schema: public; Owner: farmer
--

ALTER TABLE ONLY jobcommandhistory
    ADD CONSTRAINT fkey_host FOREIGN KEY (fkeyhost) REFERENCES host(keyhost) ON UPDATE CASCADE ON DELETE SET NULL;


--
-- Name: fkey_hostinterface_host; Type: FK CONSTRAINT; Schema: public; Owner: farmer
--

ALTER TABLE ONLY hostinterface
    ADD CONSTRAINT fkey_hostinterface_host FOREIGN KEY (fkeyhost) REFERENCES host(keyhost) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: fkey_hostport_host; Type: FK CONSTRAINT; Schema: public; Owner: farmer
--

ALTER TABLE ONLY hostport
    ADD CONSTRAINT fkey_hostport_host FOREIGN KEY (fkeyhost) REFERENCES host(keyhost) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: fkey_hostservice_host; Type: FK CONSTRAINT; Schema: public; Owner: farmer
--

ALTER TABLE ONLY hostservice
    ADD CONSTRAINT fkey_hostservice_host FOREIGN KEY (fkeyhost) REFERENCES host(keyhost) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: fkey_hostservice_service; Type: FK CONSTRAINT; Schema: public; Owner: farmer
--

ALTER TABLE ONLY hostservice
    ADD CONSTRAINT fkey_hostservice_service FOREIGN KEY (fkeyservice) REFERENCES service(keyservice) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: fkey_hostsoftware_host; Type: FK CONSTRAINT; Schema: public; Owner: farmer
--

ALTER TABLE ONLY hostsoftware
    ADD CONSTRAINT fkey_hostsoftware_host FOREIGN KEY (fkeyhost) REFERENCES host(keyhost) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: fkey_hostsoftware_software; Type: FK CONSTRAINT; Schema: public; Owner: farmer
--

ALTER TABLE ONLY hostsoftware
    ADD CONSTRAINT fkey_hostsoftware_software FOREIGN KEY (fkeysoftware) REFERENCES software(keysoftware) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: fkey_hoststatus_host; Type: FK CONSTRAINT; Schema: public; Owner: farmer
--

ALTER TABLE ONLY hoststatus
    ADD CONSTRAINT fkey_hoststatus_host FOREIGN KEY (fkeyhost) REFERENCES host(keyhost) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: fkey_jobassignment; Type: FK CONSTRAINT; Schema: public; Owner: farmer
--

ALTER TABLE ONLY jobtaskassignment
    ADD CONSTRAINT fkey_jobassignment FOREIGN KEY (fkeyjobassignment) REFERENCES jobassignment(keyjobassignment) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: fkey_syslog_host; Type: FK CONSTRAINT; Schema: public; Owner: farmer
--

ALTER TABLE ONLY syslog
    ADD CONSTRAINT fkey_syslog_host FOREIGN KEY (fkeyhost) REFERENCES host(keyhost) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: fkey_syslog_realm; Type: FK CONSTRAINT; Schema: public; Owner: farmer
--

ALTER TABLE ONLY syslog
    ADD CONSTRAINT fkey_syslog_realm FOREIGN KEY (fkeysyslogrealm) REFERENCES syslogrealm(keysyslogrealm) ON UPDATE CASCADE ON DELETE RESTRICT;


--
-- Name: fkey_syslog_severity; Type: FK CONSTRAINT; Schema: public; Owner: farmer
--

ALTER TABLE ONLY syslog
    ADD CONSTRAINT fkey_syslog_severity FOREIGN KEY (fkeysyslogseverity) REFERENCES syslogseverity(keysyslogseverity) ON UPDATE CASCADE ON DELETE RESTRICT;


--
-- Name: jobtask_fkey_host; Type: FK CONSTRAINT; Schema: public; Owner: farmer
--

ALTER TABLE ONLY jobtask
    ADD CONSTRAINT jobtask_fkey_host FOREIGN KEY (fkeyhost) REFERENCES host(keyhost) ON UPDATE CASCADE ON DELETE SET NULL;


--
-- Name: public; Type: ACL; Schema: -; Owner: postgres
--

REVOKE ALL ON SCHEMA public FROM PUBLIC;
REVOKE ALL ON SCHEMA public FROM postgres;
GRANT ALL ON SCHEMA public TO postgres;
GRANT ALL ON SCHEMA public TO PUBLIC;
GRANT ALL ON SCHEMA public TO farmers;


--
-- Name: after_update_jobtask(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION after_update_jobtask() FROM PUBLIC;
REVOKE ALL ON FUNCTION after_update_jobtask() FROM farmer;
GRANT ALL ON FUNCTION after_update_jobtask() TO farmer;
GRANT ALL ON FUNCTION after_update_jobtask() TO PUBLIC;
GRANT ALL ON FUNCTION after_update_jobtask() TO farmers;


--
-- Name: are_ontens_complete(integer); Type: ACL; Schema: public; Owner: postgres
--

REVOKE ALL ON FUNCTION are_ontens_complete(_fkeyjob integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION are_ontens_complete(_fkeyjob integer) FROM postgres;
GRANT ALL ON FUNCTION are_ontens_complete(_fkeyjob integer) TO postgres;
GRANT ALL ON FUNCTION are_ontens_complete(_fkeyjob integer) TO PUBLIC;
GRANT ALL ON FUNCTION are_ontens_complete(_fkeyjob integer) TO farmers;


--
-- Name: are_ontens_dispatched(integer); Type: ACL; Schema: public; Owner: postgres
--

REVOKE ALL ON FUNCTION are_ontens_dispatched(_fkeyjob integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION are_ontens_dispatched(_fkeyjob integer) FROM postgres;
GRANT ALL ON FUNCTION are_ontens_dispatched(_fkeyjob integer) TO postgres;
GRANT ALL ON FUNCTION are_ontens_dispatched(_fkeyjob integer) TO PUBLIC;
GRANT ALL ON FUNCTION are_ontens_dispatched(_fkeyjob integer) TO farmers;


--
-- Name: assign_single_host(integer, integer, integer[]); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION assign_single_host(_keyjob integer, _keyhost integer, _tasks integer[]) FROM PUBLIC;
REVOKE ALL ON FUNCTION assign_single_host(_keyjob integer, _keyhost integer, _tasks integer[]) FROM farmer;
GRANT ALL ON FUNCTION assign_single_host(_keyjob integer, _keyhost integer, _tasks integer[]) TO farmer;
GRANT ALL ON FUNCTION assign_single_host(_keyjob integer, _keyhost integer, _tasks integer[]) TO PUBLIC;
GRANT ALL ON FUNCTION assign_single_host(_keyjob integer, _keyhost integer, _tasks integer[]) TO farmers;


--
-- Name: assign_single_host(integer, integer, integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION assign_single_host(_keyjob integer, _keyhost integer, _packetsize integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION assign_single_host(_keyjob integer, _keyhost integer, _packetsize integer) FROM farmer;
GRANT ALL ON FUNCTION assign_single_host(_keyjob integer, _keyhost integer, _packetsize integer) TO farmer;
GRANT ALL ON FUNCTION assign_single_host(_keyjob integer, _keyhost integer, _packetsize integer) TO PUBLIC;
GRANT ALL ON FUNCTION assign_single_host(_keyjob integer, _keyhost integer, _packetsize integer) TO farmers;


--
-- Name: assign_single_host_2(integer, integer, integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION assign_single_host_2(_keyjob integer, _keyhost integer, _packetsize integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION assign_single_host_2(_keyjob integer, _keyhost integer, _packetsize integer) FROM farmer;
GRANT ALL ON FUNCTION assign_single_host_2(_keyjob integer, _keyhost integer, _packetsize integer) TO farmer;
GRANT ALL ON FUNCTION assign_single_host_2(_keyjob integer, _keyhost integer, _packetsize integer) TO PUBLIC;
GRANT ALL ON FUNCTION assign_single_host_2(_keyjob integer, _keyhost integer, _packetsize integer) TO farmers;


--
-- Name: assignment_status_count(integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION assignment_status_count(fkeyjobassignmentstatus integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION assignment_status_count(fkeyjobassignmentstatus integer) FROM farmer;
GRANT ALL ON FUNCTION assignment_status_count(fkeyjobassignmentstatus integer) TO farmer;
GRANT ALL ON FUNCTION assignment_status_count(fkeyjobassignmentstatus integer) TO PUBLIC;
GRANT ALL ON FUNCTION assignment_status_count(fkeyjobassignmentstatus integer) TO farmers;


--
-- Name: cancel_job_assignment(integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION cancel_job_assignment(_keyjobassignment integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION cancel_job_assignment(_keyjobassignment integer) FROM farmer;
GRANT ALL ON FUNCTION cancel_job_assignment(_keyjobassignment integer) TO farmer;
GRANT ALL ON FUNCTION cancel_job_assignment(_keyjobassignment integer) TO PUBLIC;
GRANT ALL ON FUNCTION cancel_job_assignment(_keyjobassignment integer) TO farmers;


--
-- Name: cancel_job_assignment(integer, text, text); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION cancel_job_assignment(_keyjobassignment integer, _reason text, _nextstate text) FROM PUBLIC;
REVOKE ALL ON FUNCTION cancel_job_assignment(_keyjobassignment integer, _reason text, _nextstate text) FROM farmer;
GRANT ALL ON FUNCTION cancel_job_assignment(_keyjobassignment integer, _reason text, _nextstate text) TO farmer;
GRANT ALL ON FUNCTION cancel_job_assignment(_keyjobassignment integer, _reason text, _nextstate text) TO PUBLIC;
GRANT ALL ON FUNCTION cancel_job_assignment(_keyjobassignment integer, _reason text, _nextstate text) TO farmers;


--
-- Name: cancel_job_assignments(integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION cancel_job_assignments(_keyjob integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION cancel_job_assignments(_keyjob integer) FROM farmer;
GRANT ALL ON FUNCTION cancel_job_assignments(_keyjob integer) TO farmer;
GRANT ALL ON FUNCTION cancel_job_assignments(_keyjob integer) TO PUBLIC;
GRANT ALL ON FUNCTION cancel_job_assignments(_keyjob integer) TO farmers;


--
-- Name: cancel_job_assignments(integer, text, text); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION cancel_job_assignments(_keyjob integer, _reason text, _nextstate text) FROM PUBLIC;
REVOKE ALL ON FUNCTION cancel_job_assignments(_keyjob integer, _reason text, _nextstate text) FROM farmer;
GRANT ALL ON FUNCTION cancel_job_assignments(_keyjob integer, _reason text, _nextstate text) TO farmer;
GRANT ALL ON FUNCTION cancel_job_assignments(_keyjob integer, _reason text, _nextstate text) TO PUBLIC;
GRANT ALL ON FUNCTION cancel_job_assignments(_keyjob integer, _reason text, _nextstate text) TO farmers;


--
-- Name: epoch_to_timestamp(double precision); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION epoch_to_timestamp(double precision) FROM PUBLIC;
REVOKE ALL ON FUNCTION epoch_to_timestamp(double precision) FROM farmer;
GRANT ALL ON FUNCTION epoch_to_timestamp(double precision) TO farmer;
GRANT ALL ON FUNCTION epoch_to_timestamp(double precision) TO PUBLIC;
GRANT ALL ON FUNCTION epoch_to_timestamp(double precision) TO farmers;


--
-- Name: fix_jobstatus(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION fix_jobstatus() FROM PUBLIC;
REVOKE ALL ON FUNCTION fix_jobstatus() FROM farmer;
GRANT ALL ON FUNCTION fix_jobstatus() TO farmer;
GRANT ALL ON FUNCTION fix_jobstatus() TO PUBLIC;
GRANT ALL ON FUNCTION fix_jobstatus() TO farmers;


--
-- Name: get_continuous_tasks(integer, integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION get_continuous_tasks(_fkeyjob integer, max_tasks integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION get_continuous_tasks(_fkeyjob integer, max_tasks integer) FROM farmer;
GRANT ALL ON FUNCTION get_continuous_tasks(_fkeyjob integer, max_tasks integer) TO farmer;
GRANT ALL ON FUNCTION get_continuous_tasks(_fkeyjob integer, max_tasks integer) TO PUBLIC;
GRANT ALL ON FUNCTION get_continuous_tasks(_fkeyjob integer, max_tasks integer) TO farmers;


--
-- Name: get_iterative_tasks(integer, integer, integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION get_iterative_tasks(_fkeyjob integer, max_tasks integer, stripe integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION get_iterative_tasks(_fkeyjob integer, max_tasks integer, stripe integer) FROM farmer;
GRANT ALL ON FUNCTION get_iterative_tasks(_fkeyjob integer, max_tasks integer, stripe integer) TO farmer;
GRANT ALL ON FUNCTION get_iterative_tasks(_fkeyjob integer, max_tasks integer, stripe integer) TO PUBLIC;
GRANT ALL ON FUNCTION get_iterative_tasks(_fkeyjob integer, max_tasks integer, stripe integer) TO farmers;


--
-- Name: get_iterative_tasks_2(integer, integer, integer, boolean); Type: ACL; Schema: public; Owner: postgres
--

REVOKE ALL ON FUNCTION get_iterative_tasks_2(_fkeyjob integer, max_tasks integer, stripe integer, strict_stripe boolean) FROM PUBLIC;
REVOKE ALL ON FUNCTION get_iterative_tasks_2(_fkeyjob integer, max_tasks integer, stripe integer, strict_stripe boolean) FROM postgres;
GRANT ALL ON FUNCTION get_iterative_tasks_2(_fkeyjob integer, max_tasks integer, stripe integer, strict_stripe boolean) TO postgres;
GRANT ALL ON FUNCTION get_iterative_tasks_2(_fkeyjob integer, max_tasks integer, stripe integer, strict_stripe boolean) TO PUBLIC;
GRANT ALL ON FUNCTION get_iterative_tasks_2(_fkeyjob integer, max_tasks integer, stripe integer, strict_stripe boolean) TO farmers;


--
-- Name: get_iterative_tasks_debug(integer, integer, integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION get_iterative_tasks_debug(_fkeyjob integer, max_tasks integer, stripe integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION get_iterative_tasks_debug(_fkeyjob integer, max_tasks integer, stripe integer) FROM farmer;
GRANT ALL ON FUNCTION get_iterative_tasks_debug(_fkeyjob integer, max_tasks integer, stripe integer) TO farmer;
GRANT ALL ON FUNCTION get_iterative_tasks_debug(_fkeyjob integer, max_tasks integer, stripe integer) TO PUBLIC;
GRANT ALL ON FUNCTION get_iterative_tasks_debug(_fkeyjob integer, max_tasks integer, stripe integer) TO farmers;


--
-- Name: get_job_efficiency(integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION get_job_efficiency(_keyhost integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION get_job_efficiency(_keyhost integer) FROM farmer;
GRANT ALL ON FUNCTION get_job_efficiency(_keyhost integer) TO farmer;
GRANT ALL ON FUNCTION get_job_efficiency(_keyhost integer) TO PUBLIC;
GRANT ALL ON FUNCTION get_job_efficiency(_keyhost integer) TO farmers;


--
-- Name: get_pass_preseed(text, text); Type: ACL; Schema: public; Owner: postgres
--

REVOKE ALL ON FUNCTION get_pass_preseed(_shot text, _pass text) FROM PUBLIC;
REVOKE ALL ON FUNCTION get_pass_preseed(_shot text, _pass text) FROM postgres;
GRANT ALL ON FUNCTION get_pass_preseed(_shot text, _pass text) TO postgres;
GRANT ALL ON FUNCTION get_pass_preseed(_shot text, _pass text) TO PUBLIC;
GRANT ALL ON FUNCTION get_pass_preseed(_shot text, _pass text) TO farmers;


--
-- Name: get_wayward_hosts(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION get_wayward_hosts() FROM PUBLIC;
REVOKE ALL ON FUNCTION get_wayward_hosts() FROM farmer;
GRANT ALL ON FUNCTION get_wayward_hosts() TO farmer;
GRANT ALL ON FUNCTION get_wayward_hosts() TO PUBLIC;
GRANT ALL ON FUNCTION get_wayward_hosts() TO farmers;


--
-- Name: get_wayward_hosts_2(interval, interval); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION get_wayward_hosts_2(pulse_period interval, loop_time interval) FROM PUBLIC;
REVOKE ALL ON FUNCTION get_wayward_hosts_2(pulse_period interval, loop_time interval) FROM farmer;
GRANT ALL ON FUNCTION get_wayward_hosts_2(pulse_period interval, loop_time interval) TO farmer;
GRANT ALL ON FUNCTION get_wayward_hosts_2(pulse_period interval, loop_time interval) TO PUBLIC;
GRANT ALL ON FUNCTION get_wayward_hosts_2(pulse_period interval, loop_time interval) TO farmers;


--
-- Name: countercache; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE countercache FROM PUBLIC;
REVOKE ALL ON TABLE countercache FROM farmer;
GRANT ALL ON TABLE countercache TO farmer;
GRANT ALL ON TABLE countercache TO farmers;


--
-- Name: getcounterstate(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION getcounterstate() FROM PUBLIC;
REVOKE ALL ON FUNCTION getcounterstate() FROM farmer;
GRANT ALL ON FUNCTION getcounterstate() TO farmer;
GRANT ALL ON FUNCTION getcounterstate() TO PUBLIC;
GRANT ALL ON FUNCTION getcounterstate() TO farmers;


--
-- Name: hosthistory_keyhosthistory_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE hosthistory_keyhosthistory_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE hosthistory_keyhosthistory_seq FROM farmer;
GRANT ALL ON SEQUENCE hosthistory_keyhosthistory_seq TO farmer;
GRANT ALL ON SEQUENCE hosthistory_keyhosthistory_seq TO farmers;


--
-- Name: hosthistory; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE hosthistory FROM PUBLIC;
REVOKE ALL ON TABLE hosthistory FROM farmer;
GRANT ALL ON TABLE hosthistory TO farmer;
GRANT ALL ON TABLE hosthistory TO farmers;


--
-- Name: hosthistory_dynamic_query(text, text); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION hosthistory_dynamic_query(where_stmt text, order_limit text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hosthistory_dynamic_query(where_stmt text, order_limit text) FROM farmer;
GRANT ALL ON FUNCTION hosthistory_dynamic_query(where_stmt text, order_limit text) TO farmer;
GRANT ALL ON FUNCTION hosthistory_dynamic_query(where_stmt text, order_limit text) TO PUBLIC;
GRANT ALL ON FUNCTION hosthistory_dynamic_query(where_stmt text, order_limit text) TO farmers;


--
-- Name: hosthistory_overlaps_timespan(timestamp without time zone, timestamp without time zone, text, text); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION hosthistory_overlaps_timespan(time_start timestamp without time zone, time_end timestamp without time zone, where_stmt text, order_limit text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hosthistory_overlaps_timespan(time_start timestamp without time zone, time_end timestamp without time zone, where_stmt text, order_limit text) FROM farmer;
GRANT ALL ON FUNCTION hosthistory_overlaps_timespan(time_start timestamp without time zone, time_end timestamp without time zone, where_stmt text, order_limit text) TO farmer;
GRANT ALL ON FUNCTION hosthistory_overlaps_timespan(time_start timestamp without time zone, time_end timestamp without time zone, where_stmt text, order_limit text) TO PUBLIC;
GRANT ALL ON FUNCTION hosthistory_overlaps_timespan(time_start timestamp without time zone, time_end timestamp without time zone, where_stmt text, order_limit text) TO farmers;


--
-- Name: hosthistory_status_percentages(text); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION hosthistory_status_percentages(query_input text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hosthistory_status_percentages(query_input text) FROM farmer;
GRANT ALL ON FUNCTION hosthistory_status_percentages(query_input text) TO farmer;
GRANT ALL ON FUNCTION hosthistory_status_percentages(query_input text) TO PUBLIC;
GRANT ALL ON FUNCTION hosthistory_status_percentages(query_input text) TO farmers;


--
-- Name: hosthistory_status_percentages_duration_adjusted(timestamp without time zone, timestamp without time zone); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION hosthistory_status_percentages_duration_adjusted(history_start timestamp without time zone, history_end timestamp without time zone) FROM PUBLIC;
REVOKE ALL ON FUNCTION hosthistory_status_percentages_duration_adjusted(history_start timestamp without time zone, history_end timestamp without time zone) FROM farmer;
GRANT ALL ON FUNCTION hosthistory_status_percentages_duration_adjusted(history_start timestamp without time zone, history_end timestamp without time zone) TO farmer;
GRANT ALL ON FUNCTION hosthistory_status_percentages_duration_adjusted(history_start timestamp without time zone, history_end timestamp without time zone) TO PUBLIC;
GRANT ALL ON FUNCTION hosthistory_status_percentages_duration_adjusted(history_start timestamp without time zone, history_end timestamp without time zone) TO farmers;


--
-- Name: hosthistory_status_summary(text); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION hosthistory_status_summary(query_input text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hosthistory_status_summary(query_input text) FROM farmer;
GRANT ALL ON FUNCTION hosthistory_status_summary(query_input text) TO farmer;
GRANT ALL ON FUNCTION hosthistory_status_summary(query_input text) TO PUBLIC;
GRANT ALL ON FUNCTION hosthistory_status_summary(query_input text) TO farmers;


--
-- Name: hosthistory_status_summary_duration_adjusted(timestamp without time zone, timestamp without time zone); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION hosthistory_status_summary_duration_adjusted(history_start timestamp without time zone, history_end timestamp without time zone) FROM PUBLIC;
REVOKE ALL ON FUNCTION hosthistory_status_summary_duration_adjusted(history_start timestamp without time zone, history_end timestamp without time zone) FROM farmer;
GRANT ALL ON FUNCTION hosthistory_status_summary_duration_adjusted(history_start timestamp without time zone, history_end timestamp without time zone) TO farmer;
GRANT ALL ON FUNCTION hosthistory_status_summary_duration_adjusted(history_start timestamp without time zone, history_end timestamp without time zone) TO PUBLIC;
GRANT ALL ON FUNCTION hosthistory_status_summary_duration_adjusted(history_start timestamp without time zone, history_end timestamp without time zone) TO farmers;


--
-- Name: hosthistory_timespan_duration_adjusted(timestamp without time zone, timestamp without time zone); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION hosthistory_timespan_duration_adjusted(time_start timestamp without time zone, time_end timestamp without time zone) FROM PUBLIC;
REVOKE ALL ON FUNCTION hosthistory_timespan_duration_adjusted(time_start timestamp without time zone, time_end timestamp without time zone) FROM farmer;
GRANT ALL ON FUNCTION hosthistory_timespan_duration_adjusted(time_start timestamp without time zone, time_end timestamp without time zone) TO farmer;
GRANT ALL ON FUNCTION hosthistory_timespan_duration_adjusted(time_start timestamp without time zone, time_end timestamp without time zone) TO PUBLIC;
GRANT ALL ON FUNCTION hosthistory_timespan_duration_adjusted(time_start timestamp without time zone, time_end timestamp without time zone) TO farmers;


--
-- Name: hosthistory_timespan_duration_adjusted(timestamp without time zone, timestamp without time zone, text, text); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION hosthistory_timespan_duration_adjusted(time_start timestamp without time zone, time_end timestamp without time zone, where_stmt text, order_limit text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hosthistory_timespan_duration_adjusted(time_start timestamp without time zone, time_end timestamp without time zone, where_stmt text, order_limit text) FROM farmer;
GRANT ALL ON FUNCTION hosthistory_timespan_duration_adjusted(time_start timestamp without time zone, time_end timestamp without time zone, where_stmt text, order_limit text) TO farmer;
GRANT ALL ON FUNCTION hosthistory_timespan_duration_adjusted(time_start timestamp without time zone, time_end timestamp without time zone, where_stmt text, order_limit text) TO PUBLIC;
GRANT ALL ON FUNCTION hosthistory_timespan_duration_adjusted(time_start timestamp without time zone, time_end timestamp without time zone, where_stmt text, order_limit text) TO farmers;


--
-- Name: hosthistory_user_slave_summary(timestamp without time zone, timestamp without time zone); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION hosthistory_user_slave_summary(history_start timestamp without time zone, history_end timestamp without time zone) FROM PUBLIC;
REVOKE ALL ON FUNCTION hosthistory_user_slave_summary(history_start timestamp without time zone, history_end timestamp without time zone) FROM farmer;
GRANT ALL ON FUNCTION hosthistory_user_slave_summary(history_start timestamp without time zone, history_end timestamp without time zone) TO farmer;
GRANT ALL ON FUNCTION hosthistory_user_slave_summary(history_start timestamp without time zone, history_end timestamp without time zone) TO PUBLIC;
GRANT ALL ON FUNCTION hosthistory_user_slave_summary(history_start timestamp without time zone, history_end timestamp without time zone) TO farmers;


--
-- Name: hoststatus_update(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION hoststatus_update() FROM PUBLIC;
REVOKE ALL ON FUNCTION hoststatus_update() FROM farmer;
GRANT ALL ON FUNCTION hoststatus_update() TO farmer;
GRANT ALL ON FUNCTION hoststatus_update() TO PUBLIC;
GRANT ALL ON FUNCTION hoststatus_update() TO farmers;


--
-- Name: increment_loadavgadjust(integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION increment_loadavgadjust(_fkeyhost integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION increment_loadavgadjust(_fkeyhost integer) FROM farmer;
GRANT ALL ON FUNCTION increment_loadavgadjust(_fkeyhost integer) TO farmer;
GRANT ALL ON FUNCTION increment_loadavgadjust(_fkeyhost integer) TO PUBLIC;
GRANT ALL ON FUNCTION increment_loadavgadjust(_fkeyhost integer) TO farmers;


--
-- Name: insert_jobtask(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION insert_jobtask() FROM PUBLIC;
REVOKE ALL ON FUNCTION insert_jobtask() FROM farmer;
GRANT ALL ON FUNCTION insert_jobtask() TO farmer;
GRANT ALL ON FUNCTION insert_jobtask() TO PUBLIC;
GRANT ALL ON FUNCTION insert_jobtask() TO farmers;


--
-- Name: interval_divide(interval, interval); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION interval_divide(numerator interval, denominator interval) FROM PUBLIC;
REVOKE ALL ON FUNCTION interval_divide(numerator interval, denominator interval) FROM farmer;
GRANT ALL ON FUNCTION interval_divide(numerator interval, denominator interval) TO farmer;
GRANT ALL ON FUNCTION interval_divide(numerator interval, denominator interval) TO PUBLIC;
GRANT ALL ON FUNCTION interval_divide(numerator interval, denominator interval) TO farmers;


--
-- Name: job_delete(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION job_delete() FROM PUBLIC;
REVOKE ALL ON FUNCTION job_delete() FROM farmer;
GRANT ALL ON FUNCTION job_delete() TO farmer;
GRANT ALL ON FUNCTION job_delete() TO PUBLIC;
GRANT ALL ON FUNCTION job_delete() TO farmers;


--
-- Name: job_error_increment(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION job_error_increment() FROM PUBLIC;
REVOKE ALL ON FUNCTION job_error_increment() FROM farmer;
GRANT ALL ON FUNCTION job_error_increment() TO farmer;
GRANT ALL ON FUNCTION job_error_increment() TO PUBLIC;
GRANT ALL ON FUNCTION job_error_increment() TO farmers;


--
-- Name: job_error_insert(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION job_error_insert() FROM PUBLIC;
REVOKE ALL ON FUNCTION job_error_insert() FROM farmer;
GRANT ALL ON FUNCTION job_error_insert() TO farmer;
GRANT ALL ON FUNCTION job_error_insert() TO PUBLIC;
GRANT ALL ON FUNCTION job_error_insert() TO farmers;


--
-- Name: job_gatherstats(integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION job_gatherstats(_keyjob integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION job_gatherstats(_keyjob integer) FROM farmer;
GRANT ALL ON FUNCTION job_gatherstats(_keyjob integer) TO farmer;
GRANT ALL ON FUNCTION job_gatherstats(_keyjob integer) TO PUBLIC;
GRANT ALL ON FUNCTION job_gatherstats(_keyjob integer) TO farmers;


--
-- Name: job_gatherstats_2(integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION job_gatherstats_2(_keyjob integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION job_gatherstats_2(_keyjob integer) FROM farmer;
GRANT ALL ON FUNCTION job_gatherstats_2(_keyjob integer) TO farmer;
GRANT ALL ON FUNCTION job_gatherstats_2(_keyjob integer) TO PUBLIC;
GRANT ALL ON FUNCTION job_gatherstats_2(_keyjob integer) TO farmers;


--
-- Name: job_insert(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION job_insert() FROM PUBLIC;
REVOKE ALL ON FUNCTION job_insert() FROM farmer;
GRANT ALL ON FUNCTION job_insert() TO farmer;
GRANT ALL ON FUNCTION job_insert() TO PUBLIC;
GRANT ALL ON FUNCTION job_insert() TO farmers;


--
-- Name: job_update(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION job_update() FROM PUBLIC;
REVOKE ALL ON FUNCTION job_update() FROM farmer;
GRANT ALL ON FUNCTION job_update() TO farmer;
GRANT ALL ON FUNCTION job_update() TO PUBLIC;
GRANT ALL ON FUNCTION job_update() TO farmers;


--
-- Name: jobassignment_after_update(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION jobassignment_after_update() FROM PUBLIC;
REVOKE ALL ON FUNCTION jobassignment_after_update() FROM farmer;
GRANT ALL ON FUNCTION jobassignment_after_update() TO farmer;
GRANT ALL ON FUNCTION jobassignment_after_update() TO PUBLIC;
GRANT ALL ON FUNCTION jobassignment_after_update() TO farmers;


--
-- Name: jobassignment_delete(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION jobassignment_delete() FROM PUBLIC;
REVOKE ALL ON FUNCTION jobassignment_delete() FROM farmer;
GRANT ALL ON FUNCTION jobassignment_delete() TO farmer;
GRANT ALL ON FUNCTION jobassignment_delete() TO PUBLIC;
GRANT ALL ON FUNCTION jobassignment_delete() TO farmers;


--
-- Name: jobassignment_insert(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION jobassignment_insert() FROM PUBLIC;
REVOKE ALL ON FUNCTION jobassignment_insert() FROM farmer;
GRANT ALL ON FUNCTION jobassignment_insert() TO farmer;
GRANT ALL ON FUNCTION jobassignment_insert() TO PUBLIC;
GRANT ALL ON FUNCTION jobassignment_insert() TO farmers;


--
-- Name: jobassignment_update(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION jobassignment_update() FROM PUBLIC;
REVOKE ALL ON FUNCTION jobassignment_update() FROM farmer;
GRANT ALL ON FUNCTION jobassignment_update() TO farmer;
GRANT ALL ON FUNCTION jobassignment_update() TO PUBLIC;
GRANT ALL ON FUNCTION jobassignment_update() TO farmers;


--
-- Name: jobdep_delete(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION jobdep_delete() FROM PUBLIC;
REVOKE ALL ON FUNCTION jobdep_delete() FROM farmer;
GRANT ALL ON FUNCTION jobdep_delete() TO farmer;
GRANT ALL ON FUNCTION jobdep_delete() TO PUBLIC;
GRANT ALL ON FUNCTION jobdep_delete() TO farmers;


--
-- Name: jobdep_keyjobdep_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobdep_keyjobdep_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobdep_keyjobdep_seq FROM farmer;
GRANT ALL ON SEQUENCE jobdep_keyjobdep_seq TO farmer;
GRANT ALL ON SEQUENCE jobdep_keyjobdep_seq TO farmers;


--
-- Name: jobdep; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobdep FROM PUBLIC;
REVOKE ALL ON TABLE jobdep FROM farmer;
GRANT ALL ON TABLE jobdep TO farmer;
GRANT ALL ON TABLE jobdep TO farmers;


--
-- Name: jobdep_recursive(text); Type: ACL; Schema: public; Owner: postgres
--

REVOKE ALL ON FUNCTION jobdep_recursive(keylist text) FROM PUBLIC;
REVOKE ALL ON FUNCTION jobdep_recursive(keylist text) FROM postgres;
GRANT ALL ON FUNCTION jobdep_recursive(keylist text) TO postgres;
GRANT ALL ON FUNCTION jobdep_recursive(keylist text) TO PUBLIC;
GRANT ALL ON FUNCTION jobdep_recursive(keylist text) TO farmers;


--
-- Name: joberror_inc(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION joberror_inc() FROM PUBLIC;
REVOKE ALL ON FUNCTION joberror_inc() FROM farmer;
GRANT ALL ON FUNCTION joberror_inc() TO farmer;
GRANT ALL ON FUNCTION joberror_inc() TO PUBLIC;
GRANT ALL ON FUNCTION joberror_inc() TO farmers;


--
-- Name: jobtaskassignment_update(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION jobtaskassignment_update() FROM PUBLIC;
REVOKE ALL ON FUNCTION jobtaskassignment_update() FROM farmer;
GRANT ALL ON FUNCTION jobtaskassignment_update() TO farmer;
GRANT ALL ON FUNCTION jobtaskassignment_update() TO PUBLIC;
GRANT ALL ON FUNCTION jobtaskassignment_update() TO farmers;


--
-- Name: pg_stat_statements(); Type: ACL; Schema: public; Owner: postgres
--

REVOKE ALL ON FUNCTION pg_stat_statements(OUT userid oid, OUT dbid oid, OUT query text, OUT calls bigint, OUT total_time double precision, OUT rows bigint, OUT shared_blks_hit bigint, OUT shared_blks_read bigint, OUT shared_blks_written bigint, OUT local_blks_hit bigint, OUT local_blks_read bigint, OUT local_blks_written bigint, OUT temp_blks_read bigint, OUT temp_blks_written bigint) FROM PUBLIC;
REVOKE ALL ON FUNCTION pg_stat_statements(OUT userid oid, OUT dbid oid, OUT query text, OUT calls bigint, OUT total_time double precision, OUT rows bigint, OUT shared_blks_hit bigint, OUT shared_blks_read bigint, OUT shared_blks_written bigint, OUT local_blks_hit bigint, OUT local_blks_read bigint, OUT local_blks_written bigint, OUT temp_blks_read bigint, OUT temp_blks_written bigint) FROM postgres;
GRANT ALL ON FUNCTION pg_stat_statements(OUT userid oid, OUT dbid oid, OUT query text, OUT calls bigint, OUT total_time double precision, OUT rows bigint, OUT shared_blks_hit bigint, OUT shared_blks_read bigint, OUT shared_blks_written bigint, OUT local_blks_hit bigint, OUT local_blks_read bigint, OUT local_blks_written bigint, OUT temp_blks_read bigint, OUT temp_blks_written bigint) TO postgres;
GRANT ALL ON FUNCTION pg_stat_statements(OUT userid oid, OUT dbid oid, OUT query text, OUT calls bigint, OUT total_time double precision, OUT rows bigint, OUT shared_blks_hit bigint, OUT shared_blks_read bigint, OUT shared_blks_written bigint, OUT local_blks_hit bigint, OUT local_blks_read bigint, OUT local_blks_written bigint, OUT temp_blks_read bigint, OUT temp_blks_written bigint) TO PUBLIC;
GRANT ALL ON FUNCTION pg_stat_statements(OUT userid oid, OUT dbid oid, OUT query text, OUT calls bigint, OUT total_time double precision, OUT rows bigint, OUT shared_blks_hit bigint, OUT shared_blks_read bigint, OUT shared_blks_written bigint, OUT local_blks_hit bigint, OUT local_blks_read bigint, OUT local_blks_written bigint, OUT temp_blks_read bigint, OUT temp_blks_written bigint) TO farmers;


--
-- Name: pg_stat_statements_reset(); Type: ACL; Schema: public; Owner: postgres
--

REVOKE ALL ON FUNCTION pg_stat_statements_reset() FROM PUBLIC;
REVOKE ALL ON FUNCTION pg_stat_statements_reset() FROM postgres;
GRANT ALL ON FUNCTION pg_stat_statements_reset() TO postgres;
GRANT ALL ON FUNCTION pg_stat_statements_reset() TO farmers;


--
-- Name: return_slave_tasks(integer, boolean, boolean); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION return_slave_tasks(_keyhost integer, commithoststatus boolean, preassigned boolean) FROM PUBLIC;
REVOKE ALL ON FUNCTION return_slave_tasks(_keyhost integer, commithoststatus boolean, preassigned boolean) FROM farmer;
GRANT ALL ON FUNCTION return_slave_tasks(_keyhost integer, commithoststatus boolean, preassigned boolean) TO farmer;
GRANT ALL ON FUNCTION return_slave_tasks(_keyhost integer, commithoststatus boolean, preassigned boolean) TO PUBLIC;
GRANT ALL ON FUNCTION return_slave_tasks(_keyhost integer, commithoststatus boolean, preassigned boolean) TO farmers;


--
-- Name: return_slave_tasks_2(integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION return_slave_tasks_2(_keyhost integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION return_slave_tasks_2(_keyhost integer) FROM farmer;
GRANT ALL ON FUNCTION return_slave_tasks_2(_keyhost integer) TO farmer;
GRANT ALL ON FUNCTION return_slave_tasks_2(_keyhost integer) TO PUBLIC;
GRANT ALL ON FUNCTION return_slave_tasks_2(_keyhost integer) TO farmers;


--
-- Name: return_slave_tasks_3(integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION return_slave_tasks_3(_keyhost integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION return_slave_tasks_3(_keyhost integer) FROM farmer;
GRANT ALL ON FUNCTION return_slave_tasks_3(_keyhost integer) TO farmer;
GRANT ALL ON FUNCTION return_slave_tasks_3(_keyhost integer) TO PUBLIC;
GRANT ALL ON FUNCTION return_slave_tasks_3(_keyhost integer) TO farmers;


--
-- Name: sync_host_to_hoststatus(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION sync_host_to_hoststatus() FROM PUBLIC;
REVOKE ALL ON FUNCTION sync_host_to_hoststatus() FROM farmer;
GRANT ALL ON FUNCTION sync_host_to_hoststatus() TO farmer;
GRANT ALL ON FUNCTION sync_host_to_hoststatus() TO PUBLIC;
GRANT ALL ON FUNCTION sync_host_to_hoststatus() TO farmers;


--
-- Name: sync_host_to_hoststatus_trigger(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION sync_host_to_hoststatus_trigger() FROM PUBLIC;
REVOKE ALL ON FUNCTION sync_host_to_hoststatus_trigger() FROM farmer;
GRANT ALL ON FUNCTION sync_host_to_hoststatus_trigger() TO farmer;
GRANT ALL ON FUNCTION sync_host_to_hoststatus_trigger() TO PUBLIC;
GRANT ALL ON FUNCTION sync_host_to_hoststatus_trigger() TO farmers;


--
-- Name: update_host(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION update_host() FROM PUBLIC;
REVOKE ALL ON FUNCTION update_host() FROM farmer;
GRANT ALL ON FUNCTION update_host() TO farmer;
GRANT ALL ON FUNCTION update_host() TO PUBLIC;
GRANT ALL ON FUNCTION update_host() TO farmers;


--
-- Name: update_host_assignment_count(integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION update_host_assignment_count(_keyhost integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION update_host_assignment_count(_keyhost integer) FROM farmer;
GRANT ALL ON FUNCTION update_host_assignment_count(_keyhost integer) TO farmer;
GRANT ALL ON FUNCTION update_host_assignment_count(_keyhost integer) TO PUBLIC;
GRANT ALL ON FUNCTION update_host_assignment_count(_keyhost integer) TO farmers;


--
-- Name: update_hostload(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION update_hostload() FROM PUBLIC;
REVOKE ALL ON FUNCTION update_hostload() FROM farmer;
GRANT ALL ON FUNCTION update_hostload() TO farmer;
GRANT ALL ON FUNCTION update_hostload() TO PUBLIC;
GRANT ALL ON FUNCTION update_hostload() TO farmers;


--
-- Name: update_hostservice(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION update_hostservice() FROM PUBLIC;
REVOKE ALL ON FUNCTION update_hostservice() FROM farmer;
GRANT ALL ON FUNCTION update_hostservice() TO farmer;
GRANT ALL ON FUNCTION update_hostservice() TO PUBLIC;
GRANT ALL ON FUNCTION update_hostservice() TO farmers;


--
-- Name: update_job_deps(integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION update_job_deps(_keyjob integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION update_job_deps(_keyjob integer) FROM farmer;
GRANT ALL ON FUNCTION update_job_deps(_keyjob integer) TO farmer;
GRANT ALL ON FUNCTION update_job_deps(_keyjob integer) TO PUBLIC;
GRANT ALL ON FUNCTION update_job_deps(_keyjob integer) TO farmers;


--
-- Name: update_job_hard_deps(integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION update_job_hard_deps(_keyjob integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION update_job_hard_deps(_keyjob integer) FROM farmer;
GRANT ALL ON FUNCTION update_job_hard_deps(_keyjob integer) TO farmer;
GRANT ALL ON FUNCTION update_job_hard_deps(_keyjob integer) TO PUBLIC;
GRANT ALL ON FUNCTION update_job_hard_deps(_keyjob integer) TO farmers;


--
-- Name: update_job_hard_deps_2(integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION update_job_hard_deps_2(_keyjob integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION update_job_hard_deps_2(_keyjob integer) FROM farmer;
GRANT ALL ON FUNCTION update_job_hard_deps_2(_keyjob integer) TO farmer;
GRANT ALL ON FUNCTION update_job_hard_deps_2(_keyjob integer) TO PUBLIC;
GRANT ALL ON FUNCTION update_job_hard_deps_2(_keyjob integer) TO farmers;


--
-- Name: update_job_health(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION update_job_health() FROM PUBLIC;
REVOKE ALL ON FUNCTION update_job_health() FROM farmer;
GRANT ALL ON FUNCTION update_job_health() TO farmer;
GRANT ALL ON FUNCTION update_job_health() TO PUBLIC;
GRANT ALL ON FUNCTION update_job_health() TO farmers;


--
-- Name: update_job_health_by_key(integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION update_job_health_by_key(_jobkey integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION update_job_health_by_key(_jobkey integer) FROM farmer;
GRANT ALL ON FUNCTION update_job_health_by_key(_jobkey integer) TO farmer;
GRANT ALL ON FUNCTION update_job_health_by_key(_jobkey integer) TO PUBLIC;
GRANT ALL ON FUNCTION update_job_health_by_key(_jobkey integer) TO farmers;


--
-- Name: update_job_links(integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION update_job_links(_keyjob integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION update_job_links(_keyjob integer) FROM farmer;
GRANT ALL ON FUNCTION update_job_links(_keyjob integer) TO farmer;
GRANT ALL ON FUNCTION update_job_links(_keyjob integer) TO PUBLIC;
GRANT ALL ON FUNCTION update_job_links(_keyjob integer) TO farmers;


--
-- Name: update_job_other_deps(integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION update_job_other_deps(_keyjob integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION update_job_other_deps(_keyjob integer) FROM farmer;
GRANT ALL ON FUNCTION update_job_other_deps(_keyjob integer) TO farmer;
GRANT ALL ON FUNCTION update_job_other_deps(_keyjob integer) TO PUBLIC;
GRANT ALL ON FUNCTION update_job_other_deps(_keyjob integer) TO farmers;


--
-- Name: update_job_soft_deps(integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION update_job_soft_deps(_keyjob integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION update_job_soft_deps(_keyjob integer) FROM farmer;
GRANT ALL ON FUNCTION update_job_soft_deps(_keyjob integer) TO farmer;
GRANT ALL ON FUNCTION update_job_soft_deps(_keyjob integer) TO PUBLIC;
GRANT ALL ON FUNCTION update_job_soft_deps(_keyjob integer) TO farmers;


--
-- Name: update_job_stats(integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION update_job_stats(_keyjob integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION update_job_stats(_keyjob integer) FROM farmer;
GRANT ALL ON FUNCTION update_job_stats(_keyjob integer) TO farmer;
GRANT ALL ON FUNCTION update_job_stats(_keyjob integer) TO PUBLIC;
GRANT ALL ON FUNCTION update_job_stats(_keyjob integer) TO farmers;


--
-- Name: update_job_tallies(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION update_job_tallies() FROM PUBLIC;
REVOKE ALL ON FUNCTION update_job_tallies() FROM farmer;
GRANT ALL ON FUNCTION update_job_tallies() TO farmer;
GRANT ALL ON FUNCTION update_job_tallies() TO PUBLIC;
GRANT ALL ON FUNCTION update_job_tallies() TO farmers;


--
-- Name: update_job_task_counts(integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION update_job_task_counts(_keyjob integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION update_job_task_counts(_keyjob integer) FROM farmer;
GRANT ALL ON FUNCTION update_job_task_counts(_keyjob integer) TO farmer;
GRANT ALL ON FUNCTION update_job_task_counts(_keyjob integer) TO PUBLIC;
GRANT ALL ON FUNCTION update_job_task_counts(_keyjob integer) TO farmers;


--
-- Name: update_job_task_counts_2(integer); Type: ACL; Schema: public; Owner: postgres
--

REVOKE ALL ON FUNCTION update_job_task_counts_2(_keyjob integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION update_job_task_counts_2(_keyjob integer) FROM postgres;
GRANT ALL ON FUNCTION update_job_task_counts_2(_keyjob integer) TO postgres;
GRANT ALL ON FUNCTION update_job_task_counts_2(_keyjob integer) TO PUBLIC;
GRANT ALL ON FUNCTION update_job_task_counts_2(_keyjob integer) TO farmers;


--
-- Name: update_jobtask_counts(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION update_jobtask_counts() FROM PUBLIC;
REVOKE ALL ON FUNCTION update_jobtask_counts() FROM farmer;
GRANT ALL ON FUNCTION update_jobtask_counts() TO farmer;
GRANT ALL ON FUNCTION update_jobtask_counts() TO PUBLIC;
GRANT ALL ON FUNCTION update_jobtask_counts() TO farmers;


--
-- Name: update_laststatuschange(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION update_laststatuschange() FROM PUBLIC;
REVOKE ALL ON FUNCTION update_laststatuschange() FROM farmer;
GRANT ALL ON FUNCTION update_laststatuschange() TO farmer;
GRANT ALL ON FUNCTION update_laststatuschange() TO PUBLIC;
GRANT ALL ON FUNCTION update_laststatuschange() TO farmers;


--
-- Name: update_project_tempo(); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION update_project_tempo() FROM PUBLIC;
REVOKE ALL ON FUNCTION update_project_tempo() FROM farmer;
GRANT ALL ON FUNCTION update_project_tempo() TO farmer;
GRANT ALL ON FUNCTION update_project_tempo() TO PUBLIC;
GRANT ALL ON FUNCTION update_project_tempo() TO farmers;


--
-- Name: job_keyjob_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE job_keyjob_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE job_keyjob_seq FROM farmer;
GRANT ALL ON SEQUENCE job_keyjob_seq TO farmer;
GRANT ALL ON SEQUENCE job_keyjob_seq TO farmers;


--
-- Name: job; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE job FROM PUBLIC;
REVOKE ALL ON TABLE job FROM farmer;
GRANT ALL ON TABLE job TO farmer;
GRANT ALL ON TABLE job TO farmers;


--
-- Name: update_single_job_health(job); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION update_single_job_health(j job) FROM PUBLIC;
REVOKE ALL ON FUNCTION update_single_job_health(j job) FROM farmer;
GRANT ALL ON FUNCTION update_single_job_health(j job) TO farmer;
GRANT ALL ON FUNCTION update_single_job_health(j job) TO PUBLIC;
GRANT ALL ON FUNCTION update_single_job_health(j job) TO farmers;


--
-- Name: updatejoblicensecounts(integer, integer); Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON FUNCTION updatejoblicensecounts(_fkeyjob integer, countchange integer) FROM PUBLIC;
REVOKE ALL ON FUNCTION updatejoblicensecounts(_fkeyjob integer, countchange integer) FROM farmer;
GRANT ALL ON FUNCTION updatejoblicensecounts(_fkeyjob integer, countchange integer) TO farmer;
GRANT ALL ON FUNCTION updatejoblicensecounts(_fkeyjob integer, countchange integer) TO PUBLIC;
GRANT ALL ON FUNCTION updatejoblicensecounts(_fkeyjob integer, countchange integer) TO farmers;


--
-- Name: host_keyhost_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE host_keyhost_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE host_keyhost_seq FROM farmer;
GRANT ALL ON SEQUENCE host_keyhost_seq TO farmer;
GRANT ALL ON SEQUENCE host_keyhost_seq TO farmers;


--
-- Name: host; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE host FROM PUBLIC;
REVOKE ALL ON TABLE host FROM farmer;
GRANT ALL ON TABLE host TO farmer;
GRANT ALL ON TABLE host TO farmers;


--
-- Name: hostinterface_keyhostinterface_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE hostinterface_keyhostinterface_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE hostinterface_keyhostinterface_seq FROM farmer;
GRANT ALL ON SEQUENCE hostinterface_keyhostinterface_seq TO farmer;
GRANT ALL ON SEQUENCE hostinterface_keyhostinterface_seq TO farmers;


--
-- Name: hostinterface; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE hostinterface FROM PUBLIC;
REVOKE ALL ON TABLE hostinterface FROM farmer;
GRANT ALL ON TABLE hostinterface TO farmer;
GRANT ALL ON TABLE hostinterface TO farmers;


--
-- Name: HostInterfacesVerbose; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE "HostInterfacesVerbose" FROM PUBLIC;
REVOKE ALL ON TABLE "HostInterfacesVerbose" FROM farmer;
GRANT ALL ON TABLE "HostInterfacesVerbose" TO farmer;
GRANT ALL ON TABLE "HostInterfacesVerbose" TO farmers;


--
-- Name: jobtask_keyjobtask_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobtask_keyjobtask_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobtask_keyjobtask_seq FROM farmer;
GRANT ALL ON SEQUENCE jobtask_keyjobtask_seq TO farmer;
GRANT ALL ON SEQUENCE jobtask_keyjobtask_seq TO farmers;


--
-- Name: jobtask; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobtask FROM PUBLIC;
REVOKE ALL ON TABLE jobtask FROM farmer;
GRANT ALL ON TABLE jobtask TO farmer;
GRANT ALL ON TABLE jobtask TO farmers;


--
-- Name: jobtype_keyjobtype_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobtype_keyjobtype_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobtype_keyjobtype_seq FROM farmer;
GRANT ALL ON SEQUENCE jobtype_keyjobtype_seq TO farmer;
GRANT ALL ON SEQUENCE jobtype_keyjobtype_seq TO farmers;


--
-- Name: jobtype; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobtype FROM PUBLIC;
REVOKE ALL ON TABLE jobtype FROM farmer;
GRANT ALL ON TABLE jobtype TO farmer;
GRANT ALL ON TABLE jobtype TO farmers;


--
-- Name: StatsByType; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE "StatsByType" FROM PUBLIC;
REVOKE ALL ON TABLE "StatsByType" FROM farmer;
GRANT ALL ON TABLE "StatsByType" TO farmer;
GRANT ALL ON TABLE "StatsByType" TO farmers;


--
-- Name: abdownloadstat_keyabdownloadstat_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE abdownloadstat_keyabdownloadstat_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE abdownloadstat_keyabdownloadstat_seq FROM farmer;
GRANT ALL ON SEQUENCE abdownloadstat_keyabdownloadstat_seq TO farmer;
GRANT ALL ON SEQUENCE abdownloadstat_keyabdownloadstat_seq TO farmers;


--
-- Name: abdownloadstat; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE abdownloadstat FROM PUBLIC;
REVOKE ALL ON TABLE abdownloadstat FROM farmer;
GRANT ALL ON TABLE abdownloadstat TO farmer;
GRANT ALL ON TABLE abdownloadstat TO farmers;


--
-- Name: annotation_keyannotation_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE annotation_keyannotation_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE annotation_keyannotation_seq FROM farmer;
GRANT ALL ON SEQUENCE annotation_keyannotation_seq TO farmer;
GRANT ALL ON SEQUENCE annotation_keyannotation_seq TO farmers;


--
-- Name: annotation; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE annotation FROM PUBLIC;
REVOKE ALL ON TABLE annotation FROM farmer;
GRANT ALL ON TABLE annotation TO farmer;
GRANT ALL ON TABLE annotation TO farmers;


--
-- Name: element_keyelement_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE element_keyelement_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE element_keyelement_seq FROM farmer;
GRANT ALL ON SEQUENCE element_keyelement_seq TO farmer;
GRANT ALL ON SEQUENCE element_keyelement_seq TO farmers;


--
-- Name: element; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE element FROM PUBLIC;
REVOKE ALL ON TABLE element FROM farmer;
GRANT ALL ON TABLE element TO farmer;
GRANT ALL ON TABLE element TO farmers;


--
-- Name: asset; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE asset FROM PUBLIC;
REVOKE ALL ON TABLE asset FROM farmer;
GRANT ALL ON TABLE asset TO farmer;
GRANT ALL ON TABLE asset TO farmers;


--
-- Name: assetdep; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE assetdep FROM PUBLIC;
REVOKE ALL ON TABLE assetdep FROM farmer;
GRANT ALL ON TABLE assetdep TO farmer;
GRANT ALL ON TABLE assetdep TO farmers;


--
-- Name: assetdep_keyassetdep_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE assetdep_keyassetdep_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE assetdep_keyassetdep_seq FROM farmer;
GRANT ALL ON SEQUENCE assetdep_keyassetdep_seq TO farmer;
GRANT ALL ON SEQUENCE assetdep_keyassetdep_seq TO farmers;


--
-- Name: assetgroup; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE assetgroup FROM PUBLIC;
REVOKE ALL ON TABLE assetgroup FROM farmer;
GRANT ALL ON TABLE assetgroup TO farmer;
GRANT ALL ON TABLE assetgroup TO farmers;


--
-- Name: assetprop; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE assetprop FROM PUBLIC;
REVOKE ALL ON TABLE assetprop FROM farmer;
GRANT ALL ON TABLE assetprop TO farmer;
GRANT ALL ON TABLE assetprop TO farmers;


--
-- Name: assetprop_keyassetprop_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE assetprop_keyassetprop_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE assetprop_keyassetprop_seq FROM farmer;
GRANT ALL ON SEQUENCE assetprop_keyassetprop_seq TO farmer;
GRANT ALL ON SEQUENCE assetprop_keyassetprop_seq TO farmers;


--
-- Name: assetproperty_keyassetproperty_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE assetproperty_keyassetproperty_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE assetproperty_keyassetproperty_seq FROM farmer;
GRANT ALL ON SEQUENCE assetproperty_keyassetproperty_seq TO farmer;
GRANT ALL ON SEQUENCE assetproperty_keyassetproperty_seq TO farmers;


--
-- Name: assetproperty; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE assetproperty FROM PUBLIC;
REVOKE ALL ON TABLE assetproperty FROM farmer;
GRANT ALL ON TABLE assetproperty TO farmer;
GRANT ALL ON TABLE assetproperty TO farmers;


--
-- Name: assetproptype; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE assetproptype FROM PUBLIC;
REVOKE ALL ON TABLE assetproptype FROM farmer;
GRANT ALL ON TABLE assetproptype TO farmer;
GRANT ALL ON TABLE assetproptype TO farmers;


--
-- Name: assetproptype_keyassetproptype_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE assetproptype_keyassetproptype_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE assetproptype_keyassetproptype_seq FROM farmer;
GRANT ALL ON SEQUENCE assetproptype_keyassetproptype_seq TO farmer;
GRANT ALL ON SEQUENCE assetproptype_keyassetproptype_seq TO farmers;


--
-- Name: assetset_keyassetset_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE assetset_keyassetset_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE assetset_keyassetset_seq FROM farmer;
GRANT ALL ON SEQUENCE assetset_keyassetset_seq TO farmer;
GRANT ALL ON SEQUENCE assetset_keyassetset_seq TO farmers;


--
-- Name: assetset; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE assetset FROM PUBLIC;
REVOKE ALL ON TABLE assetset FROM farmer;
GRANT ALL ON TABLE assetset TO farmer;
GRANT ALL ON TABLE assetset TO farmers;


--
-- Name: assetsetitem_keyassetsetitem_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE assetsetitem_keyassetsetitem_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE assetsetitem_keyassetsetitem_seq FROM farmer;
GRANT ALL ON SEQUENCE assetsetitem_keyassetsetitem_seq TO farmer;
GRANT ALL ON SEQUENCE assetsetitem_keyassetsetitem_seq TO farmers;


--
-- Name: assetsetitem; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE assetsetitem FROM PUBLIC;
REVOKE ALL ON TABLE assetsetitem FROM farmer;
GRANT ALL ON TABLE assetsetitem TO farmer;
GRANT ALL ON TABLE assetsetitem TO farmers;


--
-- Name: assettemplate_keyassettemplate_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE assettemplate_keyassettemplate_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE assettemplate_keyassettemplate_seq FROM farmer;
GRANT ALL ON SEQUENCE assettemplate_keyassettemplate_seq TO farmer;
GRANT ALL ON SEQUENCE assettemplate_keyassettemplate_seq TO farmers;


--
-- Name: assettemplate; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE assettemplate FROM PUBLIC;
REVOKE ALL ON TABLE assettemplate FROM farmer;
GRANT ALL ON TABLE assettemplate TO farmer;
GRANT ALL ON TABLE assettemplate TO farmers;


--
-- Name: assettype_keyassettype_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE assettype_keyassettype_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE assettype_keyassettype_seq FROM farmer;
GRANT ALL ON SEQUENCE assettype_keyassettype_seq TO farmer;
GRANT ALL ON SEQUENCE assettype_keyassettype_seq TO farmers;


--
-- Name: assettype; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE assettype FROM PUBLIC;
REVOKE ALL ON TABLE assettype FROM farmer;
GRANT ALL ON TABLE assettype TO farmer;
GRANT ALL ON TABLE assettype TO farmers;


--
-- Name: attachment_keyattachment_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE attachment_keyattachment_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE attachment_keyattachment_seq FROM farmer;
GRANT ALL ON SEQUENCE attachment_keyattachment_seq TO farmer;
GRANT ALL ON SEQUENCE attachment_keyattachment_seq TO farmers;


--
-- Name: attachment; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE attachment FROM PUBLIC;
REVOKE ALL ON TABLE attachment FROM farmer;
GRANT ALL ON TABLE attachment TO farmer;
GRANT ALL ON TABLE attachment TO farmers;


--
-- Name: attachmenttype_keyattachmenttype_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE attachmenttype_keyattachmenttype_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE attachmenttype_keyattachmenttype_seq FROM farmer;
GRANT ALL ON SEQUENCE attachmenttype_keyattachmenttype_seq TO farmer;
GRANT ALL ON SEQUENCE attachmenttype_keyattachmenttype_seq TO farmers;


--
-- Name: attachmenttype; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE attachmenttype FROM PUBLIC;
REVOKE ALL ON TABLE attachmenttype FROM farmer;
GRANT ALL ON TABLE attachmenttype TO farmer;
GRANT ALL ON TABLE attachmenttype TO farmers;


--
-- Name: calendar_keycalendar_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE calendar_keycalendar_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE calendar_keycalendar_seq FROM farmer;
GRANT ALL ON SEQUENCE calendar_keycalendar_seq TO farmer;
GRANT ALL ON SEQUENCE calendar_keycalendar_seq TO farmers;


--
-- Name: calendar; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE calendar FROM PUBLIC;
REVOKE ALL ON TABLE calendar FROM farmer;
GRANT ALL ON TABLE calendar TO farmer;
GRANT ALL ON TABLE calendar TO farmers;


--
-- Name: calendarcategory_keycalendarcategory_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE calendarcategory_keycalendarcategory_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE calendarcategory_keycalendarcategory_seq FROM farmer;
GRANT ALL ON SEQUENCE calendarcategory_keycalendarcategory_seq TO farmer;
GRANT ALL ON SEQUENCE calendarcategory_keycalendarcategory_seq TO farmers;


--
-- Name: calendarcategory; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE calendarcategory FROM PUBLIC;
REVOKE ALL ON TABLE calendarcategory FROM farmer;
GRANT ALL ON TABLE calendarcategory TO farmer;
GRANT ALL ON TABLE calendarcategory TO farmers;


--
-- Name: checklistitem_keychecklistitem_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE checklistitem_keychecklistitem_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE checklistitem_keychecklistitem_seq FROM farmer;
GRANT ALL ON SEQUENCE checklistitem_keychecklistitem_seq TO farmer;
GRANT ALL ON SEQUENCE checklistitem_keychecklistitem_seq TO farmers;


--
-- Name: checklistitem; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE checklistitem FROM PUBLIC;
REVOKE ALL ON TABLE checklistitem FROM farmer;
GRANT ALL ON TABLE checklistitem TO farmer;
GRANT ALL ON TABLE checklistitem TO farmers;


--
-- Name: checkliststatus_keycheckliststatus_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE checkliststatus_keycheckliststatus_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE checkliststatus_keycheckliststatus_seq FROM farmer;
GRANT ALL ON SEQUENCE checkliststatus_keycheckliststatus_seq TO farmer;
GRANT ALL ON SEQUENCE checkliststatus_keycheckliststatus_seq TO farmers;


--
-- Name: checkliststatus; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE checkliststatus FROM PUBLIC;
REVOKE ALL ON TABLE checkliststatus FROM farmer;
GRANT ALL ON TABLE checkliststatus TO farmer;
GRANT ALL ON TABLE checkliststatus TO farmers;


--
-- Name: client_keyclient_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE client_keyclient_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE client_keyclient_seq FROM farmer;
GRANT ALL ON SEQUENCE client_keyclient_seq TO farmer;
GRANT ALL ON SEQUENCE client_keyclient_seq TO farmers;


--
-- Name: client; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE client FROM PUBLIC;
REVOKE ALL ON TABLE client FROM farmer;
GRANT ALL ON TABLE client TO farmer;
GRANT ALL ON TABLE client TO farmers;


--
-- Name: config_keyconfig_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE config_keyconfig_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE config_keyconfig_seq FROM farmer;
GRANT ALL ON SEQUENCE config_keyconfig_seq TO farmer;
GRANT ALL ON SEQUENCE config_keyconfig_seq TO farmers;


--
-- Name: config; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE config FROM PUBLIC;
REVOKE ALL ON TABLE config FROM farmer;
GRANT ALL ON TABLE config TO farmer;
GRANT ALL ON TABLE config TO farmers;


--
-- Name: darwinweight; Type: ACL; Schema: public; Owner: farmers
--

REVOKE ALL ON TABLE darwinweight FROM PUBLIC;
REVOKE ALL ON TABLE darwinweight FROM farmers;
GRANT ALL ON TABLE darwinweight TO farmers;


--
-- Name: darwinweight_keydarwinscore_seq; Type: ACL; Schema: public; Owner: farmers
--

REVOKE ALL ON SEQUENCE darwinweight_keydarwinscore_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE darwinweight_keydarwinscore_seq FROM farmers;
GRANT ALL ON SEQUENCE darwinweight_keydarwinscore_seq TO farmers;


--
-- Name: delivery; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE delivery FROM PUBLIC;
REVOKE ALL ON TABLE delivery FROM farmer;
GRANT ALL ON TABLE delivery TO farmer;
GRANT ALL ON TABLE delivery TO farmers;


--
-- Name: deliveryelement_keydeliveryshot_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE deliveryelement_keydeliveryshot_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE deliveryelement_keydeliveryshot_seq FROM farmer;
GRANT ALL ON SEQUENCE deliveryelement_keydeliveryshot_seq TO farmer;
GRANT ALL ON SEQUENCE deliveryelement_keydeliveryshot_seq TO farmers;


--
-- Name: deliveryelement; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE deliveryelement FROM PUBLIC;
REVOKE ALL ON TABLE deliveryelement FROM farmer;
GRANT ALL ON TABLE deliveryelement TO farmer;
GRANT ALL ON TABLE deliveryelement TO farmers;


--
-- Name: demoreel_keydemoreel_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE demoreel_keydemoreel_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE demoreel_keydemoreel_seq FROM farmer;
GRANT ALL ON SEQUENCE demoreel_keydemoreel_seq TO farmer;
GRANT ALL ON SEQUENCE demoreel_keydemoreel_seq TO farmers;


--
-- Name: demoreel; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE demoreel FROM PUBLIC;
REVOKE ALL ON TABLE demoreel FROM farmer;
GRANT ALL ON TABLE demoreel TO farmer;
GRANT ALL ON TABLE demoreel TO farmers;


--
-- Name: diskimage_keydiskimage_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE diskimage_keydiskimage_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE diskimage_keydiskimage_seq FROM farmer;
GRANT ALL ON SEQUENCE diskimage_keydiskimage_seq TO farmer;
GRANT ALL ON SEQUENCE diskimage_keydiskimage_seq TO farmers;


--
-- Name: diskimage; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE diskimage FROM PUBLIC;
REVOKE ALL ON TABLE diskimage FROM farmer;
GRANT ALL ON TABLE diskimage TO farmer;
GRANT ALL ON TABLE diskimage TO farmers;


--
-- Name: dynamichostgroup_keydynamichostgroup_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE dynamichostgroup_keydynamichostgroup_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE dynamichostgroup_keydynamichostgroup_seq FROM farmer;
GRANT ALL ON SEQUENCE dynamichostgroup_keydynamichostgroup_seq TO farmer;
GRANT ALL ON SEQUENCE dynamichostgroup_keydynamichostgroup_seq TO farmers;


--
-- Name: hostgroup_keyhostgroup_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE hostgroup_keyhostgroup_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE hostgroup_keyhostgroup_seq FROM farmer;
GRANT ALL ON SEQUENCE hostgroup_keyhostgroup_seq TO farmer;
GRANT ALL ON SEQUENCE hostgroup_keyhostgroup_seq TO farmers;


--
-- Name: hostgroup; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE hostgroup FROM PUBLIC;
REVOKE ALL ON TABLE hostgroup FROM farmer;
GRANT ALL ON TABLE hostgroup TO farmer;
GRANT ALL ON TABLE hostgroup TO farmers;


--
-- Name: dynamichostgroup; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE dynamichostgroup FROM PUBLIC;
REVOKE ALL ON TABLE dynamichostgroup FROM farmer;
GRANT ALL ON TABLE dynamichostgroup TO farmer;
GRANT ALL ON TABLE dynamichostgroup TO farmers;


--
-- Name: elementdep_keyelementdep_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE elementdep_keyelementdep_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE elementdep_keyelementdep_seq FROM farmer;
GRANT ALL ON SEQUENCE elementdep_keyelementdep_seq TO farmer;
GRANT ALL ON SEQUENCE elementdep_keyelementdep_seq TO farmers;


--
-- Name: elementdep; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE elementdep FROM PUBLIC;
REVOKE ALL ON TABLE elementdep FROM farmer;
GRANT ALL ON TABLE elementdep TO farmer;
GRANT ALL ON TABLE elementdep TO farmers;


--
-- Name: elementstatus_keyelementstatus_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE elementstatus_keyelementstatus_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE elementstatus_keyelementstatus_seq FROM farmer;
GRANT ALL ON SEQUENCE elementstatus_keyelementstatus_seq TO farmer;
GRANT ALL ON SEQUENCE elementstatus_keyelementstatus_seq TO farmers;


--
-- Name: elementstatus; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE elementstatus FROM PUBLIC;
REVOKE ALL ON TABLE elementstatus FROM farmer;
GRANT ALL ON TABLE elementstatus TO farmer;
GRANT ALL ON TABLE elementstatus TO farmers;


--
-- Name: elementthread_keyelementthread_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE elementthread_keyelementthread_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE elementthread_keyelementthread_seq FROM farmer;
GRANT ALL ON SEQUENCE elementthread_keyelementthread_seq TO farmer;
GRANT ALL ON SEQUENCE elementthread_keyelementthread_seq TO farmers;


--
-- Name: elementthread; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE elementthread FROM PUBLIC;
REVOKE ALL ON TABLE elementthread FROM farmer;
GRANT ALL ON TABLE elementthread TO farmer;
GRANT ALL ON TABLE elementthread TO farmers;


--
-- Name: elementtype_keyelementtype_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE elementtype_keyelementtype_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE elementtype_keyelementtype_seq FROM farmer;
GRANT ALL ON SEQUENCE elementtype_keyelementtype_seq TO farmer;
GRANT ALL ON SEQUENCE elementtype_keyelementtype_seq TO farmers;


--
-- Name: elementtype; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE elementtype FROM PUBLIC;
REVOKE ALL ON TABLE elementtype FROM farmer;
GRANT ALL ON TABLE elementtype TO farmer;
GRANT ALL ON TABLE elementtype TO farmers;


--
-- Name: elementtypetasktype_keyelementtypetasktype_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE elementtypetasktype_keyelementtypetasktype_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE elementtypetasktype_keyelementtypetasktype_seq FROM farmer;
GRANT ALL ON SEQUENCE elementtypetasktype_keyelementtypetasktype_seq TO farmer;
GRANT ALL ON SEQUENCE elementtypetasktype_keyelementtypetasktype_seq TO farmers;


--
-- Name: elementtypetasktype; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE elementtypetasktype FROM PUBLIC;
REVOKE ALL ON TABLE elementtypetasktype FROM farmer;
GRANT ALL ON TABLE elementtypetasktype TO farmer;
GRANT ALL ON TABLE elementtypetasktype TO farmers;


--
-- Name: elementuser_keyelementuser_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE elementuser_keyelementuser_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE elementuser_keyelementuser_seq FROM farmer;
GRANT ALL ON SEQUENCE elementuser_keyelementuser_seq TO farmer;
GRANT ALL ON SEQUENCE elementuser_keyelementuser_seq TO farmers;


--
-- Name: elementuser; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE elementuser FROM PUBLIC;
REVOKE ALL ON TABLE elementuser FROM farmer;
GRANT ALL ON TABLE elementuser TO farmer;
GRANT ALL ON TABLE elementuser TO farmers;


--
-- Name: usr; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE usr FROM PUBLIC;
REVOKE ALL ON TABLE usr FROM farmer;
GRANT ALL ON TABLE usr TO farmer;
GRANT ALL ON TABLE usr TO farmers;


--
-- Name: employee; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE employee FROM PUBLIC;
REVOKE ALL ON TABLE employee FROM farmer;
GRANT ALL ON TABLE employee TO farmer;
GRANT ALL ON TABLE employee TO farmers;


--
-- Name: eventalert_keyEventAlert_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE "eventalert_keyEventAlert_seq" FROM PUBLIC;
REVOKE ALL ON SEQUENCE "eventalert_keyEventAlert_seq" FROM farmer;
GRANT ALL ON SEQUENCE "eventalert_keyEventAlert_seq" TO farmer;
GRANT ALL ON SEQUENCE "eventalert_keyEventAlert_seq" TO farmers;


--
-- Name: eventalert; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE eventalert FROM PUBLIC;
REVOKE ALL ON TABLE eventalert FROM farmer;
GRANT ALL ON TABLE eventalert TO farmer;
GRANT ALL ON TABLE eventalert TO farmers;


--
-- Name: filetemplate_keyfiletemplate_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE filetemplate_keyfiletemplate_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE filetemplate_keyfiletemplate_seq FROM farmer;
GRANT ALL ON SEQUENCE filetemplate_keyfiletemplate_seq TO farmer;
GRANT ALL ON SEQUENCE filetemplate_keyfiletemplate_seq TO farmers;


--
-- Name: filetemplate; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE filetemplate FROM PUBLIC;
REVOKE ALL ON TABLE filetemplate FROM farmer;
GRANT ALL ON TABLE filetemplate TO farmer;
GRANT ALL ON TABLE filetemplate TO farmers;


--
-- Name: filetracker_keyfiletracker_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE filetracker_keyfiletracker_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE filetracker_keyfiletracker_seq FROM farmer;
GRANT ALL ON SEQUENCE filetracker_keyfiletracker_seq TO farmer;
GRANT ALL ON SEQUENCE filetracker_keyfiletracker_seq TO farmers;


--
-- Name: filetracker; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE filetracker FROM PUBLIC;
REVOKE ALL ON TABLE filetracker FROM farmer;
GRANT ALL ON TABLE filetracker TO farmer;
GRANT ALL ON TABLE filetracker TO farmers;


--
-- Name: filetrackerdep_keyfiletrackerdep_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE filetrackerdep_keyfiletrackerdep_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE filetrackerdep_keyfiletrackerdep_seq FROM farmer;
GRANT ALL ON SEQUENCE filetrackerdep_keyfiletrackerdep_seq TO farmer;
GRANT ALL ON SEQUENCE filetrackerdep_keyfiletrackerdep_seq TO farmers;


--
-- Name: filetrackerdep; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE filetrackerdep FROM PUBLIC;
REVOKE ALL ON TABLE filetrackerdep FROM farmer;
GRANT ALL ON TABLE filetrackerdep TO farmer;
GRANT ALL ON TABLE filetrackerdep TO farmers;


--
-- Name: fileversion_keyfileversion_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE fileversion_keyfileversion_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE fileversion_keyfileversion_seq FROM farmer;
GRANT ALL ON SEQUENCE fileversion_keyfileversion_seq TO farmer;
GRANT ALL ON SEQUENCE fileversion_keyfileversion_seq TO farmers;


--
-- Name: fileversion; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE fileversion FROM PUBLIC;
REVOKE ALL ON TABLE fileversion FROM farmer;
GRANT ALL ON TABLE fileversion TO farmer;
GRANT ALL ON TABLE fileversion TO farmers;


--
-- Name: folder_keyfolder_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE folder_keyfolder_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE folder_keyfolder_seq FROM farmer;
GRANT ALL ON SEQUENCE folder_keyfolder_seq TO farmer;
GRANT ALL ON SEQUENCE folder_keyfolder_seq TO farmers;


--
-- Name: folder; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE folder FROM PUBLIC;
REVOKE ALL ON TABLE folder FROM farmer;
GRANT ALL ON TABLE folder TO farmer;
GRANT ALL ON TABLE folder TO farmers;


--
-- Name: graph_keygraph_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE graph_keygraph_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE graph_keygraph_seq FROM farmer;
GRANT ALL ON SEQUENCE graph_keygraph_seq TO farmer;
GRANT ALL ON SEQUENCE graph_keygraph_seq TO farmers;


--
-- Name: graph; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE graph FROM PUBLIC;
REVOKE ALL ON TABLE graph FROM farmer;
GRANT ALL ON TABLE graph TO farmer;
GRANT ALL ON TABLE graph TO farmers;


--
-- Name: graphds_keygraphds_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE graphds_keygraphds_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE graphds_keygraphds_seq FROM farmer;
GRANT ALL ON SEQUENCE graphds_keygraphds_seq TO farmer;
GRANT ALL ON SEQUENCE graphds_keygraphds_seq TO farmers;


--
-- Name: graphds; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE graphds FROM PUBLIC;
REVOKE ALL ON TABLE graphds FROM farmer;
GRANT ALL ON TABLE graphds TO farmer;
GRANT ALL ON TABLE graphds TO farmers;


--
-- Name: graphpage_keygraphpage_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE graphpage_keygraphpage_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE graphpage_keygraphpage_seq FROM farmer;
GRANT ALL ON SEQUENCE graphpage_keygraphpage_seq TO farmer;
GRANT ALL ON SEQUENCE graphpage_keygraphpage_seq TO farmers;


--
-- Name: graphpage; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE graphpage FROM PUBLIC;
REVOKE ALL ON TABLE graphpage FROM farmer;
GRANT ALL ON TABLE graphpage TO farmer;
GRANT ALL ON TABLE graphpage TO farmers;


--
-- Name: graphrelationship_keygraphrelationship_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE graphrelationship_keygraphrelationship_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE graphrelationship_keygraphrelationship_seq FROM farmer;
GRANT ALL ON SEQUENCE graphrelationship_keygraphrelationship_seq TO farmer;
GRANT ALL ON SEQUENCE graphrelationship_keygraphrelationship_seq TO farmers;


--
-- Name: graphrelationship; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE graphrelationship FROM PUBLIC;
REVOKE ALL ON TABLE graphrelationship FROM farmer;
GRANT ALL ON TABLE graphrelationship TO farmer;
GRANT ALL ON TABLE graphrelationship TO farmers;


--
-- Name: gridtemplate_keygridtemplate_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE gridtemplate_keygridtemplate_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE gridtemplate_keygridtemplate_seq FROM farmer;
GRANT ALL ON SEQUENCE gridtemplate_keygridtemplate_seq TO farmer;
GRANT ALL ON SEQUENCE gridtemplate_keygridtemplate_seq TO farmers;


--
-- Name: gridtemplate; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE gridtemplate FROM PUBLIC;
REVOKE ALL ON TABLE gridtemplate FROM farmer;
GRANT ALL ON TABLE gridtemplate TO farmer;
GRANT ALL ON TABLE gridtemplate TO farmers;


--
-- Name: gridtemplateitem_keygridtemplateitem_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE gridtemplateitem_keygridtemplateitem_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE gridtemplateitem_keygridtemplateitem_seq FROM farmer;
GRANT ALL ON SEQUENCE gridtemplateitem_keygridtemplateitem_seq TO farmer;
GRANT ALL ON SEQUENCE gridtemplateitem_keygridtemplateitem_seq TO farmers;


--
-- Name: gridtemplateitem; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE gridtemplateitem FROM PUBLIC;
REVOKE ALL ON TABLE gridtemplateitem FROM farmer;
GRANT ALL ON TABLE gridtemplateitem TO farmer;
GRANT ALL ON TABLE gridtemplateitem TO farmers;


--
-- Name: groupmapping_keygroupmapping_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE groupmapping_keygroupmapping_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE groupmapping_keygroupmapping_seq FROM farmer;
GRANT ALL ON SEQUENCE groupmapping_keygroupmapping_seq TO farmer;
GRANT ALL ON SEQUENCE groupmapping_keygroupmapping_seq TO farmers;


--
-- Name: groupmapping; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE groupmapping FROM PUBLIC;
REVOKE ALL ON TABLE groupmapping FROM farmer;
GRANT ALL ON TABLE groupmapping TO farmer;
GRANT ALL ON TABLE groupmapping TO farmers;


--
-- Name: grp_keygrp_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE grp_keygrp_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE grp_keygrp_seq FROM farmer;
GRANT ALL ON SEQUENCE grp_keygrp_seq TO farmer;
GRANT ALL ON SEQUENCE grp_keygrp_seq TO farmers;


--
-- Name: grp; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE grp FROM PUBLIC;
REVOKE ALL ON TABLE grp FROM farmer;
GRANT ALL ON TABLE grp TO farmer;
GRANT ALL ON TABLE grp TO farmers;


--
-- Name: gruntscript_keygruntscript_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE gruntscript_keygruntscript_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE gruntscript_keygruntscript_seq FROM farmer;
GRANT ALL ON SEQUENCE gruntscript_keygruntscript_seq TO farmer;
GRANT ALL ON SEQUENCE gruntscript_keygruntscript_seq TO farmers;


--
-- Name: gruntscript; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE gruntscript FROM PUBLIC;
REVOKE ALL ON TABLE gruntscript FROM farmer;
GRANT ALL ON TABLE gruntscript TO farmer;
GRANT ALL ON TABLE gruntscript TO farmers;


--
-- Name: history_keyhistory_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE history_keyhistory_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE history_keyhistory_seq FROM farmer;
GRANT ALL ON SEQUENCE history_keyhistory_seq TO farmer;
GRANT ALL ON SEQUENCE history_keyhistory_seq TO farmers;


--
-- Name: history; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE history FROM PUBLIC;
REVOKE ALL ON TABLE history FROM farmer;
GRANT ALL ON TABLE history TO farmer;
GRANT ALL ON TABLE history TO farmers;


--
-- Name: hostdailystat_keyhostdailystat_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE hostdailystat_keyhostdailystat_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE hostdailystat_keyhostdailystat_seq FROM farmer;
GRANT ALL ON SEQUENCE hostdailystat_keyhostdailystat_seq TO farmer;
GRANT ALL ON SEQUENCE hostdailystat_keyhostdailystat_seq TO farmers;


--
-- Name: hostdailystat; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE hostdailystat FROM PUBLIC;
REVOKE ALL ON TABLE hostdailystat FROM farmer;
GRANT ALL ON TABLE hostdailystat TO farmer;
GRANT ALL ON TABLE hostdailystat TO farmers;


--
-- Name: hostgroupitem_keyhostgroupitem_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE hostgroupitem_keyhostgroupitem_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE hostgroupitem_keyhostgroupitem_seq FROM farmer;
GRANT ALL ON SEQUENCE hostgroupitem_keyhostgroupitem_seq TO farmer;
GRANT ALL ON SEQUENCE hostgroupitem_keyhostgroupitem_seq TO farmers;


--
-- Name: hostgroupitem; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE hostgroupitem FROM PUBLIC;
REVOKE ALL ON TABLE hostgroupitem FROM farmer;
GRANT ALL ON TABLE hostgroupitem TO farmer;
GRANT ALL ON TABLE hostgroupitem TO farmers;


--
-- Name: hostinterfacetype_keyhostinterfacetype_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE hostinterfacetype_keyhostinterfacetype_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE hostinterfacetype_keyhostinterfacetype_seq FROM farmer;
GRANT ALL ON SEQUENCE hostinterfacetype_keyhostinterfacetype_seq TO farmer;
GRANT ALL ON SEQUENCE hostinterfacetype_keyhostinterfacetype_seq TO farmers;


--
-- Name: hostinterfacetype; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE hostinterfacetype FROM PUBLIC;
REVOKE ALL ON TABLE hostinterfacetype FROM farmer;
GRANT ALL ON TABLE hostinterfacetype TO farmer;
GRANT ALL ON TABLE hostinterfacetype TO farmers;


--
-- Name: hostload_keyhostload_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE hostload_keyhostload_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE hostload_keyhostload_seq FROM farmer;
GRANT ALL ON SEQUENCE hostload_keyhostload_seq TO farmer;
GRANT ALL ON SEQUENCE hostload_keyhostload_seq TO farmers;


--
-- Name: hostload; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE hostload FROM PUBLIC;
REVOKE ALL ON TABLE hostload FROM farmer;
GRANT ALL ON TABLE hostload TO farmer;
GRANT ALL ON TABLE hostload TO farmers;


--
-- Name: hostmapping_keyhostmapping_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE hostmapping_keyhostmapping_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE hostmapping_keyhostmapping_seq FROM farmer;
GRANT ALL ON SEQUENCE hostmapping_keyhostmapping_seq TO farmer;
GRANT ALL ON SEQUENCE hostmapping_keyhostmapping_seq TO farmers;


--
-- Name: hostmapping; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE hostmapping FROM PUBLIC;
REVOKE ALL ON TABLE hostmapping FROM farmer;
GRANT ALL ON TABLE hostmapping TO farmer;
GRANT ALL ON TABLE hostmapping TO farmers;


--
-- Name: hostport_keyhostport_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE hostport_keyhostport_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE hostport_keyhostport_seq FROM farmer;
GRANT ALL ON SEQUENCE hostport_keyhostport_seq TO farmer;
GRANT ALL ON SEQUENCE hostport_keyhostport_seq TO farmers;


--
-- Name: hostport; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE hostport FROM PUBLIC;
REVOKE ALL ON TABLE hostport FROM farmer;
GRANT ALL ON TABLE hostport TO farmer;
GRANT ALL ON TABLE hostport TO farmers;


--
-- Name: hostresource_keyhostresource_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE hostresource_keyhostresource_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE hostresource_keyhostresource_seq FROM farmer;
GRANT ALL ON SEQUENCE hostresource_keyhostresource_seq TO farmer;
GRANT ALL ON SEQUENCE hostresource_keyhostresource_seq TO farmers;


--
-- Name: hostresource; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE hostresource FROM PUBLIC;
REVOKE ALL ON TABLE hostresource FROM farmer;
GRANT ALL ON TABLE hostresource TO farmer;
GRANT ALL ON TABLE hostresource TO farmers;


--
-- Name: hosts_active; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE hosts_active FROM PUBLIC;
REVOKE ALL ON TABLE hosts_active FROM farmer;
GRANT ALL ON TABLE hosts_active TO farmer;
GRANT ALL ON TABLE hosts_active TO PUBLIC;
GRANT ALL ON TABLE hosts_active TO farmers;


--
-- Name: hosts_ready; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE hosts_ready FROM PUBLIC;
REVOKE ALL ON TABLE hosts_ready FROM farmer;
GRANT ALL ON TABLE hosts_ready TO farmer;
GRANT ALL ON TABLE hosts_ready TO farmers;


--
-- Name: hosts_total; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE hosts_total FROM PUBLIC;
REVOKE ALL ON TABLE hosts_total FROM farmer;
GRANT ALL ON TABLE hosts_total TO farmer;
GRANT ALL ON TABLE hosts_total TO farmers;


--
-- Name: hostservice_keyhostservice_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE hostservice_keyhostservice_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE hostservice_keyhostservice_seq FROM farmer;
GRANT ALL ON SEQUENCE hostservice_keyhostservice_seq TO farmer;
GRANT ALL ON SEQUENCE hostservice_keyhostservice_seq TO farmers;


--
-- Name: hostservice; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE hostservice FROM PUBLIC;
REVOKE ALL ON TABLE hostservice FROM farmer;
GRANT ALL ON TABLE hostservice TO farmer;
GRANT ALL ON TABLE hostservice TO farmers;


--
-- Name: hostsoftware_keyhostsoftware_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE hostsoftware_keyhostsoftware_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE hostsoftware_keyhostsoftware_seq FROM farmer;
GRANT ALL ON SEQUENCE hostsoftware_keyhostsoftware_seq TO farmer;
GRANT ALL ON SEQUENCE hostsoftware_keyhostsoftware_seq TO farmers;


--
-- Name: hostsoftware; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE hostsoftware FROM PUBLIC;
REVOKE ALL ON TABLE hostsoftware FROM farmer;
GRANT ALL ON TABLE hostsoftware TO farmer;
GRANT ALL ON TABLE hostsoftware TO farmers;


--
-- Name: hoststatus_keyhoststatus_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE hoststatus_keyhoststatus_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE hoststatus_keyhoststatus_seq FROM farmer;
GRANT ALL ON SEQUENCE hoststatus_keyhoststatus_seq TO farmer;
GRANT ALL ON SEQUENCE hoststatus_keyhoststatus_seq TO farmers;


--
-- Name: hoststatus; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE hoststatus FROM PUBLIC;
REVOKE ALL ON TABLE hoststatus FROM farmer;
GRANT ALL ON TABLE hoststatus TO farmer;
GRANT ALL ON TABLE hoststatus TO farmers;


--
-- Name: job3delight; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE job3delight FROM PUBLIC;
REVOKE ALL ON TABLE job3delight FROM farmer;
GRANT ALL ON TABLE job3delight TO farmer;
GRANT ALL ON TABLE job3delight TO farmers;


--
-- Name: jobassignment_old; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobassignment_old FROM PUBLIC;
REVOKE ALL ON TABLE jobassignment_old FROM farmer;
GRANT ALL ON TABLE jobassignment_old TO farmer;
GRANT ALL ON TABLE jobassignment_old TO farmers;


--
-- Name: jobassignment_keyjobassignment_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobassignment_keyjobassignment_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobassignment_keyjobassignment_seq FROM farmer;
GRANT ALL ON SEQUENCE jobassignment_keyjobassignment_seq TO farmer;
GRANT ALL ON SEQUENCE jobassignment_keyjobassignment_seq TO farmers;


--
-- Name: jobassignment; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobassignment FROM PUBLIC;
REVOKE ALL ON TABLE jobassignment FROM farmer;
GRANT ALL ON TABLE jobassignment TO farmer;
GRANT ALL ON TABLE jobassignment TO PUBLIC;
GRANT ALL ON TABLE jobassignment TO farmers;


--
-- Name: jobassignmentstatus; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobassignmentstatus FROM PUBLIC;
REVOKE ALL ON TABLE jobassignmentstatus FROM farmer;
GRANT ALL ON TABLE jobassignmentstatus TO farmer;
GRANT ALL ON TABLE jobassignmentstatus TO farmers;


--
-- Name: jobassignmentstatus_keyjobassignmentstatus_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobassignmentstatus_keyjobassignmentstatus_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobassignmentstatus_keyjobassignmentstatus_seq FROM farmer;
GRANT ALL ON SEQUENCE jobassignmentstatus_keyjobassignmentstatus_seq TO farmer;
GRANT ALL ON SEQUENCE jobassignmentstatus_keyjobassignmentstatus_seq TO farmers;


--
-- Name: jobbatch; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobbatch FROM PUBLIC;
REVOKE ALL ON TABLE jobbatch FROM farmer;
GRANT ALL ON TABLE jobbatch TO farmer;
GRANT ALL ON TABLE jobbatch TO farmers;


--
-- Name: jobcannedbatch_keyjobcannedbatch_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobcannedbatch_keyjobcannedbatch_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobcannedbatch_keyjobcannedbatch_seq FROM farmer;
GRANT ALL ON SEQUENCE jobcannedbatch_keyjobcannedbatch_seq TO farmer;
GRANT ALL ON SEQUENCE jobcannedbatch_keyjobcannedbatch_seq TO farmers;


--
-- Name: jobcannedbatch; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobcannedbatch FROM PUBLIC;
REVOKE ALL ON TABLE jobcannedbatch FROM farmer;
GRANT ALL ON TABLE jobcannedbatch TO farmer;
GRANT ALL ON TABLE jobcannedbatch TO farmers;


--
-- Name: jobcommandhistory_keyjobcommandhistory_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobcommandhistory_keyjobcommandhistory_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobcommandhistory_keyjobcommandhistory_seq FROM farmer;
GRANT ALL ON SEQUENCE jobcommandhistory_keyjobcommandhistory_seq TO farmer;
GRANT ALL ON SEQUENCE jobcommandhistory_keyjobcommandhistory_seq TO farmers;


--
-- Name: jobcommandhistory; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobcommandhistory FROM PUBLIC;
REVOKE ALL ON TABLE jobcommandhistory FROM farmer;
GRANT ALL ON TABLE jobcommandhistory TO farmer;
GRANT ALL ON TABLE jobcommandhistory TO farmers;


--
-- Name: jobenvironment; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobenvironment FROM PUBLIC;
REVOKE ALL ON TABLE jobenvironment FROM farmer;
GRANT ALL ON TABLE jobenvironment TO farmer;
GRANT ALL ON TABLE jobenvironment TO PUBLIC;
GRANT ALL ON TABLE jobenvironment TO farmers;


--
-- Name: jobenvironment_keyjobenvironment_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobenvironment_keyjobenvironment_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobenvironment_keyjobenvironment_seq FROM farmer;
GRANT ALL ON SEQUENCE jobenvironment_keyjobenvironment_seq TO farmer;
GRANT ALL ON SEQUENCE jobenvironment_keyjobenvironment_seq TO farmers;


--
-- Name: joberror_keyjoberror_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE joberror_keyjoberror_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE joberror_keyjoberror_seq FROM farmer;
GRANT ALL ON SEQUENCE joberror_keyjoberror_seq TO farmer;
GRANT ALL ON SEQUENCE joberror_keyjoberror_seq TO farmers;


--
-- Name: joberror; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE joberror FROM PUBLIC;
REVOKE ALL ON TABLE joberror FROM farmer;
GRANT ALL ON TABLE joberror TO farmer;
GRANT ALL ON TABLE joberror TO farmers;


--
-- Name: joberrorhandler_keyjoberrorhandler_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE joberrorhandler_keyjoberrorhandler_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE joberrorhandler_keyjoberrorhandler_seq FROM farmer;
GRANT ALL ON SEQUENCE joberrorhandler_keyjoberrorhandler_seq TO farmer;
GRANT ALL ON SEQUENCE joberrorhandler_keyjoberrorhandler_seq TO farmers;


--
-- Name: joberrorhandler; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE joberrorhandler FROM PUBLIC;
REVOKE ALL ON TABLE joberrorhandler FROM farmer;
GRANT ALL ON TABLE joberrorhandler TO farmer;
GRANT ALL ON TABLE joberrorhandler TO farmers;


--
-- Name: joberrorhandlerscript_keyjoberrorhandlerscript_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE joberrorhandlerscript_keyjoberrorhandlerscript_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE joberrorhandlerscript_keyjoberrorhandlerscript_seq FROM farmer;
GRANT ALL ON SEQUENCE joberrorhandlerscript_keyjoberrorhandlerscript_seq TO farmer;
GRANT ALL ON SEQUENCE joberrorhandlerscript_keyjoberrorhandlerscript_seq TO farmers;


--
-- Name: joberrorhandlerscript; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE joberrorhandlerscript FROM PUBLIC;
REVOKE ALL ON TABLE joberrorhandlerscript FROM farmer;
GRANT ALL ON TABLE joberrorhandlerscript TO farmer;
GRANT ALL ON TABLE joberrorhandlerscript TO farmers;


--
-- Name: jobfiltermessage; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobfiltermessage FROM PUBLIC;
REVOKE ALL ON TABLE jobfiltermessage FROM farmer;
GRANT ALL ON TABLE jobfiltermessage TO farmer;
GRANT ALL ON TABLE jobfiltermessage TO farmers;
GRANT ALL ON TABLE jobfiltermessage TO PUBLIC;


--
-- Name: jobfiltermessage_keyjobfiltermessage_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobfiltermessage_keyjobfiltermessage_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobfiltermessage_keyjobfiltermessage_seq FROM farmer;
GRANT ALL ON SEQUENCE jobfiltermessage_keyjobfiltermessage_seq TO farmer;
GRANT ALL ON SEQUENCE jobfiltermessage_keyjobfiltermessage_seq TO farmers;


--
-- Name: jobfilterset; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobfilterset FROM PUBLIC;
REVOKE ALL ON TABLE jobfilterset FROM farmer;
GRANT ALL ON TABLE jobfilterset TO farmer;
GRANT ALL ON TABLE jobfilterset TO farmers;
GRANT ALL ON TABLE jobfilterset TO PUBLIC;


--
-- Name: jobfilterset_keyjobfilterset_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobfilterset_keyjobfilterset_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobfilterset_keyjobfilterset_seq FROM farmer;
GRANT ALL ON SEQUENCE jobfilterset_keyjobfilterset_seq TO farmer;
GRANT ALL ON SEQUENCE jobfilterset_keyjobfilterset_seq TO farmers;


--
-- Name: jobfiltertype; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobfiltertype FROM PUBLIC;
REVOKE ALL ON TABLE jobfiltertype FROM farmer;
GRANT ALL ON TABLE jobfiltertype TO farmer;
GRANT ALL ON TABLE jobfiltertype TO farmers;
GRANT ALL ON TABLE jobfiltertype TO PUBLIC;


--
-- Name: jobfiltertype_keyjobfiltertype_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobfiltertype_keyjobfiltertype_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobfiltertype_keyjobfiltertype_seq FROM farmer;
GRANT ALL ON SEQUENCE jobfiltertype_keyjobfiltertype_seq TO farmer;
GRANT ALL ON SEQUENCE jobfiltertype_keyjobfiltertype_seq TO farmers;


--
-- Name: jobhistory_keyjobhistory_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobhistory_keyjobhistory_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobhistory_keyjobhistory_seq FROM farmer;
GRANT ALL ON SEQUENCE jobhistory_keyjobhistory_seq TO farmer;
GRANT ALL ON SEQUENCE jobhistory_keyjobhistory_seq TO farmers;


--
-- Name: jobhistory; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobhistory FROM PUBLIC;
REVOKE ALL ON TABLE jobhistory FROM farmer;
GRANT ALL ON TABLE jobhistory TO farmer;
GRANT ALL ON TABLE jobhistory TO farmers;


--
-- Name: jobhistorytype_keyjobhistorytype_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobhistorytype_keyjobhistorytype_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobhistorytype_keyjobhistorytype_seq FROM farmer;
GRANT ALL ON SEQUENCE jobhistorytype_keyjobhistorytype_seq TO farmer;
GRANT ALL ON SEQUENCE jobhistorytype_keyjobhistorytype_seq TO farmers;


--
-- Name: jobhistorytype; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobhistorytype FROM PUBLIC;
REVOKE ALL ON TABLE jobhistorytype FROM farmer;
GRANT ALL ON TABLE jobhistorytype TO farmer;
GRANT ALL ON TABLE jobhistorytype TO farmers;


--
-- Name: jobmantra100; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobmantra100 FROM PUBLIC;
REVOKE ALL ON TABLE jobmantra100 FROM farmer;
GRANT ALL ON TABLE jobmantra100 TO farmer;
GRANT ALL ON TABLE jobmantra100 TO farmers;


--
-- Name: jobmantra100_keyjobmantra100_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobmantra100_keyjobmantra100_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobmantra100_keyjobmantra100_seq FROM farmer;
GRANT ALL ON SEQUENCE jobmantra100_keyjobmantra100_seq TO farmer;
GRANT ALL ON SEQUENCE jobmantra100_keyjobmantra100_seq TO farmers;


--
-- Name: jobmantra95; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobmantra95 FROM PUBLIC;
REVOKE ALL ON TABLE jobmantra95 FROM farmer;
GRANT ALL ON TABLE jobmantra95 TO farmer;
GRANT ALL ON TABLE jobmantra95 TO farmers;


--
-- Name: jobmantra95_keyjobmantra95_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobmantra95_keyjobmantra95_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobmantra95_keyjobmantra95_seq FROM farmer;
GRANT ALL ON SEQUENCE jobmantra95_keyjobmantra95_seq TO farmer;
GRANT ALL ON SEQUENCE jobmantra95_keyjobmantra95_seq TO farmers;


--
-- Name: jobmax; Type: ACL; Schema: public; Owner: postgres
--

REVOKE ALL ON TABLE jobmax FROM PUBLIC;
REVOKE ALL ON TABLE jobmax FROM postgres;
GRANT ALL ON TABLE jobmax TO postgres;
GRANT ALL ON TABLE jobmax TO farmers;


--
-- Name: jobmax10; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobmax10 FROM PUBLIC;
REVOKE ALL ON TABLE jobmax10 FROM farmer;
GRANT ALL ON TABLE jobmax10 TO farmer;
GRANT ALL ON TABLE jobmax10 TO PUBLIC;
GRANT ALL ON TABLE jobmax10 TO farmers;


--
-- Name: jobmax2009; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobmax2009 FROM PUBLIC;
REVOKE ALL ON TABLE jobmax2009 FROM farmer;
GRANT ALL ON TABLE jobmax2009 TO farmer;
GRANT ALL ON TABLE jobmax2009 TO PUBLIC;
GRANT ALL ON TABLE jobmax2009 TO farmers;


--
-- Name: jobmax2010; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobmax2010 FROM PUBLIC;
REVOKE ALL ON TABLE jobmax2010 FROM farmer;
GRANT ALL ON TABLE jobmax2010 TO farmer;
GRANT ALL ON TABLE jobmax2010 TO PUBLIC;
GRANT ALL ON TABLE jobmax2010 TO farmers;


--
-- Name: jobmaxscript; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobmaxscript FROM PUBLIC;
REVOKE ALL ON TABLE jobmaxscript FROM farmer;
GRANT ALL ON TABLE jobmaxscript TO farmer;
GRANT ALL ON TABLE jobmaxscript TO PUBLIC;
GRANT ALL ON TABLE jobmaxscript TO farmers;


--
-- Name: jobmaya; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobmaya FROM PUBLIC;
REVOKE ALL ON TABLE jobmaya FROM farmer;
GRANT ALL ON TABLE jobmaya TO farmer;
GRANT ALL ON TABLE jobmaya TO farmers;


--
-- Name: jobmaya2008; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobmaya2008 FROM PUBLIC;
REVOKE ALL ON TABLE jobmaya2008 FROM farmer;
GRANT ALL ON TABLE jobmaya2008 TO farmer;
GRANT ALL ON TABLE jobmaya2008 TO farmers;


--
-- Name: jobmaya2009; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobmaya2009 FROM PUBLIC;
REVOKE ALL ON TABLE jobmaya2009 FROM farmer;
GRANT ALL ON TABLE jobmaya2009 TO farmer;
GRANT ALL ON TABLE jobmaya2009 TO farmers;


--
-- Name: jobmaya2011; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobmaya2011 FROM PUBLIC;
REVOKE ALL ON TABLE jobmaya2011 FROM farmer;
GRANT ALL ON TABLE jobmaya2011 TO farmer;
GRANT ALL ON TABLE jobmaya2011 TO farmers;


--
-- Name: jobmaya7; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobmaya7 FROM PUBLIC;
REVOKE ALL ON TABLE jobmaya7 FROM farmer;
GRANT ALL ON TABLE jobmaya7 TO farmer;
GRANT ALL ON TABLE jobmaya7 TO farmers;


--
-- Name: jobmaya8; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobmaya8 FROM PUBLIC;
REVOKE ALL ON TABLE jobmaya8 FROM farmer;
GRANT ALL ON TABLE jobmaya8 TO farmer;
GRANT ALL ON TABLE jobmaya8 TO farmers;


--
-- Name: jobmaya85; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobmaya85 FROM PUBLIC;
REVOKE ALL ON TABLE jobmaya85 FROM farmer;
GRANT ALL ON TABLE jobmaya85 TO farmer;
GRANT ALL ON TABLE jobmaya85 TO farmers;


--
-- Name: jobmentalray2009; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobmentalray2009 FROM PUBLIC;
REVOKE ALL ON TABLE jobmentalray2009 FROM farmer;
GRANT ALL ON TABLE jobmentalray2009 TO farmer;
GRANT ALL ON TABLE jobmentalray2009 TO farmers;


--
-- Name: jobmentalray2011; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobmentalray2011 FROM PUBLIC;
REVOKE ALL ON TABLE jobmentalray2011 FROM farmer;
GRANT ALL ON TABLE jobmentalray2011 TO farmer;
GRANT ALL ON TABLE jobmentalray2011 TO farmers;


--
-- Name: jobmentalray7; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobmentalray7 FROM PUBLIC;
REVOKE ALL ON TABLE jobmentalray7 FROM farmer;
GRANT ALL ON TABLE jobmentalray7 TO farmer;
GRANT ALL ON TABLE jobmentalray7 TO farmers;


--
-- Name: jobmentalray8; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobmentalray8 FROM PUBLIC;
REVOKE ALL ON TABLE jobmentalray8 FROM farmer;
GRANT ALL ON TABLE jobmentalray8 TO farmer;
GRANT ALL ON TABLE jobmentalray8 TO farmers;


--
-- Name: jobmentalray85; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobmentalray85 FROM PUBLIC;
REVOKE ALL ON TABLE jobmentalray85 FROM farmer;
GRANT ALL ON TABLE jobmentalray85 TO farmer;
GRANT ALL ON TABLE jobmentalray85 TO farmers;


--
-- Name: jobnaiad; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobnaiad FROM PUBLIC;
REVOKE ALL ON TABLE jobnaiad FROM farmer;
GRANT ALL ON TABLE jobnaiad TO farmer;
GRANT ALL ON TABLE jobnaiad TO PUBLIC;
GRANT ALL ON TABLE jobnaiad TO farmers;


--
-- Name: jobnuke; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobnuke FROM PUBLIC;
REVOKE ALL ON TABLE jobnuke FROM farmer;
GRANT ALL ON TABLE jobnuke TO farmer;
GRANT ALL ON TABLE jobnuke TO PUBLIC;
GRANT ALL ON TABLE jobnuke TO farmers;


--
-- Name: jobnuke51; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobnuke51 FROM PUBLIC;
REVOKE ALL ON TABLE jobnuke51 FROM farmer;
GRANT ALL ON TABLE jobnuke51 TO farmer;
GRANT ALL ON TABLE jobnuke51 TO farmers;


--
-- Name: jobnuke52; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobnuke52 FROM PUBLIC;
REVOKE ALL ON TABLE jobnuke52 FROM farmer;
GRANT ALL ON TABLE jobnuke52 TO farmer;
GRANT ALL ON TABLE jobnuke52 TO farmers;


--
-- Name: joboutput_keyjoboutput_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE joboutput_keyjoboutput_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE joboutput_keyjoboutput_seq FROM farmer;
GRANT ALL ON SEQUENCE joboutput_keyjoboutput_seq TO farmer;
GRANT ALL ON SEQUENCE joboutput_keyjoboutput_seq TO farmers;


--
-- Name: joboutput; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE joboutput FROM PUBLIC;
REVOKE ALL ON TABLE joboutput FROM farmer;
GRANT ALL ON TABLE joboutput TO farmer;
GRANT ALL ON TABLE joboutput TO farmers;


--
-- Name: jobrenderman_keyjob_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobrenderman_keyjob_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobrenderman_keyjob_seq FROM farmer;
GRANT ALL ON SEQUENCE jobrenderman_keyjob_seq TO farmer;
GRANT ALL ON SEQUENCE jobrenderman_keyjob_seq TO farmers;


--
-- Name: jobribgen_keyjob_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobribgen_keyjob_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobribgen_keyjob_seq FROM farmer;
GRANT ALL ON SEQUENCE jobribgen_keyjob_seq TO farmer;
GRANT ALL ON SEQUENCE jobribgen_keyjob_seq TO farmers;


--
-- Name: jobservice_keyjobservice_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobservice_keyjobservice_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobservice_keyjobservice_seq FROM farmer;
GRANT ALL ON SEQUENCE jobservice_keyjobservice_seq TO farmer;
GRANT ALL ON SEQUENCE jobservice_keyjobservice_seq TO farmers;


--
-- Name: jobservice; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobservice FROM PUBLIC;
REVOKE ALL ON TABLE jobservice FROM farmer;
GRANT ALL ON TABLE jobservice TO farmer;
GRANT ALL ON TABLE jobservice TO farmers;


--
-- Name: jobstat_keyjobstat_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobstat_keyjobstat_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobstat_keyjobstat_seq FROM farmer;
GRANT ALL ON SEQUENCE jobstat_keyjobstat_seq TO farmer;
GRANT ALL ON SEQUENCE jobstat_keyjobstat_seq TO farmers;


--
-- Name: jobstat; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobstat FROM PUBLIC;
REVOKE ALL ON TABLE jobstat FROM farmer;
GRANT ALL ON TABLE jobstat TO farmer;
GRANT ALL ON TABLE jobstat TO farmers;


--
-- Name: jobstateaction; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobstateaction FROM PUBLIC;
REVOKE ALL ON TABLE jobstateaction FROM farmer;
GRANT ALL ON TABLE jobstateaction TO farmer;
GRANT ALL ON TABLE jobstateaction TO PUBLIC;
GRANT ALL ON TABLE jobstateaction TO farmers;


--
-- Name: jobstateaction_keyjobstateaction_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobstateaction_keyjobstateaction_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobstateaction_keyjobstateaction_seq FROM farmer;
GRANT ALL ON SEQUENCE jobstateaction_keyjobstateaction_seq TO farmer;
GRANT ALL ON SEQUENCE jobstateaction_keyjobstateaction_seq TO farmers;


--
-- Name: jobstatus_keyjobstatus_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobstatus_keyjobstatus_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobstatus_keyjobstatus_seq FROM farmer;
GRANT ALL ON SEQUENCE jobstatus_keyjobstatus_seq TO farmer;
GRANT ALL ON SEQUENCE jobstatus_keyjobstatus_seq TO farmers;


--
-- Name: jobstatus; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobstatus FROM PUBLIC;
REVOKE ALL ON TABLE jobstatus FROM farmer;
GRANT ALL ON TABLE jobstatus TO farmer;
GRANT ALL ON TABLE jobstatus TO PUBLIC;
GRANT ALL ON TABLE jobstatus TO farmers;


--
-- Name: jobstatus_old; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobstatus_old FROM PUBLIC;
REVOKE ALL ON TABLE jobstatus_old FROM farmer;
GRANT ALL ON TABLE jobstatus_old TO farmer;
GRANT ALL ON TABLE jobstatus_old TO farmers;


--
-- Name: jobstatusskipreason; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobstatusskipreason FROM PUBLIC;
REVOKE ALL ON TABLE jobstatusskipreason FROM farmer;
GRANT ALL ON TABLE jobstatusskipreason TO farmer;
GRANT ALL ON TABLE jobstatusskipreason TO PUBLIC;
GRANT ALL ON TABLE jobstatusskipreason TO farmers;


--
-- Name: jobstatusskipreason_keyjobstatusskipreason_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobstatusskipreason_keyjobstatusskipreason_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobstatusskipreason_keyjobstatusskipreason_seq FROM farmer;
GRANT ALL ON SEQUENCE jobstatusskipreason_keyjobstatusskipreason_seq TO farmer;
GRANT ALL ON SEQUENCE jobstatusskipreason_keyjobstatusskipreason_seq TO farmers;


--
-- Name: jobtaskassignment_old; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobtaskassignment_old FROM PUBLIC;
REVOKE ALL ON TABLE jobtaskassignment_old FROM farmer;
GRANT ALL ON TABLE jobtaskassignment_old TO farmer;
GRANT ALL ON TABLE jobtaskassignment_old TO farmers;


--
-- Name: jobtaskassignment_keyjobtaskassignment_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobtaskassignment_keyjobtaskassignment_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobtaskassignment_keyjobtaskassignment_seq FROM farmer;
GRANT ALL ON SEQUENCE jobtaskassignment_keyjobtaskassignment_seq TO farmer;
GRANT ALL ON SEQUENCE jobtaskassignment_keyjobtaskassignment_seq TO farmers;


--
-- Name: jobtaskassignment; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobtaskassignment FROM PUBLIC;
REVOKE ALL ON TABLE jobtaskassignment FROM farmer;
GRANT ALL ON TABLE jobtaskassignment TO farmer;
GRANT ALL ON TABLE jobtaskassignment TO PUBLIC;
GRANT ALL ON TABLE jobtaskassignment TO farmers;


--
-- Name: jobtypemapping_keyjobtypemapping_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE jobtypemapping_keyjobtypemapping_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE jobtypemapping_keyjobtypemapping_seq FROM farmer;
GRANT ALL ON SEQUENCE jobtypemapping_keyjobtypemapping_seq TO farmer;
GRANT ALL ON SEQUENCE jobtypemapping_keyjobtypemapping_seq TO farmers;


--
-- Name: jobtypemapping; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE jobtypemapping FROM PUBLIC;
REVOKE ALL ON TABLE jobtypemapping FROM farmer;
GRANT ALL ON TABLE jobtypemapping TO farmer;
GRANT ALL ON TABLE jobtypemapping TO farmers;


--
-- Name: license_keylicense_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE license_keylicense_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE license_keylicense_seq FROM farmer;
GRANT ALL ON SEQUENCE license_keylicense_seq TO farmer;
GRANT ALL ON SEQUENCE license_keylicense_seq TO farmers;


--
-- Name: license; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE license FROM PUBLIC;
REVOKE ALL ON TABLE license FROM farmer;
GRANT ALL ON TABLE license TO farmer;
GRANT ALL ON TABLE license TO farmers;
GRANT ALL ON TABLE license TO PUBLIC;


--
-- Name: service_keyservice_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE service_keyservice_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE service_keyservice_seq FROM farmer;
GRANT ALL ON SEQUENCE service_keyservice_seq TO farmer;
GRANT ALL ON SEQUENCE service_keyservice_seq TO farmers;


--
-- Name: service; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE service FROM PUBLIC;
REVOKE ALL ON TABLE service FROM farmer;
GRANT ALL ON TABLE service TO farmer;
GRANT ALL ON TABLE service TO farmers;


--
-- Name: license_usage; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE license_usage FROM PUBLIC;
REVOKE ALL ON TABLE license_usage FROM farmer;
GRANT ALL ON TABLE license_usage TO farmer;
GRANT ALL ON TABLE license_usage TO PUBLIC;
GRANT ALL ON TABLE license_usage TO farmers;


--
-- Name: license_usage_2; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE license_usage_2 FROM PUBLIC;
REVOKE ALL ON TABLE license_usage_2 FROM farmer;
GRANT ALL ON TABLE license_usage_2 TO farmer;
GRANT ALL ON TABLE license_usage_2 TO PUBLIC;
GRANT ALL ON TABLE license_usage_2 TO farmers;


--
-- Name: location_keylocation_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE location_keylocation_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE location_keylocation_seq FROM farmer;
GRANT ALL ON SEQUENCE location_keylocation_seq TO farmer;
GRANT ALL ON SEQUENCE location_keylocation_seq TO farmers;


--
-- Name: location; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE location FROM PUBLIC;
REVOKE ALL ON TABLE location FROM farmer;
GRANT ALL ON TABLE location TO farmer;
GRANT ALL ON TABLE location TO farmers;


--
-- Name: mapping_keymapping_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE mapping_keymapping_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE mapping_keymapping_seq FROM farmer;
GRANT ALL ON SEQUENCE mapping_keymapping_seq TO farmer;
GRANT ALL ON SEQUENCE mapping_keymapping_seq TO farmers;


--
-- Name: mapping; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE mapping FROM PUBLIC;
REVOKE ALL ON TABLE mapping FROM farmer;
GRANT ALL ON TABLE mapping TO farmer;
GRANT ALL ON TABLE mapping TO farmers;


--
-- Name: mappingtype_keymappingtype_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE mappingtype_keymappingtype_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE mappingtype_keymappingtype_seq FROM farmer;
GRANT ALL ON SEQUENCE mappingtype_keymappingtype_seq TO farmer;
GRANT ALL ON SEQUENCE mappingtype_keymappingtype_seq TO farmers;


--
-- Name: mappingtype; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE mappingtype FROM PUBLIC;
REVOKE ALL ON TABLE mappingtype FROM farmer;
GRANT ALL ON TABLE mappingtype TO farmer;
GRANT ALL ON TABLE mappingtype TO farmers;


--
-- Name: methodperms_keymethodperms_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE methodperms_keymethodperms_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE methodperms_keymethodperms_seq FROM farmer;
GRANT ALL ON SEQUENCE methodperms_keymethodperms_seq TO farmer;
GRANT ALL ON SEQUENCE methodperms_keymethodperms_seq TO farmers;


--
-- Name: methodperms; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE methodperms FROM PUBLIC;
REVOKE ALL ON TABLE methodperms FROM farmer;
GRANT ALL ON TABLE methodperms TO farmer;
GRANT ALL ON TABLE methodperms TO farmers;


--
-- Name: notification_keynotification_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE notification_keynotification_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE notification_keynotification_seq FROM farmer;
GRANT ALL ON SEQUENCE notification_keynotification_seq TO farmer;
GRANT ALL ON SEQUENCE notification_keynotification_seq TO farmers;


--
-- Name: notification; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE notification FROM PUBLIC;
REVOKE ALL ON TABLE notification FROM farmer;
GRANT ALL ON TABLE notification TO farmer;
GRANT ALL ON TABLE notification TO farmers;


--
-- Name: notificationdestination_keynotificationdestination_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE notificationdestination_keynotificationdestination_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE notificationdestination_keynotificationdestination_seq FROM farmer;
GRANT ALL ON SEQUENCE notificationdestination_keynotificationdestination_seq TO farmer;
GRANT ALL ON SEQUENCE notificationdestination_keynotificationdestination_seq TO farmers;


--
-- Name: notificationdestination; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE notificationdestination FROM PUBLIC;
REVOKE ALL ON TABLE notificationdestination FROM farmer;
GRANT ALL ON TABLE notificationdestination TO farmer;
GRANT ALL ON TABLE notificationdestination TO farmers;


--
-- Name: notificationmethod_keynotificationmethod_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE notificationmethod_keynotificationmethod_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE notificationmethod_keynotificationmethod_seq FROM farmer;
GRANT ALL ON SEQUENCE notificationmethod_keynotificationmethod_seq TO farmer;
GRANT ALL ON SEQUENCE notificationmethod_keynotificationmethod_seq TO farmers;


--
-- Name: notificationmethod; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE notificationmethod FROM PUBLIC;
REVOKE ALL ON TABLE notificationmethod FROM farmer;
GRANT ALL ON TABLE notificationmethod TO farmer;
GRANT ALL ON TABLE notificationmethod TO farmers;


--
-- Name: notificationroute_keynotificationuserroute_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE notificationroute_keynotificationuserroute_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE notificationroute_keynotificationuserroute_seq FROM farmer;
GRANT ALL ON SEQUENCE notificationroute_keynotificationuserroute_seq TO farmer;
GRANT ALL ON SEQUENCE notificationroute_keynotificationuserroute_seq TO farmers;


--
-- Name: notificationroute; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE notificationroute FROM PUBLIC;
REVOKE ALL ON TABLE notificationroute FROM farmer;
GRANT ALL ON TABLE notificationroute TO farmer;
GRANT ALL ON TABLE notificationroute TO farmers;


--
-- Name: notify_keynotify_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE notify_keynotify_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE notify_keynotify_seq FROM farmer;
GRANT ALL ON SEQUENCE notify_keynotify_seq TO farmer;
GRANT ALL ON SEQUENCE notify_keynotify_seq TO farmers;


--
-- Name: notify; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE notify FROM PUBLIC;
REVOKE ALL ON TABLE notify FROM farmer;
GRANT ALL ON TABLE notify TO farmer;
GRANT ALL ON TABLE notify TO farmers;


--
-- Name: notifymethod_keynotifymethod_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE notifymethod_keynotifymethod_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE notifymethod_keynotifymethod_seq FROM farmer;
GRANT ALL ON SEQUENCE notifymethod_keynotifymethod_seq TO farmer;
GRANT ALL ON SEQUENCE notifymethod_keynotifymethod_seq TO farmers;


--
-- Name: notifymethod; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE notifymethod FROM PUBLIC;
REVOKE ALL ON TABLE notifymethod FROM farmer;
GRANT ALL ON TABLE notifymethod TO farmer;
GRANT ALL ON TABLE notifymethod TO farmers;


--
-- Name: notifysent_keynotifysent_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE notifysent_keynotifysent_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE notifysent_keynotifysent_seq FROM farmer;
GRANT ALL ON SEQUENCE notifysent_keynotifysent_seq TO farmer;
GRANT ALL ON SEQUENCE notifysent_keynotifysent_seq TO farmers;


--
-- Name: notifysent; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE notifysent FROM PUBLIC;
REVOKE ALL ON TABLE notifysent FROM farmer;
GRANT ALL ON TABLE notifysent TO farmer;
GRANT ALL ON TABLE notifysent TO farmers;


--
-- Name: notifywho_keynotifywho_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE notifywho_keynotifywho_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE notifywho_keynotifywho_seq FROM farmer;
GRANT ALL ON SEQUENCE notifywho_keynotifywho_seq TO farmer;
GRANT ALL ON SEQUENCE notifywho_keynotifywho_seq TO farmers;


--
-- Name: notifywho; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE notifywho FROM PUBLIC;
REVOKE ALL ON TABLE notifywho FROM farmer;
GRANT ALL ON TABLE notifywho TO farmer;
GRANT ALL ON TABLE notifywho TO farmers;


--
-- Name: package; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE package FROM PUBLIC;
REVOKE ALL ON TABLE package FROM farmer;
GRANT ALL ON TABLE package TO farmer;
GRANT ALL ON TABLE package TO farmers;


--
-- Name: package_keypackage_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE package_keypackage_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE package_keypackage_seq FROM farmer;
GRANT ALL ON SEQUENCE package_keypackage_seq TO farmer;
GRANT ALL ON SEQUENCE package_keypackage_seq TO farmers;


--
-- Name: packageoutput; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE packageoutput FROM PUBLIC;
REVOKE ALL ON TABLE packageoutput FROM farmer;
GRANT ALL ON TABLE packageoutput TO farmer;
GRANT ALL ON TABLE packageoutput TO farmers;


--
-- Name: packageoutput_keypackageoutput_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE packageoutput_keypackageoutput_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE packageoutput_keypackageoutput_seq FROM farmer;
GRANT ALL ON SEQUENCE packageoutput_keypackageoutput_seq TO farmer;
GRANT ALL ON SEQUENCE packageoutput_keypackageoutput_seq TO farmers;


--
-- Name: pathsynctarget_keypathsynctarget_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE pathsynctarget_keypathsynctarget_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE pathsynctarget_keypathsynctarget_seq FROM farmer;
GRANT ALL ON SEQUENCE pathsynctarget_keypathsynctarget_seq TO farmer;
GRANT ALL ON SEQUENCE pathsynctarget_keypathsynctarget_seq TO farmers;


--
-- Name: pathsynctarget; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE pathsynctarget FROM PUBLIC;
REVOKE ALL ON TABLE pathsynctarget FROM farmer;
GRANT ALL ON TABLE pathsynctarget TO farmer;
GRANT ALL ON TABLE pathsynctarget TO farmers;


--
-- Name: pathtemplate_keypathtemplate_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE pathtemplate_keypathtemplate_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE pathtemplate_keypathtemplate_seq FROM farmer;
GRANT ALL ON SEQUENCE pathtemplate_keypathtemplate_seq TO farmer;
GRANT ALL ON SEQUENCE pathtemplate_keypathtemplate_seq TO farmers;


--
-- Name: pathtemplate; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE pathtemplate FROM PUBLIC;
REVOKE ALL ON TABLE pathtemplate FROM farmer;
GRANT ALL ON TABLE pathtemplate TO farmer;
GRANT ALL ON TABLE pathtemplate TO farmers;


--
-- Name: pathtracker_keypathtracker_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE pathtracker_keypathtracker_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE pathtracker_keypathtracker_seq FROM farmer;
GRANT ALL ON SEQUENCE pathtracker_keypathtracker_seq TO farmer;
GRANT ALL ON SEQUENCE pathtracker_keypathtracker_seq TO farmers;


--
-- Name: pathtracker; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE pathtracker FROM PUBLIC;
REVOKE ALL ON TABLE pathtracker FROM farmer;
GRANT ALL ON TABLE pathtracker TO farmer;
GRANT ALL ON TABLE pathtracker TO farmers;


--
-- Name: permission_keypermission_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE permission_keypermission_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE permission_keypermission_seq FROM farmer;
GRANT ALL ON SEQUENCE permission_keypermission_seq TO farmer;
GRANT ALL ON SEQUENCE permission_keypermission_seq TO farmers;


--
-- Name: permission; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE permission FROM PUBLIC;
REVOKE ALL ON TABLE permission FROM farmer;
GRANT ALL ON TABLE permission TO farmer;
GRANT ALL ON TABLE permission TO farmers;


--
-- Name: pg_stat_statements; Type: ACL; Schema: public; Owner: postgres
--

REVOKE ALL ON TABLE pg_stat_statements FROM PUBLIC;
REVOKE ALL ON TABLE pg_stat_statements FROM postgres;
GRANT ALL ON TABLE pg_stat_statements TO postgres;
GRANT SELECT ON TABLE pg_stat_statements TO PUBLIC;
GRANT ALL ON TABLE pg_stat_statements TO farmers;


--
-- Name: phoneno_keyphoneno_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE phoneno_keyphoneno_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE phoneno_keyphoneno_seq FROM farmer;
GRANT ALL ON SEQUENCE phoneno_keyphoneno_seq TO farmer;
GRANT ALL ON SEQUENCE phoneno_keyphoneno_seq TO farmers;


--
-- Name: phoneno; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE phoneno FROM PUBLIC;
REVOKE ALL ON TABLE phoneno FROM farmer;
GRANT ALL ON TABLE phoneno TO farmer;
GRANT ALL ON TABLE phoneno TO farmers;


--
-- Name: phonetype_keyphonetype_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE phonetype_keyphonetype_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE phonetype_keyphonetype_seq FROM farmer;
GRANT ALL ON SEQUENCE phonetype_keyphonetype_seq TO farmer;
GRANT ALL ON SEQUENCE phonetype_keyphonetype_seq TO farmers;


--
-- Name: phonetype; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE phonetype FROM PUBLIC;
REVOKE ALL ON TABLE phonetype FROM farmer;
GRANT ALL ON TABLE phonetype TO farmer;
GRANT ALL ON TABLE phonetype TO farmers;


--
-- Name: project; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE project FROM PUBLIC;
REVOKE ALL ON TABLE project FROM farmer;
GRANT ALL ON TABLE project TO farmer;
GRANT ALL ON TABLE project TO farmers;


--
-- Name: project_slots_current; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE project_slots_current FROM PUBLIC;
REVOKE ALL ON TABLE project_slots_current FROM farmer;
GRANT ALL ON TABLE project_slots_current TO farmer;
GRANT ALL ON TABLE project_slots_current TO PUBLIC;
GRANT ALL ON TABLE project_slots_current TO farmers;


--
-- Name: project_slots_limits; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE project_slots_limits FROM PUBLIC;
REVOKE ALL ON TABLE project_slots_limits FROM farmer;
GRANT ALL ON TABLE project_slots_limits TO farmer;
GRANT ALL ON TABLE project_slots_limits TO PUBLIC;
GRANT ALL ON TABLE project_slots_limits TO farmers;


--
-- Name: projectresolution_keyprojectresolution_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE projectresolution_keyprojectresolution_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE projectresolution_keyprojectresolution_seq FROM farmer;
GRANT ALL ON SEQUENCE projectresolution_keyprojectresolution_seq TO farmer;
GRANT ALL ON SEQUENCE projectresolution_keyprojectresolution_seq TO farmers;


--
-- Name: projectresolution; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE projectresolution FROM PUBLIC;
REVOKE ALL ON TABLE projectresolution FROM farmer;
GRANT ALL ON TABLE projectresolution TO farmer;
GRANT ALL ON TABLE projectresolution TO farmers;


--
-- Name: projectstatus_keyprojectstatus_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE projectstatus_keyprojectstatus_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE projectstatus_keyprojectstatus_seq FROM farmer;
GRANT ALL ON SEQUENCE projectstatus_keyprojectstatus_seq TO farmer;
GRANT ALL ON SEQUENCE projectstatus_keyprojectstatus_seq TO farmers;


--
-- Name: projectstatus; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE projectstatus FROM PUBLIC;
REVOKE ALL ON TABLE projectstatus FROM farmer;
GRANT ALL ON TABLE projectstatus TO farmer;
GRANT ALL ON TABLE projectstatus TO farmers;


--
-- Name: projectstorage_keyprojectstorage_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE projectstorage_keyprojectstorage_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE projectstorage_keyprojectstorage_seq FROM farmer;
GRANT ALL ON SEQUENCE projectstorage_keyprojectstorage_seq TO farmer;
GRANT ALL ON SEQUENCE projectstorage_keyprojectstorage_seq TO farmers;


--
-- Name: projectstorage; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE projectstorage FROM PUBLIC;
REVOKE ALL ON TABLE projectstorage FROM farmer;
GRANT ALL ON TABLE projectstorage TO farmer;
GRANT ALL ON TABLE projectstorage TO farmers;


--
-- Name: projecttempo; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE projecttempo FROM PUBLIC;
REVOKE ALL ON TABLE projecttempo FROM farmer;
GRANT ALL ON TABLE projecttempo TO farmer;
GRANT ALL ON TABLE projecttempo TO farmers;


--
-- Name: queueorder; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE queueorder FROM PUBLIC;
REVOKE ALL ON TABLE queueorder FROM farmer;
GRANT ALL ON TABLE queueorder TO farmer;
GRANT ALL ON TABLE queueorder TO farmers;


--
-- Name: rangefiletracker; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE rangefiletracker FROM PUBLIC;
REVOKE ALL ON TABLE rangefiletracker FROM farmer;
GRANT ALL ON TABLE rangefiletracker TO farmer;
GRANT ALL ON TABLE rangefiletracker TO farmers;


--
-- Name: renderframe_keyrenderframe_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE renderframe_keyrenderframe_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE renderframe_keyrenderframe_seq FROM farmer;
GRANT ALL ON SEQUENCE renderframe_keyrenderframe_seq TO farmer;
GRANT ALL ON SEQUENCE renderframe_keyrenderframe_seq TO farmers;


--
-- Name: renderframe; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE renderframe FROM PUBLIC;
REVOKE ALL ON TABLE renderframe FROM farmer;
GRANT ALL ON TABLE renderframe TO farmer;
GRANT ALL ON TABLE renderframe TO farmers;


--
-- Name: running_shots_averagetime; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE running_shots_averagetime FROM PUBLIC;
REVOKE ALL ON TABLE running_shots_averagetime FROM farmer;
GRANT ALL ON TABLE running_shots_averagetime TO farmer;
GRANT ALL ON TABLE running_shots_averagetime TO PUBLIC;
GRANT ALL ON TABLE running_shots_averagetime TO farmers;


--
-- Name: running_shots_averagetime_2; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE running_shots_averagetime_2 FROM PUBLIC;
REVOKE ALL ON TABLE running_shots_averagetime_2 FROM farmer;
GRANT ALL ON TABLE running_shots_averagetime_2 TO farmer;
GRANT ALL ON TABLE running_shots_averagetime_2 TO PUBLIC;
GRANT ALL ON TABLE running_shots_averagetime_2 TO farmers;


--
-- Name: running_shots_averagetime_3; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE running_shots_averagetime_3 FROM PUBLIC;
REVOKE ALL ON TABLE running_shots_averagetime_3 FROM farmer;
GRANT ALL ON TABLE running_shots_averagetime_3 TO farmer;
GRANT ALL ON TABLE running_shots_averagetime_3 TO PUBLIC;
GRANT ALL ON TABLE running_shots_averagetime_3 TO farmers;


--
-- Name: running_shots_averagetime_4; Type: ACL; Schema: public; Owner: postgres
--

REVOKE ALL ON TABLE running_shots_averagetime_4 FROM PUBLIC;
REVOKE ALL ON TABLE running_shots_averagetime_4 FROM postgres;
GRANT ALL ON TABLE running_shots_averagetime_4 TO postgres;
GRANT ALL ON TABLE running_shots_averagetime_4 TO farmers;


--
-- Name: schedule_keyschedule_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE schedule_keyschedule_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE schedule_keyschedule_seq FROM farmer;
GRANT ALL ON SEQUENCE schedule_keyschedule_seq TO farmer;
GRANT ALL ON SEQUENCE schedule_keyschedule_seq TO farmers;


--
-- Name: schedule; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE schedule FROM PUBLIC;
REVOKE ALL ON TABLE schedule FROM farmer;
GRANT ALL ON TABLE schedule TO farmer;
GRANT ALL ON TABLE schedule TO farmers;


--
-- Name: serverfileaction; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE serverfileaction FROM PUBLIC;
REVOKE ALL ON TABLE serverfileaction FROM farmer;
GRANT ALL ON TABLE serverfileaction TO farmer;
GRANT ALL ON TABLE serverfileaction TO farmers;


--
-- Name: serverfileaction_keyserverfileaction_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE serverfileaction_keyserverfileaction_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE serverfileaction_keyserverfileaction_seq FROM farmer;
GRANT ALL ON SEQUENCE serverfileaction_keyserverfileaction_seq TO farmer;
GRANT ALL ON SEQUENCE serverfileaction_keyserverfileaction_seq TO farmers;


--
-- Name: serverfileactionstatus; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE serverfileactionstatus FROM PUBLIC;
REVOKE ALL ON TABLE serverfileactionstatus FROM farmer;
GRANT ALL ON TABLE serverfileactionstatus TO farmer;
GRANT ALL ON TABLE serverfileactionstatus TO farmers;


--
-- Name: serverfileactionstatus_keyserverfileactionstatus_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE serverfileactionstatus_keyserverfileactionstatus_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE serverfileactionstatus_keyserverfileactionstatus_seq FROM farmer;
GRANT ALL ON SEQUENCE serverfileactionstatus_keyserverfileactionstatus_seq TO farmer;
GRANT ALL ON SEQUENCE serverfileactionstatus_keyserverfileactionstatus_seq TO farmers;


--
-- Name: serverfileactiontype; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE serverfileactiontype FROM PUBLIC;
REVOKE ALL ON TABLE serverfileactiontype FROM farmer;
GRANT ALL ON TABLE serverfileactiontype TO farmer;
GRANT ALL ON TABLE serverfileactiontype TO farmers;


--
-- Name: serverfileactiontype_keyserverfileactiontype_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE serverfileactiontype_keyserverfileactiontype_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE serverfileactiontype_keyserverfileactiontype_seq FROM farmer;
GRANT ALL ON SEQUENCE serverfileactiontype_keyserverfileactiontype_seq TO farmer;
GRANT ALL ON SEQUENCE serverfileactiontype_keyserverfileactiontype_seq TO farmers;


--
-- Name: sessions; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE sessions FROM PUBLIC;
REVOKE ALL ON TABLE sessions FROM farmer;
GRANT ALL ON TABLE sessions TO farmer;
GRANT ALL ON TABLE sessions TO farmers;


--
-- Name: shot; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE shot FROM PUBLIC;
REVOKE ALL ON TABLE shot FROM farmer;
GRANT ALL ON TABLE shot TO farmer;
GRANT ALL ON TABLE shot TO farmers;


--
-- Name: shotgroup; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE shotgroup FROM PUBLIC;
REVOKE ALL ON TABLE shotgroup FROM farmer;
GRANT ALL ON TABLE shotgroup TO farmer;
GRANT ALL ON TABLE shotgroup TO farmers;


--
-- Name: slots_total; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE slots_total FROM PUBLIC;
REVOKE ALL ON TABLE slots_total FROM farmer;
GRANT ALL ON TABLE slots_total TO farmer;
GRANT ALL ON TABLE slots_total TO PUBLIC;
GRANT ALL ON TABLE slots_total TO farmers;


--
-- Name: software_keysoftware_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE software_keysoftware_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE software_keysoftware_seq FROM farmer;
GRANT ALL ON SEQUENCE software_keysoftware_seq TO farmer;
GRANT ALL ON SEQUENCE software_keysoftware_seq TO farmers;


--
-- Name: software; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE software FROM PUBLIC;
REVOKE ALL ON TABLE software FROM farmer;
GRANT ALL ON TABLE software TO farmer;
GRANT ALL ON TABLE software TO farmers;


--
-- Name: status; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE status FROM PUBLIC;
REVOKE ALL ON TABLE status FROM farmer;
GRANT ALL ON TABLE status TO farmer;
GRANT ALL ON TABLE status TO farmers;


--
-- Name: status_keystatus_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE status_keystatus_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE status_keystatus_seq FROM farmer;
GRANT ALL ON SEQUENCE status_keystatus_seq TO farmer;
GRANT ALL ON SEQUENCE status_keystatus_seq TO farmers;


--
-- Name: statusset_keystatusset_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE statusset_keystatusset_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE statusset_keystatusset_seq FROM farmer;
GRANT ALL ON SEQUENCE statusset_keystatusset_seq TO farmer;
GRANT ALL ON SEQUENCE statusset_keystatusset_seq TO farmers;


--
-- Name: statusset; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE statusset FROM PUBLIC;
REVOKE ALL ON TABLE statusset FROM farmer;
GRANT ALL ON TABLE statusset TO farmer;
GRANT ALL ON TABLE statusset TO farmers;


--
-- Name: syslog_keysyslog_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE syslog_keysyslog_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE syslog_keysyslog_seq FROM farmer;
GRANT ALL ON SEQUENCE syslog_keysyslog_seq TO farmer;
GRANT ALL ON SEQUENCE syslog_keysyslog_seq TO farmers;


--
-- Name: syslog; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE syslog FROM PUBLIC;
REVOKE ALL ON TABLE syslog FROM farmer;
GRANT ALL ON TABLE syslog TO farmer;
GRANT ALL ON TABLE syslog TO farmers;


--
-- Name: syslog_count_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE syslog_count_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE syslog_count_seq FROM farmer;
GRANT ALL ON SEQUENCE syslog_count_seq TO farmer;
GRANT ALL ON SEQUENCE syslog_count_seq TO farmers;


--
-- Name: syslogrealm_keysyslogrealm_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE syslogrealm_keysyslogrealm_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE syslogrealm_keysyslogrealm_seq FROM farmer;
GRANT ALL ON SEQUENCE syslogrealm_keysyslogrealm_seq TO farmer;
GRANT ALL ON SEQUENCE syslogrealm_keysyslogrealm_seq TO farmers;


--
-- Name: syslogrealm; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE syslogrealm FROM PUBLIC;
REVOKE ALL ON TABLE syslogrealm FROM farmer;
GRANT ALL ON TABLE syslogrealm TO farmer;
GRANT ALL ON TABLE syslogrealm TO farmers;


--
-- Name: syslogseverity_keysyslogseverity_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE syslogseverity_keysyslogseverity_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE syslogseverity_keysyslogseverity_seq FROM farmer;
GRANT ALL ON SEQUENCE syslogseverity_keysyslogseverity_seq TO farmer;
GRANT ALL ON SEQUENCE syslogseverity_keysyslogseverity_seq TO farmers;


--
-- Name: syslogseverity; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE syslogseverity FROM PUBLIC;
REVOKE ALL ON TABLE syslogseverity FROM farmer;
GRANT ALL ON TABLE syslogseverity TO farmer;
GRANT ALL ON TABLE syslogseverity TO farmers;


--
-- Name: task; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE task FROM PUBLIC;
REVOKE ALL ON TABLE task FROM farmer;
GRANT ALL ON TABLE task TO farmer;
GRANT ALL ON TABLE task TO farmers;


--
-- Name: tasktype_keytasktype_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE tasktype_keytasktype_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE tasktype_keytasktype_seq FROM farmer;
GRANT ALL ON SEQUENCE tasktype_keytasktype_seq TO farmer;
GRANT ALL ON SEQUENCE tasktype_keytasktype_seq TO farmers;


--
-- Name: tasktype; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE tasktype FROM PUBLIC;
REVOKE ALL ON TABLE tasktype FROM farmer;
GRANT ALL ON TABLE tasktype TO farmer;
GRANT ALL ON TABLE tasktype TO farmers;


--
-- Name: taskuser_keytaskuser_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE taskuser_keytaskuser_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE taskuser_keytaskuser_seq FROM farmer;
GRANT ALL ON SEQUENCE taskuser_keytaskuser_seq TO farmer;
GRANT ALL ON SEQUENCE taskuser_keytaskuser_seq TO farmers;


--
-- Name: taskuser; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE taskuser FROM PUBLIC;
REVOKE ALL ON TABLE taskuser FROM farmer;
GRANT ALL ON TABLE taskuser TO farmer;
GRANT ALL ON TABLE taskuser TO farmers;


--
-- Name: thread_keythread_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE thread_keythread_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE thread_keythread_seq FROM farmer;
GRANT ALL ON SEQUENCE thread_keythread_seq TO farmer;
GRANT ALL ON SEQUENCE thread_keythread_seq TO farmers;


--
-- Name: thread; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE thread FROM PUBLIC;
REVOKE ALL ON TABLE thread FROM farmer;
GRANT ALL ON TABLE thread TO farmer;
GRANT ALL ON TABLE thread TO farmers;


--
-- Name: threadcategory_keythreadcategory_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE threadcategory_keythreadcategory_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE threadcategory_keythreadcategory_seq FROM farmer;
GRANT ALL ON SEQUENCE threadcategory_keythreadcategory_seq TO farmer;
GRANT ALL ON SEQUENCE threadcategory_keythreadcategory_seq TO farmers;


--
-- Name: threadcategory; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE threadcategory FROM PUBLIC;
REVOKE ALL ON TABLE threadcategory FROM farmer;
GRANT ALL ON TABLE threadcategory TO farmer;
GRANT ALL ON TABLE threadcategory TO farmers;


--
-- Name: threadnotify_keythreadnotify_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE threadnotify_keythreadnotify_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE threadnotify_keythreadnotify_seq FROM farmer;
GRANT ALL ON SEQUENCE threadnotify_keythreadnotify_seq TO farmer;
GRANT ALL ON SEQUENCE threadnotify_keythreadnotify_seq TO farmers;


--
-- Name: threadnotify; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE threadnotify FROM PUBLIC;
REVOKE ALL ON TABLE threadnotify FROM farmer;
GRANT ALL ON TABLE threadnotify TO farmer;
GRANT ALL ON TABLE threadnotify TO farmers;


--
-- Name: thumbnail_keythumbnail_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE thumbnail_keythumbnail_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE thumbnail_keythumbnail_seq FROM farmer;
GRANT ALL ON SEQUENCE thumbnail_keythumbnail_seq TO farmer;
GRANT ALL ON SEQUENCE thumbnail_keythumbnail_seq TO farmers;


--
-- Name: thumbnail; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE thumbnail FROM PUBLIC;
REVOKE ALL ON TABLE thumbnail FROM farmer;
GRANT ALL ON TABLE thumbnail TO farmer;
GRANT ALL ON TABLE thumbnail TO farmers;


--
-- Name: timesheet_keytimesheet_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE timesheet_keytimesheet_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE timesheet_keytimesheet_seq FROM farmer;
GRANT ALL ON SEQUENCE timesheet_keytimesheet_seq TO farmer;
GRANT ALL ON SEQUENCE timesheet_keytimesheet_seq TO farmers;


--
-- Name: timesheet; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE timesheet FROM PUBLIC;
REVOKE ALL ON TABLE timesheet FROM farmer;
GRANT ALL ON TABLE timesheet TO farmer;
GRANT ALL ON TABLE timesheet TO farmers;


--
-- Name: timesheetcategory_keytimesheetcategory_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE timesheetcategory_keytimesheetcategory_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE timesheetcategory_keytimesheetcategory_seq FROM farmer;
GRANT ALL ON SEQUENCE timesheetcategory_keytimesheetcategory_seq TO farmer;
GRANT ALL ON SEQUENCE timesheetcategory_keytimesheetcategory_seq TO farmers;


--
-- Name: timesheetcategory; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE timesheetcategory FROM PUBLIC;
REVOKE ALL ON TABLE timesheetcategory FROM farmer;
GRANT ALL ON TABLE timesheetcategory TO farmer;
GRANT ALL ON TABLE timesheetcategory TO farmers;


--
-- Name: tracker_keytracker_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE tracker_keytracker_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE tracker_keytracker_seq FROM farmer;
GRANT ALL ON SEQUENCE tracker_keytracker_seq TO farmer;
GRANT ALL ON SEQUENCE tracker_keytracker_seq TO farmers;


--
-- Name: tracker; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE tracker FROM PUBLIC;
REVOKE ALL ON TABLE tracker FROM farmer;
GRANT ALL ON TABLE tracker TO farmer;
GRANT ALL ON TABLE tracker TO farmers;


--
-- Name: trackercategory_keytrackercategory_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE trackercategory_keytrackercategory_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE trackercategory_keytrackercategory_seq FROM farmer;
GRANT ALL ON SEQUENCE trackercategory_keytrackercategory_seq TO farmer;
GRANT ALL ON SEQUENCE trackercategory_keytrackercategory_seq TO farmers;


--
-- Name: trackercategory; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE trackercategory FROM PUBLIC;
REVOKE ALL ON TABLE trackercategory FROM farmer;
GRANT ALL ON TABLE trackercategory TO farmer;
GRANT ALL ON TABLE trackercategory TO farmers;


--
-- Name: trackerlog_keytrackerlog_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE trackerlog_keytrackerlog_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE trackerlog_keytrackerlog_seq FROM farmer;
GRANT ALL ON SEQUENCE trackerlog_keytrackerlog_seq TO farmer;
GRANT ALL ON SEQUENCE trackerlog_keytrackerlog_seq TO farmers;


--
-- Name: trackerlog; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE trackerlog FROM PUBLIC;
REVOKE ALL ON TABLE trackerlog FROM farmer;
GRANT ALL ON TABLE trackerlog TO farmer;
GRANT ALL ON TABLE trackerlog TO farmers;


--
-- Name: trackerqueue_keytrackerqueue_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE trackerqueue_keytrackerqueue_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE trackerqueue_keytrackerqueue_seq FROM farmer;
GRANT ALL ON SEQUENCE trackerqueue_keytrackerqueue_seq TO farmer;
GRANT ALL ON SEQUENCE trackerqueue_keytrackerqueue_seq TO farmers;


--
-- Name: trackerqueue; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE trackerqueue FROM PUBLIC;
REVOKE ALL ON TABLE trackerqueue FROM farmer;
GRANT ALL ON TABLE trackerqueue TO farmer;
GRANT ALL ON TABLE trackerqueue TO farmers;


--
-- Name: trackerseverity_keytrackerseverity_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE trackerseverity_keytrackerseverity_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE trackerseverity_keytrackerseverity_seq FROM farmer;
GRANT ALL ON SEQUENCE trackerseverity_keytrackerseverity_seq TO farmer;
GRANT ALL ON SEQUENCE trackerseverity_keytrackerseverity_seq TO farmers;


--
-- Name: trackerseverity; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE trackerseverity FROM PUBLIC;
REVOKE ALL ON TABLE trackerseverity FROM farmer;
GRANT ALL ON TABLE trackerseverity TO farmer;
GRANT ALL ON TABLE trackerseverity TO farmers;


--
-- Name: trackerstatus_keytrackerstatus_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE trackerstatus_keytrackerstatus_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE trackerstatus_keytrackerstatus_seq FROM farmer;
GRANT ALL ON SEQUENCE trackerstatus_keytrackerstatus_seq TO farmer;
GRANT ALL ON SEQUENCE trackerstatus_keytrackerstatus_seq TO farmers;


--
-- Name: trackerstatus; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE trackerstatus FROM PUBLIC;
REVOKE ALL ON TABLE trackerstatus FROM farmer;
GRANT ALL ON TABLE trackerstatus TO farmer;
GRANT ALL ON TABLE trackerstatus TO farmers;


--
-- Name: user_service_current; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE user_service_current FROM PUBLIC;
REVOKE ALL ON TABLE user_service_current FROM farmer;
GRANT ALL ON TABLE user_service_current TO farmer;
GRANT ALL ON TABLE user_service_current TO PUBLIC;
GRANT ALL ON TABLE user_service_current TO farmers;


--
-- Name: userservice; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE userservice FROM PUBLIC;
REVOKE ALL ON TABLE userservice FROM farmer;
GRANT ALL ON TABLE userservice TO farmer;
GRANT ALL ON TABLE userservice TO PUBLIC;
GRANT ALL ON TABLE userservice TO farmers;


--
-- Name: user_service_limits; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE user_service_limits FROM PUBLIC;
REVOKE ALL ON TABLE user_service_limits FROM farmer;
GRANT ALL ON TABLE user_service_limits TO farmer;
GRANT ALL ON TABLE user_service_limits TO PUBLIC;
GRANT ALL ON TABLE user_service_limits TO farmers;


--
-- Name: user_slots_current; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE user_slots_current FROM PUBLIC;
REVOKE ALL ON TABLE user_slots_current FROM farmer;
GRANT ALL ON TABLE user_slots_current TO farmer;
GRANT ALL ON TABLE user_slots_current TO PUBLIC;
GRANT ALL ON TABLE user_slots_current TO farmers;


--
-- Name: user_slots_limits; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE user_slots_limits FROM PUBLIC;
REVOKE ALL ON TABLE user_slots_limits FROM farmer;
GRANT ALL ON TABLE user_slots_limits TO farmer;
GRANT ALL ON TABLE user_slots_limits TO PUBLIC;
GRANT ALL ON TABLE user_slots_limits TO farmers;


--
-- Name: userelement_keyuserelement_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE userelement_keyuserelement_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE userelement_keyuserelement_seq FROM farmer;
GRANT ALL ON SEQUENCE userelement_keyuserelement_seq TO farmer;
GRANT ALL ON SEQUENCE userelement_keyuserelement_seq TO farmers;


--
-- Name: userelement; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE userelement FROM PUBLIC;
REVOKE ALL ON TABLE userelement FROM farmer;
GRANT ALL ON TABLE userelement TO farmer;
GRANT ALL ON TABLE userelement TO farmers;


--
-- Name: usermapping_keyusermapping_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE usermapping_keyusermapping_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE usermapping_keyusermapping_seq FROM farmer;
GRANT ALL ON SEQUENCE usermapping_keyusermapping_seq TO farmer;
GRANT ALL ON SEQUENCE usermapping_keyusermapping_seq TO farmers;


--
-- Name: usermapping; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE usermapping FROM PUBLIC;
REVOKE ALL ON TABLE usermapping FROM farmer;
GRANT ALL ON TABLE usermapping TO farmer;
GRANT ALL ON TABLE usermapping TO farmers;


--
-- Name: userrole_keyuserrole_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE userrole_keyuserrole_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE userrole_keyuserrole_seq FROM farmer;
GRANT ALL ON SEQUENCE userrole_keyuserrole_seq TO farmer;
GRANT ALL ON SEQUENCE userrole_keyuserrole_seq TO farmers;


--
-- Name: userrole; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE userrole FROM PUBLIC;
REVOKE ALL ON TABLE userrole FROM farmer;
GRANT ALL ON TABLE userrole TO farmer;
GRANT ALL ON TABLE userrole TO farmers;


--
-- Name: userservice_keyuserservice_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE userservice_keyuserservice_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE userservice_keyuserservice_seq FROM farmer;
GRANT ALL ON SEQUENCE userservice_keyuserservice_seq TO farmer;
GRANT ALL ON SEQUENCE userservice_keyuserservice_seq TO farmers;


--
-- Name: usrgrp_keyusrgrp_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE usrgrp_keyusrgrp_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE usrgrp_keyusrgrp_seq FROM farmer;
GRANT ALL ON SEQUENCE usrgrp_keyusrgrp_seq TO farmer;
GRANT ALL ON SEQUENCE usrgrp_keyusrgrp_seq TO farmers;


--
-- Name: usrgrp; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE usrgrp FROM PUBLIC;
REVOKE ALL ON TABLE usrgrp FROM farmer;
GRANT ALL ON TABLE usrgrp TO farmer;
GRANT ALL ON TABLE usrgrp TO farmers;


--
-- Name: version; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE version FROM PUBLIC;
REVOKE ALL ON TABLE version FROM farmer;
GRANT ALL ON TABLE version TO farmer;
GRANT ALL ON TABLE version TO farmers;


--
-- Name: version_keyversion_seq; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON SEQUENCE version_keyversion_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE version_keyversion_seq FROM farmer;
GRANT ALL ON SEQUENCE version_keyversion_seq TO farmer;
GRANT ALL ON SEQUENCE version_keyversion_seq TO farmers;


--
-- Name: versionfiletracker; Type: ACL; Schema: public; Owner: farmer
--

REVOKE ALL ON TABLE versionfiletracker FROM PUBLIC;
REVOKE ALL ON TABLE versionfiletracker FROM farmer;
GRANT ALL ON TABLE versionfiletracker TO farmer;
GRANT ALL ON TABLE versionfiletracker TO farmers;


--
-- Name: DEFAULT PRIVILEGES FOR TABLES; Type: DEFAULT ACL; Schema: public; Owner: farmer
--

ALTER DEFAULT PRIVILEGES FOR ROLE farmer IN SCHEMA public REVOKE ALL ON TABLES  FROM PUBLIC;
ALTER DEFAULT PRIVILEGES FOR ROLE farmer IN SCHEMA public REVOKE ALL ON TABLES  FROM farmer;
ALTER DEFAULT PRIVILEGES FOR ROLE farmer IN SCHEMA public GRANT ALL ON TABLES  TO PUBLIC;
ALTER DEFAULT PRIVILEGES FOR ROLE farmer IN SCHEMA public GRANT ALL ON TABLES  TO farmers;


--
-- PostgreSQL database dump complete
--

