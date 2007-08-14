
CREATE TABLE sm_admins (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  authtype varchar(16) NOT NULL CHECK(authtype IN ('steam', 'ip', 'name')),
  identity varchar(65) NOT NULL,
  password varchar(65),
  flags varchar(30) NOT NULL,
  name varchar(65) NOT NULL
);

CREATE TABLE sm_groups (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  immunity varchar(16) NOT NULL CHECK(immunity IN ('none', 'default', 'global', 'all')),
  flags varchar(30) NOT NULL,
  name varchar(120) NOT NULL,
  groups_immune varchar(255)
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
