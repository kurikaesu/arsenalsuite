
-- stored proc and trigger automatically keep hoststatus table lined up with host

DROP FUNCTION IF EXISTS sync_host_to_hoststatus() CASCADE;

CREATE OR REPLACE FUNCTION sync_host_to_hoststatus() RETURNS VOID AS $$
DECLARE
BEGIN
	DELETE FROM HostStatus WHERE fkeyhost NOT IN (SELECT keyhost FROM Host);
	INSERT INTO HostStatus (fkeyhost) SELECT keyhost FROM Host WHERE keyhost NOT IN (SELECT fkeyhost FROM HostStatus WHERE fkeyhost IS NOT NULL);
END;
$$ LANGUAGE 'plpgsql';

CREATE OR REPLACE FUNCTION sync_host_to_hoststatus_trigger() RETURNS trigger AS $$
BEGIN
	PERFORM sync_host_to_hoststatus();
	RETURN NEW;
END
$$ LANGUAGE 'plpgsql';


DROP TRIGGER IF EXISTS sync_host_to_hoststatus_trigger ON host;

CREATE TRIGGER sync_host_to_hoststatus_trigger
AFTER INSERT OR DELETE
ON host
FOR EACH STATEMENT
EXECUTE PROCEDURE sync_host_to_hoststatus_trigger();

select sync_host_to_hoststatus();
