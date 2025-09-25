/*
 *
  ***** BEGIN LICENSE BLOCK *****
 
  Copyright (C) 2021-2022 Olof Hagsand and Rubicon Communications, LLC(Netgate)

  This file is part of CLIXON.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Alternatively, the contents of this file may be used under the terms of
  the GNU General Public License Version 3 or later (the "GPL"),
  in which case the provisions of the GPL are applicable instead
  of those above. If you wish to allow use of your version of this file only
  under the terms of the GPL, and not to allow others to
  use your version of this file under the terms of Apache License version 2, indicate
  your decision by deleting the provisions above and replace them with the 
  notice and other provisions required by the GPL. If you do not delete
  the provisions above, a recipient may use your version of this file under
  the terms of any one of the Apache License version 2 or the GPL.

  ***** END LICENSE BLOCK *****
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/time.h>

/* clicon */
#include <cligen/cligen.h>

/* Clicon library functions. */
#include <clixon/clixon.h>

/* These include signatures for plugin and transaction callbacks. */
#include <clixon/clixon_backend.h> 
#include <clixon/clixon_log.h>

#define NAME "eth-switch-backend"

static int trans_begin(clixon_handle h, transaction_data td)
{
    int   retval = -1;
    clixon_log(h, LOG_INFO, "[%s]: trans_begin run", NAME);

done:
    return retval;
}

int trans_commit(clixon_handle h, transaction_data td)
{
    int   retval = -1;
    cxobj *xmlconfig;
    cvec *nsc;
    
    clixon_log(h, LOG_INFO, "[%s]: trans_commit run", NAME);

    nsc = clicon_nsctx_global_get(h);
    if (nsc == NULL) {
        /* crash protection */
        clixon_log(h, LOG_ERR, "[%s]: bad global namespace context", NAME);
        goto done;
    }

    xmlconfig = transaction_target(td);
    if (xmlconfig == NULL) {
        /* crash protection */
        clixon_log(h, LOG_ERR, "[%s]: bad target DB pointer", NAME);
        goto done;
    }

    xml_print(stdout, xmlconfig);

    retval = 0;
 done:
    return retval;
}

static int system_only(clixon_handle h, cvec *nsc, char *xpath, cxobj *xtop)
{
    int   retval = -1;
    clixon_log(h, LOG_INFO, "[%s]: system_only run", NAME);

done:
    return retval;
}

static int statedata(clixon_handle h, cvec *nsc, char *xpath, cxobj *xtop)
{
    int   retval = -1;

    clixon_log(h, LOG_INFO, "[%s]: statedata run", NAME);

    if (nsc == NULL) {
        /* crash protection */
        clixon_log(h, LOG_ERR, "[%s]: bad global namespace context", NAME);
        goto done;
    }

    if (xtop == NULL) {
        /* crash protection */
        clixon_log(h, LOG_ERR, "[%s]: bad target DB pointer", NAME);
        goto done;
    }

    if (xpath == NULL) {
        /* crash protection */
        clixon_log(h, LOG_ERR, "[%s]: bad xpath pointer", NAME);
        goto done;
    }

    if (clixon_xml_parse_string("<store xmlns=\"urn:example:std\"><keys><key><name>a</name></key></keys></store>",
                               YB_NONE, 0, &xtop, 0) < 0) {
        clixon_log(h, LOG_ERR, "[%s]: bad XML", NAME);
        goto done;
    }

    xml_print(stdout, xtop);

    retval = 0;
 done:
    return retval;
}

/* Forward declaration */
clixon_plugin_api *clixon_plugin_init(clixon_handle h);

static clixon_plugin_api api = {
    NAME,
    clixon_plugin_init,
    .ca_trans_begin = trans_begin,
    .ca_trans_commit = trans_commit,
    .ca_statedata = statedata,
    .ca_system_only = system_only
};

clixon_plugin_api *clixon_plugin_init(clixon_handle h) {
    clixon_log(h, LOG_INFO, "[%s]: plugin_init run", NAME);
    return &api;
}
