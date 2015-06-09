

CREATE OR REPLACE FUNCTION element_fkey_check () RETURNS TRIGGER AS '
	DECLARE
	temp RECORD;
	BEGIN
	IF NEW.fkeyelement IS NULL THEN
		RETURN NEW;
	END IF;
	SELECT INTO temp 1 FROM Element WHERE keyelement=NEW.fkeyelement;
	IF FOUND THEN
		RETURN NEW;
	END IF;
	RETURN NULL;
	end;
' LANGUAGE 'plpgsql';

DROP TRIGGER element_fkeyelement_trigger ON Element;
DROP TRIGGER shot_fkeyelement_trigger ON Shot;
DROP TRIGGER task_fkeyelement_trigger ON Task;
DROP TRIGGER thumbnail_fkeyelement_trigger ON Thumbnail;
DROP TRIGGER project_fkeyelement_trigger ON Project;
DROP TRIGGER asset_fkeyelement_trigger ON Asset;
DROP TRIGGER shotgroup_fkeyelement_trigger ON ShotGroup;

CREATE TRIGGER element_fkeyelement_trigger BEFORE INSERT OR UPDATE ON Element FOR EACH ROW EXECUTE PROCEDURE element_fkey_check();
CREATE TRIGGER shot_fkeyelement_trigger BEFORE INSERT OR UPDATE ON Shot FOR EACH ROW EXECUTE PROCEDURE
element_fkey_check();
CREATE TRIGGER task_fkeyelement_trigger BEFORE INSERT OR UPDATE ON Task FOR EACH ROW EXECUTE PROCEDURE
element_fkey_check();
CREATE TRIGGER thumbnail_fkeyelement_trigger BEFORE INSERT OR UPDATE ON Thumbnail FOR EACH ROW EXECUTE PROCEDURE
element_fkey_check();
CREATE TRIGGER project_fkeyelement_trigger BEFORE INSERT OR UPDATE ON Project FOR EACH ROW EXECUTE PROCEDURE
element_fkey_check();
CREATE TRIGGER asset_fkeyelement_trigger BEFORE INSERT OR UPDATE ON Asset FOR EACH ROW EXECUTE PROCEDURE
element_fkey_check();
CREATE TRIGGER shotgroup_fkeyelement_trigger BEFORE INSERT OR UPDATE ON ShotGroup FOR EACH ROW EXECUTE PROCEDURE
element_fkey_check();

CREATE OR REPLACE FUNCTION element_delete_children () RETURNS TRIGGER AS '
	BEGIN
		DELETE FROM Element WHERE fkeyelement=OLD.keyelement;
		DELETE FROM ElementDep WHERE fkeyelement=OLD.keyelement OR fkeyelementdep=OLD.keyelement;
		DELETE FROM UserElement WHERE fkeyelement=OLD.keyelement;
		DELETE FROM CheckListStatus WHERE fkeyelement=OLD.keyelement;
		DELETE FROM ElementThread WHERE fkeyelement=OLD.keyelement;
		DELETE FROM History WHERE fkeyelement=OLD.keyelement;
		UPDATE TimeSheet SET fkeyelement=NULL WHERE fkeyelement=OLD.keyelement;
		IF TG_RELNAME=''task'' THEN
			DELETE FROM TaskUser WHERE fkeytask=OLD.keyelement;
		END IF;
		IF TG_RELNAME=''project'' THEN
			DELETE FROM GridTemplate WHERE fkeyproject=OLD.keyelement;
		END IF;
		IF TG_RELNAME=''usr'' OR TG_RELNAME=''employee'' THEN
			DELETE FROM UserRole WHERE fkeyusr=OLD.keyelement;
		END IF;
		RETURN NULL;
	END;
' LANGUAGE 'plpgsql';

DROP TRIGGER element_fkeyelement_delete_trigger ON Element;
DROP TRIGGER element_fkeyelement_delete_trigger ON Shot;
DROP TRIGGER element_fkeyelement_delete_trigger ON Task;
DROP TRIGGER element_fkeyelement_delete_trigger ON Thumbnail;
DROP TRIGGER element_fkeyelement_delete_trigger ON Project;
DROP TRIGGER element_fkeyelement_delete_trigger ON Asset;

CREATE TRIGGER element_fkeyelement_delete_trigger AFTER DELETE ON Element FOR EACH ROW EXECUTE PROCEDURE
element_delete_children();
CREATE TRIGGER shot_fkeyelement_delete_trigger AFTER DELETE ON Shot FOR EACH ROW EXECUTE PROCEDURE
element_delete_children();
CREATE TRIGGER task_fkeyelement_delete_trigger AFTER DELETE ON Task FOR EACH ROW EXECUTE PROCEDURE
element_delete_children();
CREATE TRIGGER thumbnail_fkeyelement_delete_trigger AFTER DELETE ON Thumbnail FOR EACH ROW EXECUTE PROCEDURE
element_delete_children();
CREATE TRIGGER project_fkeyelement_delete_trigger AFTER DELETE ON Project FOR EACH ROW EXECUTE PROCEDURE
element_delete_children();
CREATE TRIGGER asset_fkeyelement_delete_trigger AFTER DELETE ON Asset FOR EACH ROW EXECUTE PROCEDURE
element_delete_children();
CREATE TRIGGER shotgroup_fkeyelement_delete_trigger AFTER DELETE ON ShotGroup FOR EACH ROW EXECUTE PROCEDURE
element_delete_children();

