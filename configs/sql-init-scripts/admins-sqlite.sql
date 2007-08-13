
CREATE TABLE sm_admins (
  id int(10) NOT NULL,
  authtype varchar(16) NOT NULL,
  identity varchar(65) NOT NULL,
  password varchar(65),
  flags varchar(30) NOT NULL,
  name varchar(65) NOT NULL,
  PRIMARY KEY (id)
);

CREATE TABLE sm_groups (
  id int(10) NOT NULL,
  immunity varchar(16) NOT NULL,
  flags varchar(30) NOT NULL,
  name varchar(120) NOT NULL,
  groups_immune varchar(255),
  PRIMARY KEY (id)
);

CREATE TABLE sm_group_overrides (
  group_id int(10) NOT NULL,
  type varchar(16) NOT NULL,
  name varchar(32) NOT NULL,
  access varchar(16) NOT NULL,
  PRIMARY KEY (group_id, type, name)
);

CREATE TABLE sm_overrides (
  type varchar(16) NOT NULL,
  name varchar(32) NOT NULL,
  flags varchar(30) NOT NULL,
  PRIMARY KEY (type,name)
);

CREATE TABLE sm_admins_groups (
  admin_id int(10) NOT NULL,
  group_id int(10) NOT NULL,
  inherit_order int(10) NOT NULL,
  PRIMARY KEY (admin_id, group_id)
);
