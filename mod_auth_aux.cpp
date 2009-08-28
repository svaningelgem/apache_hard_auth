#include "httpd.h"
#include "http_config.h"
#include "http_log.h"
#include "mod_auth_aux.h"
#include <SQLAPI.h> 

extern "C" void LogFailedUser(request_rec *r, char *auth_pwfile);
extern "C" int IsAccountLocked(request_rec *r, char *auth_pwfile);
extern "C" unsigned int GetSleepTimeForFailedAuth(request_rec *r, char *auth_pwfile);
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
    mod_auth_aux_rec *modcfg = (mod_auth_aux_rec *)ap_get_module_config(r->server->module_config, &auth_module);

    if (!g_pCon)
    {
        CreateDBConnection(r->server);
    }

    try
    {
        //1. write the failed attempt in failed_logins
        SACommand insertcmd(g_pCon, "INSERT INTO failed_logins(ip, user, passfile, time) VALUES(:1, :2, :3, NOW())");
        insertcmd.Param(1).setAsString() = r->connection->remote_ip;
        insertcmd.Param(2).setAsString() = r->user;
        insertcmd.Param(3).setAsString() = auth_pwfile;
        insertcmd.Execute();
        g_pCon->Commit();

        //2. if (SELECT COUNT(DISTINCT ip) FROM failed_logins WHERE now - time < DistributedTime) > DistributedIPs then lock user 
        SACommand selectcmd(g_pCon, "SELECT COUNT(DISTINCT ip) AS count FROM failed_logins WHERE NOW() - TIME < :1");
        selectcmd.Param(1).setAsULong() = modcfg->DistributedTime;
        selectcmd.Execute();

        unsigned int nFailedLogins = 0;
        while (selectcmd.FetchNext())
        {
            nFailedLogins = selectcmd.Field("count").asLong();
        }
        
        if (nFailedLogins > modcfg->DistributedIPs)
        {
            //if the user is already locked then just update locked_at else insert a new row in the locked_accounts
            SACommand updatecmd(g_pCon, "UPDATE locked_accounts SET locked_at = NOW() WHERE user=:1 AND passfile=:2");
            updatecmd.Param(1).setAsString() = r->user;
            updatecmd.Param(2).setAsString() = auth_pwfile;
            updatecmd.Execute();
            if (updatecmd.RowsAffected() == 0)
            {
                SACommand insertcmd2(g_pCon, "INSERT INTO locked_accounts(user, passfile, locked_at) VALUES(:1, :2, NOW())");
                insertcmd2.Param(1).setAsString() = r->user;
                insertcmd2.Param(2).setAsString() = auth_pwfile;
                insertcmd2.Execute();
                g_pCon->Commit();
            }
        }
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

int IsAccountLocked(request_rec *r, char *auth_pwfile)
{
    mod_auth_aux_rec *modcfg = (mod_auth_aux_rec *)ap_get_module_config(r->server->module_config, &auth_module);

    if (!g_pCon)
    {
        CreateDBConnection(r->server);
    }

    int nRes = 0;

    try
    {
        //1. if user is in locked_accounts then dt = (now - locked_at) 
        SACommand selectcmd(g_pCon, "SELECT COUNT(*) AS count, NOW()-locked_at AS timedelta FROM locked_accounts WHERE user=:1 AND passfile=:2");
        selectcmd.Param(1).setAsString() = r->user;
        selectcmd.Param(2).setAsString() = auth_pwfile;
        selectcmd.Execute();

        unsigned int nTimeDelta = 0;
        unsigned int nCount = 0;
        while (selectcmd.FetchNext())
        {
            nTimeDelta = selectcmd.Field("timedelta").asLong();
            nCount = selectcmd.Field("count").asLong();
        }

        if (nCount > 0)
        {
            if (nTimeDelta < modcfg->LockoutTime)
            {
                //2. if dt < lockout-time return true(1)
                nRes = 1;
            }
            else
            {
                //3. else delete it from locked_accounts and return false(0)
                SACommand deletecmd(g_pCon, "DELETE FROM locked_accounts WHERE user=:1 AND passfile=:2");
                deletecmd.Param(1).setAsString() = r->user;
                deletecmd.Param(2).setAsString() = auth_pwfile;
                deletecmd.Execute();
                g_pCon->Commit();

                nRes = 0;
            }
        }
        else
        {
            //this user is not in locked_accounts
            nRes = 0;
        }
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

    return nRes;
}


//1. get the last sleep time = lst for this user/ip and the time it was calculated == calculated_at from last_penalty_time
//2. if not found new sleep time == 1, go to 6
//3. dt = now - calculated_at (in seconds)
//4. new sleep time = floor((lst / (DiminishModifier ^(floor(dt / DiminishTime))))) * WaitModifier
//5. update new sleep time in last_penalty_time and return it
//6. insert new sleep time = 1 and return itunsigned int GetSleepTimeForFailedAuth(request_rec *r, char *auth_pwfile)
unsigned int GetSleepTimeForFailedAuth(request_rec *r, char *auth_pwfile)
{
    unsigned int nRes = 0;


    
    return nRes;
}
