#include "httpd.h"
#include "http_config.h"
#include "http_log.h"
#include "mod_auth_hard_aux.h"
#include <SQLAPI.h> 
#include <math.h>

extern "C" void LogFailedUser(request_rec *r, char *auth_pwfile);
extern "C" int IsAccountLocked(request_rec *r, char *auth_pwfile);
extern "C" unsigned int GetSleepTimeForFailedAuthInSec(request_rec *r, char *auth_pwfile);
extern "C" module AP_MODULE_DECLARE_DATA auth_hard_module;

SAConnection *g_pCon = NULL;

bool CreateDBConnection(server_rec *s)
{
    WARN("CreateDBConnection(s: %p)", s);

    auth_config_rec_server *modcfg = (auth_config_rec_server*)ap_get_module_config(s->module_config, &auth_hard_module);

    bool bRes = false;

    try
    {
        g_pCon = new SAConnection();

        g_pCon->Connect(modcfg->DBName, modcfg->DBUser, modcfg->DBPassword, SA_MySQL_Client);


        ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, "Connection to mod_auth database successfully created");

        bRes = true;
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

    return bRes;
}

void LogFailedUser(request_rec *r, char *auth_pwfile)
{
    WARN("LogFailedUser(r: %p, auth_pwfile: '%s')", r, NUL(auth_pwfile));

    auth_config_rec *modcfg = (auth_config_rec *)ap_get_module_config(r->per_dir_config, &auth_hard_module);

    if (!g_pCon)
    {
        if (!CreateDBConnection(r->server))
        {
            return;
        }
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
    WARN("IsAccountLocked(r: %p, auth_pwfile: '%s')", r, NUL(auth_pwfile));

    auth_config_rec *modcfg = (auth_config_rec *)ap_get_module_config(r->per_dir_config, &auth_hard_module);

    if (!g_pCon)
    {
        if (!CreateDBConnection(r->server))
        {
            return 0;
        }
    }

    int nRes = 0;

    try
    {
        //1. if user is in locked_accounts then dt = (now - locked_at) 
        SACommand selectcmd(g_pCon, "SELECT NOW()-locked_at AS timedelta FROM locked_accounts WHERE user=:1 AND passfile=:2");
        selectcmd.Param(1).setAsString() = r->user;
        selectcmd.Param(2).setAsString() = auth_pwfile;
        selectcmd.Execute();

        unsigned int nTimeDelta = 0;
        bool bIsUseFound = false;
        while (selectcmd.FetchNext())
        {
            nTimeDelta = selectcmd.Field("timedelta").asLong();
            bIsUseFound = true;
        }

        if (bIsUseFound)
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

unsigned int GetSleepTimeForFailedAuthInSec(request_rec *r, char *auth_pwfile)
{
    WARN("GetSleepTimeForFailedAuthInSec(r: %p, auth_pwfile: '%s')", r, NUL(auth_pwfile));

    auth_config_rec *modcfg = (auth_config_rec *)ap_get_module_config(r->per_dir_config, &auth_hard_module);

    if (!g_pCon)
    {
        if (!CreateDBConnection(r->server))
        {
            return 0;
        }
    }

    unsigned int nNewSleepTimeInSec = 0;

    try
    {
        //1. get the last sleep time = lst for this ip and the time it was calculated == calculated_at from last_penalty_time
        SACommand selectcmd(g_pCon, "SELECT sleeptime_in_sec, NOW() - calculated_at AS timedelta FROM last_penalty_time WHERE ip=:1");
        selectcmd.Param(1).setAsString() = r->connection->remote_ip;
        selectcmd.Execute();

        unsigned int nLastSleepTime = 0;
        unsigned int nTimeDelta = 0;
        bool bIsUseFound = false;
        while (selectcmd.FetchNext())
        {
            nLastSleepTime = selectcmd.Field("sleeptime_in_sec").asLong();
            nTimeDelta = selectcmd.Field("timedelta").asLong();
            bIsUseFound = true;
        }

        if (!bIsUseFound)
        {
            //2. if not found insert new sleep time = 1 in last_penalty_time and return it 
            nNewSleepTimeInSec = 1;

            SACommand insertcmd(g_pCon, "INSERT INTO last_penalty_time(ip, sleeptime_in_sec, calculated_at) VALUES(:1, :2, NOW())");
            insertcmd.Param(1).setAsString() = r->connection->remote_ip;
            insertcmd.Param(2).setAsLong() = nNewSleepTimeInSec;
            insertcmd.Execute();
            g_pCon->Commit();
        }
        else
        {
            //4. new sleep time = floor((lst / (DiminishModifier ^(floor(timedelta / DiminishTime))))) * WaitModifier
            double param1 = floor((double)nTimeDelta / (double)modcfg->DiminishTime);
            double param2 = pow((double)modcfg->DiminishModifier, param1);
            nNewSleepTimeInSec = (unsigned int)(floor((double)nLastSleepTime / param2) * (double)modcfg->WaitModifier);
            if (nNewSleepTimeInSec == 0)
            {
                nNewSleepTimeInSec = 1;
            }

            //5. update new sleep time in last_penalty_time and return it
            SACommand updatecmd(g_pCon, "UPDATE last_penalty_time SET sleeptime_in_sec=:1, calculated_at=NOW() WHERE ip=:2");
            updatecmd.Param(1).setAsLong() = nNewSleepTimeInSec;
            updatecmd.Param(2).setAsString() = r->connection->remote_ip;
            updatecmd.Execute();
            g_pCon->Commit();
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
    
    return nNewSleepTimeInSec;
}
