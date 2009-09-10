#ifndef MOD_AUTH_AUX_H
#define MOD_AUTH_AUX_H


//#define WARN(...) ap_log_error(__FILE__, __LINE__, APLOG_WARNING, 0, s, __VA_ARGS__)
#define WARN(x, ...) { FILE * fp = fopen( "/tmp/auth_hard.log", "ab" ); fprintf( fp, "%s (%d): " x "\n", __FILE__, __LINE__, __VA_ARGS__ ); fclose(fp); }

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