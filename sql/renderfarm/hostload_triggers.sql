

-- stored proc and trigger to auto-update hostload.lastStatusChange
-- with NOW() whenever hostload.slavestatus changes
-- Also Performs inserts and updates to hostloadhistory when the slavestatus
-- or fkeyjobtask changes

CREATE OR REPLACE FUNCTION increment_loadavgadjust(_fkeyhost int) RETURNS VOID AS $$
DECLARE
	loadavgadjust_inc float;
BEGIN
	SELECT INTO loadavgadjust_inc value::float FROM config WHERE config='assburnerLoadAvgAdjustInc';
	UPDATE HostLoad SET loadavgadjust=loadavgadjust_inc+loadavgadjust WHERE fkeyhost=_fkeyhost;
END;
$$ LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION update_hostload() RETURNS trigger AS $$
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
$$ LANGUAGE 'plpgsql';


--DROP TRIGGER update_lastStatusChange on hostload;
DROP TRIGGER update_hostload on hostload;

CREATE TRIGGER update_hostload
BEFORE UPDATE
ON hostload
FOR EACH ROW
EXECUTE PROCEDURE update_hostload();

