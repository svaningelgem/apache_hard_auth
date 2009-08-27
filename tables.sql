DROP DATABASE IF EXISTS mod_auth;
CREATE DATABASE mod_auth;
USE mod_auth;

DROP TABLE IF EXISTS failedlogins;
CREATE TABLE failedlogins
(
	id					INT(9)			NOT NULL AUTO_INCREMENT PRIMARY KEY,
	ip					VARCHAR(15),	
	user				VARCHAR(128),
	conffile			VARCHAR(255),
	time				TIMESTAMP,
	INDEX(ip),
	INDEX(user),
	INDEX(conffile)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;