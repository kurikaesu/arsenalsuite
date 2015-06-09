
-- Example: select delete_jabber_user_realm( 'newellm', true, 'jabber.blur.com' )
CREATE OR REPLACE FUNCTION delete_jabber_user_realm( user_name text, delete_from_rosters boolean, user_realm text ) RETURNS VOID AS $$
DECLARE
	user_jid text := user_name || '@' || user_realm;
BEGIN
	RAISE NOTICE 'Deleting Jabber User %s', user_jid;
	delete from jabberd.authreg where username=user_name;
	delete from jabberd.active where "collection-owner"=user_jid;
	
	IF delete_from_rosters THEN
		delete from jabberd."roster-items" where jid=user_jid or "collection-owner"=user_jid;
		delete from jabberd."roster-groups" where jid=user_jid or "collection-owner"=user_jid;
	END IF;
	
	delete from jabberd.logout where "collection-owner"=user_jid;
	delete from jabberd."motd-message" where "collection-owner"=user_jid;
	delete from jabberd."motd-times" where "collection-owner"=user_jid;
	delete from jabberd."privacy-default" where "collection-owner"=user_jid;
	delete from jabberd."privacy-items" where "collection-owner"=user_jid;
	delete from jabberd."private" where "collection-owner"=user_jid;
	delete from jabberd.queue where "collection-owner"=user_jid;
	delete from jabberd."vacation-settings" where "collection-owner"=user_jid;
	delete from jabberd.vcard where "collection-owner"=user_jid;
END;
$$ LANGUAGE 'plpgsql';

-- Example: select delete_jabber_user_blur( 'newellm', true );
CREATE OR REPLACE FUNCTION delete_jabber_user_blur( user_name text, delete_from_rosters boolean ) RETURNS VOID AS $$
BEGIN
	PERFORM delete_jabber_user_realm( user_name, delete_from_rosters, 'jabber.blur.com' );
END;
$$ LANGUAGE 'plpgsql';

-- Example: select delete_disabled_blur_employees_from_jabber();
CREATE OR REPLACE FUNCTION delete_disabled_blur_employees_from_jabber() RETURNS VOID AS $$
DECLARE
	user_name text;
BEGIN
	FOR user_name IN SELECT name FROM employee WHERE disabled=1 and dateoftermination is not null AND name NOT IN (SELECT name FROM employee WHERE coalesce(disabled,0)=0 AND dateoftermination IS NULL) LOOP
		PERFORM delete_jabber_user_blur( user_name, true );
	END LOOP;
END;
$$ LANGUAGE 'plpgsql';
