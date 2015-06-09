
-- Provide backwards compatibility for Host.slavePulse
CREATE OR REPLACE FUNCTION update_HostService() RETURNS trigger AS $$
DECLARE
BEGIN
IF (NEW.pulse != OLD.pulse) OR (NEW.pulse IS NOT NULL AND OLD.pulse IS NULL) THEN
	UPDATE HostStatus Set slavepulse=NEW.pulse WHERE HostStatus.fkeyhost=NEW.fkeyhost;
END IF;
RETURN NEW;
END;
$$ LANGUAGE 'plpgsql';


DROP TRIGGER IF EXISTS update_HostService on hostservice;

CREATE TRIGGER update_HostService
BEFORE UPDATE
ON hostservice
FOR EACH ROW
EXECUTE PROCEDURE update_HostService();

