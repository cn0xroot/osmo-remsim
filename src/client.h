#pragma once

#include <osmocom/core/fsm.h>
#include <osmocom/abis/ipa.h>
#include <osmocom/rspro/RsproPDU.h>

#include "rspro_util.h"
#include "rspro_client_fsm.h"
#include "slotmap.h"
#include "debug.h"

/* main.c */

struct bankd_client {
	/* connection to the remsim-server (control) */
	struct rspro_server_conn srv_conn;
	/* connection to the remsim-bankd (data) */
	struct rspro_server_conn bankd_conn;

	/* remote component ID */
	struct app_comp_id peer_comp_id;

	struct bank_slot bankd_slot;
#if 0
	/* connection to the remsim-bankd */
	char *bankd_host;
	uint16_t bankd_port;
	struct ipa_client_conn *bankd_conn;
	struct osmo_fsm_inst *bankd_fi;
#endif
};
