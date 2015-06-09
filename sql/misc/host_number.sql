
CREATE OR REPLACE FUNCTION host_number(host_name text) RETURNS int AS $$
	BEGIN
		RETURN substring(host_name,E'\\D+(\\d+)')::int;
	END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION host_name_strip_number(host_name text) RETURNS text AS $$
BEGIN
	RETURN substring(host_name,E'^(\\D+)');
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION host_in_range(host_name text, min_ int, max_ int) RETURNS boolean AS $$
	DECLARE
		host_num int;
	BEGIN
		SELECT INTO host_num host_number(host_name);
		RETURN host_num >= min_ AND host_num <= max_;
	END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION named_host_in_range(host_name text, host_name_prefix text, min_ int, max_ int) RETURNS boolean AS $$
	BEGIN
		RETURN substring(host_name,E'^(\\D+)') = host_name_prefix AND host_in_range(host_name,min_,max_);
	END;
$$ LANGUAGE plpgsql;
