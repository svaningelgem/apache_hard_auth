CREATE TABLE IF NOT EXISTS failed_logins
(
    id              INT(9) NOT NULL AUTO_INCREMENT PRIMARY KEY,
    ip              VARCHAR(15),    
    user            VARCHAR(128),
    passfile        VARCHAR(255),
    time            TIMESTAMP,
    INDEX(ip),
    INDEX(user),
    INDEX(passfile)
);

CREATE TABLE IF NOT EXISTS locked_accounts
(
    id              INT(9) NOT NULL AUTO_INCREMENT PRIMARY KEY,
    user            VARCHAR(128),
    passfile        VARCHAR(255),
    locked_at       TIMESTAMP,  
    INDEX(user),
    INDEX(passfile)
);

CREATE TABLE IF NOT EXISTS last_penalty_time
(
    id               INT(9) NOT NULL AUTO_INCREMENT PRIMARY KEY,
    user             VARCHAR(128),
    passfile         VARCHAR(255),
    ip               VARCHAR(15),    
    sleeptime_in_sec INT(9),
    calculated_at    TIMESTAMP,
    INDEX(user),
    INDEX(passfile)
);
