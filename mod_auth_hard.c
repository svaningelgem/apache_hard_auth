/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
   
/* 
 * http_auth: authentication
 * 
 * Rob McCool
 * 
 * Adapted to Apache by rst.
 *
 * dirkx - Added Authoritative control to allow passing on to lower
 *         modules if and only if the userid is not known to this
 *         module. A known user with a faulty or absent password still
 *         causes an AuthRequired. The default is 'Authoritative', i.e.
 *         no control is passed along.
 */

#include "apr_strings.h"
#include "apr_md5.h"            /* for apr_password_validate */

#include "ap_config.h"
#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"

#include "mod_auth_hard_aux.h"

module AP_MODULE_DECLARE_DATA auth_hard_module;


static void *create_auth_dir_config(apr_pool_t *p, char *d)
{
    WARN("create_auth_dir_config(p: %p, d: '%s')", p, NUL(d));

    auth_config_rec *conf = apr_palloc(p, sizeof(*conf));

    conf->auth_pwfile         = NULL;     /* just to illustrate the default really */
    conf->auth_grpfile        = NULL;    /* unless you have a broken HP cc */
    conf->auth_authoritative  = 1; /* keep the fortress secure by default */

    conf->WaitModifier        = 2;
    conf->DiminishTime        = 60;
    conf->DiminishModifier    = 2;
    conf->DistributedIPs      = 10;
    conf->DistributedTime     = 60;
    conf->LockoutTime         = 1200;

    return conf;
}

static void *create_perserver_config(apr_pool_t *p, server_rec *s)
{
    WARN("create_perserver_config(p: %p, s: %p)", p, s);

    auth_config_rec_server *conf = (auth_config_rec_server*)apr_pcalloc(p, sizeof(auth_config_rec_server));

    conf->DBName = "mod_auth";
    conf->DBUser = "";
    conf->DBPassword = "";

    return conf;
}

static const char *set_auth_slot(cmd_parms *cmd, void *offset, const char *f, 
                                 const char *t)
{
    WARN("set_auth_slot(cmd: %p, offset: %p, f: '%s', t: '%s')", cmd, offset, NUL(f), NUL(t));

    if (t && strcmp(t, "standard")) {
        return apr_pstrcat(cmd->pool, "Invalid auth file type: ", t, NULL);
    }

    return ap_set_file_slot(cmd, offset, f);
}

static const char *set_DBName(cmd_parms *cmd, void *offset, const char *f)
{
    WARN("set_DBName(cmd: %p, offset: %p, f: '%s')", cmd, offset, NUL(f));

    auth_config_rec_server *modcfg = ap_get_module_config(cmd->server->module_config, &auth_hard_module );
    modcfg->DBName = (char*)f;
    return NULL;
}

static const char *set_DBUser(cmd_parms *cmd, void *offset, const char *f)
{
    WARN("set_DBUser(cmd: %p, offset: %p, f: '%s')", cmd, offset, NUL(f));

    auth_config_rec_server *modcfg = ap_get_module_config(cmd->server->module_config, &auth_hard_module );
    modcfg->DBUser = (char*)f;
    return NULL;
}

static const char *set_DBPassword(cmd_parms *cmd, void *offset, const char *f)
{
    WARN("set_DBPassword(cmd: %p, offset: %p, f: '%s')", cmd, offset, NUL(f));

    auth_config_rec_server *modcfg = ap_get_module_config(cmd->server->module_config, &auth_hard_module );
    modcfg->DBPassword = (char*)f;
    return NULL;
}

static const command_rec auth_cmds[] =
{
    AP_INIT_TAKE12("AuthUserFile", set_auth_slot,
                   (void *)APR_OFFSETOF(auth_config_rec, auth_pwfile),
                   OR_AUTHCFG, "text file containing user IDs and passwords"),
    AP_INIT_TAKE12("AuthGroupFile", set_auth_slot,
                   (void *)APR_OFFSETOF(auth_config_rec, auth_grpfile),
                   OR_AUTHCFG,
                   "text file containing group names and member user IDs"),
    AP_INIT_FLAG("AuthAuthoritative", ap_set_flag_slot,
                 (void *)APR_OFFSETOF(auth_config_rec, auth_authoritative),
                 OR_AUTHCFG,
                 "Set to 'no' to allow access control to be passed along to "
                 "lower modules if the UserID is not known to this module"),
    AP_INIT_TAKE1("ModAuth_WaitModifier", ap_set_int_slot, 
                   (void *)APR_OFFSETOF(auth_config_rec, WaitModifier), OR_AUTHCFG, "Default is 2"),
    AP_INIT_TAKE1("ModAuth_DiminishTime", ap_set_int_slot, 
                   (void *)APR_OFFSETOF(auth_config_rec, DiminishTime), OR_AUTHCFG, "Default is 60"),
    AP_INIT_TAKE1("ModAuth_DiminishModifier", ap_set_int_slot, 
                   (void *)APR_OFFSETOF(auth_config_rec, DiminishModifier), OR_AUTHCFG, "Default is 2"),
    AP_INIT_TAKE1("ModAuth_DistributedIPs", ap_set_int_slot, 
                   (void *)APR_OFFSETOF(auth_config_rec, DistributedIPs), OR_AUTHCFG, "Default is 10"),
    AP_INIT_TAKE1("ModAuth_DistributedTime", ap_set_int_slot, 
                   (void *)APR_OFFSETOF(auth_config_rec, DistributedTime), OR_AUTHCFG, "Default is 60"),
    AP_INIT_TAKE1("ModAuth_LockoutTime", ap_set_int_slot, 
                   (void *)APR_OFFSETOF(auth_config_rec, LockoutTime), OR_AUTHCFG, "Default is 1200"),
    AP_INIT_TAKE1("ModAuth_DBName", set_DBName, NULL, RSRC_CONF, "Default is mod_auth"),
    AP_INIT_TAKE1("ModAuth_DBUser", set_DBUser, NULL, RSRC_CONF, "No Default"),
    AP_INIT_TAKE1("ModAuth_DBPassword", set_DBPassword, NULL, RSRC_CONF, "Default is empty"),
    {NULL}
};

static char *get_pw(request_rec *r, char *user, char *auth_pwfile)
{
    WARN("get_pw(r: %p, user: '%s', auth_pwfile: '%s')", r, NUL(user), NUL(auth_pwfile));

    ap_configfile_t *f;
    char l[MAX_STRING_LEN];
    const char *rpw, *w;
    apr_status_t status;

    if ((status = ap_pcfg_openfile(&f, r->pool, auth_pwfile)) != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, status, r,
                      "Could not open password file: %s", auth_pwfile);
        return NULL;
    }
    while (!(ap_cfg_getline(l, MAX_STRING_LEN, f))) {
        if ((l[0] == '#') || (!l[0])) {
            continue;
        }
        rpw = l;
        w = ap_getword(r->pool, &rpw, ':');

        if (!strcmp(user, w)) {
            ap_cfg_closefile(f);
            return ap_getword(r->pool, &rpw, ':');
        }
    }
    ap_cfg_closefile(f);
    return NULL;
}

static apr_table_t *groups_for_user(request_rec *r, char *user, char *grpfile)
{
    WARN("groups_for_user(r: %p, user: '%s', grpfile: '%s')", r, NUL(user), NUL(grpfile));

    apr_pool_t *p = r->pool;
    ap_configfile_t *f;
    apr_table_t *grps = apr_table_make(p, 15);
    apr_pool_t *sp;
    char l[MAX_STRING_LEN];
    const char *group_name, *ll, *w;
    apr_status_t status;

    if ((status = ap_pcfg_openfile(&f, p, grpfile)) != APR_SUCCESS) {
         ap_log_rerror(APLOG_MARK, APLOG_ERR, status, r,
                       "Could not open group file: %s", grpfile);
        return NULL;
    }

    apr_pool_create(&sp, p);

    while (!(ap_cfg_getline(l, MAX_STRING_LEN, f))) {
        if ((l[0] == '#') || (!l[0])) {
            continue;
        }
        ll = l;
        apr_pool_clear(sp);

        group_name = ap_getword(sp, &ll, ':');

        while (ll[0]) {
            w = ap_getword_conf(sp, &ll);
            if (!strcmp(w, user)) {
                apr_table_setn(grps, apr_pstrdup(p, group_name), "in");
                break;
            }
        }
    }
    ap_cfg_closefile(f);
    apr_pool_destroy(sp);
    return grps;
}

/* These functions return 0 if client is OK, and proper error status
 * if not... either HTTP_UNAUTHORIZED, if we made a check, and it failed, or
 * HTTP_INTERNAL_SERVER_ERROR, if things are so totally confused that we
 * couldn't figure out how to tell if the client is authorized or not.
 *
 * If they return DECLINED, and all other modules also decline, that's
 * treated by the server core as a configuration error, logged and
 * reported as such.
 */

/* Determine user ID, and check if it really is that user, for HTTP
 * basic authentication...
 */
void LogFailedUser(request_rec *r, char *auth_pwfile);
int IsAccountLocked(request_rec *r, char *auth_pwfile);
unsigned int GetSleepTimeForFailedAuthInSec(request_rec *r, char *auth_pwfile);

static int authenticate_basic_user(request_rec *r)
{
    WARN("authenticate_basic_user(r: %p)", r);

    auth_config_rec *conf = ap_get_module_config(r->per_dir_config,
                                                 &auth_hard_module );
    const char *sent_pw;
    char *real_pw;
    apr_status_t invalid_pw;
    int res;

    if ((res = ap_get_basic_auth_pw(r, &sent_pw))) {
        return res;
    }

    if (!conf->auth_pwfile) {
        return DECLINED;
    }

    if (IsAccountLocked(r, conf->auth_pwfile))
    {
        return HTTP_UNAUTHORIZED;
    }

    if (!(real_pw = get_pw(r, r->user, conf->auth_pwfile))) {
        if (!(conf->auth_authoritative)) {
            return DECLINED;
        }
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "user %s not found: %s", r->user, r->uri);
        ap_note_basic_auth_failure(r);

        LogFailedUser(r, conf->auth_pwfile);

        apr_sleep(GetSleepTimeForFailedAuthInSec(r, conf->auth_pwfile) * APR_USEC_PER_SEC);


        return HTTP_UNAUTHORIZED;
    }
    invalid_pw = apr_password_validate(sent_pw, real_pw);
    if (invalid_pw != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "user %s: authentication failure for \"%s\": "
                      "Password Mismatch",
                      r->user, r->uri);
        ap_note_basic_auth_failure(r);

        LogFailedUser(r, conf->auth_pwfile);

        apr_sleep(GetSleepTimeForFailedAuthInSec(r, conf->auth_pwfile) * APR_USEC_PER_SEC);

        return HTTP_UNAUTHORIZED;
    }
    return OK;
}

/* Checking ID */

static int check_user_access(request_rec *r)
{
    WARN("check_user_access(r: %p)", r);

    auth_config_rec *conf = ap_get_module_config(r->per_dir_config,
                                                 &auth_hard_module );
    char *user = r->user;
    int m = r->method_number;
    int method_restricted = 0;
    register int x;
    const char *t, *w;
    apr_table_t *grpstatus;
    const apr_array_header_t *reqs_arr = ap_requires(r);
    require_line *reqs;

    /* BUG FIX: tadc, 11-Nov-1995.  If there is no "requires" directive, 
     * then any user will do.
     */
    if (!reqs_arr) {
        return OK;
    }
    reqs = (require_line *)reqs_arr->elts;

    if (conf->auth_grpfile) {
        grpstatus = groups_for_user(r, user, conf->auth_grpfile);
    }
    else {
        grpstatus = NULL;
    }

    for (x = 0; x < reqs_arr->nelts; x++) {

        if (!(reqs[x].method_mask & (AP_METHOD_BIT << m))) {
            continue;
        }

        method_restricted = 1;

        t = reqs[x].requirement;
        w = ap_getword_white(r->pool, &t);
        if (!strcmp(w, "valid-user")) {
            return OK;
        }
        if (!strcmp(w, "user")) {
            while (t[0]) {
                w = ap_getword_conf(r->pool, &t);
                if (!strcmp(user, w)) {
                    return OK;
                }
            }
        }
        else if (!strcmp(w, "group")) {
            if (!grpstatus) {
                return DECLINED;        /* DBM group?  Something else? */
            }

            while (t[0]) {
                w = ap_getword_conf(r->pool, &t);
                if (apr_table_get(grpstatus, w)) {
                    return OK;
                }
            }
        }
        else if (conf->auth_authoritative) {
            /* if we aren't authoritative, any require directive could be
             * valid even if we don't grok it.  However, if we are 
             * authoritative, we can warn the user they did something wrong.
             * That something could be a missing "AuthAuthoritative off", but
             * more likely is a typo in the require directive.
             */
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                          "access to %s failed, reason: unknown require "
                          "directive:\"%s\"", r->uri, reqs[x].requirement);
        }
    }

    if (!method_restricted) {
        return OK;
    }

    if (!(conf->auth_authoritative)) {
        return DECLINED;
    }

    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                  "access to %s failed, reason: user %s not allowed access",
                  r->uri, user);
        
    ap_note_basic_auth_failure(r);
    return HTTP_UNAUTHORIZED;
}

static void register_hooks(apr_pool_t *p)
{
    WARN("register_hooks(p: %p)", p);

    ap_hook_check_user_id(authenticate_basic_user,NULL,NULL,APR_HOOK_FIRST);
    ap_hook_auth_checker(check_user_access,NULL,NULL,APR_HOOK_FIRST);
}

module AP_MODULE_DECLARE_DATA auth_hard_module =
{
    STANDARD20_MODULE_STUFF,
    create_auth_dir_config,     /* dir config creater */
    NULL,                       /* dir merger --- default is to override */
    create_perserver_config,    /* server config */
    NULL,                       /* merge server config */
    auth_cmds,                  /* command apr_table_t */
    register_hooks              /* register hooks */
};
