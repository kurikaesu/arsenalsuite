ALTER TABLE license DROP CONSTRAINT check_licenses_inuse;

ALTER TABLE license
   ADD CONSTRAINT check_licenses_inuse CHECK (inuse <= total);
