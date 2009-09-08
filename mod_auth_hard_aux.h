#ifndef MOD_AUTH_AUX_H
#define MOD_AUTH_AUX_H

typedef struct {
    char *auth_pwfile;
    char *auth_grpfile;
    int auth_authoritative;

    //mod_auth_hard specific
    unsigned int WaitModifier;
    unsigned int DiminishTime;
    unsigned int DiminishModifier;
    unsigned int DistributedIPs;
    unsigned int DistributedTime;
    unsigned int LockoutTime;
} auth_config_rec;

typedef struct 
{
    char *DBName;
    char *DBUser;
    char *DBPassword;
} auth_config_rec_server;

#endif