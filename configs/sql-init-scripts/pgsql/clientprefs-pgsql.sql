CREATE TABLE IF NOT EXISTS sm_cookies
(
	id serial,
	name varchar(30) NOT NULL UNIQUE,
	description varchar(255),
	access INTEGER,
	PRIMARY KEY (id)
);

CREATE TABLE IF NOT EXISTS sm_cookie_cache
(
	player varchar(65) NOT NULL,
	cookie_id int NOT NULL,
	value varchar(100),
	timestamp int NOT NULL,
	PRIMARY KEY (player, cookie_id)
);

CREATE LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION add_or_update_cookie(in_player VARCHAR(65), in_cookie INT, in_value VARCHAR(100), in_time INT) RETURNS VOID AS
$$
BEGIN
  LOOP
    -- first try to update the it.
    UPDATE sm_cookie_cache SET value = in_value, timestamp = in_time WHERE player = in_player AND cookie_id = in_cookie;
    IF found THEN
      RETURN;
    END IF;
    -- not there, so try to insert.
    -- if someone else inserts the same key concurrently, we could get a unique-key failure.
    BEGIN
      INSERT INTO sm_cookie_cache (player, cookie_id, value, timestamp) VALUES (in_player, in_cookie, in_value, in_time);
      RETURN;
    EXCEPTION WHEN unique_violation THEN
      -- do nothing...  loop again, and we'll update.
    END;
  END LOOP;
END;
$$
LANGUAGE plpgsql;