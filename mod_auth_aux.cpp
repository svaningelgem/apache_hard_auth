#include "httpd.h"
#include "http_config.h"
#include "http_log.h"
#include "mod_auth_aux.h"
#include <SQLAPI.h> 

extern "C" void LogFailedUser(request_rec *r, char *auth_pwfile);
extern "C" void IsAccountBlocked(request_rec *r, char *auth_pwfile);
extern "C" void GetSleepTimeForFailedAuth(request_rec *r, char *auth_pwfile);
extern "C" module AP_MODULE_DECLARE_DATA auth_module;

SAConnection *g_pCon = NULL;

void CreateDBConnection(server_rec *s)
{
	mod_auth_aux_rec *modcfg = (mod_auth_aux_rec *)ap_get_module_config(s->module_config, &auth_module);

	try
    {
		g_pCon = new SAConnection();

		g_pCon->Connect(modcfg->DBName, modcfg->DBUser, modcfg->DBPassword, SA_MySQL_Client);

		ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, "Connection to mod_auth database successfully created");
    }
    catch(SAException &x)
    {
        try
        {
            g_pCon->Rollback();
        }
        catch(SAException &)
        {
        }
		delete g_pCon; g_pCon = NULL;

		ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, (const char*)x.ErrText());
    }
}

void LogFailedUser(request_rec *r, char *auth_pwfile)
{
	//1. write the failed attempt it failed_logins
	//2. if (SELECT COUNT(*) FROM failed_logins WHERE now - time < DistributedTime) > DistributedIPs then lock user 

	if (!g_pCon)
	{
		CreateDBConnection(r->server);
	}

	try
    {
		SACommand cmd(g_pCon, "INSERT INTO failed_logins(ip, user, conffile, time) VALUES(:1, :2, :3, :4)");

		cmd.Param(1).setAsString() = r->connection->remote_ip;
        cmd.Param(2).setAsString() = r->user;
		cmd.Param(3).setAsString() = auth_pwfile;

		apr_time_exp_t localTime;
		apr_explode_localtime(&localTime, r->request_time);

		cmd.Param(4).setAsDateTime() = SADateTime(localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday, localTime.tm_hour, localTime.tm_min, localTime.tm_sec);

        // Insert first row
        cmd.Execute();

        g_pCon->Commit();
    }
    catch(SAException &x)
    {
        try
        {
            g_pCon->Rollback();
			g_pCon->Disconnect();delete g_pCon; g_pCon = NULL;
        }
        catch(SAException &)
        {
        }

		ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, (const char*)x.ErrText());
    }
}

bool IsAccountBlocked(request_rec *r, char *auth_pwfile)
{
	bool bRes = false;

	//1. if user is in locked_accounts then dt = (now - locked_at) 
	//2. if dt < lockout-time return true
	//3. else delete it from table and return false

	return bRes;
}

unsigned int GetSleepTimeForFailedAuth(request_rec *r, char *auth_pwfile)
{
	unsigned int nRes = 0;

	//1. get the last sleep time = lst for this user/ip and the time it was calculated == calculated_at from last_penalty_time
	//2. if not found new sleep time == 1, go to 5
	//3. dt = now - calculated_at (in seconds)
	//4. new sleep time = floor((lst / (DiminishModifier ^(floor(dt / DiminishTime))))) * WaitModifier
	//5. store new sleep time in last_penalty_time and return in it
	
	return nRes;
}
