CREATE INDEX x_jobnuke_shotname
    ON jobnuke
    USING btree
    (shotname);
CREATE INDEX x_jobbatch_shotname
    ON jobbatch
    USING btree
    (shotname);
CREATE INDEX x_job3delight_shotname
    ON job3delight
    USING btree
    (shotname);
