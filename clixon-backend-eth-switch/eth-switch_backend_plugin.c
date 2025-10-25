/*
 *
  ***** BEGIN LICENSE BLOCK *****
 
  Copyright (C) 2021-2022 Olof Hagsand and Rubicon Communications, LLC(Netgate)
  Copyright (C) 2025 Albrecht Lohofener <albrechtloh@gmx.de>

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
#define JSON_BUF_SIZE BUFSIZ * 4

#define NAME "eth-switch-backend"

static int start(clixon_handle h)
{
	cxobj *xt = NULL;
	cbuf  *cb = NULL;
	cbuf  *cbret = NULL;
	int   retval = -1;
	yang_stmt *yspec;

	json_t *root = NULL;
	json_error_t error;
	FILE *fp = NULL;
	char cmd[128];
	size_t len;
	char *buf;

	clixon_log(h, LOG_INFO, "[%s]: start run", NAME);

	if ((cb = cbuf_new()) == NULL) {
		clixon_log(h, LOG_ERR, "[%s]: cbuf_new cb", NAME);
		goto done;
	}

	if ((cbret = cbuf_new()) == NULL){
        clixon_log(h, LOG_ERR, "[%s]: cbuf_new cbret", NAME);
        goto done;
    }

	/* Read existing Ethernet interfaces */
	buf = malloc(JSON_BUF_SIZE);
	if (!buf)
		goto done;

	snprintf(cmd, sizeof(cmd), "ip -j link show");
	fp = popen(cmd, "r");
	if (!fp)
		goto done;

	len = fread(buf, 1, JSON_BUF_SIZE, fp);
	if (len == 0 || ferror(fp))
		goto done;

	root = json_loadb(buf, len, 0, &error);
	if (!root) {
		clixon_log(h, LOG_ERR, "[%s]: Error parsing ip json \"%s\"", NAME, error.text);
		goto done;
	}

	/* Create XML */
	cprintf(cb, "<interfaces xmlns=\"http://openconfig.net/yang/interfaces\">");

	/* Loop over all interfaces and extract ethernet interface names */
	if (json_is_array(root)) {
		size_t arrlen = json_array_size(root);
		for (size_t i = 0; i < arrlen; i++) {
			json_t *entry = json_array_get(root, i);
			if (!json_is_object(entry))
				continue;
			json_t *lt = json_object_get(entry, "link_type");
			/* Consider only link_type == "ether" as Ethernet interfaces */
			if (!lt || !json_is_string(lt) || strcmp(json_string_value(lt), "ether") != 0)
				continue;
			json_t *jn = json_object_get(entry, "ifname");
			if (!jn || !json_is_string(jn))
				continue;
			
			const char *ifname = json_string_value(jn);

			clixon_log(h, LOG_INFO, "[%s]: adding ethernet interface %s", NAME, ifname);
			
			cprintf(cb, "  <interface>");
			cprintf(cb, "    <name>%s</name>", ifname);
			cprintf(cb, "    <config>");
			cprintf(cb, "      <name>%s</name>", ifname);
			cprintf(cb, "      <type xmlns:ianaift=\"urn:ietf:params:xml:ns:yang:iana-if-type\">ianaift:ethernetCsmacd</type>");
			cprintf(cb, "    </config>");
			cprintf(cb, "  </interface>");
			
		}
	} else {
		clixon_log(h, LOG_ERR, "[%s]: Expect a JSON array here", NAME);
		goto done;
	}

	cprintf(cb, "</interfaces>");

    /* get top-level yang spec (used by parser for binding) */
    yspec = clicon_dbspec_yang(h);

	if (clixon_xml_parse_string(cbuf_get(cb), YB_MODULE, yspec, &xt, NULL) < 0) {
		clixon_log(h, LOG_ERR, "[%s]: Error parsing initial configuration", NAME);
		goto done;
	}

	/* The parser returns a top-level tree — the datastore expects <config> top */
    xml_name_set(xt, "config");

	/* Merge the parsed default config into the temp DB (db is e.g. "tmp" or "startup"). 
       Use OP_MERGE to merge with existing data; use OP_REPLACE/OP_CREATE if you want different semantics. */
    if (xmldb_put(h, "candidate",  OP_MERGE, xt, clicon_username_get(h), cbret) < 1) {
		clixon_log(h, LOG_ERR, "[%s]: xmldb_put error (%s)", NAME, cbuf_get(cbret));
    	goto done;
	}

	if(candidate_commit(h, NULL, "candidate", 0, 0, cbret) < 0) {
		clixon_log(h, LOG_ERR, "[%s]: candidate_commit error (%s)", NAME, cbuf_get(cbret));
    	goto done;
	}

	retval = 0;
done:
	if (cbret) {
		cbuf_free(cbret);
	}
	if (cb) {
		cbuf_free(cb);
	}
	if (xt != NULL) {
		xml_free(xt);
	}
	return retval;
}

int reset(clixon_handle h, const char   *db)
{
 	clixon_log(h, LOG_INFO, "[%s]: reset run", NAME);

    return 0;
}

static int trans_begin(clixon_handle h, transaction_data td)
{
    clixon_log(h, LOG_INFO, "[%s]: trans_begin run", NAME);

    return 0;
}

char* get_interface_name_from_parent(cxobj *xml_node)
{
	cxobj *parent = xml_node;

	// go back in XML tree to interface node
	while(strcmp("interface", xml_name(parent)) != 0) {
		parent = xml_parent(parent);
	}

	return xml_find_body(parent, "name");
}

int systemf(const char *fmt, ...) {
	int ret = 0;
    char cmd[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(cmd, sizeof(cmd), fmt, args);
    va_end(args);

	char cmd2[2048];
	//snprintf(cmd2, 2048, "sudo %s", cmd);
	snprintf(cmd2, 2048, "%s", cmd);

	ret = system(cmd2);
	if (ret != 0) {
		clixon_err(OE_FATAL, 0, "Failed to execute system command: \"%s\"", cmd);
	}
	
    return ret;
}

int trans_commit(clixon_handle h, transaction_data td)
{
    int   retval = -1;
    cxobj *xmlconfig_target, *xmlconfig_src;
	cxobj **vec = NULL;
    int     i;
    size_t  len;
    
    clixon_log(h, LOG_INFO, "[%s]: trans_commit run", NAME);

    xmlconfig_target = transaction_target(td); /* wanted XML tree */
	xmlconfig_src = transaction_src(td); /* existing XML tree */

    //xml_print(stdout, xmlconfig);

	/* Removed entries */
	if (xpath_vec_flag(xmlconfig_src, NULL, "//interface/ethernet/switched-vlan/config/access-vlan", XML_FLAG_DEL| XML_FLAG_CHANGE, &vec, &len) < 0)
        goto done;
	for (i=0; i<len; i++) {
		char* value = xml_value(xml_body_get(vec[i]));
		char* interface_name = get_interface_name_from_parent(vec[i]);
		clixon_log(h, LOG_INFO, "[%s]: Remove access VLAN ID \"%s\" from interface \"%s\"", NAME, value, interface_name);

		if(systemf("bridge vlan del dev %s vid %s", interface_name, value) != 0)
			goto done;
	}

	if (xpath_vec_flag(xmlconfig_src, NULL, "//interface/ethernet/switched-vlan/config/trunk-vlans", XML_FLAG_DEL, &vec, &len) < 0)
        goto done;
	for (i=0; i<len; i++) {
		char* value = xml_value(xml_body_get(vec[i]));
		char* interface_name = get_interface_name_from_parent(vec[i]);
		clixon_log(h, LOG_INFO, "[%s]: Remove trunk VLAN ID \"%s\" from interface \"%s\"", NAME, value, interface_name);

		if(systemf("bridge vlan del dev %s vid %s", interface_name, value) != 0)
			goto done;
	}

	/* New entries*/
	if (xpath_vec_flag(xmlconfig_target, NULL, "//interface/ethernet/switched-vlan/config/access-vlan", XML_FLAG_ADD | XML_FLAG_CHANGE, &vec, &len) < 0)
        goto done;
	for (i=0; i<len; i++) {
		char* value = xml_value(xml_body_get(vec[i]));
		char* interface_name = get_interface_name_from_parent(vec[i]);
		clixon_log(h, LOG_INFO, "[%s]: Add access VLAN ID \"%s\" to interface \"%s\"", NAME, value, interface_name);

		if(systemf("bridge vlan add dev %s vid %s pvid untagged", interface_name, value) != 0)
			goto done;
	}

	if (xpath_vec_flag(xmlconfig_target, NULL, "//interface/ethernet/switched-vlan/config/trunk-vlans", XML_FLAG_ADD | XML_FLAG_CHANGE, &vec, &len) < 0)
        goto done;
	for (i=0; i<len; i++) {
		char* value = xml_value(xml_body_get(vec[i]));
		char* interface_name = get_interface_name_from_parent(vec[i]);
		clixon_log(h, LOG_INFO, "[%s]: Add trunk VLAN ID \"%s\" to interface \"%s\"", NAME, value, interface_name);

		if(systemf("bridge vlan add dev %s vid %s pvid", interface_name, value) != 0)
			goto done;
	}

	retval = 0;

 done:
	if(vec)
		free(vec);
    return retval;
}

static int system_only(clixon_handle h, cvec *nsc, char *xpath, cxobj *xtop)
{
    clixon_log(h, LOG_INFO, "[%s]: system_only run", NAME);

    return 0;
}

/* Verify obj is a string and then lowercase print it in fmt */
void cprint_from_json(cbuf *cb, const char *fmt, json_t *obj, const char *key)
{
	json_t *val;
	char *str;

	/* Support dotted paths like "stats64.rx.packets" */
	json_t *cur = obj;
	char *path = strdup(key);
	char *tok, *saveptr;

	if (!path) {
		val = NULL;
	} else {
		tok = strtok_r(path, ".", &saveptr);
		while (tok && cur) {
			if (!json_is_object(cur)) {
				cur = NULL;
				break;
			}
			cur = json_object_get(cur, tok);
			tok = strtok_r(NULL, ".", &saveptr);
		}
		free(path);
		val = cur;
	}
	if (!val || !(json_is_string(val) || json_is_integer(val)))
		return;

	char tmp[64];
	if (json_is_string(val)) {
		str = strdup(json_string_value(val));
	} else if (json_is_integer(val)) {
		json_int_t iv = json_integer_value(val);
		snprintf(tmp, sizeof(tmp), "%" PRId64, (int64_t)iv);
		str = strdup(tmp);
	} else {
		str = NULL;
	}
	if (!str)
		return;

	for (size_t i = 0; i < strlen(str); i++)
		str[i] = tolower(str[i]);
	cprintf(cb, fmt, str);
	free(str);
}

void cprint_ifdata(clixon_handle h, cbuf *cb, char *ifname)
{
	json_t *root = NULL, *json_obj;
	json_error_t error;
	FILE *fp = NULL;
	char cmd[128];
	size_t len;
	char *buf;

	buf = malloc(JSON_BUF_SIZE);
	if (!buf)
		goto err;

	snprintf(cmd, sizeof(cmd), "ip -j -d -s link show %s", ifname);
	fp = popen(cmd, "r");
	if (!fp)
		goto err;

	len = fread(buf, 1, JSON_BUF_SIZE, fp);
	if (len == 0 || ferror(fp))
		goto err;

	root = json_loadb(buf, len, 0, &error);
		if (!root) {
		clixon_log(h, LOG_ERR, "[%s]: Error parsing ip link show json \"%s\"", NAME, error.text);
		goto err;
	}
	if (!json_is_array(root)) {
		if (!json_is_object(root))
			goto err;
		json_obj = root;
	} else
		json_obj = json_array_get(root, 0);

	cprintf(cb,
		"<interface>\n"
		" <name>%s</name>\n", ifname);
	cprintf(cb,
		"  <state>\n");
    cprint_from_json(cb, 
		"   <oper-status>%s</oper-status>\n", json_obj, "operstate");
	cprintf(cb,
		"    <counters>\n");
	cprint_from_json(cb, "<in-octets>%s</in-octets>\n", json_obj, "stats64.rx.bytes");
    cprint_from_json(cb, "<in-pkts>%s</in-pkts>\n", json_obj, "stats64.rx.packets");
	cprint_from_json(cb, "<out-octets>%s</out-octets>\n", json_obj, "stats64.tx.bytes");
    cprint_from_json(cb, "<out-pkts>%s</out-pkts>\n", json_obj, "stats64.tx.packets");
	cprintf(cb,
		"    </counters>\n"
		"   </state>>\n"
		"  <ethernet xmlns=\"http://openconfig.net/yang/interfaces/ethernet\">\n"
		"    <state>>\n");
    cprint_from_json(cb, 
		"     <hw-mac-address>%s</hw-mac-address>\n", json_obj, "address");
    cprintf(cb,
		"    </state>>\n"
        "  </ethernet>>\n"
	    "</interface>");
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

	clixon_log(h, LOG_INFO, "[%s]: statedata run", NAME);

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
			cprint_ifdata(h, cb, xml_body(xvec[i]));
		cprintf(cb, "</interfaces>");
		if (clixon_xml_parse_string(cbuf_get(cb), YB_NONE, NULL, &xtop, NULL) < 0)
			goto done;
	}
	retval = 0;

    //xml_print(stdout, xtop);
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
	.ca_reset = reset,
    .ca_trans_begin = trans_begin,
    .ca_trans_commit = trans_commit,
    .ca_statedata = statedata,
    .ca_system_only = system_only
};

clixon_plugin_api *clixon_plugin_init(clixon_handle h) {
    clixon_log(h, LOG_INFO, "[%s]: plugin_init run", NAME);

    return &api;
}
