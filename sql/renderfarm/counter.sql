
DROP TABLE CounterCache CASCADE;
CREATE TABLE CounterCache ( hostsTotal int, hostsActive int, hostsReady int, jobsTotal int, jobsActive int, jobsDone int, lastUpdated timestamp );

CREATE OR REPLACE FUNCTION getcounterstate() RETURNS countercache AS $$
	DECLARE
		cache countercache;
	BEGIN
		SELECT INTO cache * FROM countercache LIMIT 1;
		IF NOT FOUND THEN
			INSERT INTO countercache (hoststotal) values (null);
			cache.lastUpdated := now() - interval'1 year';
		END IF;
		IF now() - cache.lastUpdated > interval'10 seconds' THEN
			DECLARE
				hosts_total int;
				hosts_active int;
				hosts_ready int;
				jobs_total int;
				jobs_active int;
				jobs_done int;
			BEGIN
				SELECT count(*) INTO hosts_total FROM HostService INNER JOIN HostStatus ON HostStatus.fkeyHost=HostService.fkeyHost WHERE fkeyService=23 AND online=1;
				SELECT count(fkeyHost) INTO hosts_active FROM HostStatus WHERE slaveStatus IN('busy','copy', 'assigned') AND online=1;
				SELECT count(fkeyHost) INTO hosts_ready FROM HostStatus WHERE slaveStatus='ready';
				SELECT count(keyJob) INTO jobs_total FROM Job WHERE status IN('submit','verify','holding','ready', 'busy', 'started','suspended','done');
				SELECT count(keyJob) INTO jobs_active FROM Job WHERE status IN ('ready', 'started');
				SELECT count(keyJob) INTO jobs_done FROM Job WHERE status='done';
				UPDATE CounterCache SET hoststotal=hosts_total, hostsactive=hosts_active, hostsReady=hosts_ready,
					jobsTotal=jobs_total, jobsActive=jobs_active, jobsDone=jobs_done, lastUpdated=now();
				SELECT INTO cache * FROM countercache LIMIT 1;
			END;
		END IF;
		RETURN cache;
	END;
$$ LANGUAGE plpgsql;