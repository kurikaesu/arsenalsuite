CREATE OR REPLACE FUNCTION epoch_to_timestamp(epoch_float float8) RETURNS "timestamp" AS $$
BEGIN
	RETURN ('epoch'::timestamp + epoch_float * '1 second'::interval - '8 hours'::interval)::timestamp;
END;
$$ LANGUAGE 'plpgsql' IMMUTABLE STRICT;