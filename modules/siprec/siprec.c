/*
 * Copyright (C) 2017 OpenSIPS Project
 *
 * This file is part of opensips, a free SIP server.
 *
 * opensips is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * opensips is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 * History:
 * ---------
 *  2017-06-20  created (razvanc)
 */

#include "../../mem/shm_mem.h"
#include "../../sr_module.h"
#include "../../mod_fix.h"
#include "../../dprint.h"
#include "../../ut.h"

#include "siprec_sess.h"
#include "siprec_logic.h"

static int mod_init(void);
static int child_init(int);
static void mod_destroy(void);

static int siprec_start_rec(struct sip_msg *msg, char *_srs, char *_grp,
		char *_cA, char *_cB, char *_rtp, char *_m_ip);
static int free_fixup_siprec_rec(void **param, int param_no);
static int free_free_fixup_siprec_rec(void **param, int param_no);

/* modules dependencies */
static dep_export_t deps = {
	{ /* OpenSIPS module dependencies */
		{ MOD_TYPE_DEFAULT, "tm", DEP_ABORT },
		{ MOD_TYPE_DEFAULT, "dialog", DEP_ABORT },
		{ MOD_TYPE_DEFAULT, "b2b_entities", DEP_ABORT },
		{ MOD_TYPE_DEFAULT, "rtpproxy", DEP_ABORT },
		{ MOD_TYPE_NULL, NULL, 0 },
	},
	{ /* modparam dependencies */
		{ "db_url", get_deps_sqldb_url },
		{ NULL, NULL },
	},
};

/* exported commands */
static cmd_export_t cmds[] = {
	{"siprec_start_recording",(cmd_function)siprec_start_rec, 1,
		free_fixup_siprec_rec, free_free_fixup_siprec_rec, REQUEST_ROUTE },
	{"siprec_start_recording",(cmd_function)siprec_start_rec, 2,
		free_fixup_siprec_rec, free_free_fixup_siprec_rec, REQUEST_ROUTE },
	{"siprec_start_recording",(cmd_function)siprec_start_rec, 3,
		free_fixup_siprec_rec, free_free_fixup_siprec_rec, REQUEST_ROUTE },
	{"siprec_start_recording",(cmd_function)siprec_start_rec, 4,
		free_fixup_siprec_rec, free_free_fixup_siprec_rec, REQUEST_ROUTE },
	{"siprec_start_recording",(cmd_function)siprec_start_rec, 5,
		free_fixup_siprec_rec, free_free_fixup_siprec_rec, REQUEST_ROUTE },
	{"siprec_start_recording",(cmd_function)siprec_start_rec, 6,
		free_fixup_siprec_rec, free_free_fixup_siprec_rec, REQUEST_ROUTE },
	{0, 0, 0, 0, 0, 0}
};

/* exported parameters */
static param_export_t params[] = {
	{"media_port_min",		INT_PARAM, &siprec_port_min },
	{"media_port_max",		INT_PARAM, &siprec_port_max },
	{"skip_failover_codes",	STR_PARAM, &skip_failover_codes.s },
	{0, 0, 0}
};

/* module exports */
struct module_exports exports = {
	"siprec",						/* module name */
	MOD_TYPE_DEFAULT,				/* class of this module */
	MODULE_VERSION,
	DEFAULT_DLFLAGS,				/* dlopen flags */
	&deps,						    /* OpenSIPS module dependencies */
	cmds,							/* exported functions */
	0,								/* exported async functions */
	params,							/* exported parameters */
	0,								/* exported statistics */
	0,								/* exported MI functions */
	0,								/* exported pseudo-variables */
	0,								/* extra processes */
	0,								/* extra transformations */
	mod_init,						/* module initialization function */
	(response_function) 0,			/* response handling function */
	(destroy_function)mod_destroy,	/* destroy function */
	child_init						/* per-child init function */
};

/**
 * init module function
 */
static int mod_init(void)
{
	LM_DBG("initializing siprec module ...\n");

	if (srs_init() < 0) {
		LM_ERR("cannot initialize srs structures!\n");
		return -1;
	}

	if (src_init() < 0) {
		LM_ERR("cannot initialize src structures!\n");
		return -1;
	}

	if (load_dlg_api(&srec_dlg) != 0) {
		LM_ERR("dialog module not loaded! Cannot use siprec module\n");
		return -1;
	}

	if (load_tm_api(&srec_tm) != 0) {
		LM_ERR("tm module not loaded! Cannot use siprec module\n");
		return -1;
	}

	if (load_b2b_api(&srec_b2b) != 0) {
		LM_ERR("b2b_entities module not loaded! Cannot use siprec module\n");
		return -1;
	}

	if (load_rtpproxy_api(&srec_rtp) != 0) {
		LM_ERR("rtpproxy module not loaded! Cannot use siprec module\n");
		return -1;
	}

	if (srec_dlg.register_dlgcb(NULL, DLGCB_LOADED, srec_loaded_callback,
			NULL, NULL) < 0)
		LM_WARN("cannot register callback for loaded dialogs - will not be "
				"able to terminate siprec sessions after a restart!\n");

	return 0;
}

/*
 * function called when a child process starts
 */
static int child_init(int rank)
{
	return 0;
}

/*
 * function called after OpenSIPS has been stopped to cleanup resources
 */
static void mod_destroy(void)
{
}

/*
 * fixup siprec function
 */
static int free_fixup_siprec_rec(void **param, int param_no)
{
	if (param_no > 0 && param_no < 7)
		return fixup_spve(param);
	LM_ERR("Unsupported parameter %d\n", param_no);
	return E_CFG;
}

static int free_free_fixup_siprec_rec(void **param, int param_no)
{
	if (param_no > 0 && param_no < 3)
		return fixup_free_spve(param);
	LM_ERR("Unsupported parameter %d\n", param_no);
	return E_CFG;
}

/*
 * function that simply prints the parameters passed
 */
static int siprec_start_rec(struct sip_msg *msg, char *_srs, char *_grp,
		char *_cA, char *_cB, char *_rtp, char *_m_ip)
{
	int ret;
	str srs, rtp, m_ip, group, tmp_str, *aor, *display, *xml_val;
	struct src_sess *ss;
	struct dlg_cell *dlg;

	if (!_srs) {
		LM_ERR("No siprec SRS uri specified!\n");
		return -1;
	}
	if (_rtp && fixup_get_svalue(msg, (gparam_p)_rtp, &rtp) < 0) {
		LM_ERR("cannot fetch media rtpproxy server!\n");
		return -1;
	}

	if (fixup_get_svalue(msg, (gparam_p)_srs, &srs) < 0) {
		LM_ERR("cannot fetch set!\n");
		return -1;
	}
	if (_grp && fixup_get_svalue(msg, (gparam_p)_grp, &group) < 0) {
		LM_ERR("cannot fetch group for this session!\n");
		return -1;
	}

	if (_m_ip && fixup_get_svalue(msg, (gparam_p)_m_ip, &m_ip) < 0) {
		LM_ERR("cannot fetch media IP!\n");
		return -1;
	}

	/* create the dialog, if does not exist yet */
	dlg = srec_dlg.get_dlg();
	if (!dlg) {
		if (srec_dlg.create_dlg(msg, 0) < 0) {
			LM_ERR("cannot create dialog!\n");
			return -2;
		}
		dlg = srec_dlg.get_dlg();
	}

	/* XXX: if there is a forced send socket in the message, use it
	 * this is the only way to provide a different socket for SRS, but
	 * we might need to take a different approach */
	/* check if the current dialog has a siprec session ongoing */
	if (!(ss = src_new_session(&srs, (_rtp ? &rtp : NULL), (_m_ip ? &m_ip : NULL),
				(_grp ? &group : NULL), msg->force_send_socket))) {
		LM_ERR("cannot create siprec session!\n");
		return -2;
	}

	/* we ref the dialog to make sure it does not dissapear until we receive
	 * the reply from the SRS */
	srec_dlg.ref_dlg(dlg, 1);
	ss->dlg = dlg;

	ret = -2;

	/* caller info */
	if (_cA) {
		if (fixup_get_svalue(msg, (gparam_p)_cA, &tmp_str) < 0) {
			LM_ERR("cannot fetch caller information!\n");
			goto session_cleanup;
		}
		xml_val = &tmp_str;
		display = aor = NULL;
	} else {
		if (parse_from_header(msg) < 0) {
			LM_ERR("cannot parse from header!\n");
			goto session_cleanup;
		}
		aor = &get_from(msg)->uri;
		display = (get_from(msg)->display.s ? &get_from(msg)->display : NULL);
		xml_val = NULL;
	}

	if (src_add_participant(ss, aor, display, xml_val, NULL) < 0) {
		LM_ERR("cannot add caller participant!\n");
		goto session_cleanup;
	}
	if (srs_fill_sdp_stream(msg, ss, &ss->participants[0], 0) < 0) {
		LM_ERR("cannot add SDP for caller!\n");
		goto session_cleanup;
	}
	/* caller info */
	if (_cB) {
		if (fixup_get_svalue(msg, (gparam_p)_cB, &tmp_str) < 0) {
			LM_ERR("cannot fetch callee information!\n");
			goto session_cleanup;
		}
		xml_val = &tmp_str;
	} else {
		if ((!msg->to && parse_headers(msg, HDR_TO_F, 0) < 0) || !msg->to) {
			LM_ERR("inexisting or invalid to header!\n");
			goto session_cleanup;
		}
		aor = &get_to(msg)->uri;
		display = (get_to(msg)->display.s ? &get_to(msg)->display : NULL);
		xml_val = NULL;
	}

	if (src_add_participant(ss, aor, display, xml_val, NULL) < 0) {
		LM_ERR("cannot add callee pariticipant!\n");
		goto session_cleanup;
	}

	SIPREC_REF_UNSAFE(ss);
	if (srec_tm.register_tmcb(msg, 0, TMCB_RESPONSE_OUT, tm_start_recording,
			ss, src_unref_session) <= 0) {
		LM_ERR("cannot register tm callbacks\n");
		SIPREC_UNREF_UNSAFE(ss);
		goto session_cleanup;
	}
	return 1;

session_cleanup:
	src_free_session(ss);
	return ret;
}
