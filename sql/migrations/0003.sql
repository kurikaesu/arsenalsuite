ALTER TABLE job ADD COLUMN maxquiettime integer;
UPDATE job SET maxquiettime = 3600;
ALTER TABLE job ALTER COLUMN maxquiettime SET NOT NULL;
ALTER TABLE job ALTER COLUMN maxquiettime SET DEFAULT 3600;
