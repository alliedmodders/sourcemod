
ALTER TABLE sm_admins ADD immunity INT UNSIGNED NOT NULL;

ALTER TABLE sm_groups ADD immunity_level INT UNSIGNED NOT NULL;
UPDATE sm_groups SET immunity_level = 2 WHERE immunity = 'default';
UPDATE sm_groups SET immunity_level = 1 WHERE immunity = 'global';
ALTER TABLE sm_groups DROP immunity;

CREATE TABLE sm_config (
  cfg_key varchar(32) NOT NULL,
  cfg_value varchar(255) NOT NULL,
  PRIMARY KEY (cfg_key)
);
INSERT INTO sm_config (cfg_key, cfg_value) VALUES ('admin_version', '1.0.0.1409') ON DUPLICATE KEY UPDATE cfg_value = '1.0.0.1409';

