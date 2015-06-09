
DROP TYPE hosthistory_status_summary_return CASCADE;
CREATE TYPE hosthistory_status_summary_return AS (status text, loading bool, wasted bool, count int, total_time interval, avg_time interval, max_time interval, min_time interval);

--
-- Constructs a query on hosthistory given the where statement where_stmt and the order/limit clause order_limit
-- both where_stmt and order_limit can be empty strings
--
CREATE OR REPLACE FUNCTION hosthistory_dynamic_query(where_stmt text, order_limit text) RETURNS SETOF hosthistory AS $$
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
$$ LANGUAGE 'PLPGSQL';

--
--  Returns all the rows that overlap the timespan between time_start and time_end
--  that are returned from hosthistory_dynamic_query using where_stmt and order_limit
--
CREATE OR REPLACE FUNCTION hosthistory_overlaps_timespan(time_start timestamp, time_end timestamp, where_stmt text, order_limit text) RETURNS SETOF hosthistory AS $$
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
$$ LANGUAGE 'PLPGSQL';

--
-- Returns ALL records that intersect with the intput timespan
-- Adjusts duration to be the amount of time actually inside the
-- input timespan
-- Original rows retrieved via hosthistory_overlaps_timespan, given
-- the inputed where_stmt and order_limit
--
CREATE OR REPLACE FUNCTION hosthistory_timespan_duration_adjusted(time_start timestamp, time_end timestamp, where_stmt text, order_limit text ) RETURNS SETOF hosthistory AS $$
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
$$ LANGUAGE 'PLPGSQL';

--
-- Creates a summary based on an inputed query that returns hosthistory
-- records
--
CREATE OR REPLACE FUNCTION hosthistory_status_summary(query_input text) RETURNS SETOF hosthistory_status_summary_return AS $$
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
$$ LANGUAGE 'PLPGSQL';

--
-- Using the inputed timespan(history_start to history_end), returns a summary
-- of all rows returned by hosthistory_timespan_duration_adjusted
--
CREATE OR REPLACE FUNCTION hosthistory_status_summary_duration_adjusted( history_start timestamp, history_end timestamp ) RETURNS SETOF hosthistory_status_summary_return AS $$
DECLARE
	rec hosthistory_status_summary_return;
BEGIN
	FOR rec IN SELECT * FROM hosthistory_status_summary( 
		'hosthistory_timespan_duration_adjusted( ' || quote_literal(history_start) || ', ' || quote_literal(history_end) || ','''','''')' ) LOOP
		RETURN NEXT rec;
	END LOOP;
END;
$$ LANGUAGE 'PLPGSQL';

DROP TYPE hosthistory_status_percentages_return CASCADE;
CREATE TYPE hosthistory_status_percentages_return AS (status text, loading boolean, wasted boolean, total_time interval, total_perc float, non_idle_perc float, non_idle_non_wasted_perc float);

CREATE OR REPLACE FUNCTION interval_divide(numerator interval, denominator interval) RETURNS float AS $$
BEGIN
	RETURN extract(epoch from numerator) / extract(epoch from denominator)::float;
END;
$$ LANGUAGE 'PLPGSQL';

CREATE OPERATOR / (
	leftarg = interval,
	rightarg = interval,
	procedure = interval_divide
);

--
-- Using rows from the query_input query, constructs a percentage summary of hosthistory rows,
--  based on hosthistory_status_summary results
--
CREATE OR REPLACE FUNCTION hosthistory_status_percentages( query_input text ) RETURNS SETOF hosthistory_status_percentages_return AS $$
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
$$ LANGUAGE 'PLPGSQL';

--
-- Using the inputed timespan(history_start to history_end), returns a percentage summary
-- of all rows returned by hosthistory_timespan_duration_adjusted
--
CREATE OR REPLACE FUNCTION hosthistory_status_percentages_duration_adjusted( history_start timestamp, history_end timestamp ) RETURNS SETOF hosthistory_status_percentages_return AS $$
DECLARE
	rec hosthistory_status_percentages_return;
BEGIN
	FOR rec IN SELECT * FROM hosthistory_status_percentages( 
		'hosthistory_timespan_duration_adjusted( ' || quote_literal(history_start) || ', ' || quote_literal(history_end) || ','''','''')' ) LOOP
		RETURN NEXT rec;
	END LOOP;
END;
$$ LANGUAGE 'PLPGSQL';

DROP TYPE hosthistory_user_slave_summary_return CASCADE;
CREATE TYPE hosthistory_user_slave_summary_return AS (usr text, host text, hours numeric);

CREATE OR REPLACE FUNCTION hosthistory_user_slave_summary(history_start timestamp, history_end timestamp) RETURNS SETOF hosthistory_user_slave_summary_return AS $$
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
$$ LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION hosthistory_summarize_daily_stats(summarize_date date) RETURNS VOID AS $$
DECLARE
	hostrow record;
BEGIN
	CREATE TEMP TABLE host_status_temp AS select fkeyhost, status, fkeyjoberror is not null as error, fkeyjobtask is null as load, sum(duration) as duration, sum(success::int) as tasksDone, count(*) as cnt from hosthistory_timespan_duration_adjusted( summarize_date::timestamp, (summarize_date + '1 day'::interval)::timestamp, '', '' ) group by fkeyhost, status,  error, load order by fkeyhost desc;

	FOR hostrow IN
		SELECT fkeyhost FROM host_status_temp GROUP BY fkeyhost
	LOOP
		DECLARE
			temprow record;
			new_row hostdailystat;
		BEGIN
			new_row.fkeyhost := hostrow.fkeyhost;
			new_row."date" := summarize_date;
			FOR temprow IN SELECT * FROM host_status_temp WHERE fkeyhost=hostrow.fkeyhost LOOP
				IF temprow.status = 'ready' THEN
					new_row.readyTime := temprow.duration;
				ELSIF temprow.status = 'copy' THEN
					new_row.copyTime := temprow.duration;
				ELSIF temprow.status = 'busy' THEN
					IF temprow.load THEN
						IF temprow.error THEN
							new_row.loadErrorTime := temprow.duration;
							new_row.loadErrorCount := temprow.cnt;
						ELSE
							new_row.loadTime := temprow.duration;
						END IF;
					ELSE
						IF temprow.error THEN
							new_row.taskErrorTime := temprow.duration;
							new_row.taskErrorCount := temprow.cnt;
						ELSE
							new_row.taskTime := temprow.duration;
							new_row.tasksDone := temprow.tasksDone;
						END IF;
					END IF;
				ELSIF temprow.status = 'assigned' THEN
					new_row.assignedTime := temprow.duration;
				ELSIF temprow.status = 'offline' THEN
					new_row.offlineTime := temprow.duration;
				END IF;
			END LOOP;
			INSERT INTO hostdailystat
				(fkeyhost, readytime, assignedtime, copytime, loadtime, tasktime, offlinetime, "date",
				 tasksdone, loaderrorcount, taskerrorcount, loaderrortime, taskerrortime, loadcount)
			VALUES
				(new_row.fkeyhost, new_row.readytime, new_row.assignedtime, new_row.copytime, new_row.loadtime, new_row.tasktime, new_row.offlinetime, new_row."date",
				new_row.tasksdone, new_row.loaderrorcount, new_row.taskerrorcount, new_row.loaderrortime, new_row.taskerrortime, new_row.loadcount );
		END;
	END LOOP;
	DROP TABLE host_status_temp;
END;
$$ LANGUAGE 'plpgsql';

