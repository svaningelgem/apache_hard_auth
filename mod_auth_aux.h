#ifndef MOD_AUTH_AUX_H
#define MOD_AUTH_AUX_H

typedef struct {
    //configuration directives
    unsigned int WaitModifier;
    unsigned int DiminishTime;
    unsigned int DiminishModifier;
    unsigned int DistributedIPs;
    unsigned int DistributedTime;
    unsigned int LockoutTime;

    char *DBName;
    char *DBUser;
    char *DBPassword;
} mod_auth_aux_rec;

#endif