
-- Sets host.fkeyjobtask when a host begins working on a jobtask
-- Sets hosthistory.success when a task is completed
CREATE OR REPLACE FUNCTION update_JobTask_debug() RETURNS trigger AS $$
DECLARE
BEGIN
	NEW.change_from_ip := inet_client_addr();
	NEW.lastChangeTime := now();
	--
	-- Update HostStatus to show current command history and task
	--
RETURN new;
END;
$$ LANGUAGE 'plpgsql';

DROP TRIGGER IF EXISTS update_JobTask_debug ON jobtask;
CREATE TRIGGER update_JobTask_debug
BEFORE UPDATE
ON jobtask
FOR EACH ROW
EXECUTE PROCEDURE update_JobTask_debug();
