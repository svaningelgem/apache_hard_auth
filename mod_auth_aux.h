#ifndef MOD_AUTH_AUX_H
#define MOD_AUTH_AUX_H

typedef struct {
	//configuration directives
	int WaitModifier;
	int DiminishTime;
	int DiminishModifier;
	int DistributedIPs;
	int DistributedTime;
	int LockoutTime;

	char *DBName;
	char *DBUser;
	char *DBPassword;
} mod_auth_aux_rec;

#endif