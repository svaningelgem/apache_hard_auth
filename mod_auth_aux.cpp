#include "httpd.h"
#include "http_config.h"
#include "http_log.h"
#include "mod_auth_aux.h"
#include <SQLAPI.h> 

extern "C" void LogFailedUser(request_rec *r, char *auth_pwfile);
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
	if (!g_pCon)
	{
		CreateDBConnection(r->server);
	}

	try
    {
		SACommand cmd(g_pCon, "INSERT INTO failedlogins(ip, user, conffile, time) VALUES(:1, :2, :3, :4)");

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