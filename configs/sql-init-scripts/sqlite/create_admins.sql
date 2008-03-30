
CREATE TABLE sm_admins (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  authtype varchar(16) NOT NULL CHECK(authtype IN ('steam', 'ip', 'name')),
  identity varchar(65) NOT NULL,
  password varchar(65),
  flags varchar(30) NOT NULL,
  name varchar(65) NOT NULL,
  immunity INTEGER NOT NULL
);

CREATE TABLE sm_groups (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  flags varchar(30) NOT NULL,
  name varchar(120) NOT NULL,
  immunity_level INTEGER NOT NULL
);

CREATE TABLE sm_group_immunity (
  group_id INTEGER NOT NULL,
  other_id INTEGER NOT NULL,
  PRIMARY KEY (group_id, other_id)
);

CREATE TABLE sm_group_overrides (
  group_id INTEGER NOT NULL,
  type varchar(16) NOT NULL CHECK (type IN ('command', 'group')),
  name varchar(32) NOT NULL,
  access varchar(16) NOT NULL CHECK (access IN ('allow', 'deny')),
  PRIMARY KEY (group_id, type, name)
);

CREATE TABLE sm_overrides (
  type varchar(16) NOT NULL CHECK (type IN ('command', 'group')),
  name varchar(32) NOT NULL,
  flags varchar(30) NOT NULL,
  PRIMARY KEY (type,name)
);

CREATE TABLE sm_admins_groups (
  admin_id INTEGER NOT NULL,
  group_id INTEGER NOT NULL,
  inherit_order int(10) NOT NULL,
  PRIMARY KEY (admin_id, group_id)
);

CREATE TABLE IF NOT EXISTS sm_config (
  cfg_key varchar(32) NOT NULL,
  cfg_value varchar(255) NOT NULL,
  PRIMARY KEY (cfg_key)
);

REPLACE INTO sm_config (cfg_key, cfg_value) VALUES ('admin_version', '1.0.0.1409');

