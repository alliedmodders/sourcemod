
ALTER TABLE sm_admins ADD immunity INTEGER DEFAULT 0 NOT NULL;

CREATE TABLE _sm_groups_temp (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  flags varchar(30) NOT NULL,
  name varchar(120) NOT NULL,
  immunity_level INTEGER DEFAULT 0 NOT NULL
);
INSERT INTO _sm_groups_temp (id, flags, name) SELECT id, flags, name FROM sm_groups;
UPDATE _sm_groups_temp SET immunity_level = 2 WHERE id IN (SELECT g.id FROM sm_groups g WHERE g.immunity = 'global');
UPDATE _sm_groups_temp SET immunity_level = 1 WHERE id IN (SELECT g.id FROM sm_groups g WHERE g.immunity = 'default');
DROP TABLE sm_groups;
ALTER TABLE _sm_groups_temp RENAME TO sm_groups;

CREATE TABLE IF NOT EXISTS sm_config (
  cfg_key varchar(32) NOT NULL,
  cfg_value varchar(255) NOT NULL,
  PRIMARY KEY (cfg_key)
);

REPLACE INTO sm_config (cfg_key, cfg_value) VALUES ('admin_version', '1.0.0.1409');

