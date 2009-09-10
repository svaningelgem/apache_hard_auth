#ifndef MOD_AUTH_AUX_H
#define MOD_AUTH_AUX_H


/*
#define tmp(x) ap_log_error(__FILE__, __LINE__, APLOG_WARNING, 0, NULL, x)
#define NUL(x) (x ? x : "(null)")
#define WARN(...)                                                           \
{                                                                           \
  time_t now = time(NULL);                                                  \
  struct tm _tm;                                                            \
  char buf[256];                                                            \
                                                                            \
  memcpy( &_tm, localtime(&now), sizeof(struct tm) );                       \
  strftime( buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S] ", &_tm );               \
                                                                            \
  FILE * fp = fopen( "/tmp/auth_hard.log", "ab" );                          \
  if ( fp )                                                                 \
  {                                                                         \
    fprintf( fp, buf );                                                     \
    fprintf( fp, "%s (% 5d): ", __FILE__, __LINE__ );                       \
    fprintf( fp, __VA_ARGS__ );                                             \
    fprintf( fp, "\n" );                                                    \
    fclose(fp);                                                             \
  }                                                                         \
  else                                                                      \
  {                                                                         \
    ap_log_error(__FILE__, __LINE__, APLOG_WARNING, 0, NULL, __VA_ARGS__);  \
  }                                                                         \
}
*/
#define WARN(...)

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