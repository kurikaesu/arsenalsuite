CREATE OR REPLACE FUNCTION hoststatus_update()
  RETURNS trigger AS
$BODY$
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
$BODY$
  LANGUAGE plpgsql VOLATILE
  COST 100;

