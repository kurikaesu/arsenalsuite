CREATE OR REPLACE FUNCTION get_wayward_hosts()
  RETURNS SETOF int4 AS
$BODY$
DECLARE
	hs hoststatus;
BEGIN
	FOR hs IN SELECT * FROM HostStatus WHERE slavestatus IS NOT NULL AND slavestatus NOT IN ('offline','restart','sleeping','waking','no-ping','stopping') AND now() - slavepulse > '10 minutes'::interval LOOP
		RETURN NEXT hs.fkeyhost;
	END LOOP;

	FOR hs in SELECT * FROM HostStatus WHERE slavestatus='assigned' AND now() - laststatuschange > '5 minutes'::interval LOOP
		RETURN NEXT hs.fkeyhost;
	END LOOP;
END;
$BODY$
  LANGUAGE 'plpgsql' VOLATILE;

-- reason is 1 for laststatuschange, 2 for pulse
CREATE TYPE WaywardHostRet AS ( keyhost int, reason int );

DROP FUNCTION get_wayward_hosts_2(interval,interval);

CREATE OR REPLACE FUNCTION get_wayward_hosts_2( pulse_period interval, loop_time interval )
  RETURNS SETOF WaywardHostRet AS
$BODY$
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

	FOR hs IN SELECT * FROM HostStatus WHERE slavestatus IS NOT NULL AND slavestatus IN ('ready','copy','busy','offline') AND (slavepulse IS NULL OR (now() - slavepulse) > (pulse_period + loop_time)) LOOP
		ret := ROW(hs.fkeyhost,2);
		RETURN NEXT ret;
	END LOOP;
END;
$BODY$
  LANGUAGE 'plpgsql' VOLATILE;
