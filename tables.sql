DROP DATABASE IF EXISTS mod_auth;
CREATE DATABASE mod_auth;
USE mod_auth;

DROP TABLE IF EXISTS failed_logins;
CREATE TABLE failed_logins
(
	id				INT(9) NOT NULL AUTO_INCREMENT PRIMARY KEY,
	ip				VARCHAR(15),	
	user			VARCHAR(128),
	conffile		VARCHAR(255),
	time			TIMESTAMP,
	INDEX(ip),
	INDEX(user),
	INDEX(conffile)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;


DROP TABLE IF EXISTS locked_accounts;
CREATE TABLE locked_accounts
(
	id				INT(9) NOT NULL AUTO_INCREMENT PRIMARY KEY,
	user			VARCHAR(128),
	conffile		VARCHAR(255),
	locked_at	    TIMESTAMP,	
	INDEX(user),
	INDEX(conffile)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS last_penalty_time;
CREATE TABLE last_penalty_time
(
	id				INT(9) NOT NULL AUTO_INCREMENT PRIMARY KEY,
	user			VARCHAR(128),
	conffile		VARCHAR(255),
	ip				VARCHAR(15),	
	sleeptime		INT(9),
	calculated_at	TIMESTAMP
	INDEX(user),
	INDEX(conffile)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;