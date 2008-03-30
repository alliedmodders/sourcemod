
CREATE TABLE sm_admins (
  id int(10) unsigned NOT NULL auto_increment,
  authtype enum('steam','name','ip') NOT NULL,
  identity varchar(65) NOT NULL,
  password varchar(65),
  flags varchar(30) NOT NULL,
  name varchar(65) NOT NULL,
  immunity int(10) unsigned NOT NULL,
  PRIMARY KEY (id)
);

CREATE TABLE sm_groups (
  id int(10) unsigned NOT NULL auto_increment,
  flags varchar(30) NOT NULL,
  name varchar(120) NOT NULL,
  immunity_level int(1) unsigned NOT NULL,
  PRIMARY KEY (id)
);

CREATE TABLE sm_group_immunity (
  group_id int(10) unsigned NOT NULL,
  other_id int(10) unsigned NOT NULL,
  PRIMARY KEY (group_id, other_id)
);

CREATE TABLE sm_group_overrides (
  group_id int(10) unsigned NOT NULL,
  type enum('command','group') NOT NULL,
  name varchar(32) NOT NULL,
  access enum('allow','deny') NOT NULL,
  PRIMARY KEY (group_id, type, name)
);

CREATE TABLE sm_overrides (
  type enum('command','group') NOT NULL,
  name varchar(32) NOT NULL,
  flags varchar(30) NOT NULL,
  PRIMARY KEY (type,name)
);

CREATE TABLE sm_admins_groups (
  admin_id int(10) unsigned NOT NULL,
  group_id int(10) unsigned NOT NULL,
  inherit_order int(10) NOT NULL,
  PRIMARY KEY (admin_id, group_id)
);

CREATE TABLE IF NOT EXISTS sm_config (
  cfg_key varchar(32) NOT NULL,
  cfg_value varchar(255) NOT NULL,
  PRIMARY KEY (cfg_key)
);

INSERT INTO sm_config (cfg_key, cfg_value) VALUES ('admin_version', '1.0.0.1409') ON DUPLICATE KEY UPDATE cfg_value = '1.0.0.1409';

