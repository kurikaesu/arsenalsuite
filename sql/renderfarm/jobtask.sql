
-- Returns jobtask primary keys
-- In order to be most efficient, we try to find a contionous sequence exactly the size of max_tasks
CREATE OR REPLACE FUNCTION get_continuous_tasks( fkeyjob int, max_tasks int ) RETURNS SETOF int AS $$
DECLARE
	task RECORD;
	start_task int;
	last_task int;
	cont_len int := -1;
	sav_start_task int;
	sav_end_task int;
	sav_cont_len int := 0;
BEGIN
	FOR task IN SELECT count(*) as cnt, jobtask FROM JobTask WHERE JobTask.fkeyjob=fkeyjob AND status='new' GROUP BY jobtask ORDER BY jobtask ASC LOOP
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
		FOR task IN SELECT * FROM JobTask WHERE JobTask.fkeyjob=fkeyjob AND status='new' AND jobtask >= sav_start_task AND jobtask <= sav_end_task ORDER BY jobtask ASC LOOP
			RETURN NEXT task.keyjobtask;
		END LOOP;
	END IF;

	RETURN;
END;
$$ LANGUAGE 'plpgsql';

