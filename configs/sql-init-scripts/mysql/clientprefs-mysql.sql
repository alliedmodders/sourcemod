CREATE TABLE sm_cookies
(
	id INTEGER unsigned NOT NULL auto_increment,
	name varchar(30) NOT NULL UNIQUE,
	description varchar(255),
	access INTEGER,
	PRIMARY KEY (id)
);

CREATE TABLE sm_cookie_cache
(
	player varchar(65) NOT NULL,
	cookie_id int(10) NOT NULL,
	value varchar(100),
	timestamp int NOT NULL,
	PRIMARY KEY (player, cookie_id)
);
