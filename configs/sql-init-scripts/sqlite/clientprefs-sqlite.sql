CREATE TABLE sm_cookies
(
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	name varchar(30) NOT NULL UNIQUE,
	description varchar(255),
	access INTEGER
);

CREATE TABLE sm_cookie_cache
(
	player varchar(65) NOT NULL,
	cookie_id int(10) NOT NULL,
	value varchar(100),
	timestamp int,
	PRIMARY KEY (player, cookie_id)
);