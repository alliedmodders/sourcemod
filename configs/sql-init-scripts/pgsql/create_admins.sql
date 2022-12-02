CREATE TABLE sm_admins (
  id serial,
  authtype varchar(6) NOT NULL,
  CHECK (authtype in ('steam', 'name', 'ip')),
  identity varchar(65) NOT NULL,
  password varchar(65),
  flags varchar(30) NOT NULL,
  name varchar(65) NOT NULL,
  immunity int NOT NULL,
  PRIMARY KEY (id)
);

CREATE TABLE sm_groups (
  id serial,
  flags varchar(30) NOT NULL,
  name varchar(120) NOT NULL,
  immunity_level int NOT NULL,
  PRIMARY KEY (id)
);

CREATE TABLE sm_group_immunity (
  group_id int NOT NULL,
  other_id int NOT NULL,
  FOREIGN KEY (group_id) REFERENCES sm_groups(id) ON DELETE CASCADE,
  FOREIGN KEY (other_id) REFERENCES sm_groups(id) ON DELETE CASCADE,
  PRIMARY KEY (group_id, other_id)
);

CREATE TABLE sm_group_overrides (
  group_id int NOT NULL,
  FOREIGN KEY (group_id) REFERENCES sm_groups(id) ON DELETE CASCADE,
  type varchar(10) NOT NULL,
  CHECK (type in ('command', 'group')),
  name varchar(32) NOT NULL,
  access varchar(5) NOT NULL,
  CHECK (access in ('allow', 'deny')),
  PRIMARY KEY (group_id, type, name)
);

CREATE TABLE sm_overrides (
  type varchar(10) NOT NULL,
  CHECK (type in ('command', 'group')),
  name varchar(32) NOT NULL,
  flags varchar(30) NOT NULL,
  PRIMARY KEY (type,name)
);

CREATE TABLE sm_admins_groups (
  admin_id int NOT NULL,
  group_id int NOT NULL,
  FOREIGN KEY (admin_id) REFERENCES sm_admins(id) ON DELETE CASCADE,
  FOREIGN KEY (group_id) REFERENCES sm_groups(id) ON DELETE CASCADE,
  inherit_order int NOT NULL,
  PRIMARY KEY (admin_id, group_id)
);

-- side note, this is pgsql module, sm_config will not exist if the above stuff exists... and it's being left to the admin
-- to figure out if it exists.
CREATE TABLE sm_config (
  cfg_key varchar(32) NOT NULL,
  cfg_value varchar(255) NOT NULL,
  PRIMARY KEY (cfg_key)
);

INSERT INTO sm_config (cfg_key, cfg_value) VALUES ('admin_version', '1.0.0.1409');