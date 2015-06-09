
-- stored proc and trigger to auto-update hoststatus.lastStatusChange
-- with NOW() whenever hoststatus.slavestatus changes
-- Also Performs inserts and updates to hosthistory when the slavestatus
-- or fkeyjobtask changes

CREATE OR REPLACE FUNCTION hoststatus_update() RETURNS trigger AS $$
BEGIN
	IF (NEW.slavestatus IS NOT NULL)
	AND (NEW.slavestatus != OLD.slavestatus) THEN
		NEW.lastStatusChange = NOW();
	END IF;

	IF (NEW.slavestatus IS NOT NULL)
	AND (NEW.slavestatus != OLD.slavestatus) THEN
		UPDATE hosthistory SET duration = now() - datetime, nextstatus=NEW.slavestatus WHERE duration is null and fkeyhost=NEW.fkeyhost;
		INSERT INTO hosthistory (datetime,fkeyhost,status,laststatus, change_from_ip) VALUES (now(),NEW.fkeyhost,NEW.slavestatus,OLD.slavestatus,inet_client_addr());
	END IF;
	
	RETURN NEW;
END;
$$ LANGUAGE 'plpgsql';

DROP TRIGGER IF EXISTS hoststatus_update_trigger ON hoststatus;

CREATE TRIGGER hoststatus_update_trigger
BEFORE UPDATE
ON hoststatus
FOR EACH ROW
EXECUTE PROCEDURE hoststatus_update();

UPDATE HostStatus SET activeassignmentcount=0 WHERE activeassignmentcount IS NULL;
ALTER TABLE HostStatus ALTER COLUMN activeassignmentcount SET DEFAULT 0;
