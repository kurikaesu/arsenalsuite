
-- Function to get the persons real name, or username if real name doesn't exist
CREATE OR REPLACE FUNCTION user_fullname( key_user INT ) RETURNS text AS $$
	DECLARE
		temp RECORD;
	BEGIN
		SELECT INTO temp * FROM employee WHERE keyelement=key_user;
		IF NOT FOUND THEN
			SELECT INTO temp * FROM usr WHERE keyelement=key_user;
			IF NOT FOUND THEN
				RETURN '';
			END IF;
			RETURN temp.name;
		END IF;
		RETURN temp.namefirst || ' ' || temp.namelast;
	END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION email_check_atblur( email text ) RETURNS text AS $$
	DECLARE
	BEGIN
		IF contains( email, '@' ) THEN
			RETURN email;
		END IF;
		RETURN email || ''@jabber.blur.com'';
	END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION user_jid( key_user INT ) RETURNS text AS $$
	DECLARE
		temp RECORD;
	BEGIN
		SELECT INTO temp * FROM usr WHERE keyelement=key_user;
		RETURN temp.jid || '@jabber.blur.com';
	END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION jabber_cross_roster( key_user INT, other_key_user INT ) RETURNS void AS $$
	DECLARE
		ownerjid TEXT;
		otherjid TEXT;
	BEGIN
		ownerjid := user_jid( key_user );
		otherjid := user_jid( other_key_user );
		PERFORM * FROM jabberd."roster-items" WHERE "collection-owner"=ownerjid AND jid=otherjid;
		IF NOT FOUND THEN
			INSERT INTO jabberd."roster-items" VALUES (ownerjid, nextval('jabberd.object-sequence'), otherjid, user_fullname(key_user), true, true, null );
		END IF;
		PERFORM jabber_cross_group( key_user, other_key_user );
		RETURN;
	END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION jabber_cross_group( key_user INT, other_key_user INT ) RETURNS void AS $$
	DECLARE
		owner RECORD;
		other RECORD;
		grpiter RECORD;
	BEGIN
		SELECT INTO owner * FROM usr WHERE keyelement=key_user;
		SELECT INTO other * FROM usr WHERE keyelement=other_key_user;
		FOR grpiter IN
			SELECT * FROM grp INNER JOIN usrgrp ON usrgrp.fkeygrp=grp.keygrp 
			WHERE usrgrp.fkeyusr=other.keyelement AND grp.grp NOT IN ('SMB', 'EMail')
		LOOP
			PERFORM * FROM jabberd."roster-groups" WHERE "group"=grpiter.grp AND "collection-owner"=user_jid(key_user) AND jid=user_jid(other_key_user);
			IF NOT FOUND THEN
				INSERT INTO jabberd."roster-groups" VALUES ( user_jid(key_user), nextval('jabberd.object-sequence'), user_jid(other_key_user), grpiter.grp);
			END IF;
		END LOOP;
		RETURN;
	END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION jabber_check_user( key_user INT ) RETURNS void AS $$
	DECLARE
		user RECORD;
		jid TEXT;
		iter RECORD;
	BEGIN
		jid := user_jid($1);
		SELECT INTO user * FROM usr WHERE keyelement=key_user;

		PERFORM * FROM jabberd.authreg WHERE username=user.jid;
		IF NOT FOUND THEN
			INSERT INTO jabberd.authreg( username, realm, password ) VALUES ( user.name, 'jabber.blur.com', user.password );
		END IF;
		PERFORM * FROM jabberd.active WHERE "collection-owner"=jid;
		IF NOT FOUND THEN
			INSERT INTO jabberd.active( "collection-owner", "object-sequence", time ) VALUES ( jid, nextval('jabberd.object-sequence'), extract( epoch from now() )::int );
		END IF;
		PERFORM * FROM jabberd.vcard WHERE "collection-owner"=jid;
		IF NOT FOUND THEN
			DECLARE
				emp RECORD;
			BEGIN
				SELECT INTO emp * FROM employee WHERE keyelement=$1;
				IF NOT FOUND THEN
				INSERT INTO jabberd.vcard VALUES ( jid, nextval( 'jabberd.object-sequence' ), user_fullname($1), null,
					null, null, email_check_atblur(emp.email), null, null, emp.dateofbirth, emp.comment, null,
					null, null, null, null, null, null, null, null, null );
				END IF;
			END;
		END IF;
		FOR iter IN SELECT * FROM usr WHERE disabled=0 AND length(usr.jid) > 0 AND usr.name!=user.name LOOP
			PERFORM jabber_cross_roster ( user.keyelement, iter.keyelement );
			PERFORM jabber_cross_roster ( iter.keyelement, user.keyelement );
		END LOOP;
		RETURN;
	END;
$$ LANGUAGE plpgsql;

-- Updates the jabber registration tables to reflect the usr tables
CREATE OR REPLACE FUNCTION jabber_updatetables() RETURNS void AS $$
	DECLARE
		toadd usr;
		usriter usr;
	BEGIN
		-- Loop through current users that should have a jabber account
		FOR toadd IN SELECT * FROM usr WHERE disabled=0 AND jid!='' AND jid IS NOT NULL LOOP
			PERFORM jabber_check_user( toadd.keyelement );
		END LOOP;
		
		-- Clean out user that arent valid
		DELETE FROM jabberd.active WHERE jabberd.active."collection-owner" NOT IN
			( SELECT name || '@jabber.blur.com' FROM usr WHERE disabled=0 AND jid!='' );
			
		RETURN;
	END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION checkuser_trig() RETURNS trigger AS $$
	BEGIN
		PERFORM jabber_check_user(NEW.keyelement);
		RETURN null;
	END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION userdelete_trig() RETURNS trigger AS $$
	BEGIN
		DELETE FROM jabberd.active WHERE jabberd.active."collection-owner"=user_jid(OLD.keyelement);
		RETURN null;
	END;
$$ LANGUAGE plpgsql;

DROP TRIGGER jabber_trigger ON usr;
DROP TRIGGER jabber_trigger ON employee;
DROP TRIGGER user_delete ON usr;
DROP TRIGGER user_delete ON employee;
CREATE TRIGGER jabber_trigger AFTER INSERT OR UPDATE ON usr FOR EACH ROW EXECUTE PROCEDURE checkuser_trig();
CREATE TRIGGER jabber_trigger AFTER INSERT OR UPDATE ON employee FOR EACH ROW EXECUTE PROCEDURE checkuser_trig();
CREATE TRIGGER user_delete AFTER DELETE ON usr FOR EACH ROW EXECUTE PROCEDURE userdelete_trig();
CREATE TRIGGER user_delete AFTER DELETE ON employee FOR EACH ROW EXECUTE PROCEDURE userdelete_trig();

