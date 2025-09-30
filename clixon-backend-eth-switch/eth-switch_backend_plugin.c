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
#include <ctype.h>
#include <ifaddrs.h>
#include <net/if.h>

/* clicon */
#include <cligen/cligen.h>

/* Clicon library functions. */
#include <clixon/clixon.h>

/* These include signatures for plugin and transaction callbacks. */
#include <clixon/clixon_backend.h> 
#include <clixon/clixon_log.h>

/* JSON handling */
#include <jansson.h>


#define NAME "eth-switch-backend"

static int start(clixon_handle h)
{
	cxobj *xtop = NULL;
	cbuf  *cb = NULL;
	cvec  *nsc1 = NULL;
	cxobj *xt = NULL;
	int   retval = -1;

	clixon_log(h, LOG_INFO, "[%s]: start run", NAME);

	if ((cb = cbuf_new()) == NULL) {
		clixon_log(h, LOG_ERR, "[%s]: cbuf_new", NAME);
		goto done;
	}

	/* Get running datastore root */
	/* ToDo: Not working*/
	if (xmldb_get(h, "running", NULL, "/", &xtop) < 0)
		goto done;

	cprintf(cb, "<config>\n"
   "<interfaces xmlns=\"http://openconfig.net/yang/interfaces\">\n"
      "<interface>\n"
         "<name>lan1</name>\n"
         "<config xmlns:ianaift=\"urn:ietf:params:xml:ns:yang:iana-if-type\">\n"
            "<name>lan1</name>\n"
            "<type>ianaift:ethernetCsmacd</type>\n"
            "<loopback-mode>NONE</loopback-mode>\n"
            "<enabled>true</enabled>\n"
            "<tpid xmlns=\"http://openconfig.net/yang/vlan\">oc-vlan-types:TPID_0X8100</tpid>\n"
         "</config>\n"
         "<hold-time>\n"
            "<config>\n"
               "<up>0</up>\n"
               "<down>0</down>\n"
            "</config>\n"
         "</hold-time>\n"
         "<penalty-based-aied>\n"
            "<config>\n"
               "<max-suppress-time>0</max-suppress-time>\n"
               "<decay-half-life>0</decay-half-life>\n"
               "<suppress-threshold>0</suppress-threshold>\n"
               "<reuse-threshold>0</reuse-threshold>\n"
               "<flap-penalty>0</flap-penalty>\n"
            "</config>\n"
         "</penalty-based-aied>\n"
         "<ethernet xmlns=\"http://openconfig.net/yang/interfaces/ethernet\">\n"
            "<config>\n"
               "<enable-flow-control>false</enable-flow-control>\n"
               "<auto-negotiate>true</auto-negotiate>\n"
               "<standalone-link-training>false</standalone-link-training>\n"
            "</config>\n"
         "</ethernet>\n"
      "</interface>\n"
   "</interfaces>\n"
"</config>");

	if (clixon_xml_parse_string(cbuf_get(cb), YB_NONE, NULL, &xtop, NULL) < 0) {
		clixon_log(h, LOG_ERR, "[%s]: Error parsing initial configuration", NAME);
		goto done;
	}

    retval = 0;
done:
    return retval;
}

static int trans_begin(clixon_handle h, transaction_data td)
{
    int   retval = -1;
    clixon_log(h, LOG_INFO, "[%s]: trans_begin run", NAME);

    retval = 0;
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

/* Verify obj is a string and then lowercase print it in fmt */
void cprint_xml_string(cbuf *cb, const char *fmt, json_t *obj, const char *key)
{
	json_t *val;
	char *str;

	val = json_object_get(obj, key);
	if (!val || !json_is_string(val))
		return;

	str = strdup(json_string_value(val));
	if (!str)
		return;

	for (size_t i = 0; i < strlen(str); i++)
		str[i] = tolower(str[i]);
	cprintf(cb, fmt, str);
	free(str);
}

void cprint_ifdata(cbuf *cb, char *ifname)
{
	json_t *root = NULL, *obj;
	json_error_t error;
	FILE *fp = NULL;
	char cmd[128];
	size_t len;
	char *buf;

	buf = malloc(BUFSIZ);
	if (!buf)
		goto err;

	snprintf(cmd, sizeof(cmd), "ip -j -p link show %s", ifname);
	fp = popen(cmd, "r");
	if (!fp)
		goto err;

	len = fread(buf, 1, 4096, fp);
	if (len == 0 || ferror(fp))
		goto err;

	root = json_loadb(buf, len, 0, &error);
	if (!root)
		goto err;
	if (!json_is_array(root)) {
		if (!json_is_object(root))
			goto err;
		obj = root;
	} else
		obj = json_array_get(root, 0);

	cprintf(cb,
		"<interface>\n"
        "<config xmlns:ianaift=\"urn:ietf:params:xml:ns:yang:iana-if-type\">\n"
		"  <name>%s</name>\n"
		"  <type>ex:eth</type>\n", ifname);
    cprintf(cb, "</config>");
	cprintf(cb,"<ethernet xmlns=\"http://openconfig.net/yang/interfaces/ethernet\">\n"
            "<state>>\n");
    cprint_xml_string(cb, "  <hw-mac-address>%s</hw-mac-address>\n", obj, "address");
    cprintf(cb,"</state>>\n"
         "</ethernet>>\n");
	cprintf(cb, "</interface>");
err:
	if (root)
		json_decref(root);
	if (fp)
		pclose(fp);
	if (buf)
		free(buf);
}

static int statedata(clixon_handle h, cvec *nsc, char *xpath, cxobj *xtop)
{
	cxobj    **xvec = NULL;
	size_t     xlen = 0;
	cbuf      *cb = NULL;
	cxobj     *xt = NULL;
	cvec      *nsc1 = NULL;
	int        retval = -1;

	if ((cb = cbuf_new()) == NULL) {
        clixon_log(h, LOG_ERR, "[%s]: cbuf_new faild", NAME);
		goto done;
	}

	if ((nsc1 = xml_nsctx_init(NULL, "http://openconfig.net/yang/interfaces")) == NULL)
		goto done;
	if (xmldb_get0(h, "running", YB_MODULE, nsc1, "/interfaces/interface/config", 1, 0, &xt, NULL, NULL) < 0)
		goto done;
	if (xpath_vec(xt, nsc1, "/interfaces/interface/name", &xvec, &xlen) < 0)
		goto done;
	if (xlen) {
		cprintf(cb, "<interfaces xmlns=\"http://openconfig.net/yang/interfaces\">");
		for (size_t i = 0; i < xlen; i++)
			cprint_ifdata(cb, xml_body(xvec[i]));
		cprintf(cb, "</interfaces>");
		if (clixon_xml_parse_string(cbuf_get(cb), YB_NONE, NULL, &xtop, NULL) < 0)
			goto done;
	}
	retval = 0;

    xml_print(stdout, xtop);
done:
	if (nsc1)
		xml_nsctx_free(nsc1);
	if (xt)
		xml_free(xt);
	if (cb)
		cbuf_free(cb);
	if (xvec)
		free(xvec);
	return retval;
}

/* Forward declaration */
clixon_plugin_api *clixon_plugin_init(clixon_handle h);

static clixon_plugin_api api = {
    .ca_name = NAME,
    .ca_init = clixon_plugin_init,
	.ca_start = start,
    .ca_trans_begin = trans_begin,
    .ca_trans_commit = trans_commit,
    .ca_statedata = statedata,
    .ca_system_only = system_only
};

clixon_plugin_api *clixon_plugin_init(clixon_handle h) {
    clixon_log(h, LOG_INFO, "[%s]: plugin_init run", NAME);

    return &api;
}
