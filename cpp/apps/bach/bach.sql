--
-- PostgreSQL database dump
--

SET client_encoding = 'UTF8';
SET standard_conforming_strings = off;
SET check_function_bodies = false;
SET client_min_messages = warning;
SET escape_string_warning = off;

--
-- Name: SCHEMA public; Type: COMMENT; Schema: -; Owner: postgres
--

COMMENT ON SCHEMA public IS 'Standard public schema';


SET search_path = public, pg_catalog;

--
-- Name: gtsq; Type: SHELL TYPE; Schema: public; Owner: barry
--

CREATE TYPE gtsq;


--
-- Name: gtsq_in(cstring); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gtsq_in(cstring) RETURNS gtsq
    AS '$libdir/tsearch2', 'gtsq_in'
    LANGUAGE c STRICT;


ALTER FUNCTION public.gtsq_in(cstring) OWNER TO barry;

--
-- Name: gtsq_out(gtsq); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gtsq_out(gtsq) RETURNS cstring
    AS '$libdir/tsearch2', 'gtsq_out'
    LANGUAGE c STRICT;


ALTER FUNCTION public.gtsq_out(gtsq) OWNER TO barry;

--
-- Name: gtsq; Type: TYPE; Schema: public; Owner: barry
--

CREATE TYPE gtsq (
    INTERNALLENGTH = 8,
    INPUT = gtsq_in,
    OUTPUT = gtsq_out,
    ALIGNMENT = int4,
    STORAGE = plain
);


ALTER TYPE public.gtsq OWNER TO barry;

--
-- Name: gtsvector; Type: SHELL TYPE; Schema: public; Owner: barry
--

CREATE TYPE gtsvector;


--
-- Name: gtsvector_in(cstring); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gtsvector_in(cstring) RETURNS gtsvector
    AS '$libdir/tsearch2', 'gtsvector_in'
    LANGUAGE c STRICT;


ALTER FUNCTION public.gtsvector_in(cstring) OWNER TO barry;

--
-- Name: gtsvector_out(gtsvector); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gtsvector_out(gtsvector) RETURNS cstring
    AS '$libdir/tsearch2', 'gtsvector_out'
    LANGUAGE c STRICT;


ALTER FUNCTION public.gtsvector_out(gtsvector) OWNER TO barry;

--
-- Name: gtsvector; Type: TYPE; Schema: public; Owner: barry
--

CREATE TYPE gtsvector (
    INTERNALLENGTH = variable,
    INPUT = gtsvector_in,
    OUTPUT = gtsvector_out,
    ALIGNMENT = int4,
    STORAGE = plain
);


ALTER TYPE public.gtsvector OWNER TO barry;

--
-- Name: tsquery; Type: SHELL TYPE; Schema: public; Owner: barry
--

CREATE TYPE tsquery;


--
-- Name: tsquery_in(cstring); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsquery_in(cstring) RETURNS tsquery
    AS '$libdir/tsearch2', 'tsquery_in'
    LANGUAGE c STRICT;


ALTER FUNCTION public.tsquery_in(cstring) OWNER TO barry;

--
-- Name: tsquery_out(tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsquery_out(tsquery) RETURNS cstring
    AS '$libdir/tsearch2', 'tsquery_out'
    LANGUAGE c STRICT;


ALTER FUNCTION public.tsquery_out(tsquery) OWNER TO barry;

--
-- Name: tsquery; Type: TYPE; Schema: public; Owner: barry
--

CREATE TYPE tsquery (
    INTERNALLENGTH = variable,
    INPUT = tsquery_in,
    OUTPUT = tsquery_out,
    ALIGNMENT = int4,
    STORAGE = plain
);


ALTER TYPE public.tsquery OWNER TO barry;

--
-- Name: tsvector; Type: SHELL TYPE; Schema: public; Owner: barry
--

CREATE TYPE tsvector;


--
-- Name: tsvector_in(cstring); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsvector_in(cstring) RETURNS tsvector
    AS '$libdir/tsearch2', 'tsvector_in'
    LANGUAGE c STRICT;


ALTER FUNCTION public.tsvector_in(cstring) OWNER TO barry;

--
-- Name: tsvector_out(tsvector); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsvector_out(tsvector) RETURNS cstring
    AS '$libdir/tsearch2', 'tsvector_out'
    LANGUAGE c STRICT;


ALTER FUNCTION public.tsvector_out(tsvector) OWNER TO barry;

--
-- Name: tsvector; Type: TYPE; Schema: public; Owner: barry
--

CREATE TYPE tsvector (
    INTERNALLENGTH = variable,
    INPUT = tsvector_in,
    OUTPUT = tsvector_out,
    ALIGNMENT = int4,
    STORAGE = extended
);


ALTER TYPE public.tsvector OWNER TO barry;

--
-- Name: statinfo; Type: TYPE; Schema: public; Owner: barry
--

CREATE TYPE statinfo AS (
	word text,
	ndoc integer,
	nentry integer
);


ALTER TYPE public.statinfo OWNER TO barry;

--
-- Name: tokenout; Type: TYPE; Schema: public; Owner: barry
--

CREATE TYPE tokenout AS (
	tokid integer,
	token text
);


ALTER TYPE public.tokenout OWNER TO barry;

--
-- Name: tokentype; Type: TYPE; Schema: public; Owner: barry
--

CREATE TYPE tokentype AS (
	tokid integer,
	alias text,
	descr text
);


ALTER TYPE public.tokentype OWNER TO barry;

--
-- Name: tsdebug; Type: TYPE; Schema: public; Owner: barry
--

CREATE TYPE tsdebug AS (
	ts_name text,
	tok_type text,
	description text,
	token text,
	dict_name text[],
	tsvector tsvector
);


ALTER TYPE public.tsdebug OWNER TO barry;

--
-- Name: _get_parser_from_curcfg(); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION _get_parser_from_curcfg() RETURNS text
    AS $$ select prs_name from pg_ts_cfg where oid = show_curcfg() $$
    LANGUAGE sql IMMUTABLE STRICT;


ALTER FUNCTION public._get_parser_from_curcfg() OWNER TO barry;

--
-- Name: concat(tsvector, tsvector); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION concat(tsvector, tsvector) RETURNS tsvector
    AS '$libdir/tsearch2', 'concat'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.concat(tsvector, tsvector) OWNER TO barry;

--
-- Name: dex_init(internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION dex_init(internal) RETURNS internal
    AS '$libdir/tsearch2', 'dex_init'
    LANGUAGE c;


ALTER FUNCTION public.dex_init(internal) OWNER TO barry;

--
-- Name: dex_lexize(internal, internal, integer); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION dex_lexize(internal, internal, integer) RETURNS internal
    AS '$libdir/tsearch2', 'dex_lexize'
    LANGUAGE c STRICT;


ALTER FUNCTION public.dex_lexize(internal, internal, integer) OWNER TO barry;

--
-- Name: exectsq(tsvector, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION exectsq(tsvector, tsquery) RETURNS boolean
    AS '$libdir/tsearch2', 'exectsq'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.exectsq(tsvector, tsquery) OWNER TO barry;

--
-- Name: FUNCTION exectsq(tsvector, tsquery); Type: COMMENT; Schema: public; Owner: barry
--

COMMENT ON FUNCTION exectsq(tsvector, tsquery) IS 'boolean operation with text index';


--
-- Name: get_covers(tsvector, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION get_covers(tsvector, tsquery) RETURNS text
    AS '$libdir/tsearch2', 'get_covers'
    LANGUAGE c STRICT;


ALTER FUNCTION public.get_covers(tsvector, tsquery) OWNER TO barry;

--
-- Name: gin_extract_tsquery(tsquery, internal, internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gin_extract_tsquery(tsquery, internal, internal) RETURNS internal
    AS '$libdir/tsearch2', 'gin_extract_tsquery'
    LANGUAGE c STRICT;


ALTER FUNCTION public.gin_extract_tsquery(tsquery, internal, internal) OWNER TO barry;

--
-- Name: gin_extract_tsvector(tsvector, internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gin_extract_tsvector(tsvector, internal) RETURNS internal
    AS '$libdir/tsearch2', 'gin_extract_tsvector'
    LANGUAGE c STRICT;


ALTER FUNCTION public.gin_extract_tsvector(tsvector, internal) OWNER TO barry;

--
-- Name: gin_ts_consistent(internal, internal, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gin_ts_consistent(internal, internal, tsquery) RETURNS boolean
    AS '$libdir/tsearch2', 'gin_ts_consistent'
    LANGUAGE c STRICT;


ALTER FUNCTION public.gin_ts_consistent(internal, internal, tsquery) OWNER TO barry;

--
-- Name: gtsq_compress(internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gtsq_compress(internal) RETURNS internal
    AS '$libdir/tsearch2', 'gtsq_compress'
    LANGUAGE c;


ALTER FUNCTION public.gtsq_compress(internal) OWNER TO barry;

--
-- Name: gtsq_consistent(gtsq, internal, integer); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gtsq_consistent(gtsq, internal, integer) RETURNS boolean
    AS '$libdir/tsearch2', 'gtsq_consistent'
    LANGUAGE c;


ALTER FUNCTION public.gtsq_consistent(gtsq, internal, integer) OWNER TO barry;

--
-- Name: gtsq_decompress(internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gtsq_decompress(internal) RETURNS internal
    AS '$libdir/tsearch2', 'gtsq_decompress'
    LANGUAGE c;


ALTER FUNCTION public.gtsq_decompress(internal) OWNER TO barry;

--
-- Name: gtsq_penalty(internal, internal, internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gtsq_penalty(internal, internal, internal) RETURNS internal
    AS '$libdir/tsearch2', 'gtsq_penalty'
    LANGUAGE c STRICT;


ALTER FUNCTION public.gtsq_penalty(internal, internal, internal) OWNER TO barry;

--
-- Name: gtsq_picksplit(internal, internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gtsq_picksplit(internal, internal) RETURNS internal
    AS '$libdir/tsearch2', 'gtsq_picksplit'
    LANGUAGE c;


ALTER FUNCTION public.gtsq_picksplit(internal, internal) OWNER TO barry;

--
-- Name: gtsq_same(gtsq, gtsq, internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gtsq_same(gtsq, gtsq, internal) RETURNS internal
    AS '$libdir/tsearch2', 'gtsq_same'
    LANGUAGE c;


ALTER FUNCTION public.gtsq_same(gtsq, gtsq, internal) OWNER TO barry;

--
-- Name: gtsq_union(bytea, internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gtsq_union(bytea, internal) RETURNS integer[]
    AS '$libdir/tsearch2', 'gtsq_union'
    LANGUAGE c;


ALTER FUNCTION public.gtsq_union(bytea, internal) OWNER TO barry;

--
-- Name: gtsvector_compress(internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gtsvector_compress(internal) RETURNS internal
    AS '$libdir/tsearch2', 'gtsvector_compress'
    LANGUAGE c;


ALTER FUNCTION public.gtsvector_compress(internal) OWNER TO barry;

--
-- Name: gtsvector_consistent(gtsvector, internal, integer); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gtsvector_consistent(gtsvector, internal, integer) RETURNS boolean
    AS '$libdir/tsearch2', 'gtsvector_consistent'
    LANGUAGE c;


ALTER FUNCTION public.gtsvector_consistent(gtsvector, internal, integer) OWNER TO barry;

--
-- Name: gtsvector_decompress(internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gtsvector_decompress(internal) RETURNS internal
    AS '$libdir/tsearch2', 'gtsvector_decompress'
    LANGUAGE c;


ALTER FUNCTION public.gtsvector_decompress(internal) OWNER TO barry;

--
-- Name: gtsvector_penalty(internal, internal, internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gtsvector_penalty(internal, internal, internal) RETURNS internal
    AS '$libdir/tsearch2', 'gtsvector_penalty'
    LANGUAGE c STRICT;


ALTER FUNCTION public.gtsvector_penalty(internal, internal, internal) OWNER TO barry;

--
-- Name: gtsvector_picksplit(internal, internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gtsvector_picksplit(internal, internal) RETURNS internal
    AS '$libdir/tsearch2', 'gtsvector_picksplit'
    LANGUAGE c;


ALTER FUNCTION public.gtsvector_picksplit(internal, internal) OWNER TO barry;

--
-- Name: gtsvector_same(gtsvector, gtsvector, internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gtsvector_same(gtsvector, gtsvector, internal) RETURNS internal
    AS '$libdir/tsearch2', 'gtsvector_same'
    LANGUAGE c;


ALTER FUNCTION public.gtsvector_same(gtsvector, gtsvector, internal) OWNER TO barry;

--
-- Name: gtsvector_union(internal, internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION gtsvector_union(internal, internal) RETURNS integer[]
    AS '$libdir/tsearch2', 'gtsvector_union'
    LANGUAGE c;


ALTER FUNCTION public.gtsvector_union(internal, internal) OWNER TO barry;

--
-- Name: headline(oid, text, tsquery, text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION headline(oid, text, tsquery, text) RETURNS text
    AS '$libdir/tsearch2', 'headline'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.headline(oid, text, tsquery, text) OWNER TO barry;

--
-- Name: headline(oid, text, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION headline(oid, text, tsquery) RETURNS text
    AS '$libdir/tsearch2', 'headline'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.headline(oid, text, tsquery) OWNER TO barry;

--
-- Name: headline(text, text, tsquery, text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION headline(text, text, tsquery, text) RETURNS text
    AS '$libdir/tsearch2', 'headline_byname'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.headline(text, text, tsquery, text) OWNER TO barry;

--
-- Name: headline(text, text, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION headline(text, text, tsquery) RETURNS text
    AS '$libdir/tsearch2', 'headline_byname'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.headline(text, text, tsquery) OWNER TO barry;

--
-- Name: headline(text, tsquery, text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION headline(text, tsquery, text) RETURNS text
    AS '$libdir/tsearch2', 'headline_current'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.headline(text, tsquery, text) OWNER TO barry;

--
-- Name: headline(text, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION headline(text, tsquery) RETURNS text
    AS '$libdir/tsearch2', 'headline_current'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.headline(text, tsquery) OWNER TO barry;

--
-- Name: length(tsvector); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION length(tsvector) RETURNS integer
    AS '$libdir/tsearch2', 'tsvector_length'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.length(tsvector) OWNER TO barry;

--
-- Name: lexize(oid, text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION lexize(oid, text) RETURNS text[]
    AS '$libdir/tsearch2', 'lexize'
    LANGUAGE c STRICT;


ALTER FUNCTION public.lexize(oid, text) OWNER TO barry;

--
-- Name: lexize(text, text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION lexize(text, text) RETURNS text[]
    AS '$libdir/tsearch2', 'lexize_byname'
    LANGUAGE c STRICT;


ALTER FUNCTION public.lexize(text, text) OWNER TO barry;

--
-- Name: lexize(text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION lexize(text) RETURNS text[]
    AS '$libdir/tsearch2', 'lexize_bycurrent'
    LANGUAGE c STRICT;


ALTER FUNCTION public.lexize(text) OWNER TO barry;

--
-- Name: make_tags(text, text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION make_tags(text, text) RETURNS text
    AS $_$
 select coalesce($1,'') || replace(replace($2,'/',' '),'_',' ');
$_$
    LANGUAGE sql;


ALTER FUNCTION public.make_tags(text, text) OWNER TO barry;

--
-- Name: make_tags(text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION make_tags(text) RETURNS text
    AS $_$
 select replace(replace($1,'/',' '),'_',' ');
$_$
    LANGUAGE sql;


ALTER FUNCTION public.make_tags(text) OWNER TO barry;

--
-- Name: numnode(tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION numnode(tsquery) RETURNS integer
    AS '$libdir/tsearch2', 'tsquery_numnode'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.numnode(tsquery) OWNER TO barry;

--
-- Name: parse(oid, text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION parse(oid, text) RETURNS SETOF tokenout
    AS '$libdir/tsearch2', 'parse'
    LANGUAGE c STRICT;


ALTER FUNCTION public.parse(oid, text) OWNER TO barry;

--
-- Name: parse(text, text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION parse(text, text) RETURNS SETOF tokenout
    AS '$libdir/tsearch2', 'parse_byname'
    LANGUAGE c STRICT;


ALTER FUNCTION public.parse(text, text) OWNER TO barry;

--
-- Name: parse(text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION parse(text) RETURNS SETOF tokenout
    AS '$libdir/tsearch2', 'parse_current'
    LANGUAGE c STRICT;


ALTER FUNCTION public.parse(text) OWNER TO barry;

--
-- Name: plainto_tsquery(oid, text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION plainto_tsquery(oid, text) RETURNS tsquery
    AS '$libdir/tsearch2', 'plainto_tsquery'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.plainto_tsquery(oid, text) OWNER TO barry;

--
-- Name: plainto_tsquery(text, text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION plainto_tsquery(text, text) RETURNS tsquery
    AS '$libdir/tsearch2', 'plainto_tsquery_name'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.plainto_tsquery(text, text) OWNER TO barry;

--
-- Name: plainto_tsquery(text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION plainto_tsquery(text) RETURNS tsquery
    AS '$libdir/tsearch2', 'plainto_tsquery_current'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.plainto_tsquery(text) OWNER TO barry;

--
-- Name: prsd_end(internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION prsd_end(internal) RETURNS void
    AS '$libdir/tsearch2', 'prsd_end'
    LANGUAGE c;


ALTER FUNCTION public.prsd_end(internal) OWNER TO barry;

--
-- Name: prsd_getlexeme(internal, internal, internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION prsd_getlexeme(internal, internal, internal) RETURNS integer
    AS '$libdir/tsearch2', 'prsd_getlexeme'
    LANGUAGE c;


ALTER FUNCTION public.prsd_getlexeme(internal, internal, internal) OWNER TO barry;

--
-- Name: prsd_headline(internal, internal, internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION prsd_headline(internal, internal, internal) RETURNS internal
    AS '$libdir/tsearch2', 'prsd_headline'
    LANGUAGE c;


ALTER FUNCTION public.prsd_headline(internal, internal, internal) OWNER TO barry;

--
-- Name: prsd_lextype(internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION prsd_lextype(internal) RETURNS internal
    AS '$libdir/tsearch2', 'prsd_lextype'
    LANGUAGE c;


ALTER FUNCTION public.prsd_lextype(internal) OWNER TO barry;

--
-- Name: prsd_start(internal, integer); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION prsd_start(internal, integer) RETURNS internal
    AS '$libdir/tsearch2', 'prsd_start'
    LANGUAGE c;


ALTER FUNCTION public.prsd_start(internal, integer) OWNER TO barry;

--
-- Name: querytree(tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION querytree(tsquery) RETURNS text
    AS '$libdir/tsearch2', 'tsquerytree'
    LANGUAGE c STRICT;


ALTER FUNCTION public.querytree(tsquery) OWNER TO barry;

--
-- Name: rank(real[], tsvector, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION rank(real[], tsvector, tsquery) RETURNS real
    AS '$libdir/tsearch2', 'rank'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.rank(real[], tsvector, tsquery) OWNER TO barry;

--
-- Name: rank(real[], tsvector, tsquery, integer); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION rank(real[], tsvector, tsquery, integer) RETURNS real
    AS '$libdir/tsearch2', 'rank'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.rank(real[], tsvector, tsquery, integer) OWNER TO barry;

--
-- Name: rank(tsvector, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION rank(tsvector, tsquery) RETURNS real
    AS '$libdir/tsearch2', 'rank_def'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.rank(tsvector, tsquery) OWNER TO barry;

--
-- Name: rank(tsvector, tsquery, integer); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION rank(tsvector, tsquery, integer) RETURNS real
    AS '$libdir/tsearch2', 'rank_def'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.rank(tsvector, tsquery, integer) OWNER TO barry;

--
-- Name: rank_cd(real[], tsvector, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION rank_cd(real[], tsvector, tsquery) RETURNS real
    AS '$libdir/tsearch2', 'rank_cd'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.rank_cd(real[], tsvector, tsquery) OWNER TO barry;

--
-- Name: rank_cd(real[], tsvector, tsquery, integer); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION rank_cd(real[], tsvector, tsquery, integer) RETURNS real
    AS '$libdir/tsearch2', 'rank_cd'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.rank_cd(real[], tsvector, tsquery, integer) OWNER TO barry;

--
-- Name: rank_cd(tsvector, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION rank_cd(tsvector, tsquery) RETURNS real
    AS '$libdir/tsearch2', 'rank_cd_def'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.rank_cd(tsvector, tsquery) OWNER TO barry;

--
-- Name: rank_cd(tsvector, tsquery, integer); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION rank_cd(tsvector, tsquery, integer) RETURNS real
    AS '$libdir/tsearch2', 'rank_cd_def'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.rank_cd(tsvector, tsquery, integer) OWNER TO barry;

--
-- Name: reset_tsearch(); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION reset_tsearch() RETURNS void
    AS '$libdir/tsearch2', 'reset_tsearch'
    LANGUAGE c STRICT;


ALTER FUNCTION public.reset_tsearch() OWNER TO barry;

--
-- Name: rewrite(tsquery, text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION rewrite(tsquery, text) RETURNS tsquery
    AS '$libdir/tsearch2', 'tsquery_rewrite'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.rewrite(tsquery, text) OWNER TO barry;

--
-- Name: rewrite(tsquery, tsquery, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION rewrite(tsquery, tsquery, tsquery) RETURNS tsquery
    AS '$libdir/tsearch2', 'tsquery_rewrite_query'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.rewrite(tsquery, tsquery, tsquery) OWNER TO barry;

--
-- Name: rewrite_accum(tsquery, tsquery[]); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION rewrite_accum(tsquery, tsquery[]) RETURNS tsquery
    AS '$libdir/tsearch2', 'rewrite_accum'
    LANGUAGE c;


ALTER FUNCTION public.rewrite_accum(tsquery, tsquery[]) OWNER TO barry;

--
-- Name: rewrite_finish(tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION rewrite_finish(tsquery) RETURNS tsquery
    AS '$libdir/tsearch2', 'rewrite_finish'
    LANGUAGE c;


ALTER FUNCTION public.rewrite_finish(tsquery) OWNER TO barry;

--
-- Name: rexectsq(tsquery, tsvector); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION rexectsq(tsquery, tsvector) RETURNS boolean
    AS '$libdir/tsearch2', 'rexectsq'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.rexectsq(tsquery, tsvector) OWNER TO barry;

--
-- Name: FUNCTION rexectsq(tsquery, tsvector); Type: COMMENT; Schema: public; Owner: barry
--

COMMENT ON FUNCTION rexectsq(tsquery, tsvector) IS 'boolean operation with text index';


--
-- Name: set_curcfg(integer); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION set_curcfg(integer) RETURNS void
    AS '$libdir/tsearch2', 'set_curcfg'
    LANGUAGE c STRICT;


ALTER FUNCTION public.set_curcfg(integer) OWNER TO barry;

--
-- Name: set_curcfg(text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION set_curcfg(text) RETURNS void
    AS '$libdir/tsearch2', 'set_curcfg_byname'
    LANGUAGE c STRICT;


ALTER FUNCTION public.set_curcfg(text) OWNER TO barry;

--
-- Name: set_curdict(integer); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION set_curdict(integer) RETURNS void
    AS '$libdir/tsearch2', 'set_curdict'
    LANGUAGE c STRICT;


ALTER FUNCTION public.set_curdict(integer) OWNER TO barry;

--
-- Name: set_curdict(text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION set_curdict(text) RETURNS void
    AS '$libdir/tsearch2', 'set_curdict_byname'
    LANGUAGE c STRICT;


ALTER FUNCTION public.set_curdict(text) OWNER TO barry;

--
-- Name: set_curprs(integer); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION set_curprs(integer) RETURNS void
    AS '$libdir/tsearch2', 'set_curprs'
    LANGUAGE c STRICT;


ALTER FUNCTION public.set_curprs(integer) OWNER TO barry;

--
-- Name: set_curprs(text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION set_curprs(text) RETURNS void
    AS '$libdir/tsearch2', 'set_curprs_byname'
    LANGUAGE c STRICT;


ALTER FUNCTION public.set_curprs(text) OWNER TO barry;

--
-- Name: setweight(tsvector, "char"); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION setweight(tsvector, "char") RETURNS tsvector
    AS '$libdir/tsearch2', 'setweight'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.setweight(tsvector, "char") OWNER TO barry;

--
-- Name: show_curcfg(); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION show_curcfg() RETURNS oid
    AS '$libdir/tsearch2', 'show_curcfg'
    LANGUAGE c STRICT;


ALTER FUNCTION public.show_curcfg() OWNER TO barry;

--
-- Name: snb_en_init(internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION snb_en_init(internal) RETURNS internal
    AS '$libdir/tsearch2', 'snb_en_init'
    LANGUAGE c;


ALTER FUNCTION public.snb_en_init(internal) OWNER TO barry;

--
-- Name: snb_lexize(internal, internal, integer); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION snb_lexize(internal, internal, integer) RETURNS internal
    AS '$libdir/tsearch2', 'snb_lexize'
    LANGUAGE c STRICT;


ALTER FUNCTION public.snb_lexize(internal, internal, integer) OWNER TO barry;

--
-- Name: snb_ru_init_koi8(internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION snb_ru_init_koi8(internal) RETURNS internal
    AS '$libdir/tsearch2', 'snb_ru_init_koi8'
    LANGUAGE c;


ALTER FUNCTION public.snb_ru_init_koi8(internal) OWNER TO barry;

--
-- Name: snb_ru_init_utf8(internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION snb_ru_init_utf8(internal) RETURNS internal
    AS '$libdir/tsearch2', 'snb_ru_init_utf8'
    LANGUAGE c;


ALTER FUNCTION public.snb_ru_init_utf8(internal) OWNER TO barry;

--
-- Name: spell_init(internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION spell_init(internal) RETURNS internal
    AS '$libdir/tsearch2', 'spell_init'
    LANGUAGE c;


ALTER FUNCTION public.spell_init(internal) OWNER TO barry;

--
-- Name: spell_lexize(internal, internal, integer); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION spell_lexize(internal, internal, integer) RETURNS internal
    AS '$libdir/tsearch2', 'spell_lexize'
    LANGUAGE c STRICT;


ALTER FUNCTION public.spell_lexize(internal, internal, integer) OWNER TO barry;

--
-- Name: stat(text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION stat(text) RETURNS SETOF statinfo
    AS '$libdir/tsearch2', 'ts_stat'
    LANGUAGE c STRICT;


ALTER FUNCTION public.stat(text) OWNER TO barry;

--
-- Name: stat(text, text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION stat(text, text) RETURNS SETOF statinfo
    AS '$libdir/tsearch2', 'ts_stat'
    LANGUAGE c STRICT;


ALTER FUNCTION public.stat(text, text) OWNER TO barry;

--
-- Name: strip(tsvector); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION strip(tsvector) RETURNS tsvector
    AS '$libdir/tsearch2', 'strip'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.strip(tsvector) OWNER TO barry;

--
-- Name: syn_init(internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION syn_init(internal) RETURNS internal
    AS '$libdir/tsearch2', 'syn_init'
    LANGUAGE c;


ALTER FUNCTION public.syn_init(internal) OWNER TO barry;

--
-- Name: syn_lexize(internal, internal, integer); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION syn_lexize(internal, internal, integer) RETURNS internal
    AS '$libdir/tsearch2', 'syn_lexize'
    LANGUAGE c STRICT;


ALTER FUNCTION public.syn_lexize(internal, internal, integer) OWNER TO barry;

--
-- Name: thesaurus_init(internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION thesaurus_init(internal) RETURNS internal
    AS '$libdir/tsearch2', 'thesaurus_init'
    LANGUAGE c;


ALTER FUNCTION public.thesaurus_init(internal) OWNER TO barry;

--
-- Name: thesaurus_lexize(internal, internal, integer, internal); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION thesaurus_lexize(internal, internal, integer, internal) RETURNS internal
    AS '$libdir/tsearch2', 'thesaurus_lexize'
    LANGUAGE c STRICT;


ALTER FUNCTION public.thesaurus_lexize(internal, internal, integer, internal) OWNER TO barry;

--
-- Name: to_tsquery(oid, text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION to_tsquery(oid, text) RETURNS tsquery
    AS '$libdir/tsearch2', 'to_tsquery'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.to_tsquery(oid, text) OWNER TO barry;

--
-- Name: to_tsquery(text, text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION to_tsquery(text, text) RETURNS tsquery
    AS '$libdir/tsearch2', 'to_tsquery_name'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.to_tsquery(text, text) OWNER TO barry;

--
-- Name: to_tsquery(text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION to_tsquery(text) RETURNS tsquery
    AS '$libdir/tsearch2', 'to_tsquery_current'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.to_tsquery(text) OWNER TO barry;

--
-- Name: to_tsvector(oid, text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION to_tsvector(oid, text) RETURNS tsvector
    AS '$libdir/tsearch2', 'to_tsvector'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.to_tsvector(oid, text) OWNER TO barry;

--
-- Name: to_tsvector(text, text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION to_tsvector(text, text) RETURNS tsvector
    AS '$libdir/tsearch2', 'to_tsvector_name'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.to_tsvector(text, text) OWNER TO barry;

--
-- Name: to_tsvector(text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION to_tsvector(text) RETURNS tsvector
    AS '$libdir/tsearch2', 'to_tsvector_current'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.to_tsvector(text) OWNER TO barry;

--
-- Name: token_type(integer); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION token_type(integer) RETURNS SETOF tokentype
    AS '$libdir/tsearch2', 'token_type'
    LANGUAGE c STRICT;


ALTER FUNCTION public.token_type(integer) OWNER TO barry;

--
-- Name: token_type(text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION token_type(text) RETURNS SETOF tokentype
    AS '$libdir/tsearch2', 'token_type_byname'
    LANGUAGE c STRICT;


ALTER FUNCTION public.token_type(text) OWNER TO barry;

--
-- Name: token_type(); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION token_type() RETURNS SETOF tokentype
    AS '$libdir/tsearch2', 'token_type_current'
    LANGUAGE c STRICT;


ALTER FUNCTION public.token_type() OWNER TO barry;

--
-- Name: ts_debug(text); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION ts_debug(text) RETURNS SETOF tsdebug
    AS $_$
select 
        m.ts_name,
        t.alias as tok_type,
        t.descr as description,
        p.token,
        m.dict_name,
        strip(to_tsvector(p.token)) as tsvector
from
        parse( _get_parser_from_curcfg(), $1 ) as p,
        token_type() as t,
        pg_ts_cfgmap as m,
        pg_ts_cfg as c
where
        t.tokid=p.tokid and
        t.alias = m.tok_alias and 
        m.ts_name=c.ts_name and 
        c.oid=show_curcfg() 
$_$
    LANGUAGE sql STRICT;


ALTER FUNCTION public.ts_debug(text) OWNER TO barry;

--
-- Name: tsearch2(); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsearch2() RETURNS "trigger"
    AS '$libdir/tsearch2', 'tsearch2'
    LANGUAGE c;


ALTER FUNCTION public.tsearch2() OWNER TO barry;

--
-- Name: tsq_mcontained(tsquery, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsq_mcontained(tsquery, tsquery) RETURNS boolean
    AS '$libdir/tsearch2', 'tsq_mcontained'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.tsq_mcontained(tsquery, tsquery) OWNER TO barry;

--
-- Name: tsq_mcontains(tsquery, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsq_mcontains(tsquery, tsquery) RETURNS boolean
    AS '$libdir/tsearch2', 'tsq_mcontains'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.tsq_mcontains(tsquery, tsquery) OWNER TO barry;

--
-- Name: tsquery_and(tsquery, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsquery_and(tsquery, tsquery) RETURNS tsquery
    AS '$libdir/tsearch2', 'tsquery_and'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.tsquery_and(tsquery, tsquery) OWNER TO barry;

--
-- Name: tsquery_cmp(tsquery, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsquery_cmp(tsquery, tsquery) RETURNS integer
    AS '$libdir/tsearch2', 'tsquery_cmp'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.tsquery_cmp(tsquery, tsquery) OWNER TO barry;

--
-- Name: tsquery_eq(tsquery, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsquery_eq(tsquery, tsquery) RETURNS boolean
    AS '$libdir/tsearch2', 'tsquery_eq'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.tsquery_eq(tsquery, tsquery) OWNER TO barry;

--
-- Name: tsquery_ge(tsquery, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsquery_ge(tsquery, tsquery) RETURNS boolean
    AS '$libdir/tsearch2', 'tsquery_ge'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.tsquery_ge(tsquery, tsquery) OWNER TO barry;

--
-- Name: tsquery_gt(tsquery, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsquery_gt(tsquery, tsquery) RETURNS boolean
    AS '$libdir/tsearch2', 'tsquery_gt'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.tsquery_gt(tsquery, tsquery) OWNER TO barry;

--
-- Name: tsquery_le(tsquery, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsquery_le(tsquery, tsquery) RETURNS boolean
    AS '$libdir/tsearch2', 'tsquery_le'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.tsquery_le(tsquery, tsquery) OWNER TO barry;

--
-- Name: tsquery_lt(tsquery, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsquery_lt(tsquery, tsquery) RETURNS boolean
    AS '$libdir/tsearch2', 'tsquery_lt'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.tsquery_lt(tsquery, tsquery) OWNER TO barry;

--
-- Name: tsquery_ne(tsquery, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsquery_ne(tsquery, tsquery) RETURNS boolean
    AS '$libdir/tsearch2', 'tsquery_ne'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.tsquery_ne(tsquery, tsquery) OWNER TO barry;

--
-- Name: tsquery_not(tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsquery_not(tsquery) RETURNS tsquery
    AS '$libdir/tsearch2', 'tsquery_not'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.tsquery_not(tsquery) OWNER TO barry;

--
-- Name: tsquery_or(tsquery, tsquery); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsquery_or(tsquery, tsquery) RETURNS tsquery
    AS '$libdir/tsearch2', 'tsquery_or'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.tsquery_or(tsquery, tsquery) OWNER TO barry;

--
-- Name: tsvector_cmp(tsvector, tsvector); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsvector_cmp(tsvector, tsvector) RETURNS integer
    AS '$libdir/tsearch2', 'tsvector_cmp'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.tsvector_cmp(tsvector, tsvector) OWNER TO barry;

--
-- Name: tsvector_eq(tsvector, tsvector); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsvector_eq(tsvector, tsvector) RETURNS boolean
    AS '$libdir/tsearch2', 'tsvector_eq'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.tsvector_eq(tsvector, tsvector) OWNER TO barry;

--
-- Name: tsvector_ge(tsvector, tsvector); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsvector_ge(tsvector, tsvector) RETURNS boolean
    AS '$libdir/tsearch2', 'tsvector_ge'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.tsvector_ge(tsvector, tsvector) OWNER TO barry;

--
-- Name: tsvector_gt(tsvector, tsvector); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsvector_gt(tsvector, tsvector) RETURNS boolean
    AS '$libdir/tsearch2', 'tsvector_gt'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.tsvector_gt(tsvector, tsvector) OWNER TO barry;

--
-- Name: tsvector_le(tsvector, tsvector); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsvector_le(tsvector, tsvector) RETURNS boolean
    AS '$libdir/tsearch2', 'tsvector_le'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.tsvector_le(tsvector, tsvector) OWNER TO barry;

--
-- Name: tsvector_lt(tsvector, tsvector); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsvector_lt(tsvector, tsvector) RETURNS boolean
    AS '$libdir/tsearch2', 'tsvector_lt'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.tsvector_lt(tsvector, tsvector) OWNER TO barry;

--
-- Name: tsvector_ne(tsvector, tsvector); Type: FUNCTION; Schema: public; Owner: barry
--

CREATE FUNCTION tsvector_ne(tsvector, tsvector) RETURNS boolean
    AS '$libdir/tsearch2', 'tsvector_ne'
    LANGUAGE c IMMUTABLE STRICT;


ALTER FUNCTION public.tsvector_ne(tsvector, tsvector) OWNER TO barry;

--
-- Name: rewrite(tsquery[]); Type: AGGREGATE; Schema: public; Owner: barry
--

CREATE AGGREGATE rewrite(tsquery[]) (
    SFUNC = rewrite_accum,
    STYPE = tsquery,
    FINALFUNC = rewrite_finish
);


ALTER AGGREGATE public.rewrite(tsquery[]) OWNER TO barry;

--
-- Name: !!; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR !! (
    PROCEDURE = tsquery_not,
    RIGHTARG = tsquery
);


ALTER OPERATOR public.!! (NONE, tsquery) OWNER TO barry;

--
-- Name: &&; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR && (
    PROCEDURE = tsquery_and,
    LEFTARG = tsquery,
    RIGHTARG = tsquery,
    COMMUTATOR = &&,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.&& (tsquery, tsquery) OWNER TO barry;

--
-- Name: <; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR < (
    PROCEDURE = tsvector_lt,
    LEFTARG = tsvector,
    RIGHTARG = tsvector,
    COMMUTATOR = >,
    NEGATOR = >=,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.< (tsvector, tsvector) OWNER TO barry;

--
-- Name: <; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR < (
    PROCEDURE = tsquery_lt,
    LEFTARG = tsquery,
    RIGHTARG = tsquery,
    COMMUTATOR = >,
    NEGATOR = >=,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.< (tsquery, tsquery) OWNER TO barry;

--
-- Name: <=; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR <= (
    PROCEDURE = tsvector_le,
    LEFTARG = tsvector,
    RIGHTARG = tsvector,
    COMMUTATOR = >=,
    NEGATOR = >,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.<= (tsvector, tsvector) OWNER TO barry;

--
-- Name: <=; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR <= (
    PROCEDURE = tsquery_le,
    LEFTARG = tsquery,
    RIGHTARG = tsquery,
    COMMUTATOR = >=,
    NEGATOR = >,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.<= (tsquery, tsquery) OWNER TO barry;

--
-- Name: <>; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR <> (
    PROCEDURE = tsvector_ne,
    LEFTARG = tsvector,
    RIGHTARG = tsvector,
    COMMUTATOR = <>,
    NEGATOR = =,
    RESTRICT = neqsel,
    JOIN = neqjoinsel
);


ALTER OPERATOR public.<> (tsvector, tsvector) OWNER TO barry;

--
-- Name: <>; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR <> (
    PROCEDURE = tsquery_ne,
    LEFTARG = tsquery,
    RIGHTARG = tsquery,
    COMMUTATOR = <>,
    NEGATOR = =,
    RESTRICT = neqsel,
    JOIN = neqjoinsel
);


ALTER OPERATOR public.<> (tsquery, tsquery) OWNER TO barry;

--
-- Name: <@; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR <@ (
    PROCEDURE = tsq_mcontained,
    LEFTARG = tsquery,
    RIGHTARG = tsquery,
    COMMUTATOR = @>,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.<@ (tsquery, tsquery) OWNER TO barry;

--
-- Name: =; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR = (
    PROCEDURE = tsvector_eq,
    LEFTARG = tsvector,
    RIGHTARG = tsvector,
    COMMUTATOR = =,
    NEGATOR = <>,
    RESTRICT = eqsel,
    JOIN = eqjoinsel,
    SORT1 = <,
    SORT2 = <,
    LTCMP = <,
    GTCMP = >
);


ALTER OPERATOR public.= (tsvector, tsvector) OWNER TO barry;

--
-- Name: =; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR = (
    PROCEDURE = tsquery_eq,
    LEFTARG = tsquery,
    RIGHTARG = tsquery,
    COMMUTATOR = =,
    NEGATOR = <>,
    RESTRICT = eqsel,
    JOIN = eqjoinsel,
    SORT1 = <,
    SORT2 = <,
    LTCMP = <,
    GTCMP = >
);


ALTER OPERATOR public.= (tsquery, tsquery) OWNER TO barry;

--
-- Name: >; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR > (
    PROCEDURE = tsvector_gt,
    LEFTARG = tsvector,
    RIGHTARG = tsvector,
    COMMUTATOR = <,
    NEGATOR = <=,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.> (tsvector, tsvector) OWNER TO barry;

--
-- Name: >; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR > (
    PROCEDURE = tsquery_gt,
    LEFTARG = tsquery,
    RIGHTARG = tsquery,
    COMMUTATOR = <,
    NEGATOR = <=,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.> (tsquery, tsquery) OWNER TO barry;

--
-- Name: >=; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR >= (
    PROCEDURE = tsvector_ge,
    LEFTARG = tsvector,
    RIGHTARG = tsvector,
    COMMUTATOR = <=,
    NEGATOR = <,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.>= (tsvector, tsvector) OWNER TO barry;

--
-- Name: >=; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR >= (
    PROCEDURE = tsquery_ge,
    LEFTARG = tsquery,
    RIGHTARG = tsquery,
    COMMUTATOR = <=,
    NEGATOR = <,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.>= (tsquery, tsquery) OWNER TO barry;

--
-- Name: @; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR @ (
    PROCEDURE = tsq_mcontains,
    LEFTARG = tsquery,
    RIGHTARG = tsquery,
    COMMUTATOR = ~,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.@ (tsquery, tsquery) OWNER TO barry;

--
-- Name: @>; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR @> (
    PROCEDURE = tsq_mcontains,
    LEFTARG = tsquery,
    RIGHTARG = tsquery,
    COMMUTATOR = <@,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.@> (tsquery, tsquery) OWNER TO barry;

--
-- Name: @@; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR @@ (
    PROCEDURE = rexectsq,
    LEFTARG = tsquery,
    RIGHTARG = tsvector,
    COMMUTATOR = @@,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.@@ (tsquery, tsvector) OWNER TO barry;

--
-- Name: @@; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR @@ (
    PROCEDURE = exectsq,
    LEFTARG = tsvector,
    RIGHTARG = tsquery,
    COMMUTATOR = @@,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.@@ (tsvector, tsquery) OWNER TO barry;

--
-- Name: @@@; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR @@@ (
    PROCEDURE = rexectsq,
    LEFTARG = tsquery,
    RIGHTARG = tsvector,
    COMMUTATOR = @@@,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.@@@ (tsquery, tsvector) OWNER TO barry;

--
-- Name: @@@; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR @@@ (
    PROCEDURE = exectsq,
    LEFTARG = tsvector,
    RIGHTARG = tsquery,
    COMMUTATOR = @@@,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.@@@ (tsvector, tsquery) OWNER TO barry;

--
-- Name: ||; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR || (
    PROCEDURE = concat,
    LEFTARG = tsvector,
    RIGHTARG = tsvector
);


ALTER OPERATOR public.|| (tsvector, tsvector) OWNER TO barry;

--
-- Name: ||; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR || (
    PROCEDURE = tsquery_or,
    LEFTARG = tsquery,
    RIGHTARG = tsquery,
    COMMUTATOR = ||,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.|| (tsquery, tsquery) OWNER TO barry;

--
-- Name: ~; Type: OPERATOR; Schema: public; Owner: barry
--

CREATE OPERATOR ~ (
    PROCEDURE = tsq_mcontained,
    LEFTARG = tsquery,
    RIGHTARG = tsquery,
    COMMUTATOR = @,
    RESTRICT = contsel,
    JOIN = contjoinsel
);


ALTER OPERATOR public.~ (tsquery, tsquery) OWNER TO barry;

--
-- Name: gin_tsvector_ops; Type: OPERATOR CLASS; Schema: public; Owner: barry
--

CREATE OPERATOR CLASS gin_tsvector_ops
    DEFAULT FOR TYPE tsvector USING gin AS
    STORAGE text ,
    OPERATOR 1 @@(tsvector,tsquery) ,
    OPERATOR 2 @@@(tsvector,tsquery) RECHECK ,
    FUNCTION 1 bttextcmp(text,text) ,
    FUNCTION 2 gin_extract_tsvector(tsvector,internal) ,
    FUNCTION 3 gin_extract_tsquery(tsquery,internal,internal) ,
    FUNCTION 4 gin_ts_consistent(internal,internal,tsquery);


ALTER OPERATOR CLASS public.gin_tsvector_ops USING gin OWNER TO barry;

--
-- Name: gist_tp_tsquery_ops; Type: OPERATOR CLASS; Schema: public; Owner: barry
--

CREATE OPERATOR CLASS gist_tp_tsquery_ops
    DEFAULT FOR TYPE tsquery USING gist AS
    STORAGE gtsq ,
    OPERATOR 7 @>(tsquery,tsquery) RECHECK ,
    OPERATOR 8 <@(tsquery,tsquery) RECHECK ,
    OPERATOR 13 @(tsquery,tsquery) RECHECK ,
    OPERATOR 14 ~(tsquery,tsquery) RECHECK ,
    FUNCTION 1 gtsq_consistent(gtsq,internal,integer) ,
    FUNCTION 2 gtsq_union(bytea,internal) ,
    FUNCTION 3 gtsq_compress(internal) ,
    FUNCTION 4 gtsq_decompress(internal) ,
    FUNCTION 5 gtsq_penalty(internal,internal,internal) ,
    FUNCTION 6 gtsq_picksplit(internal,internal) ,
    FUNCTION 7 gtsq_same(gtsq,gtsq,internal);


ALTER OPERATOR CLASS public.gist_tp_tsquery_ops USING gist OWNER TO barry;

--
-- Name: gist_tsvector_ops; Type: OPERATOR CLASS; Schema: public; Owner: barry
--

CREATE OPERATOR CLASS gist_tsvector_ops
    DEFAULT FOR TYPE tsvector USING gist AS
    STORAGE gtsvector ,
    OPERATOR 1 @@(tsvector,tsquery) RECHECK ,
    FUNCTION 1 gtsvector_consistent(gtsvector,internal,integer) ,
    FUNCTION 2 gtsvector_union(internal,internal) ,
    FUNCTION 3 gtsvector_compress(internal) ,
    FUNCTION 4 gtsvector_decompress(internal) ,
    FUNCTION 5 gtsvector_penalty(internal,internal,internal) ,
    FUNCTION 6 gtsvector_picksplit(internal,internal) ,
    FUNCTION 7 gtsvector_same(gtsvector,gtsvector,internal);


ALTER OPERATOR CLASS public.gist_tsvector_ops USING gist OWNER TO barry;

--
-- Name: tsquery_ops; Type: OPERATOR CLASS; Schema: public; Owner: barry
--

CREATE OPERATOR CLASS tsquery_ops
    DEFAULT FOR TYPE tsquery USING btree AS
    OPERATOR 1 <(tsquery,tsquery) ,
    OPERATOR 2 <=(tsquery,tsquery) ,
    OPERATOR 3 =(tsquery,tsquery) ,
    OPERATOR 4 >=(tsquery,tsquery) ,
    OPERATOR 5 >(tsquery,tsquery) ,
    FUNCTION 1 tsquery_cmp(tsquery,tsquery);


ALTER OPERATOR CLASS public.tsquery_ops USING btree OWNER TO barry;

--
-- Name: tsvector_ops; Type: OPERATOR CLASS; Schema: public; Owner: barry
--

CREATE OPERATOR CLASS tsvector_ops
    DEFAULT FOR TYPE tsvector USING btree AS
    OPERATOR 1 <(tsvector,tsvector) ,
    OPERATOR 2 <=(tsvector,tsvector) ,
    OPERATOR 3 =(tsvector,tsvector) ,
    OPERATOR 4 >=(tsvector,tsvector) ,
    OPERATOR 5 >(tsvector,tsvector) ,
    FUNCTION 1 tsvector_cmp(tsvector,tsvector);


ALTER OPERATOR CLASS public.tsvector_ops USING btree OWNER TO barry;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: abdownloadstat; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE abdownloadstat (
    keyabdownloadstat integer NOT NULL,
    "type" text,
    size integer,
    "time" integer,
    fkeyhost integer,
    abrev integer,
    finished timestamp without time zone,
    fkeyjob integer
);


ALTER TABLE public.abdownloadstat OWNER TO barry;

--
-- Name: abdownloadstat_keyabdownloadstat_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE abdownloadstat_keyabdownloadstat_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.abdownloadstat_keyabdownloadstat_seq OWNER TO barry;

--
-- Name: abdownloadstat_keyabdownloadstat_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE abdownloadstat_keyabdownloadstat_seq OWNED BY abdownloadstat.keyabdownloadstat;


--
-- Name: annotation; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE annotation (
    keyannotation integer NOT NULL,
    notes text,
    markupdata text,
    framestart integer,
    frameend integer,
    "sequence" text
);


ALTER TABLE public.annotation OWNER TO barry;

--
-- Name: annotation_keyannotation_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE annotation_keyannotation_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.annotation_keyannotation_seq OWNER TO barry;

--
-- Name: annotation_keyannotation_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE annotation_keyannotation_seq OWNED BY annotation.keyannotation;


--
-- Name: element; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE element (
    keyelement integer NOT NULL,
    daysbid double precision,
    description text,
    filepath text,
    fkeyelement integer,
    fkeyelementstatus integer,
    fkeyelementtype integer,
    fkeyproject integer,
    fkeythumbnail integer,
    name text,
    daysscheduled double precision,
    daysestimated double precision,
    fkeyassettype integer,
    fkeypathtemplate integer,
    fkeystatusset integer,
    allowtime boolean,
    datestart date,
    datecomplete date,
    fkeyassettemplate integer,
    icon bytea
);


ALTER TABLE public.element OWNER TO barry;

--
-- Name: element_keyelement_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE element_keyelement_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.element_keyelement_seq OWNER TO barry;

--
-- Name: element_keyelement_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE element_keyelement_seq OWNED BY element.keyelement;


--
-- Name: assetgroup; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE assetgroup (
)
INHERITS (element);


ALTER TABLE public.assetgroup OWNER TO barry;

--
-- Name: assetproperty; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE assetproperty (
    keyassetproperty integer NOT NULL,
    name text,
    "type" integer,
    value text,
    fkeyelement integer
);


ALTER TABLE public.assetproperty OWNER TO barry;

--
-- Name: assetproperty_keyassetproperty_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE assetproperty_keyassetproperty_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.assetproperty_keyassetproperty_seq OWNER TO barry;

--
-- Name: assetproperty_keyassetproperty_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE assetproperty_keyassetproperty_seq OWNED BY assetproperty.keyassetproperty;


--
-- Name: assettemplate; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE assettemplate (
    keyassettemplate integer NOT NULL,
    fkeyelement integer,
    fkeyassettype integer,
    fkeyproject integer,
    name text
);


ALTER TABLE public.assettemplate OWNER TO barry;

--
-- Name: assettemplate_keyassettemplate_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE assettemplate_keyassettemplate_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.assettemplate_keyassettemplate_seq OWNER TO barry;

--
-- Name: assettemplate_keyassettemplate_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE assettemplate_keyassettemplate_seq OWNED BY assettemplate.keyassettemplate;


--
-- Name: bachasset; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE bachasset (
    keybachasset integer NOT NULL,
    path text NOT NULL,
    tags text,
    fti_tags tsvector,
    imported timestamp without time zone,
    exif text,
    height integer,
    width integer
);


ALTER TABLE public.bachasset OWNER TO barry;

--
-- Name: bachasset_keybachasset_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE bachasset_keybachasset_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.bachasset_keybachasset_seq OWNER TO barry;

--
-- Name: bachasset_keybachasset_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE bachasset_keybachasset_seq OWNED BY bachasset.keybachasset;


--
-- Name: calendar; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE calendar (
    keycalendar integer NOT NULL,
    repeat integer,
    fkeycalendarcategory integer,
    url text,
    fkeyauthor integer,
    fieldname text,
    notifylist text,
    notifybatch text,
    leadtime integer,
    notifymask integer,
    fkeyusr integer,
    private integer,
    date timestamp without time zone,
    calendar text,
    fkeyproject integer
);


ALTER TABLE public.calendar OWNER TO barry;

--
-- Name: calendar_keycalendar_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE calendar_keycalendar_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.calendar_keycalendar_seq OWNER TO barry;

--
-- Name: calendar_keycalendar_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE calendar_keycalendar_seq OWNED BY calendar.keycalendar;


--
-- Name: calendarcategory; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE calendarcategory (
    keycalendarcategory integer NOT NULL,
    calendarcategory text
);


ALTER TABLE public.calendarcategory OWNER TO barry;

--
-- Name: calendarcategory_keycalendarcategory_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE calendarcategory_keycalendarcategory_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.calendarcategory_keycalendarcategory_seq OWNER TO barry;

--
-- Name: calendarcategory_keycalendarcategory_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE calendarcategory_keycalendarcategory_seq OWNED BY calendarcategory.keycalendarcategory;


--
-- Name: checklistitem; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE checklistitem (
    keychecklistitem integer NOT NULL,
    body text,
    checklistitem text,
    fkeyproject integer,
    fkeythumbnail integer,
    fkeytimesheetcategory integer,
    fkeystatusset integer
);


ALTER TABLE public.checklistitem OWNER TO barry;

--
-- Name: checklistitem_keychecklistitem_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE checklistitem_keychecklistitem_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.checklistitem_keychecklistitem_seq OWNER TO barry;

--
-- Name: checklistitem_keychecklistitem_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE checklistitem_keychecklistitem_seq OWNED BY checklistitem.keychecklistitem;


--
-- Name: checkliststatus; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE checkliststatus (
    keycheckliststatus integer NOT NULL,
    fkeychecklistitem integer,
    fkeyelement integer,
    fkeyelementstatus integer
);


ALTER TABLE public.checkliststatus OWNER TO barry;

--
-- Name: checkliststatus_keycheckliststatus_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE checkliststatus_keycheckliststatus_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.checkliststatus_keycheckliststatus_seq OWNER TO barry;

--
-- Name: checkliststatus_keycheckliststatus_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE checkliststatus_keycheckliststatus_seq OWNED BY checkliststatus.keycheckliststatus;


--
-- Name: client; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE client (
    keyclient integer NOT NULL,
    client text,
    textcard text
);


ALTER TABLE public.client OWNER TO barry;

--
-- Name: client_keyclient_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE client_keyclient_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.client_keyclient_seq OWNER TO barry;

--
-- Name: client_keyclient_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE client_keyclient_seq OWNED BY client.keyclient;


--
-- Name: config; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE config (
    keyconfig integer NOT NULL,
    config text,
    value text
);


ALTER TABLE public.config OWNER TO barry;

--
-- Name: config_keyconfig_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE config_keyconfig_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.config_keyconfig_seq OWNER TO barry;

--
-- Name: config_keyconfig_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE config_keyconfig_seq OWNED BY config.keyconfig;


--
-- Name: deliveryelement; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE deliveryelement (
    keydeliveryshot integer NOT NULL,
    fkeydelivery integer,
    fkeyelement integer,
    framestart integer,
    frameend integer
);


ALTER TABLE public.deliveryelement OWNER TO barry;

--
-- Name: deliveryelement_keydeliveryshot_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE deliveryelement_keydeliveryshot_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.deliveryelement_keydeliveryshot_seq OWNER TO barry;

--
-- Name: deliveryelement_keydeliveryshot_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE deliveryelement_keydeliveryshot_seq OWNED BY deliveryelement.keydeliveryshot;


--
-- Name: diskimage; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE diskimage (
    keydiskimage integer NOT NULL,
    diskimage text,
    path text,
    created timestamp without time zone
);


ALTER TABLE public.diskimage OWNER TO barry;

--
-- Name: diskimage_keydiskimage_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE diskimage_keydiskimage_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.diskimage_keydiskimage_seq OWNER TO barry;

--
-- Name: diskimage_keydiskimage_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE diskimage_keydiskimage_seq OWNED BY diskimage.keydiskimage;


--
-- Name: hostgroup; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE hostgroup (
    keyhostgroup integer NOT NULL,
    hostgroup text,
    fkeyusr integer,
    private boolean
);


ALTER TABLE public.hostgroup OWNER TO barry;

--
-- Name: hostgroup_keyhostgroup_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE hostgroup_keyhostgroup_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.hostgroup_keyhostgroup_seq OWNER TO barry;

--
-- Name: hostgroup_keyhostgroup_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE hostgroup_keyhostgroup_seq OWNED BY hostgroup.keyhostgroup;


--
-- Name: dynamichostgroup; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE dynamichostgroup (
    keydynamichostgroup integer NOT NULL,
    hostwhereclause text
)
INHERITS (hostgroup);


ALTER TABLE public.dynamichostgroup OWNER TO barry;

--
-- Name: dynamichostgroup_keydynamichostgroup_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE dynamichostgroup_keydynamichostgroup_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.dynamichostgroup_keydynamichostgroup_seq OWNER TO barry;

--
-- Name: dynamichostgroup_keydynamichostgroup_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE dynamichostgroup_keydynamichostgroup_seq OWNED BY dynamichostgroup.keydynamichostgroup;


--
-- Name: elementdep; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE elementdep (
    keyelementdep integer NOT NULL,
    fkeyelement integer,
    fkeyelementdep integer
);


ALTER TABLE public.elementdep OWNER TO barry;

--
-- Name: elementdep_keyelementdep_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE elementdep_keyelementdep_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.elementdep_keyelementdep_seq OWNER TO barry;

--
-- Name: elementdep_keyelementdep_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE elementdep_keyelementdep_seq OWNED BY elementdep.keyelementdep;


--
-- Name: elementstatus; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE elementstatus (
    keyelementstatus integer NOT NULL,
    name text,
    color text,
    fkeystatusset integer,
    "order" integer
);


ALTER TABLE public.elementstatus OWNER TO barry;

--
-- Name: elementstatus_keyelementstatus_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE elementstatus_keyelementstatus_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.elementstatus_keyelementstatus_seq OWNER TO barry;

--
-- Name: elementstatus_keyelementstatus_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE elementstatus_keyelementstatus_seq OWNED BY elementstatus.keyelementstatus;


--
-- Name: elementthread; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE elementthread (
    keyelementthread integer NOT NULL,
    datetime timestamp without time zone,
    elementthread text,
    fkeyelement integer,
    fkeyusr integer,
    skeyreply integer,
    topic text,
    todostatus integer,
    hasattachments integer
);


ALTER TABLE public.elementthread OWNER TO barry;

--
-- Name: elementthread_keyelementthread_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE elementthread_keyelementthread_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.elementthread_keyelementthread_seq OWNER TO barry;

--
-- Name: elementthread_keyelementthread_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE elementthread_keyelementthread_seq OWNED BY elementthread.keyelementthread;


--
-- Name: elementtype; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE elementtype (
    keyelementtype integer NOT NULL,
    elementtype text,
    sortprefix text
);


ALTER TABLE public.elementtype OWNER TO barry;

--
-- Name: elementtype_keyelementtype_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE elementtype_keyelementtype_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.elementtype_keyelementtype_seq OWNER TO barry;

--
-- Name: elementtype_keyelementtype_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE elementtype_keyelementtype_seq OWNED BY elementtype.keyelementtype;


--
-- Name: elementuser; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE elementuser (
    keyelementuser integer NOT NULL,
    fkeyelement integer,
    fkeyuser integer,
    active boolean,
    fkeyassettype integer
);


ALTER TABLE public.elementuser OWNER TO barry;

--
-- Name: elementuser_keyelementuser_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE elementuser_keyelementuser_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.elementuser_keyelementuser_seq OWNER TO barry;

--
-- Name: elementuser_keyelementuser_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE elementuser_keyelementuser_seq OWNED BY elementuser.keyelementuser;


--
-- Name: filetracker; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE filetracker (
    keyfiletracker integer NOT NULL,
    fkeyelement integer,
    name text,
    path text,
    filename text,
    fkeypathtemplate integer,
    fkeyprojectstorage integer,
    storagename text
);


ALTER TABLE public.filetracker OWNER TO barry;

--
-- Name: filetracker_keyfiletracker_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE filetracker_keyfiletracker_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.filetracker_keyfiletracker_seq OWNER TO barry;

--
-- Name: filetracker_keyfiletracker_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE filetracker_keyfiletracker_seq OWNED BY filetracker.keyfiletracker;


--
-- Name: filetrackerdep; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE filetrackerdep (
    keyfiletrackerdep integer NOT NULL,
    fkeyinput integer,
    fkeyoutput integer
);


ALTER TABLE public.filetrackerdep OWNER TO barry;

--
-- Name: filetrackerdep_keyfiletrackerdep_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE filetrackerdep_keyfiletrackerdep_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.filetrackerdep_keyfiletrackerdep_seq OWNER TO barry;

--
-- Name: filetrackerdep_keyfiletrackerdep_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE filetrackerdep_keyfiletrackerdep_seq OWNED BY filetrackerdep.keyfiletrackerdep;


--
-- Name: graph; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE graph (
    keygraph integer NOT NULL,
    height double precision,
    width double precision,
    vlabel text,
    period text,
    upperlimit double precision,
    lowerlimit double precision,
    stack boolean,
    graphmax boolean,
    fkeygraphpage integer
);


ALTER TABLE public.graph OWNER TO barry;

--
-- Name: graph_keygraph_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE graph_keygraph_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.graph_keygraph_seq OWNER TO barry;

--
-- Name: graph_keygraph_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE graph_keygraph_seq OWNED BY graph.keygraph;


--
-- Name: graphds; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE graphds (
    keygraphds integer NOT NULL,
    varname text,
    dstype text,
    fkeyhost integer,
    cdef text,
    fieldname text,
    filename text,
    negative boolean,
    graphds text
);


ALTER TABLE public.graphds OWNER TO barry;

--
-- Name: graphds_keygraphds_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE graphds_keygraphds_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.graphds_keygraphds_seq OWNER TO barry;

--
-- Name: graphds_keygraphds_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE graphds_keygraphds_seq OWNED BY graphds.keygraphds;


--
-- Name: graphpage; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE graphpage (
    keygraphpage integer NOT NULL,
    fkeygraphpage integer,
    name text
);


ALTER TABLE public.graphpage OWNER TO barry;

--
-- Name: graphpage_keygraphpage_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE graphpage_keygraphpage_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.graphpage_keygraphpage_seq OWNER TO barry;

--
-- Name: graphpage_keygraphpage_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE graphpage_keygraphpage_seq OWNED BY graphpage.keygraphpage;


--
-- Name: graphrelationship; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE graphrelationship (
    keygraphrelationship integer NOT NULL,
    fkeygraphds integer,
    fkeygraph integer
);


ALTER TABLE public.graphrelationship OWNER TO barry;

--
-- Name: graphrelationship_keygraphrelationship_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE graphrelationship_keygraphrelationship_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.graphrelationship_keygraphrelationship_seq OWNER TO barry;

--
-- Name: graphrelationship_keygraphrelationship_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE graphrelationship_keygraphrelationship_seq OWNED BY graphrelationship.keygraphrelationship;


--
-- Name: gridtemplate; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE gridtemplate (
    keygridtemplate integer NOT NULL,
    fkeyproject integer,
    gridtemplate text
);


ALTER TABLE public.gridtemplate OWNER TO barry;

--
-- Name: gridtemplate_keygridtemplate_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE gridtemplate_keygridtemplate_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.gridtemplate_keygridtemplate_seq OWNER TO barry;

--
-- Name: gridtemplate_keygridtemplate_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE gridtemplate_keygridtemplate_seq OWNED BY gridtemplate.keygridtemplate;


--
-- Name: gridtemplateitem; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE gridtemplateitem (
    keygridtemplateitem integer NOT NULL,
    fkeygridtemplate integer,
    fkeytasktype integer,
    checklistitems text,
    columntype integer,
    headername text,
    "position" integer
);


ALTER TABLE public.gridtemplateitem OWNER TO barry;

--
-- Name: gridtemplateitem_keygridtemplateitem_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE gridtemplateitem_keygridtemplateitem_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.gridtemplateitem_keygridtemplateitem_seq OWNER TO barry;

--
-- Name: gridtemplateitem_keygridtemplateitem_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE gridtemplateitem_keygridtemplateitem_seq OWNED BY gridtemplateitem.keygridtemplateitem;


--
-- Name: groupmapping; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE groupmapping (
    keygroupmapping integer NOT NULL,
    fkeygrp integer,
    fkeymapping integer
);


ALTER TABLE public.groupmapping OWNER TO barry;

--
-- Name: groupmapping_keygroupmapping_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE groupmapping_keygroupmapping_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.groupmapping_keygroupmapping_seq OWNER TO barry;

--
-- Name: groupmapping_keygroupmapping_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE groupmapping_keygroupmapping_seq OWNED BY groupmapping.keygroupmapping;


--
-- Name: grp; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE grp (
    keygrp integer NOT NULL,
    grp text
);


ALTER TABLE public.grp OWNER TO barry;

--
-- Name: grp_keygrp_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE grp_keygrp_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.grp_keygrp_seq OWNER TO barry;

--
-- Name: grp_keygrp_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE grp_keygrp_seq OWNED BY grp.keygrp;


--
-- Name: gruntscript; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE gruntscript (
    keygruntscript integer NOT NULL,
    runcount integer,
    lastrun date,
    scriptname text
);


ALTER TABLE public.gruntscript OWNER TO barry;

--
-- Name: gruntscript_keygruntscript_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE gruntscript_keygruntscript_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.gruntscript_keygruntscript_seq OWNER TO barry;

--
-- Name: gruntscript_keygruntscript_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE gruntscript_keygruntscript_seq OWNED BY gruntscript.keygruntscript;


--
-- Name: history; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE history (
    keyhistory integer NOT NULL,
    date timestamp without time zone,
    fkeyelement integer,
    fkeyusr integer,
    history text
);


ALTER TABLE public.history OWNER TO barry;

--
-- Name: history_keyhistory_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE history_keyhistory_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.history_keyhistory_seq OWNER TO barry;

--
-- Name: history_keyhistory_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE history_keyhistory_seq OWNED BY history.keyhistory;


--
-- Name: host; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE host (
    keyhost integer NOT NULL,
    abversion text,
    backupbytes text,
    cpus integer,
    cpuname text,
    architecture text,
    osversion text,
    description text,
    diskusage text,
    host text,
    manufacturer text,
    model text,
    os text,
    slavepluginlist text,
    sn text,
    version text,
    dutycycle double precision,
    memory integer,
    mhtz integer,
    online integer,
    uid integer,
    viruscount integer,
    virustimestamp date,
    fkeyhost_backup integer,
    allowmapping boolean,
    allowsleep boolean,
    loc_x double precision,
    loc_y double precision,
    loc_z double precision,
    fkeydiskimage integer,
    bootaction text,
    syncname text,
    fkeylocation integer
);


ALTER TABLE public.host OWNER TO barry;

--
-- Name: host_keyhost_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE host_keyhost_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.host_keyhost_seq OWNER TO barry;

--
-- Name: host_keyhost_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE host_keyhost_seq OWNED BY host.keyhost;


--
-- Name: hostdailystat; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE hostdailystat (
    keyhostdailystat integer NOT NULL,
    fkeyhost integer,
    readytime text,
    assignedtime text,
    copytime text,
    loadtime text,
    busytime text,
    offlinetime text,
    date date,
    tasksdone integer,
    loaderrors integer,
    taskerrors integer,
    loaderrortime text,
    busyerrortime text
);


ALTER TABLE public.hostdailystat OWNER TO barry;

--
-- Name: hostdailystat_keyhostdailystat_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE hostdailystat_keyhostdailystat_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.hostdailystat_keyhostdailystat_seq OWNER TO barry;

--
-- Name: hostdailystat_keyhostdailystat_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE hostdailystat_keyhostdailystat_seq OWNED BY hostdailystat.keyhostdailystat;


--
-- Name: hostgroupitem; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE hostgroupitem (
    keyhostgroupitem integer NOT NULL,
    fkeyhostgroup integer,
    fkeyhost integer
);


ALTER TABLE public.hostgroupitem OWNER TO barry;

--
-- Name: hostgroupitem_keyhostgroupitem_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE hostgroupitem_keyhostgroupitem_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.hostgroupitem_keyhostgroupitem_seq OWNER TO barry;

--
-- Name: hostgroupitem_keyhostgroupitem_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE hostgroupitem_keyhostgroupitem_seq OWNED BY hostgroupitem.keyhostgroupitem;


--
-- Name: hosthistory; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE hosthistory (
    keyhosthistory integer NOT NULL,
    fkeyhost integer,
    fkeyjob integer,
    fkeyjobstat integer,
    status text,
    laststatus text,
    datetime timestamp without time zone,
    fkeyjobtask integer,
    change_from_ip text,
    nextstatus text,
    fkeyjobtype integer,
    fkeyjobcommandhistory integer,
    fkeyjoberror integer
);


ALTER TABLE public.hosthistory OWNER TO barry;

--
-- Name: hosthistory_keyhosthistory_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE hosthistory_keyhosthistory_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.hosthistory_keyhosthistory_seq OWNER TO barry;

--
-- Name: hosthistory_keyhosthistory_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE hosthistory_keyhosthistory_seq OWNED BY hosthistory.keyhosthistory;


--
-- Name: hostinterface; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE hostinterface (
    keyhostinterface integer NOT NULL,
    fkeyhost integer,
    mac text,
    ip text,
    fkeyhostinterfacetype integer,
    switchport integer
);


ALTER TABLE public.hostinterface OWNER TO barry;

--
-- Name: hostinterface_keyhostinterface_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE hostinterface_keyhostinterface_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.hostinterface_keyhostinterface_seq OWNER TO barry;

--
-- Name: hostinterface_keyhostinterface_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE hostinterface_keyhostinterface_seq OWNED BY hostinterface.keyhostinterface;


--
-- Name: hostinterfacetype; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE hostinterfacetype (
    keyhostinterfacetype integer NOT NULL,
    hostinterfacetype text
);


ALTER TABLE public.hostinterfacetype OWNER TO barry;

--
-- Name: hostinterfacetype_keyhostinterfacetype_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE hostinterfacetype_keyhostinterfacetype_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.hostinterfacetype_keyhostinterfacetype_seq OWNER TO barry;

--
-- Name: hostinterfacetype_keyhostinterfacetype_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE hostinterfacetype_keyhostinterfacetype_seq OWNED BY hostinterfacetype.keyhostinterfacetype;


--
-- Name: hostload; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE hostload (
    keyhostload integer NOT NULL,
    fkeyhost integer,
    loadavg double precision,
    loadavgadjust double precision
);


ALTER TABLE public.hostload OWNER TO barry;

--
-- Name: hostload_keyhostload_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE hostload_keyhostload_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.hostload_keyhostload_seq OWNER TO barry;

--
-- Name: hostload_keyhostload_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE hostload_keyhostload_seq OWNED BY hostload.keyhostload;


--
-- Name: hostmapping; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE hostmapping (
    keyhostmapping integer NOT NULL,
    fkeyhost integer,
    fkeymapping integer
);


ALTER TABLE public.hostmapping OWNER TO barry;

--
-- Name: hostmapping_keyhostmapping_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE hostmapping_keyhostmapping_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.hostmapping_keyhostmapping_seq OWNER TO barry;

--
-- Name: hostmapping_keyhostmapping_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE hostmapping_keyhostmapping_seq OWNED BY hostmapping.keyhostmapping;


--
-- Name: hostresource; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE hostresource (
    keyhostresource integer NOT NULL,
    fkeyhost integer,
    hostresource text
);


ALTER TABLE public.hostresource OWNER TO barry;

--
-- Name: hostresource_keyhostresource_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE hostresource_keyhostresource_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.hostresource_keyhostresource_seq OWNER TO barry;

--
-- Name: hostresource_keyhostresource_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE hostresource_keyhostresource_seq OWNED BY hostresource.keyhostresource;


--
-- Name: hostservice; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE hostservice (
    keyhostservice integer NOT NULL,
    fkeyhost integer,
    fkeyservice integer,
    fkeysyslog integer,
    pulse timestamp without time zone,
    remotelogport integer,
    enabled boolean
);


ALTER TABLE public.hostservice OWNER TO barry;

--
-- Name: hostservice_keyhostservice_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE hostservice_keyhostservice_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.hostservice_keyhostservice_seq OWNER TO barry;

--
-- Name: hostservice_keyhostservice_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE hostservice_keyhostservice_seq OWNED BY hostservice.keyhostservice;


--
-- Name: hostsoftware; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE hostsoftware (
    keyhostsoftware integer NOT NULL,
    fkeyhost integer,
    fkeysoftware integer
);


ALTER TABLE public.hostsoftware OWNER TO barry;

--
-- Name: hostsoftware_keyhostsoftware_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE hostsoftware_keyhostsoftware_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.hostsoftware_keyhostsoftware_seq OWNER TO barry;

--
-- Name: hostsoftware_keyhostsoftware_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE hostsoftware_keyhostsoftware_seq OWNED BY hostsoftware.keyhostsoftware;


--
-- Name: hoststatus; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE hoststatus (
    keyhoststatus integer NOT NULL,
    fkeyhost integer,
    fkeyjob integer,
    fkeyjobtype integer,
    slavestatus text,
    laststatuschange timestamp without time zone,
    slaveframes text,
    slavepulse timestamp without time zone,
    fkeyjobtask integer,
    fkeyjobcommandhistory integer
);


ALTER TABLE public.hoststatus OWNER TO barry;

--
-- Name: hoststatus_keyhoststatus_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE hoststatus_keyhoststatus_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.hoststatus_keyhoststatus_seq OWNER TO barry;

--
-- Name: hoststatus_keyhoststatus_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE hoststatus_keyhoststatus_seq OWNED BY hoststatus.keyhoststatus;


--
-- Name: job; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE job (
    keyjob integer NOT NULL,
    fkeyelement integer,
    fkeyhost integer,
    fkeyjobtype integer,
    fkeyproject integer,
    fkeyusr integer,
    hostlist text,
    job text,
    jobtime text,
    outputpath text,
    status text,
    submitted integer,
    started integer,
    ended integer,
    deleteoncomplete integer,
    priority integer,
    packettype text,
    packetsize integer,
    notifyonerror text,
    notifyoncomplete text,
    maxtasktime integer,
    cleaned integer,
    filesize integer,
    btinfohash text,
    filename text,
    filemd5sum text,
    fkeyjobstat integer,
    username text,
    "domain" text,
    "password" text,
    stats text,
    currentmapserverweight double precision,
    prioritizeoutertasks boolean,
    outertasksassigned boolean,
    maxloadtime integer,
    maxmemory integer,
    fkeyjobparent integer,
    submittedts timestamp without time zone,
    startedts timestamp without time zone,
    endedts timestamp without time zone
);


ALTER TABLE public.job OWNER TO barry;

--
-- Name: job_keyjob_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE job_keyjob_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.job_keyjob_seq OWNER TO barry;

--
-- Name: job_keyjob_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE job_keyjob_seq OWNED BY job.keyjob;


--
-- Name: jobautodeskburn; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobautodeskburn (
    framestart integer,
    frameend integer,
    handle text
)
INHERITS (job);


ALTER TABLE public.jobautodeskburn OWNER TO barry;

--
-- Name: jobbatch; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobbatch (
    cmd text,
    restartaftershutdown boolean,
    passslaveframesasparam boolean
)
INHERITS (job);


ALTER TABLE public.jobbatch OWNER TO barry;

--
-- Name: jobcannedbatch; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobcannedbatch (
    keyjobcannedbatch integer NOT NULL,
    name text,
    "group" text,
    cmd text
);


ALTER TABLE public.jobcannedbatch OWNER TO barry;

--
-- Name: jobcannedbatch_keyjobcannedbatch_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE jobcannedbatch_keyjobcannedbatch_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.jobcannedbatch_keyjobcannedbatch_seq OWNER TO barry;

--
-- Name: jobcannedbatch_keyjobcannedbatch_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE jobcannedbatch_keyjobcannedbatch_seq OWNED BY jobcannedbatch.keyjobcannedbatch;


--
-- Name: jobcinema4d; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobcinema4d (
)
INHERITS (job);


ALTER TABLE public.jobcinema4d OWNER TO barry;

--
-- Name: jobcommandhistory; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobcommandhistory (
    keyjobcommandhistory integer NOT NULL,
    stderr text,
    "stdout" text,
    command text,
    memory integer,
    fkeyjob integer,
    fkeyhost integer
);


ALTER TABLE public.jobcommandhistory OWNER TO barry;

--
-- Name: jobcommandhistory_keyjobcommandhistory_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE jobcommandhistory_keyjobcommandhistory_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.jobcommandhistory_keyjobcommandhistory_seq OWNER TO barry;

--
-- Name: jobcommandhistory_keyjobcommandhistory_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE jobcommandhistory_keyjobcommandhistory_seq OWNED BY jobcommandhistory.keyjobcommandhistory;


--
-- Name: jobdep; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobdep (
    keyjobdep integer NOT NULL,
    fkeyjob integer,
    fkeydep integer
);


ALTER TABLE public.jobdep OWNER TO barry;

--
-- Name: jobdep_keyjobdep_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE jobdep_keyjobdep_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.jobdep_keyjobdep_seq OWNER TO barry;

--
-- Name: jobdep_keyjobdep_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE jobdep_keyjobdep_seq OWNED BY jobdep.keyjobdep;


--
-- Name: joberror; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE joberror (
    keyjoberror integer NOT NULL,
    fkeyhost integer,
    fkeyjob integer,
    frames text,
    message text,
    errortime integer,
    count integer,
    cleared boolean
);


ALTER TABLE public.joberror OWNER TO barry;

--
-- Name: joberror_keyjoberror_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE joberror_keyjoberror_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.joberror_keyjoberror_seq OWNER TO barry;

--
-- Name: joberror_keyjoberror_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE joberror_keyjoberror_seq OWNED BY joberror.keyjoberror;


--
-- Name: joberrorhandler; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE joberrorhandler (
    keyjoberrorhandler integer NOT NULL,
    fkeyjobtype integer,
    errorregex text,
    fkeyjoberrorhandlerscript integer
);


ALTER TABLE public.joberrorhandler OWNER TO barry;

--
-- Name: joberrorhandler_keyjoberrorhandler_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE joberrorhandler_keyjoberrorhandler_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.joberrorhandler_keyjoberrorhandler_seq OWNER TO barry;

--
-- Name: joberrorhandler_keyjoberrorhandler_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE joberrorhandler_keyjoberrorhandler_seq OWNED BY joberrorhandler.keyjoberrorhandler;


--
-- Name: joberrorhandlerscript; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE joberrorhandlerscript (
    keyjoberrorhandlerscript integer NOT NULL,
    script text
);


ALTER TABLE public.joberrorhandlerscript OWNER TO barry;

--
-- Name: joberrorhandlerscript_keyjoberrorhandlerscript_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE joberrorhandlerscript_keyjoberrorhandlerscript_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.joberrorhandlerscript_keyjoberrorhandlerscript_seq OWNER TO barry;

--
-- Name: joberrorhandlerscript_keyjoberrorhandlerscript_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE joberrorhandlerscript_keyjoberrorhandlerscript_seq OWNED BY joberrorhandlerscript.keyjoberrorhandlerscript;


--
-- Name: jobfusion; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobfusion (
    framelist text
)
INHERITS (job);


ALTER TABLE public.jobfusion OWNER TO barry;

--
-- Name: jobhistory; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobhistory (
    keyjobhistory integer NOT NULL,
    fkeyjobhistorytype integer,
    fkeyjob integer,
    fkeyhost integer,
    fkeyuser integer,
    message text,
    created timestamp without time zone
);


ALTER TABLE public.jobhistory OWNER TO barry;

--
-- Name: jobhistory_keyjobhistory_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE jobhistory_keyjobhistory_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.jobhistory_keyjobhistory_seq OWNER TO barry;

--
-- Name: jobhistory_keyjobhistory_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE jobhistory_keyjobhistory_seq OWNED BY jobhistory.keyjobhistory;


--
-- Name: jobhistorytype; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobhistorytype (
    keyjobhistorytype integer NOT NULL,
    "type" text
);


ALTER TABLE public.jobhistorytype OWNER TO barry;

--
-- Name: jobhistorytype_keyjobhistorytype_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE jobhistorytype_keyjobhistorytype_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.jobhistorytype_keyjobhistorytype_seq OWNER TO barry;

--
-- Name: jobhistorytype_keyjobhistorytype_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE jobhistorytype_keyjobhistorytype_seq OWNED BY jobhistorytype.keyjobhistorytype;


--
-- Name: jobmax; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobmax (
    camera text,
    elementfile text,
    fileoriginal text,
    flag_h integer,
    flag_v integer,
    flag_w integer,
    flag_x2 integer,
    flag_xa integer,
    flag_xc integer,
    flag_xd integer,
    flag_xe integer,
    flag_xf integer,
    flag_xh integer,
    flag_xk integer,
    flag_xn integer,
    flag_xo integer,
    flag_xp integer,
    flag_xv integer,
    frameend integer,
    framelist text,
    framestart integer,
    framenth integer
)
INHERITS (job);


ALTER TABLE public.jobmax OWNER TO barry;

--
-- Name: jobmax5; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobmax5 (
    camera text,
    fileoriginal text,
    flag_h integer,
    framestart integer,
    frameend integer,
    framenth integer,
    flag_w integer,
    flag_xv integer,
    flag_x2 integer,
    flag_xa integer,
    flag_xe integer,
    flag_xk integer,
    flag_xd integer,
    flag_xh integer,
    flag_xo integer,
    flag_xf integer,
    flag_xn integer,
    flag_xp integer,
    flag_xc integer,
    flag_v integer,
    elementfile text,
    framelist text
)
INHERITS (job);


ALTER TABLE public.jobmax5 OWNER TO barry;

--
-- Name: jobmax6; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobmax6 (
    camera text,
    elementfile text,
    fileoriginal text,
    flag_h integer,
    flag_v integer,
    flag_w integer,
    flag_x2 integer,
    flag_xa integer,
    flag_xc integer,
    flag_xd integer,
    flag_xe integer,
    flag_xf integer,
    flag_xh integer,
    flag_xk integer,
    flag_xn integer,
    flag_xo integer,
    flag_xp integer,
    flag_xv integer,
    frameend integer,
    framelist text,
    framenth integer,
    framestart integer
)
INHERITS (job);


ALTER TABLE public.jobmax6 OWNER TO barry;

--
-- Name: jobmax7; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobmax7 (
)
INHERITS (jobmax);


ALTER TABLE public.jobmax7 OWNER TO barry;

--
-- Name: jobmax8; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobmax8 (
)
INHERITS (jobmax);


ALTER TABLE public.jobmax8 OWNER TO barry;

--
-- Name: joboutput; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE joboutput (
    keyjoboutput integer NOT NULL,
    fkeyjob integer,
    name text,
    fkeyfiletracker integer
);


ALTER TABLE public.joboutput OWNER TO barry;

--
-- Name: joboutput_keyjoboutput_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE joboutput_keyjoboutput_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.joboutput_keyjoboutput_seq OWNER TO barry;

--
-- Name: joboutput_keyjoboutput_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE joboutput_keyjoboutput_seq OWNED BY joboutput.keyjoboutput;


--
-- Name: jobservice; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobservice (
    keyjobservice integer NOT NULL,
    fkeyjob integer,
    fkeyservice integer
);


ALTER TABLE public.jobservice OWNER TO barry;

--
-- Name: jobservice_keyjobservice_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE jobservice_keyjobservice_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.jobservice_keyjobservice_seq OWNER TO barry;

--
-- Name: jobservice_keyjobservice_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE jobservice_keyjobservice_seq OWNED BY jobservice.keyjobservice;


--
-- Name: jobstat; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobstat (
    keyjobstat integer NOT NULL,
    pass text,
    fkeyelement integer,
    taskcount integer,
    taskscompleted integer,
    tasktime integer,
    started timestamp without time zone,
    ended timestamp without time zone,
    fkeyusr integer,
    fkeyproject integer,
    name text,
    errorcount integer
);


ALTER TABLE public.jobstat OWNER TO barry;

--
-- Name: jobstat_keyjobstat_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE jobstat_keyjobstat_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.jobstat_keyjobstat_seq OWNER TO barry;

--
-- Name: jobstat_keyjobstat_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE jobstat_keyjobstat_seq OWNED BY jobstat.keyjobstat;


--
-- Name: jobstatus; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobstatus (
    keyjobstatus integer NOT NULL,
    hostsonjob integer,
    fkeyjob integer,
    tasksunassigned integer,
    taskscount integer,
    tasksdone integer,
    taskscancelled integer,
    taskssuspended integer,
    tasksassigned integer,
    tasksbusy integer,
    tasksaveragetime integer,
    health double precision,
    joblastupdated timestamp without time zone,
    errorcount integer,
    lastnotifiederrorcount integer
);


ALTER TABLE public.jobstatus OWNER TO barry;

--
-- Name: jobstatus_keyjobstatus_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE jobstatus_keyjobstatus_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.jobstatus_keyjobstatus_seq OWNER TO barry;

--
-- Name: jobstatus_keyjobstatus_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE jobstatus_keyjobstatus_seq OWNED BY jobstatus.keyjobstatus;


--
-- Name: jobtask; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobtask (
    keyjobtask integer NOT NULL,
    ended integer,
    fkeyhost integer,
    fkeyjob integer,
    status text,
    jobtask integer,
    started integer,
    label text,
    fkeyjoboutput integer,
    fkeyjobcommandhistory integer,
    memory integer,
    startedts timestamp without time zone,
    endedts timestamp without time zone
);


ALTER TABLE public.jobtask OWNER TO barry;

--
-- Name: jobtask_keyjobtask_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE jobtask_keyjobtask_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.jobtask_keyjobtask_seq OWNER TO barry;

--
-- Name: jobtask_keyjobtask_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE jobtask_keyjobtask_seq OWNED BY jobtask.keyjobtask;


--
-- Name: jobtype; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobtype (
    keyjobtype integer NOT NULL,
    jobtype text,
    fkeyservice integer
);


ALTER TABLE public.jobtype OWNER TO barry;

--
-- Name: jobtype_keyjobtype_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE jobtype_keyjobtype_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.jobtype_keyjobtype_seq OWNER TO barry;

--
-- Name: jobtype_keyjobtype_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE jobtype_keyjobtype_seq OWNED BY jobtype.keyjobtype;


--
-- Name: jobtypemapping; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobtypemapping (
    keyjobtypemapping integer NOT NULL,
    fkeyjobtype integer,
    fkeymapping integer
);


ALTER TABLE public.jobtypemapping OWNER TO barry;

--
-- Name: jobtypemapping_keyjobtypemapping_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE jobtypemapping_keyjobtypemapping_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.jobtypemapping_keyjobtypemapping_seq OWNER TO barry;

--
-- Name: jobtypemapping_keyjobtypemapping_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE jobtypemapping_keyjobtypemapping_seq OWNED BY jobtypemapping.keyjobtypemapping;


--
-- Name: jobxsi; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE jobxsi (
    pass text,
    flag_w integer,
    framelist integer,
    framestart integer,
    frameend integer,
    framenth integer,
    flag_h integer
)
INHERITS (job);


ALTER TABLE public.jobxsi OWNER TO barry;

--
-- Name: license; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE license (
    keylicense integer NOT NULL,
    license text,
    fkeyhost integer,
    fkeysoftware integer,
    total integer,
    available integer,
    reserved integer
);


ALTER TABLE public.license OWNER TO barry;

--
-- Name: license_keylicense_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE license_keylicense_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.license_keylicense_seq OWNER TO barry;

--
-- Name: license_keylicense_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE license_keylicense_seq OWNED BY license.keylicense;


--
-- Name: location; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE "location" (
    keylocation integer NOT NULL,
    name text
);


ALTER TABLE public."location" OWNER TO barry;

--
-- Name: location_keylocation_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE location_keylocation_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.location_keylocation_seq OWNER TO barry;

--
-- Name: location_keylocation_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE location_keylocation_seq OWNED BY "location".keylocation;


--
-- Name: mapping; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE mapping (
    keymapping integer NOT NULL,
    fkeyhost integer,
    "share" text,
    fkeymappingtype integer,
    mount text
);


ALTER TABLE public.mapping OWNER TO barry;

--
-- Name: mapping_keymapping_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE mapping_keymapping_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.mapping_keymapping_seq OWNER TO barry;

--
-- Name: mapping_keymapping_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE mapping_keymapping_seq OWNED BY mapping.keymapping;


--
-- Name: mappingtype; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE mappingtype (
    keymappingtype integer NOT NULL,
    name text
);


ALTER TABLE public.mappingtype OWNER TO barry;

--
-- Name: mappingtype_keymappingtype_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE mappingtype_keymappingtype_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.mappingtype_keymappingtype_seq OWNER TO barry;

--
-- Name: mappingtype_keymappingtype_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE mappingtype_keymappingtype_seq OWNED BY mappingtype.keymappingtype;


--
-- Name: notification; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE notification (
    keynotification integer NOT NULL,
    created timestamp without time zone,
    subject text,
    message text,
    component text,
    event text,
    routed timestamp without time zone
);


ALTER TABLE public.notification OWNER TO barry;

--
-- Name: notification_keynotification_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE notification_keynotification_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.notification_keynotification_seq OWNER TO barry;

--
-- Name: notification_keynotification_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE notification_keynotification_seq OWNED BY notification.keynotification;


--
-- Name: notificationdestination; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE notificationdestination (
    keynotificationdestination integer NOT NULL,
    fkeynotification integer,
    fkeynotificationmethod integer,
    delivered timestamp without time zone,
    destination text,
    fkeyuser integer,
    routed timestamp without time zone
);


ALTER TABLE public.notificationdestination OWNER TO barry;

--
-- Name: notificationdestination_keynotificationdestination_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE notificationdestination_keynotificationdestination_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.notificationdestination_keynotificationdestination_seq OWNER TO barry;

--
-- Name: notificationdestination_keynotificationdestination_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE notificationdestination_keynotificationdestination_seq OWNED BY notificationdestination.keynotificationdestination;


--
-- Name: notificationmethod; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE notificationmethod (
    keynotificationmethod integer NOT NULL,
    name text
);


ALTER TABLE public.notificationmethod OWNER TO barry;

--
-- Name: notificationmethod_keynotificationmethod_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE notificationmethod_keynotificationmethod_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.notificationmethod_keynotificationmethod_seq OWNER TO barry;

--
-- Name: notificationmethod_keynotificationmethod_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE notificationmethod_keynotificationmethod_seq OWNED BY notificationmethod.keynotificationmethod;


--
-- Name: notificationroute; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE notificationroute (
    keynotificationuserroute integer NOT NULL,
    eventmatch text,
    componentmatch text,
    fkeyuser integer,
    subjectmatch text,
    messagematch text,
    actions text,
    priority integer
);


ALTER TABLE public.notificationroute OWNER TO barry;

--
-- Name: notificationroute_keynotificationuserroute_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE notificationroute_keynotificationuserroute_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.notificationroute_keynotificationuserroute_seq OWNER TO barry;

--
-- Name: notificationroute_keynotificationuserroute_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE notificationroute_keynotificationuserroute_seq OWNED BY notificationroute.keynotificationuserroute;


--
-- Name: pathsynctarget; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE pathsynctarget (
    keypathsynctarget integer NOT NULL,
    fkeypathtracker integer,
    fkeyprojectstorage integer
);


ALTER TABLE public.pathsynctarget OWNER TO barry;

--
-- Name: pathsynctarget_keypathsynctarget_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE pathsynctarget_keypathsynctarget_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.pathsynctarget_keypathsynctarget_seq OWNER TO barry;

--
-- Name: pathsynctarget_keypathsynctarget_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE pathsynctarget_keypathsynctarget_seq OWNED BY pathsynctarget.keypathsynctarget;


--
-- Name: pathtemplate; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE pathtemplate (
    keypathtemplate integer NOT NULL,
    version integer,
    filenametemplate text,
    pathtemplate text,
    name text,
    pythoncode text
);


ALTER TABLE public.pathtemplate OWNER TO barry;

--
-- Name: pathtemplate_keypathtemplate_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE pathtemplate_keypathtemplate_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.pathtemplate_keypathtemplate_seq OWNER TO barry;

--
-- Name: pathtemplate_keypathtemplate_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE pathtemplate_keypathtemplate_seq OWNED BY pathtemplate.keypathtemplate;


--
-- Name: pathtracker; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE pathtracker (
    keypathtracker integer NOT NULL,
    fkeyelement integer,
    path text,
    fkeypathtemplate integer,
    fkeyprojectstorage integer,
    storagename text
);


ALTER TABLE public.pathtracker OWNER TO barry;

--
-- Name: pathtracker_keypathtracker_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE pathtracker_keypathtracker_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.pathtracker_keypathtracker_seq OWNER TO barry;

--
-- Name: pathtracker_keypathtracker_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE pathtracker_keypathtracker_seq OWNED BY pathtracker.keypathtracker;


--
-- Name: permission; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE permission (
    keypermission integer NOT NULL,
    methodpattern text,
    fkeyusr integer,
    permission text,
    fkeygrp integer,
    "class" text
);


ALTER TABLE public.permission OWNER TO barry;

--
-- Name: permission_keypermission_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE permission_keypermission_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.permission_keypermission_seq OWNER TO barry;

--
-- Name: permission_keypermission_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE permission_keypermission_seq OWNED BY permission.keypermission;


SET default_with_oids = true;

--
-- Name: pg_ts_cfg; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE pg_ts_cfg (
    ts_name text NOT NULL,
    prs_name text NOT NULL,
    locale text
);


ALTER TABLE public.pg_ts_cfg OWNER TO barry;

--
-- Name: pg_ts_cfgmap; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE pg_ts_cfgmap (
    ts_name text NOT NULL,
    tok_alias text NOT NULL,
    dict_name text[]
);


ALTER TABLE public.pg_ts_cfgmap OWNER TO barry;

--
-- Name: pg_ts_dict; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE pg_ts_dict (
    dict_name text NOT NULL,
    dict_init regprocedure,
    dict_initoption text,
    dict_lexize regprocedure NOT NULL,
    dict_comment text
);


ALTER TABLE public.pg_ts_dict OWNER TO barry;

--
-- Name: pg_ts_parser; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE pg_ts_parser (
    prs_name text NOT NULL,
    prs_start regprocedure NOT NULL,
    prs_nexttoken regprocedure NOT NULL,
    prs_end regprocedure NOT NULL,
    prs_headline regprocedure NOT NULL,
    prs_lextype regprocedure NOT NULL,
    prs_comment text
);


ALTER TABLE public.pg_ts_parser OWNER TO barry;

SET default_with_oids = false;

--
-- Name: phoneno; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE phoneno (
    keyphoneno integer NOT NULL,
    phoneno text,
    fkeyphonetype integer,
    fkeyemployee integer,
    "domain" text
);


ALTER TABLE public.phoneno OWNER TO barry;

--
-- Name: phoneno_keyphoneno_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE phoneno_keyphoneno_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.phoneno_keyphoneno_seq OWNER TO barry;

--
-- Name: phoneno_keyphoneno_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE phoneno_keyphoneno_seq OWNED BY phoneno.keyphoneno;


--
-- Name: phonetype; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE phonetype (
    keyphonetype integer NOT NULL,
    phonetype text
);


ALTER TABLE public.phonetype OWNER TO barry;

--
-- Name: phonetype_keyphonetype_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE phonetype_keyphonetype_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.phonetype_keyphonetype_seq OWNER TO barry;

--
-- Name: phonetype_keyphonetype_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE phonetype_keyphonetype_seq OWNED BY phonetype.keyphonetype;


--
-- Name: projectresolution; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE projectresolution (
    keyprojectresolution integer NOT NULL,
    deliveryformat text,
    fkeyproject integer,
    height integer,
    outputformat text,
    projectresolution text,
    width integer,
    pixelaspect double precision,
    fps integer
);


ALTER TABLE public.projectresolution OWNER TO barry;

--
-- Name: projectresolution_keyprojectresolution_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE projectresolution_keyprojectresolution_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.projectresolution_keyprojectresolution_seq OWNER TO barry;

--
-- Name: projectresolution_keyprojectresolution_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE projectresolution_keyprojectresolution_seq OWNED BY projectresolution.keyprojectresolution;


--
-- Name: projectstatus; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE projectstatus (
    keyprojectstatus integer NOT NULL,
    projectstatus text,
    chronology integer
);


ALTER TABLE public.projectstatus OWNER TO barry;

--
-- Name: projectstatus_keyprojectstatus_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE projectstatus_keyprojectstatus_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.projectstatus_keyprojectstatus_seq OWNER TO barry;

--
-- Name: projectstatus_keyprojectstatus_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE projectstatus_keyprojectstatus_seq OWNED BY projectstatus.keyprojectstatus;


--
-- Name: projectstorage; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE projectstorage (
    keyprojectstorage integer NOT NULL,
    name text,
    fkeyproject integer,
    fkeyhost integer,
    "location" text,
    "default" boolean
);


ALTER TABLE public.projectstorage OWNER TO barry;

--
-- Name: projectstorage_keyprojectstorage_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE projectstorage_keyprojectstorage_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.projectstorage_keyprojectstorage_seq OWNER TO barry;

--
-- Name: projectstorage_keyprojectstorage_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE projectstorage_keyprojectstorage_seq OWNED BY projectstorage.keyprojectstorage;


--
-- Name: rangefiletracker; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE rangefiletracker (
    filenametemplate text,
    framestart integer,
    frameend integer,
    fkeyresolution integer,
    renderelement text
)
INHERITS (filetracker);


ALTER TABLE public.rangefiletracker OWNER TO barry;

--
-- Name: schedule; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE schedule (
    keyschedule integer NOT NULL,
    fkeyuser integer,
    date date,
    starthour integer,
    hours integer,
    fkeyelement integer,
    fkeyassettype integer,
    fkeycreatedbyuser integer
);


ALTER TABLE public.schedule OWNER TO barry;

--
-- Name: schedule_keyschedule_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE schedule_keyschedule_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.schedule_keyschedule_seq OWNER TO barry;

--
-- Name: schedule_keyschedule_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE schedule_keyschedule_seq OWNED BY schedule.keyschedule;


--
-- Name: service; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE service (
    keyservice integer NOT NULL,
    service text,
    fkeylicense integer,
    enabled boolean,
    forbiddenprocesses text,
    active boolean,
    "unique" boolean
);


ALTER TABLE public.service OWNER TO barry;

--
-- Name: service_keyservice_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE service_keyservice_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.service_keyservice_seq OWNER TO barry;

--
-- Name: service_keyservice_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE service_keyservice_seq OWNED BY service.keyservice;


--
-- Name: shot; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE shot (
    dialog text,
    frameend integer,
    framestart integer,
    shot double precision,
    framestartedl integer,
    frameendedl integer,
    camerainfo text,
    scriptpage integer
)
INHERITS (element);


ALTER TABLE public.shot OWNER TO barry;

--
-- Name: shotgroup; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE shotgroup (
)
INHERITS (element);


ALTER TABLE public.shotgroup OWNER TO barry;

--
-- Name: software; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE software (
    keysoftware integer NOT NULL,
    software text,
    version text
);


ALTER TABLE public.software OWNER TO barry;

--
-- Name: software_keysoftware_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE software_keysoftware_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.software_keysoftware_seq OWNER TO barry;

--
-- Name: software_keysoftware_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE software_keysoftware_seq OWNED BY software.keysoftware;


--
-- Name: statusset; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE statusset (
    keystatusset integer NOT NULL,
    name text
);


ALTER TABLE public.statusset OWNER TO barry;

--
-- Name: statusset_keystatusset_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE statusset_keystatusset_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.statusset_keystatusset_seq OWNER TO barry;

--
-- Name: statusset_keystatusset_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE statusset_keystatusset_seq OWNED BY statusset.keystatusset;


--
-- Name: syslog; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE syslog (
    keysyslog integer NOT NULL,
    fkeyhost integer,
    fkeysyslogrealm integer,
    fkeysyslogseverity integer,
    message text,
    count integer,
    created timestamp without time zone,
    method text,
    "class" text,
    lastoccurrence timestamp without time zone,
    ack integer
);


ALTER TABLE public.syslog OWNER TO barry;

--
-- Name: syslog_keysyslog_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE syslog_keysyslog_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.syslog_keysyslog_seq OWNER TO barry;

--
-- Name: syslog_keysyslog_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE syslog_keysyslog_seq OWNED BY syslog.keysyslog;


--
-- Name: syslogrealm; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE syslogrealm (
    keysyslogrealm integer NOT NULL,
    syslogrealm text
);


ALTER TABLE public.syslogrealm OWNER TO barry;

--
-- Name: syslogrealm_keysyslogrealm_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE syslogrealm_keysyslogrealm_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.syslogrealm_keysyslogrealm_seq OWNER TO barry;

--
-- Name: syslogrealm_keysyslogrealm_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE syslogrealm_keysyslogrealm_seq OWNED BY syslogrealm.keysyslogrealm;


--
-- Name: syslogseverity; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE syslogseverity (
    keysyslogseverity integer NOT NULL,
    syslogseverity text,
    severity integer
);


ALTER TABLE public.syslogseverity OWNER TO barry;

--
-- Name: syslogseverity_keysyslogseverity_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE syslogseverity_keysyslogseverity_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.syslogseverity_keysyslogseverity_seq OWNER TO barry;

--
-- Name: syslogseverity_keysyslogseverity_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE syslogseverity_keysyslogseverity_seq OWNED BY syslogseverity.keysyslogseverity;


--
-- Name: threadnotify; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE threadnotify (
    keythreadnotify integer NOT NULL,
    fkeythread integer,
    fkeyuser integer,
    options integer
);


ALTER TABLE public.threadnotify OWNER TO barry;

--
-- Name: threadnotify_keythreadnotify_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE threadnotify_keythreadnotify_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.threadnotify_keythreadnotify_seq OWNER TO barry;

--
-- Name: threadnotify_keythreadnotify_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE threadnotify_keythreadnotify_seq OWNED BY threadnotify.keythreadnotify;


--
-- Name: thumbnail; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE thumbnail (
    keythumbnail integer NOT NULL,
    cliprect text,
    date timestamp without time zone,
    fkeyelement integer,
    fkeyuser integer,
    originalfile text
);


ALTER TABLE public.thumbnail OWNER TO barry;

--
-- Name: thumbnail_keythumbnail_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE thumbnail_keythumbnail_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.thumbnail_keythumbnail_seq OWNER TO barry;

--
-- Name: thumbnail_keythumbnail_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE thumbnail_keythumbnail_seq OWNED BY thumbnail.keythumbnail;


--
-- Name: timesheet; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE timesheet (
    keytimesheet integer NOT NULL,
    datetime timestamp without time zone,
    "comment" text,
    fkeyelement integer,
    fkeyemployee integer,
    fkeyproject integer,
    fkeytimesheetcategory integer,
    scheduledhour double precision,
    datetimesubmitted timestamp without time zone,
    unscheduledhour double precision
);


ALTER TABLE public.timesheet OWNER TO barry;

--
-- Name: timesheet_keytimesheet_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE timesheet_keytimesheet_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.timesheet_keytimesheet_seq OWNER TO barry;

--
-- Name: timesheet_keytimesheet_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE timesheet_keytimesheet_seq OWNED BY timesheet.keytimesheet;


--
-- Name: timesheetcategory; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE timesheetcategory (
    keytimesheetcategory integer NOT NULL,
    timesheetcategory text,
    hasdaily integer,
    disabled integer,
    fkeyelementtype integer,
    istask boolean,
    fkeypathtemplate integer,
    nameregexp text,
    allowtime boolean,
    color text,
    description text,
    sortcolumn text,
    tags text,
    sortnumber integer
);


ALTER TABLE public.timesheetcategory OWNER TO barry;

--
-- Name: timesheetcategory_keytimesheetcategory_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE timesheetcategory_keytimesheetcategory_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.timesheetcategory_keytimesheetcategory_seq OWNER TO barry;

--
-- Name: timesheetcategory_keytimesheetcategory_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE timesheetcategory_keytimesheetcategory_seq OWNED BY timesheetcategory.keytimesheetcategory;


--
-- Name: tracker; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE tracker (
    keytracker integer NOT NULL,
    tracker text,
    fkeysubmitter integer,
    fkeyassigned integer,
    fkeycategory integer,
    fkeystatus integer,
    datetarget date,
    datechanged timestamp without time zone,
    datesubmitted timestamp without time zone,
    description text,
    timeestimate integer,
    fkeytrackerqueue integer
);


ALTER TABLE public.tracker OWNER TO barry;

--
-- Name: tracker_keytracker_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE tracker_keytracker_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.tracker_keytracker_seq OWNER TO barry;

--
-- Name: tracker_keytracker_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE tracker_keytracker_seq OWNED BY tracker.keytracker;


--
-- Name: trackercategory; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE trackercategory (
    keytrackercategory integer NOT NULL,
    trackercategory text
);


ALTER TABLE public.trackercategory OWNER TO barry;

--
-- Name: trackercategory_keytrackercategory_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE trackercategory_keytrackercategory_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.trackercategory_keytrackercategory_seq OWNER TO barry;

--
-- Name: trackercategory_keytrackercategory_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE trackercategory_keytrackercategory_seq OWNED BY trackercategory.keytrackercategory;


--
-- Name: trackerqueue; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE trackerqueue (
    keytrackerqueue integer NOT NULL,
    trackerqueue text
);


ALTER TABLE public.trackerqueue OWNER TO barry;

--
-- Name: trackerqueue_keytrackerqueue_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE trackerqueue_keytrackerqueue_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.trackerqueue_keytrackerqueue_seq OWNER TO barry;

--
-- Name: trackerqueue_keytrackerqueue_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE trackerqueue_keytrackerqueue_seq OWNED BY trackerqueue.keytrackerqueue;


--
-- Name: userelement; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE userelement (
    keyuserelement integer NOT NULL,
    fkeyuser integer,
    fkeyelement integer
);


ALTER TABLE public.userelement OWNER TO barry;

--
-- Name: userelement_keyuserelement_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE userelement_keyuserelement_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.userelement_keyuserelement_seq OWNER TO barry;

--
-- Name: userelement_keyuserelement_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE userelement_keyuserelement_seq OWNED BY userelement.keyuserelement;


--
-- Name: usermapping; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE usermapping (
    keyusermapping integer NOT NULL,
    fkeyusr integer,
    fkeymapping integer
);


ALTER TABLE public.usermapping OWNER TO barry;

--
-- Name: usermapping_keyusermapping_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE usermapping_keyusermapping_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.usermapping_keyusermapping_seq OWNER TO barry;

--
-- Name: usermapping_keyusermapping_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE usermapping_keyusermapping_seq OWNED BY usermapping.keyusermapping;


--
-- Name: userrole; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE userrole (
    keyuserrole integer NOT NULL,
    fkeytasktype integer,
    fkeyusr integer
);


ALTER TABLE public.userrole OWNER TO barry;

--
-- Name: userrole_keyuserrole_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE userrole_keyuserrole_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.userrole_keyuserrole_seq OWNER TO barry;

--
-- Name: userrole_keyuserrole_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE userrole_keyuserrole_seq OWNED BY userrole.keyuserrole;


--
-- Name: usrgrp; Type: TABLE; Schema: public; Owner: barry; Tablespace: 
--

CREATE TABLE usrgrp (
    keyusrgrp integer NOT NULL,
    fkeyusr integer,
    fkeygrp integer
);


ALTER TABLE public.usrgrp OWNER TO barry;

--
-- Name: usrgrp_keyusrgrp_seq; Type: SEQUENCE; Schema: public; Owner: barry
--

CREATE SEQUENCE usrgrp_keyusrgrp_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


ALTER TABLE public.usrgrp_keyusrgrp_seq OWNER TO barry;

--
-- Name: usrgrp_keyusrgrp_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: barry
--

ALTER SEQUENCE usrgrp_keyusrgrp_seq OWNED BY usrgrp.keyusrgrp;


--
-- Name: keyabdownloadstat; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE abdownloadstat ALTER COLUMN keyabdownloadstat SET DEFAULT nextval('abdownloadstat_keyabdownloadstat_seq'::regclass);


--
-- Name: keyannotation; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE annotation ALTER COLUMN keyannotation SET DEFAULT nextval('annotation_keyannotation_seq'::regclass);


--
-- Name: keyassetproperty; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE assetproperty ALTER COLUMN keyassetproperty SET DEFAULT nextval('assetproperty_keyassetproperty_seq'::regclass);


--
-- Name: keyassettemplate; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE assettemplate ALTER COLUMN keyassettemplate SET DEFAULT nextval('assettemplate_keyassettemplate_seq'::regclass);


--
-- Name: keybachasset; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE bachasset ALTER COLUMN keybachasset SET DEFAULT nextval('bachasset_keybachasset_seq'::regclass);


--
-- Name: keycalendar; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE calendar ALTER COLUMN keycalendar SET DEFAULT nextval('calendar_keycalendar_seq'::regclass);


--
-- Name: keycalendarcategory; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE calendarcategory ALTER COLUMN keycalendarcategory SET DEFAULT nextval('calendarcategory_keycalendarcategory_seq'::regclass);


--
-- Name: keychecklistitem; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE checklistitem ALTER COLUMN keychecklistitem SET DEFAULT nextval('checklistitem_keychecklistitem_seq'::regclass);


--
-- Name: keycheckliststatus; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE checkliststatus ALTER COLUMN keycheckliststatus SET DEFAULT nextval('checkliststatus_keycheckliststatus_seq'::regclass);


--
-- Name: keyclient; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE client ALTER COLUMN keyclient SET DEFAULT nextval('client_keyclient_seq'::regclass);


--
-- Name: keyconfig; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE config ALTER COLUMN keyconfig SET DEFAULT nextval('config_keyconfig_seq'::regclass);


--
-- Name: keydeliveryshot; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE deliveryelement ALTER COLUMN keydeliveryshot SET DEFAULT nextval('deliveryelement_keydeliveryshot_seq'::regclass);


--
-- Name: keydiskimage; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE diskimage ALTER COLUMN keydiskimage SET DEFAULT nextval('diskimage_keydiskimage_seq'::regclass);


--
-- Name: keydynamichostgroup; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE dynamichostgroup ALTER COLUMN keydynamichostgroup SET DEFAULT nextval('dynamichostgroup_keydynamichostgroup_seq'::regclass);


--
-- Name: keyelement; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE element ALTER COLUMN keyelement SET DEFAULT nextval('element_keyelement_seq'::regclass);


--
-- Name: keyelementdep; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE elementdep ALTER COLUMN keyelementdep SET DEFAULT nextval('elementdep_keyelementdep_seq'::regclass);


--
-- Name: keyelementstatus; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE elementstatus ALTER COLUMN keyelementstatus SET DEFAULT nextval('elementstatus_keyelementstatus_seq'::regclass);


--
-- Name: keyelementthread; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE elementthread ALTER COLUMN keyelementthread SET DEFAULT nextval('elementthread_keyelementthread_seq'::regclass);


--
-- Name: keyelementtype; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE elementtype ALTER COLUMN keyelementtype SET DEFAULT nextval('elementtype_keyelementtype_seq'::regclass);


--
-- Name: keyelementuser; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE elementuser ALTER COLUMN keyelementuser SET DEFAULT nextval('elementuser_keyelementuser_seq'::regclass);


--
-- Name: keyfiletracker; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE filetracker ALTER COLUMN keyfiletracker SET DEFAULT nextval('filetracker_keyfiletracker_seq'::regclass);


--
-- Name: keyfiletrackerdep; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE filetrackerdep ALTER COLUMN keyfiletrackerdep SET DEFAULT nextval('filetrackerdep_keyfiletrackerdep_seq'::regclass);


--
-- Name: keygraph; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE graph ALTER COLUMN keygraph SET DEFAULT nextval('graph_keygraph_seq'::regclass);


--
-- Name: keygraphds; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE graphds ALTER COLUMN keygraphds SET DEFAULT nextval('graphds_keygraphds_seq'::regclass);


--
-- Name: keygraphpage; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE graphpage ALTER COLUMN keygraphpage SET DEFAULT nextval('graphpage_keygraphpage_seq'::regclass);


--
-- Name: keygraphrelationship; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE graphrelationship ALTER COLUMN keygraphrelationship SET DEFAULT nextval('graphrelationship_keygraphrelationship_seq'::regclass);


--
-- Name: keygridtemplate; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE gridtemplate ALTER COLUMN keygridtemplate SET DEFAULT nextval('gridtemplate_keygridtemplate_seq'::regclass);


--
-- Name: keygridtemplateitem; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE gridtemplateitem ALTER COLUMN keygridtemplateitem SET DEFAULT nextval('gridtemplateitem_keygridtemplateitem_seq'::regclass);


--
-- Name: keygroupmapping; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE groupmapping ALTER COLUMN keygroupmapping SET DEFAULT nextval('groupmapping_keygroupmapping_seq'::regclass);


--
-- Name: keygrp; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE grp ALTER COLUMN keygrp SET DEFAULT nextval('grp_keygrp_seq'::regclass);


--
-- Name: keygruntscript; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE gruntscript ALTER COLUMN keygruntscript SET DEFAULT nextval('gruntscript_keygruntscript_seq'::regclass);


--
-- Name: keyhistory; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE history ALTER COLUMN keyhistory SET DEFAULT nextval('history_keyhistory_seq'::regclass);


--
-- Name: keyhost; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE host ALTER COLUMN keyhost SET DEFAULT nextval('host_keyhost_seq'::regclass);


--
-- Name: keyhostdailystat; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE hostdailystat ALTER COLUMN keyhostdailystat SET DEFAULT nextval('hostdailystat_keyhostdailystat_seq'::regclass);


--
-- Name: keyhostgroup; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE hostgroup ALTER COLUMN keyhostgroup SET DEFAULT nextval('hostgroup_keyhostgroup_seq'::regclass);


--
-- Name: keyhostgroupitem; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE hostgroupitem ALTER COLUMN keyhostgroupitem SET DEFAULT nextval('hostgroupitem_keyhostgroupitem_seq'::regclass);


--
-- Name: keyhosthistory; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE hosthistory ALTER COLUMN keyhosthistory SET DEFAULT nextval('hosthistory_keyhosthistory_seq'::regclass);


--
-- Name: keyhostinterface; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE hostinterface ALTER COLUMN keyhostinterface SET DEFAULT nextval('hostinterface_keyhostinterface_seq'::regclass);


--
-- Name: keyhostinterfacetype; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE hostinterfacetype ALTER COLUMN keyhostinterfacetype SET DEFAULT nextval('hostinterfacetype_keyhostinterfacetype_seq'::regclass);


--
-- Name: keyhostload; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE hostload ALTER COLUMN keyhostload SET DEFAULT nextval('hostload_keyhostload_seq'::regclass);


--
-- Name: keyhostmapping; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE hostmapping ALTER COLUMN keyhostmapping SET DEFAULT nextval('hostmapping_keyhostmapping_seq'::regclass);


--
-- Name: keyhostresource; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE hostresource ALTER COLUMN keyhostresource SET DEFAULT nextval('hostresource_keyhostresource_seq'::regclass);


--
-- Name: keyhostservice; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE hostservice ALTER COLUMN keyhostservice SET DEFAULT nextval('hostservice_keyhostservice_seq'::regclass);


--
-- Name: keyhostsoftware; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE hostsoftware ALTER COLUMN keyhostsoftware SET DEFAULT nextval('hostsoftware_keyhostsoftware_seq'::regclass);


--
-- Name: keyhoststatus; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE hoststatus ALTER COLUMN keyhoststatus SET DEFAULT nextval('hoststatus_keyhoststatus_seq'::regclass);


--
-- Name: keyjob; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE job ALTER COLUMN keyjob SET DEFAULT nextval('job_keyjob_seq'::regclass);


--
-- Name: keyjobcannedbatch; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE jobcannedbatch ALTER COLUMN keyjobcannedbatch SET DEFAULT nextval('jobcannedbatch_keyjobcannedbatch_seq'::regclass);


--
-- Name: keyjobcommandhistory; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE jobcommandhistory ALTER COLUMN keyjobcommandhistory SET DEFAULT nextval('jobcommandhistory_keyjobcommandhistory_seq'::regclass);


--
-- Name: keyjobdep; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE jobdep ALTER COLUMN keyjobdep SET DEFAULT nextval('jobdep_keyjobdep_seq'::regclass);


--
-- Name: keyjoberror; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE joberror ALTER COLUMN keyjoberror SET DEFAULT nextval('joberror_keyjoberror_seq'::regclass);


--
-- Name: keyjoberrorhandler; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE joberrorhandler ALTER COLUMN keyjoberrorhandler SET DEFAULT nextval('joberrorhandler_keyjoberrorhandler_seq'::regclass);


--
-- Name: keyjoberrorhandlerscript; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE joberrorhandlerscript ALTER COLUMN keyjoberrorhandlerscript SET DEFAULT nextval('joberrorhandlerscript_keyjoberrorhandlerscript_seq'::regclass);


--
-- Name: keyjobhistory; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE jobhistory ALTER COLUMN keyjobhistory SET DEFAULT nextval('jobhistory_keyjobhistory_seq'::regclass);


--
-- Name: keyjobhistorytype; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE jobhistorytype ALTER COLUMN keyjobhistorytype SET DEFAULT nextval('jobhistorytype_keyjobhistorytype_seq'::regclass);


--
-- Name: keyjoboutput; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE joboutput ALTER COLUMN keyjoboutput SET DEFAULT nextval('joboutput_keyjoboutput_seq'::regclass);


--
-- Name: keyjobservice; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE jobservice ALTER COLUMN keyjobservice SET DEFAULT nextval('jobservice_keyjobservice_seq'::regclass);


--
-- Name: keyjobstat; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE jobstat ALTER COLUMN keyjobstat SET DEFAULT nextval('jobstat_keyjobstat_seq'::regclass);


--
-- Name: keyjobstatus; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE jobstatus ALTER COLUMN keyjobstatus SET DEFAULT nextval('jobstatus_keyjobstatus_seq'::regclass);


--
-- Name: keyjobtask; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE jobtask ALTER COLUMN keyjobtask SET DEFAULT nextval('jobtask_keyjobtask_seq'::regclass);


--
-- Name: keyjobtype; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE jobtype ALTER COLUMN keyjobtype SET DEFAULT nextval('jobtype_keyjobtype_seq'::regclass);


--
-- Name: keyjobtypemapping; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE jobtypemapping ALTER COLUMN keyjobtypemapping SET DEFAULT nextval('jobtypemapping_keyjobtypemapping_seq'::regclass);


--
-- Name: keylicense; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE license ALTER COLUMN keylicense SET DEFAULT nextval('license_keylicense_seq'::regclass);


--
-- Name: keylocation; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE "location" ALTER COLUMN keylocation SET DEFAULT nextval('location_keylocation_seq'::regclass);


--
-- Name: keymapping; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE mapping ALTER COLUMN keymapping SET DEFAULT nextval('mapping_keymapping_seq'::regclass);


--
-- Name: keymappingtype; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE mappingtype ALTER COLUMN keymappingtype SET DEFAULT nextval('mappingtype_keymappingtype_seq'::regclass);


--
-- Name: keynotification; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE notification ALTER COLUMN keynotification SET DEFAULT nextval('notification_keynotification_seq'::regclass);


--
-- Name: keynotificationdestination; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE notificationdestination ALTER COLUMN keynotificationdestination SET DEFAULT nextval('notificationdestination_keynotificationdestination_seq'::regclass);


--
-- Name: keynotificationmethod; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE notificationmethod ALTER COLUMN keynotificationmethod SET DEFAULT nextval('notificationmethod_keynotificationmethod_seq'::regclass);


--
-- Name: keynotificationuserroute; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE notificationroute ALTER COLUMN keynotificationuserroute SET DEFAULT nextval('notificationroute_keynotificationuserroute_seq'::regclass);


--
-- Name: keypathsynctarget; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE pathsynctarget ALTER COLUMN keypathsynctarget SET DEFAULT nextval('pathsynctarget_keypathsynctarget_seq'::regclass);


--
-- Name: keypathtemplate; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE pathtemplate ALTER COLUMN keypathtemplate SET DEFAULT nextval('pathtemplate_keypathtemplate_seq'::regclass);


--
-- Name: keypathtracker; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE pathtracker ALTER COLUMN keypathtracker SET DEFAULT nextval('pathtracker_keypathtracker_seq'::regclass);


--
-- Name: keypermission; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE permission ALTER COLUMN keypermission SET DEFAULT nextval('permission_keypermission_seq'::regclass);


--
-- Name: keyphoneno; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE phoneno ALTER COLUMN keyphoneno SET DEFAULT nextval('phoneno_keyphoneno_seq'::regclass);


--
-- Name: keyphonetype; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE phonetype ALTER COLUMN keyphonetype SET DEFAULT nextval('phonetype_keyphonetype_seq'::regclass);


--
-- Name: keyprojectresolution; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE projectresolution ALTER COLUMN keyprojectresolution SET DEFAULT nextval('projectresolution_keyprojectresolution_seq'::regclass);


--
-- Name: keyprojectstatus; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE projectstatus ALTER COLUMN keyprojectstatus SET DEFAULT nextval('projectstatus_keyprojectstatus_seq'::regclass);


--
-- Name: keyprojectstorage; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE projectstorage ALTER COLUMN keyprojectstorage SET DEFAULT nextval('projectstorage_keyprojectstorage_seq'::regclass);


--
-- Name: keyschedule; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE schedule ALTER COLUMN keyschedule SET DEFAULT nextval('schedule_keyschedule_seq'::regclass);


--
-- Name: keyservice; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE service ALTER COLUMN keyservice SET DEFAULT nextval('service_keyservice_seq'::regclass);


--
-- Name: keysoftware; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE software ALTER COLUMN keysoftware SET DEFAULT nextval('software_keysoftware_seq'::regclass);


--
-- Name: keystatusset; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE statusset ALTER COLUMN keystatusset SET DEFAULT nextval('statusset_keystatusset_seq'::regclass);


--
-- Name: keysyslog; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE syslog ALTER COLUMN keysyslog SET DEFAULT nextval('syslog_keysyslog_seq'::regclass);


--
-- Name: keysyslogrealm; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE syslogrealm ALTER COLUMN keysyslogrealm SET DEFAULT nextval('syslogrealm_keysyslogrealm_seq'::regclass);


--
-- Name: keysyslogseverity; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE syslogseverity ALTER COLUMN keysyslogseverity SET DEFAULT nextval('syslogseverity_keysyslogseverity_seq'::regclass);


--
-- Name: keythreadnotify; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE threadnotify ALTER COLUMN keythreadnotify SET DEFAULT nextval('threadnotify_keythreadnotify_seq'::regclass);


--
-- Name: keythumbnail; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE thumbnail ALTER COLUMN keythumbnail SET DEFAULT nextval('thumbnail_keythumbnail_seq'::regclass);


--
-- Name: keytimesheet; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE timesheet ALTER COLUMN keytimesheet SET DEFAULT nextval('timesheet_keytimesheet_seq'::regclass);


--
-- Name: keytimesheetcategory; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE timesheetcategory ALTER COLUMN keytimesheetcategory SET DEFAULT nextval('timesheetcategory_keytimesheetcategory_seq'::regclass);


--
-- Name: keytracker; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE tracker ALTER COLUMN keytracker SET DEFAULT nextval('tracker_keytracker_seq'::regclass);


--
-- Name: keytrackercategory; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE trackercategory ALTER COLUMN keytrackercategory SET DEFAULT nextval('trackercategory_keytrackercategory_seq'::regclass);


--
-- Name: keytrackerqueue; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE trackerqueue ALTER COLUMN keytrackerqueue SET DEFAULT nextval('trackerqueue_keytrackerqueue_seq'::regclass);


--
-- Name: keyuserelement; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE userelement ALTER COLUMN keyuserelement SET DEFAULT nextval('userelement_keyuserelement_seq'::regclass);


--
-- Name: keyusermapping; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE usermapping ALTER COLUMN keyusermapping SET DEFAULT nextval('usermapping_keyusermapping_seq'::regclass);


--
-- Name: keyuserrole; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE userrole ALTER COLUMN keyuserrole SET DEFAULT nextval('userrole_keyuserrole_seq'::regclass);


--
-- Name: keyusrgrp; Type: DEFAULT; Schema: public; Owner: barry
--

ALTER TABLE usrgrp ALTER COLUMN keyusrgrp SET DEFAULT nextval('usrgrp_keyusrgrp_seq'::regclass);


--
-- Name: abdownloadstat_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY abdownloadstat
    ADD CONSTRAINT abdownloadstat_pkey PRIMARY KEY (keyabdownloadstat);


--
-- Name: annotation_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY annotation
    ADD CONSTRAINT annotation_pkey PRIMARY KEY (keyannotation);


--
-- Name: assetproperty_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY assetproperty
    ADD CONSTRAINT assetproperty_pkey PRIMARY KEY (keyassetproperty);


--
-- Name: assettemplate_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY assettemplate
    ADD CONSTRAINT assettemplate_pkey PRIMARY KEY (keyassettemplate);


--
-- Name: bachasset_path_key; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY bachasset
    ADD CONSTRAINT bachasset_path_key UNIQUE (path);


--
-- Name: bachasset_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY bachasset
    ADD CONSTRAINT bachasset_pkey PRIMARY KEY (keybachasset);


--
-- Name: calendar_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY calendar
    ADD CONSTRAINT calendar_pkey PRIMARY KEY (keycalendar);


--
-- Name: calendarcategory_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY calendarcategory
    ADD CONSTRAINT calendarcategory_pkey PRIMARY KEY (keycalendarcategory);


--
-- Name: checklistitem_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY checklistitem
    ADD CONSTRAINT checklistitem_pkey PRIMARY KEY (keychecklistitem);


--
-- Name: checkliststatus_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY checkliststatus
    ADD CONSTRAINT checkliststatus_pkey PRIMARY KEY (keycheckliststatus);


--
-- Name: client_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY client
    ADD CONSTRAINT client_pkey PRIMARY KEY (keyclient);


--
-- Name: config_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY config
    ADD CONSTRAINT config_pkey PRIMARY KEY (keyconfig);


--
-- Name: deliveryelement_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY deliveryelement
    ADD CONSTRAINT deliveryelement_pkey PRIMARY KEY (keydeliveryshot);


--
-- Name: diskimage_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY diskimage
    ADD CONSTRAINT diskimage_pkey PRIMARY KEY (keydiskimage);


--
-- Name: dynamichostgroup_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY dynamichostgroup
    ADD CONSTRAINT dynamichostgroup_pkey PRIMARY KEY (keydynamichostgroup);


--
-- Name: element_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY element
    ADD CONSTRAINT element_pkey PRIMARY KEY (keyelement);


--
-- Name: elementdep_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY elementdep
    ADD CONSTRAINT elementdep_pkey PRIMARY KEY (keyelementdep);


--
-- Name: elementstatus_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY elementstatus
    ADD CONSTRAINT elementstatus_pkey PRIMARY KEY (keyelementstatus);


--
-- Name: elementthread_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY elementthread
    ADD CONSTRAINT elementthread_pkey PRIMARY KEY (keyelementthread);


--
-- Name: elementtype_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY elementtype
    ADD CONSTRAINT elementtype_pkey PRIMARY KEY (keyelementtype);


--
-- Name: elementuser_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY elementuser
    ADD CONSTRAINT elementuser_pkey PRIMARY KEY (keyelementuser);


--
-- Name: filetracker_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY filetracker
    ADD CONSTRAINT filetracker_pkey PRIMARY KEY (keyfiletracker);


--
-- Name: filetrackerdep_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY filetrackerdep
    ADD CONSTRAINT filetrackerdep_pkey PRIMARY KEY (keyfiletrackerdep);


--
-- Name: graph_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY graph
    ADD CONSTRAINT graph_pkey PRIMARY KEY (keygraph);


--
-- Name: graphds_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY graphds
    ADD CONSTRAINT graphds_pkey PRIMARY KEY (keygraphds);


--
-- Name: graphpage_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY graphpage
    ADD CONSTRAINT graphpage_pkey PRIMARY KEY (keygraphpage);


--
-- Name: graphrelationship_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY graphrelationship
    ADD CONSTRAINT graphrelationship_pkey PRIMARY KEY (keygraphrelationship);


--
-- Name: gridtemplate_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY gridtemplate
    ADD CONSTRAINT gridtemplate_pkey PRIMARY KEY (keygridtemplate);


--
-- Name: gridtemplateitem_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY gridtemplateitem
    ADD CONSTRAINT gridtemplateitem_pkey PRIMARY KEY (keygridtemplateitem);


--
-- Name: groupmapping_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY groupmapping
    ADD CONSTRAINT groupmapping_pkey PRIMARY KEY (keygroupmapping);


--
-- Name: grp_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY grp
    ADD CONSTRAINT grp_pkey PRIMARY KEY (keygrp);


--
-- Name: gruntscript_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY gruntscript
    ADD CONSTRAINT gruntscript_pkey PRIMARY KEY (keygruntscript);


--
-- Name: history_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY history
    ADD CONSTRAINT history_pkey PRIMARY KEY (keyhistory);


--
-- Name: host_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY host
    ADD CONSTRAINT host_pkey PRIMARY KEY (keyhost);


--
-- Name: hostdailystat_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY hostdailystat
    ADD CONSTRAINT hostdailystat_pkey PRIMARY KEY (keyhostdailystat);


--
-- Name: hostgroup_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY hostgroup
    ADD CONSTRAINT hostgroup_pkey PRIMARY KEY (keyhostgroup);


--
-- Name: hostgroupitem_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY hostgroupitem
    ADD CONSTRAINT hostgroupitem_pkey PRIMARY KEY (keyhostgroupitem);


--
-- Name: hosthistory_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY hosthistory
    ADD CONSTRAINT hosthistory_pkey PRIMARY KEY (keyhosthistory);


--
-- Name: hostinterface_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY hostinterface
    ADD CONSTRAINT hostinterface_pkey PRIMARY KEY (keyhostinterface);


--
-- Name: hostinterfacetype_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY hostinterfacetype
    ADD CONSTRAINT hostinterfacetype_pkey PRIMARY KEY (keyhostinterfacetype);


--
-- Name: hostload_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY hostload
    ADD CONSTRAINT hostload_pkey PRIMARY KEY (keyhostload);


--
-- Name: hostmapping_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY hostmapping
    ADD CONSTRAINT hostmapping_pkey PRIMARY KEY (keyhostmapping);


--
-- Name: hostresource_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY hostresource
    ADD CONSTRAINT hostresource_pkey PRIMARY KEY (keyhostresource);


--
-- Name: hostservice_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY hostservice
    ADD CONSTRAINT hostservice_pkey PRIMARY KEY (keyhostservice);


--
-- Name: hostsoftware_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY hostsoftware
    ADD CONSTRAINT hostsoftware_pkey PRIMARY KEY (keyhostsoftware);


--
-- Name: hoststatus_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY hoststatus
    ADD CONSTRAINT hoststatus_pkey PRIMARY KEY (keyhoststatus);


--
-- Name: job_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY job
    ADD CONSTRAINT job_pkey PRIMARY KEY (keyjob);


--
-- Name: jobcannedbatch_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY jobcannedbatch
    ADD CONSTRAINT jobcannedbatch_pkey PRIMARY KEY (keyjobcannedbatch);


--
-- Name: jobcommandhistory_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY jobcommandhistory
    ADD CONSTRAINT jobcommandhistory_pkey PRIMARY KEY (keyjobcommandhistory);


--
-- Name: jobdep_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY jobdep
    ADD CONSTRAINT jobdep_pkey PRIMARY KEY (keyjobdep);


--
-- Name: joberror_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY joberror
    ADD CONSTRAINT joberror_pkey PRIMARY KEY (keyjoberror);


--
-- Name: joberrorhandler_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY joberrorhandler
    ADD CONSTRAINT joberrorhandler_pkey PRIMARY KEY (keyjoberrorhandler);


--
-- Name: joberrorhandlerscript_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY joberrorhandlerscript
    ADD CONSTRAINT joberrorhandlerscript_pkey PRIMARY KEY (keyjoberrorhandlerscript);


--
-- Name: jobhistory_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY jobhistory
    ADD CONSTRAINT jobhistory_pkey PRIMARY KEY (keyjobhistory);


--
-- Name: jobhistorytype_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY jobhistorytype
    ADD CONSTRAINT jobhistorytype_pkey PRIMARY KEY (keyjobhistorytype);


--
-- Name: joboutput_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY joboutput
    ADD CONSTRAINT joboutput_pkey PRIMARY KEY (keyjoboutput);


--
-- Name: jobservice_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY jobservice
    ADD CONSTRAINT jobservice_pkey PRIMARY KEY (keyjobservice);


--
-- Name: jobstat_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY jobstat
    ADD CONSTRAINT jobstat_pkey PRIMARY KEY (keyjobstat);


--
-- Name: jobstatus_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY jobstatus
    ADD CONSTRAINT jobstatus_pkey PRIMARY KEY (keyjobstatus);


--
-- Name: jobtask_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY jobtask
    ADD CONSTRAINT jobtask_pkey PRIMARY KEY (keyjobtask);


--
-- Name: jobtype_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY jobtype
    ADD CONSTRAINT jobtype_pkey PRIMARY KEY (keyjobtype);


--
-- Name: jobtypemapping_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY jobtypemapping
    ADD CONSTRAINT jobtypemapping_pkey PRIMARY KEY (keyjobtypemapping);


--
-- Name: license_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY license
    ADD CONSTRAINT license_pkey PRIMARY KEY (keylicense);


--
-- Name: location_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY "location"
    ADD CONSTRAINT location_pkey PRIMARY KEY (keylocation);


--
-- Name: mapping_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY mapping
    ADD CONSTRAINT mapping_pkey PRIMARY KEY (keymapping);


--
-- Name: mappingtype_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY mappingtype
    ADD CONSTRAINT mappingtype_pkey PRIMARY KEY (keymappingtype);


--
-- Name: notification_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY notification
    ADD CONSTRAINT notification_pkey PRIMARY KEY (keynotification);


--
-- Name: notificationdestination_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY notificationdestination
    ADD CONSTRAINT notificationdestination_pkey PRIMARY KEY (keynotificationdestination);


--
-- Name: notificationmethod_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY notificationmethod
    ADD CONSTRAINT notificationmethod_pkey PRIMARY KEY (keynotificationmethod);


--
-- Name: notificationroute_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY notificationroute
    ADD CONSTRAINT notificationroute_pkey PRIMARY KEY (keynotificationuserroute);


--
-- Name: pathsynctarget_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY pathsynctarget
    ADD CONSTRAINT pathsynctarget_pkey PRIMARY KEY (keypathsynctarget);


--
-- Name: pathtemplate_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY pathtemplate
    ADD CONSTRAINT pathtemplate_pkey PRIMARY KEY (keypathtemplate);


--
-- Name: pathtracker_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY pathtracker
    ADD CONSTRAINT pathtracker_pkey PRIMARY KEY (keypathtracker);


--
-- Name: permission_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY permission
    ADD CONSTRAINT permission_pkey PRIMARY KEY (keypermission);


--
-- Name: pg_ts_cfg_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY pg_ts_cfg
    ADD CONSTRAINT pg_ts_cfg_pkey PRIMARY KEY (ts_name);


--
-- Name: pg_ts_cfgmap_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY pg_ts_cfgmap
    ADD CONSTRAINT pg_ts_cfgmap_pkey PRIMARY KEY (ts_name, tok_alias);


--
-- Name: pg_ts_dict_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY pg_ts_dict
    ADD CONSTRAINT pg_ts_dict_pkey PRIMARY KEY (dict_name);


--
-- Name: pg_ts_parser_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY pg_ts_parser
    ADD CONSTRAINT pg_ts_parser_pkey PRIMARY KEY (prs_name);


--
-- Name: phoneno_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY phoneno
    ADD CONSTRAINT phoneno_pkey PRIMARY KEY (keyphoneno);


--
-- Name: phonetype_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY phonetype
    ADD CONSTRAINT phonetype_pkey PRIMARY KEY (keyphonetype);


--
-- Name: projectresolution_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY projectresolution
    ADD CONSTRAINT projectresolution_pkey PRIMARY KEY (keyprojectresolution);


--
-- Name: projectstatus_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY projectstatus
    ADD CONSTRAINT projectstatus_pkey PRIMARY KEY (keyprojectstatus);


--
-- Name: projectstorage_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY projectstorage
    ADD CONSTRAINT projectstorage_pkey PRIMARY KEY (keyprojectstorage);


--
-- Name: schedule_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY schedule
    ADD CONSTRAINT schedule_pkey PRIMARY KEY (keyschedule);


--
-- Name: service_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY service
    ADD CONSTRAINT service_pkey PRIMARY KEY (keyservice);


--
-- Name: software_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY software
    ADD CONSTRAINT software_pkey PRIMARY KEY (keysoftware);


--
-- Name: statusset_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY statusset
    ADD CONSTRAINT statusset_pkey PRIMARY KEY (keystatusset);


--
-- Name: syslog_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY syslog
    ADD CONSTRAINT syslog_pkey PRIMARY KEY (keysyslog);


--
-- Name: syslogrealm_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY syslogrealm
    ADD CONSTRAINT syslogrealm_pkey PRIMARY KEY (keysyslogrealm);


--
-- Name: syslogseverity_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY syslogseverity
    ADD CONSTRAINT syslogseverity_pkey PRIMARY KEY (keysyslogseverity);


--
-- Name: threadnotify_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY threadnotify
    ADD CONSTRAINT threadnotify_pkey PRIMARY KEY (keythreadnotify);


--
-- Name: thumbnail_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY thumbnail
    ADD CONSTRAINT thumbnail_pkey PRIMARY KEY (keythumbnail);


--
-- Name: timesheet_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY timesheet
    ADD CONSTRAINT timesheet_pkey PRIMARY KEY (keytimesheet);


--
-- Name: timesheetcategory_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY timesheetcategory
    ADD CONSTRAINT timesheetcategory_pkey PRIMARY KEY (keytimesheetcategory);


--
-- Name: tracker_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY tracker
    ADD CONSTRAINT tracker_pkey PRIMARY KEY (keytracker);


--
-- Name: trackercategory_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY trackercategory
    ADD CONSTRAINT trackercategory_pkey PRIMARY KEY (keytrackercategory);


--
-- Name: trackerqueue_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY trackerqueue
    ADD CONSTRAINT trackerqueue_pkey PRIMARY KEY (keytrackerqueue);


--
-- Name: userelement_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY userelement
    ADD CONSTRAINT userelement_pkey PRIMARY KEY (keyuserelement);


--
-- Name: usermapping_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY usermapping
    ADD CONSTRAINT usermapping_pkey PRIMARY KEY (keyusermapping);


--
-- Name: userrole_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY userrole
    ADD CONSTRAINT userrole_pkey PRIMARY KEY (keyuserrole);


--
-- Name: usrgrp_pkey; Type: CONSTRAINT; Schema: public; Owner: barry; Tablespace: 
--

ALTER TABLE ONLY usrgrp
    ADD CONSTRAINT usrgrp_pkey PRIMARY KEY (keyusrgrp);


--
-- Name: x_bachasset_fti; Type: INDEX; Schema: public; Owner: barry; Tablespace: 
--

CREATE INDEX x_bachasset_fti ON bachasset USING gin (fti_tags);


--
-- Name: tsvectorupdate; Type: TRIGGER; Schema: public; Owner: barry
--

CREATE TRIGGER tsvectorupdate
    BEFORE INSERT OR UPDATE ON bachasset
    FOR EACH ROW
    EXECUTE PROCEDURE tsearch2('fti_tags', 'tags', 'make_tags', 'path');


--
-- Name: public; Type: ACL; Schema: -; Owner: postgres
--

REVOKE ALL ON SCHEMA public FROM PUBLIC;
REVOKE ALL ON SCHEMA public FROM postgres;
GRANT ALL ON SCHEMA public TO postgres;
GRANT ALL ON SCHEMA public TO PUBLIC;


--
-- PostgreSQL database dump complete
--

