/* (C) 2018-2019 by Harald Welte <laforge@gnumonks.org>
 *
 * All Rights Reserved
 *
 * SPDX-License-Identifier: GPL-2.0+
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>

#include <wintypes.h>
#include <winscard.h>
#include <pcsclite.h>

#include <sys/socket.h>
#include <netdb.h>

#include <osmocom/core/socket.h>
#include <osmocom/core/linuxlist.h>
#include <osmocom/core/logging.h>
#include <osmocom/core/application.h>

#include <osmocom/gsm/ipa.h>
#include <osmocom/gsm/protocol/ipaccess.h>

#include <asn_application.h>
#include <osmocom/rspro/RsproPDU.h>

#include "bankd.h"
#include "rspro_client_fsm.h"
#include "debug.h"
#include "rspro_util.h"

/* signal indicates to worker thread that its map has been deleted */
#define SIGMAPDEL	SIGRTMIN+1

static void handle_sig_usr1(int sig);
static void handle_sig_mapdel(int sig);

__thread void *talloc_asn1_ctx;
struct bankd *g_bankd;
static void *g_tall_ctx;

static void *worker_main(void *arg);

/***********************************************************************
* bankd core / main thread
***********************************************************************/

int asn_debug;

static void bankd_init(struct bankd *bankd)
{
	g_tall_ctx = talloc_named_const(NULL, 0, "global");
	osmo_init_logging2(g_tall_ctx, &log_info);

	asn_debug = 0;

	/* intialize members of 'bankd' */
	bankd->slotmaps = slotmap_init(bankd);
	INIT_LLIST_HEAD(&bankd->workers);
	pthread_mutex_init(&bankd->workers_mutex, NULL);

	/* set some defaults, overridden by commandline/config */
	bankd->cfg.bank_id = 1;
	bankd->cfg.num_slots = 8;

	bankd->comp_id.type = ComponentType_remsimBankd;
	OSMO_STRLCPY_ARRAY(bankd->comp_id.name, "fixme-name");
	OSMO_STRLCPY_ARRAY(bankd->comp_id.software, "remsim-bankd");
	OSMO_STRLCPY_ARRAY(bankd->comp_id.sw_version, PACKAGE_VERSION);
	/* FIXME: other members of app_comp_id */

	/* Np lock or mutex required for the pcsc_slot_names list, as this is only
	 * read once during bankd initialization, when the worker threads haven't
	 * started yet */
	INIT_LLIST_HEAD(&bankd->pcsc_slot_names);
	OSMO_ASSERT(bankd_pcsc_read_slotnames(bankd, "bankd_pcsc_slots.csv") == 0);
}

/* create + start a new bankd_worker thread */
static struct bankd_worker *bankd_create_worker(struct bankd *bankd, unsigned int i)
{
	struct bankd_worker *worker;
	int rc;

	worker = talloc_zero(bankd, struct bankd_worker);
	if (!worker)
		return NULL;

	worker->bankd = bankd;
	worker->num = i;

	/* in the initial state, the worker has no client.fd, bank_slot or pcsc handle yet */

	rc = pthread_create(&worker->thread, NULL, worker_main, worker);
	if (rc != 0) {
		talloc_free(worker);
		return NULL;
	}

	pthread_mutex_lock(&bankd->workers_mutex);
	llist_add_tail(&worker->list, &bankd->workers);
	pthread_mutex_unlock(&bankd->workers_mutex);

	return worker;
}

static bool terminate = false;

static void rspro2bank_slot(struct bank_slot *out, const BankSlot_t *in)
{
	out->bank_id = in->bankId;
	out->slot_nr = in->slotNr;
}

static void rspro2client_slot(struct client_slot *out, const ClientSlot_t *in)
{
	out->client_id = in->clientId;
	out->slot_nr = in->slotNr;
}

/* handle incoming messages from server */
static int bankd_srvc_handle_rx(struct rspro_server_conn *srvc, const RsproPDU_t *pdu)
{
	const CreateMappingReq_t *creq = NULL;
	const RemoveMappingReq_t *rreq = NULL;
	struct slot_mapping *map;
	struct bank_slot bs;
	struct client_slot cs;
	RsproPDU_t *resp;

	LOGPFSM(srvc->fi, "Rx RSPRO %s\n", rspro_msgt_name(pdu));

	switch (pdu->msg.present) {
	case RsproPDUchoice_PR_connectBankRes:
		/* Store 'identity' of server in srvc->peer_comp_id */
		rspro_comp_id_retrieve(&srvc->peer_comp_id, &pdu->msg.choice.connectBankRes.identity);
		osmo_fsm_inst_dispatch(srvc->fi, SRVC_E_CLIENT_CONN_RES, (void *) pdu);
		break;
	case RsproPDUchoice_PR_createMappingReq:
		creq = &pdu->msg.choice.createMappingReq;
		if (creq->bank.bankId != g_bankd->cfg.bank_id)
			resp = rspro_gen_CreateMappingRes(ResultCode_illegalBankId);
		else if (creq->bank.slotNr >= g_bankd->cfg.num_slots)
			resp = rspro_gen_CreateMappingRes(ResultCode_illegalSlotId);
		else {
			rspro2bank_slot(&bs, &creq->bank);
			rspro2client_slot(&cs, &creq->client);
			/* Add a new mapping */
			map = slotmap_add(g_bankd->slotmaps, &bs, &cs);
			if (!map)
				resp = rspro_gen_CreateMappingRes(ResultCode_illegalSlotId);
			else
				resp = rspro_gen_CreateMappingRes(ResultCode_ok);
		}
		server_conn_send_rspro(srvc, resp);
		break;
	case RsproPDUchoice_PR_removeMappingReq:
		rreq = &pdu->msg.choice.removeMappingReq;
		if (rreq->bank.bankId != g_bankd->cfg.bank_id)
			resp = rspro_gen_RemoveMappingRes(ResultCode_illegalBankId);
		else if (rreq->bank.slotNr >= g_bankd->cfg.num_slots)
			resp = rspro_gen_RemoveMappingRes(ResultCode_illegalSlotId);
		else {
			rspro2bank_slot(&bs, &rreq->bank);
			/* Remove a mapping */
			map = slotmap_by_bank(g_bankd->slotmaps, &bs);
			if (!map)
				resp = rspro_gen_RemoveMappingRes(ResultCode_unknownSlotmap);
			else {
				slotmap_del(g_bankd->slotmaps, map);
				resp = rspro_gen_RemoveMappingRes(ResultCode_ok);

				/* kill/reset the respective worker, if any! */
				struct bankd_worker *worker;
				pthread_mutex_lock(&g_bankd->workers_mutex);
				llist_for_each_entry(worker, &g_bankd->workers, list) {
					if (bs.bank_id == worker->slot.bank_id &&
					    bs.slot_nr == worker->slot.slot_nr) {
						pthread_kill(worker->thread, SIGMAPDEL);
						break;
					}
				}
				pthread_mutex_unlock(&g_bankd->workers_mutex);
			}
		}
		server_conn_send_rspro(srvc, resp);
		break;
	default:
		fprintf(stderr, "Unknown/Unsupported RSPRO PDU type: %u\n", pdu->msg.present);
		return -1;
	}

	return 0;
}


void handle_options(int argc, char **argv)
{
}

int main(int argc, char **argv)
{
	struct rspro_server_conn *srvc;
	int i, rc;

	g_bankd = talloc_zero(NULL, struct bankd);
	OSMO_ASSERT(g_bankd);

	bankd_init(g_bankd);

	srvc = &g_bankd->srvc;
	srvc->server_host = "localhost";
	srvc->server_port = 9998;
	srvc->handle_rx = bankd_srvc_handle_rx;
	srvc->own_comp_id.type = ComponentType_remsimBankd;
	OSMO_STRLCPY_ARRAY(srvc->own_comp_id.name, "fixme-name");
	OSMO_STRLCPY_ARRAY(srvc->own_comp_id.software, "remsim-bankd");
	OSMO_STRLCPY_ARRAY(srvc->own_comp_id.sw_version, PACKAGE_VERSION);

	handle_options(argc, argv);

	g_bankd->main = pthread_self();
	signal(SIGMAPDEL, handle_sig_mapdel);
	signal(SIGUSR1, handle_sig_usr1);

	/* Connection towards remsim-server */
	rc = server_conn_fsm_alloc(g_bankd, srvc);
	if (rc < 0) {
		fprintf(stderr, "Unable to create Server conn FSM: %s\n", strerror(errno));
		exit(1);
	}

	/* create listening socket for inbound client connections */
	rc = osmo_sock_init(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 9999, OSMO_SOCK_F_BIND);
	if (rc < 0)
		exit(1);
	g_bankd->accept_fd = rc;

	/* create worker threads: One per reader/slot! */
	for (i = 0; i < g_bankd->cfg.num_slots; i++) {
		struct bankd_worker *w;
		w = bankd_create_worker(g_bankd, i);
		if (!w)
			exit(21);
	}

	while (1) {
		if (terminate)
			break;
		osmo_select_main(0);
	}

	talloc_free(g_bankd);
	exit(0);
}



/***********************************************************************
 * bankd worker thread
 ***********************************************************************/

static __thread struct bankd_worker *g_worker;

struct value_string worker_state_names[] = {
	{ BW_ST_INIT, 			"INIT" },
	{ BW_ST_ACCEPTING,		"ACCEPTING" },
	{ BW_ST_CONN_WAIT_ID,		"CONN_WAIT_ID" },
	{ BW_ST_CONN_CLIENT,		"CONN_CLIENT" },
	{ BW_ST_CONN_CLIENT_WAIT_MAP,	"CONN_CLIENT_WAIT_MAP" },
	{ BW_ST_CONN_CLIENT_MAPPED,	"CONN_CLIENT_MAPPED" },
	{ BW_ST_CONN_CLIENT_MAPPED_CARD,"CONN_CLIENT_MAPPED_CARD" },
	{ BW_ST_CONN_CLIENT_UNMAPPED,	"CONN_CLIENT_UNMAPPED" },
	{ 0, NULL }
};

#define LOGW(w, fmt, args...) \
	printf("[%03u %s] %s:%u " fmt, (w)->num, get_value_string(worker_state_names, (w)->state), \
		__FILE__, __LINE__, ## args)

#define PCSC_ERROR(w, rv, text) \
if (rv != SCARD_S_SUCCESS) { \
	LOGW((w), text ": %s (0x%lX)\n", pcsc_stringify_error(rv), rv); \
	goto end; \
} else { \
        LOGW((w), ": OK\n"); \
}

static void worker_set_state(struct bankd_worker *worker, enum bankd_worker_state new_state)
{
	LOGW(worker, "Changing state to %s\n", get_value_string(worker_state_names, new_state));
	worker->state = new_state;
	worker->timeout = 0;
}

static void worker_set_state_timeout(struct bankd_worker *worker, enum bankd_worker_state new_state,
				     unsigned int timeout_secs)
{
	LOGW(worker, "Changing state to %s (timeout=%u)\n",
		get_value_string(worker_state_names, new_state), timeout_secs);
	worker->state = new_state;
	worker->timeout = timeout_secs;
}

/* signal handler for receiving SIGMAPDEL from main thread */
static void handle_sig_mapdel(int sig)
{
	LOGW(g_worker, "SIGMAPDEL received: Main thread informs us our map is gone\n");
	OSMO_ASSERT(sig == SIGMAPDEL);
	worker_set_state(g_worker, BW_ST_CONN_CLIENT_UNMAPPED);
}

static void handle_sig_usr1(int sig)
{
	OSMO_ASSERT(sig == SIGUSR1);

	if (pthread_equal(g_bankd->main, pthread_self())) {
		struct bankd_worker *worker;
		/* main thread */
		fprintf(stderr, "=== Talloc Report of main thread:\n");
		talloc_report(g_tall_ctx, stderr);

		/* iterate over worker threads and ask them to dump their talloc state */
		pthread_mutex_lock(&g_bankd->workers_mutex);
		llist_for_each_entry(worker, &g_bankd->workers, list) {
			pthread_kill(worker->thread, SIGUSR1);
		}
		pthread_mutex_unlock(&g_bankd->workers_mutex);
	} else {
		/* worker thread */
		fprintf(stderr, "=== Talloc Report of %s\n", g_worker->name);
		talloc_report(g_worker->tall_ctx, stderr);
	}
}

static void worker_cleanup(void *arg)
{
	struct bankd_worker *worker = (struct bankd_worker *) arg;
	struct bankd *bankd = worker->bankd;

	/* FIXME: should we still do this? in the thread ?!? */
	pthread_mutex_lock(&bankd->workers_mutex);
	llist_del(&worker->list);
	talloc_free(worker);	/* FIXME: is this safe? */
	pthread_mutex_unlock(&bankd->workers_mutex);
}


static int worker_open_card(struct bankd_worker *worker)
{
	long rc;

	OSMO_ASSERT(worker->state == BW_ST_CONN_CLIENT_MAPPED);

	if (!worker->reader.name) {
		/* resolve PC/SC reader name from slot_id -> name map */
		worker->reader.name = bankd_pcsc_get_slot_name(worker->bankd, &worker->slot);
		if (!worker->reader.name) {
			LOGW(worker, "No PC/SC reader name configured for %u/%u, fix your config\n",
				worker->slot.bank_id, worker->slot.slot_nr);
			rc = -1;
			goto end;
		}
	}
	OSMO_ASSERT(worker->reader.name);

	if (!worker->reader.pcsc.hContext) {
		LOGW(worker, "Attempting to open PC/SC context\n");
		/* The PC/SC context must be created inside the thread where we'll later use it */
		rc = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &worker->reader.pcsc.hContext);
		PCSC_ERROR(worker, rc, "SCardEstablishContext")
	}

	if (!worker->reader.pcsc.hCard) {
		LOGW(worker, "Attempting to open card/slot '%s'\n", worker->reader.name);
		DWORD dwActiveProtocol;
		rc = SCardConnect(worker->reader.pcsc.hContext, worker->reader.name, SCARD_SHARE_SHARED,
				  SCARD_PROTOCOL_T0, &worker->reader.pcsc.hCard, &dwActiveProtocol);
		PCSC_ERROR(worker, rc, "SCardConnect")
	}

	worker_set_state(worker, BW_ST_CONN_CLIENT_MAPPED_CARD);
	/* FIXME: notify client about this state change */

	return 0;
end:
	return rc;
}


static int blocking_ipa_read(struct bankd_worker *worker, uint8_t *buf, unsigned int buf_size)
{
	struct ipaccess_head *hh;
	uint16_t len;
	int needed, rc;

	if (buf_size < sizeof(*hh))
		return -1;

	hh = (struct ipaccess_head *) buf;

	/* we use 'recv' and not 'read' below, as 'recv' will always fail with -EINTR
	 * in case of a signal being received */

restart_hdr:
	/* 1) blocking recv from the socket (IPA header) */
	rc = recv(worker->client.fd, buf, sizeof(*hh), 0);
	if (rc == -1 && errno == EINTR) {
		if (worker->state == BW_ST_CONN_CLIENT_UNMAPPED)
			return -23;
		goto restart_hdr;
	} else if (rc < 0)
		return rc;
	else if (rc < sizeof(*hh))
		return -2;

	len = ntohs(hh->len);
	needed = len; //- sizeof(*hh);

restart_body:
	/* 2) blocking recv from the socket (payload) */
	rc = recv(worker->client.fd, buf+sizeof(*hh), needed, 0);
	if (rc == -1 && errno == EINTR) {
		if (worker->state == BW_ST_CONN_CLIENT_UNMAPPED)
			return -23;
		goto restart_body;
	} else if (rc < 0)
		return rc;
	else if (rc < needed)
		return -3;

	return len;
}

static int worker_send_rspro(struct bankd_worker *worker, RsproPDU_t *pdu)
{
	struct msgb *msg = rspro_enc_msg(pdu);
	int rc;

	if (!msg) {
		LOGW(worker, "error encoding RSPRO\n");
		return -1;
	}

	msg->l2h = msg->data;
	/* prepend the header */
	ipa_prepend_header_ext(msg, IPAC_PROTO_EXT_RSPRO);
	ipa_prepend_header(msg, IPAC_PROTO_OSMO);

	/* actually send it through the socket */
	rc = write(worker->client.fd, msgb_data(msg), msgb_length(msg));
	if (rc == msgb_length(msg))
		rc = 0;
	else {
		LOGW(worker, "error during write: %d != %d\n", rc, msgb_length(msg));
		rc = -1;
	}

	msgb_free(msg);

	return rc;
}

/* attempt to obtain slot-map */
static int worker_try_slotmap(struct bankd_worker *worker)
{
	struct slot_mapping *slmap;

	slmap = slotmap_by_client(worker->bankd->slotmaps, &worker->client.clslot);
	if (!slmap) {
		LOGW(worker, "No slotmap (yet) for client C(%u:%u)\n",
			worker->client.clslot.client_id, worker->client.clslot.slot_nr);
		/* check in 10s if the map has been installed meanwhile by main thread */
		worker_set_state_timeout(worker, BW_ST_CONN_CLIENT_WAIT_MAP, 10);
		return -1;
	} else {
		LOGW(worker, "slotmap found: C(%u:%u) -> B(%u:%u)\n",
			slmap->client.client_id, slmap->client.slot_nr,
			slmap->bank.bank_id, slmap->bank.slot_nr);
		worker->slot = slmap->bank;
		worker_set_state_timeout(worker, BW_ST_CONN_CLIENT_MAPPED, 10);
		return worker_open_card(worker);
	}
}


static int worker_handle_connectClientReq(struct bankd_worker *worker, const RsproPDU_t *pdu)
{
	const struct ComponentIdentity *cid = &pdu->msg.choice.connectClientReq.identity;
	RsproPDU_t *resp = NULL;
	e_ResultCode res;
	int rc;

	OSMO_ASSERT(pdu->msg.present == RsproPDUchoice_PR_connectClientReq);

	LOGW(worker, "connectClientReq(T=%lu, N='%s', SW='%s', VER='%s')\n",
		cid->type, cid->name.buf, cid->software.buf, cid->swVersion.buf);
	/* FIXME: store somewhere? */

	if (worker->state != BW_ST_CONN_WAIT_ID) {
		LOGW(worker, "Unexpected connectClientReq\n");
		rc = -102;
		goto respond_and_err;
	}

	if (!pdu->msg.choice.connectClientReq.clientSlot) {
		LOGW(worker, "missing clientID, aborting\n");
		res = ResultCode_illegalClientId;
		rc = -103;
		goto respond_and_err;
	}
	worker->client.clslot.client_id = pdu->msg.choice.connectClientReq.clientSlot->clientId;
	worker->client.clslot.slot_nr = pdu->msg.choice.connectClientReq.clientSlot->slotNr;
	worker_set_state(worker, BW_ST_CONN_CLIENT);

	if (worker_try_slotmap(worker) >= 0)
		res = ResultCode_ok;
	else
		res = ResultCode_cardNotPresent;

	resp = rspro_gen_ConnectClientRes(&worker->bankd->comp_id, res);
	return worker_send_rspro(worker, resp);

respond_and_err:
	if (res) {
		resp = rspro_gen_ConnectClientRes(&worker->bankd->comp_id, res);
		worker_send_rspro(worker, resp);
	}
	return rc;
}

static int worker_handle_tpduModemToCard(struct bankd_worker *worker, const RsproPDU_t *pdu)
{
	const struct TpduModemToCard *mdm2sim = &pdu->msg.choice.tpduModemToCard;
	const SCARD_IO_REQUEST *pioSendPci = SCARD_PCI_T0;
	SCARD_IO_REQUEST pioRecvPci;
	uint8_t rx_buf[1024];
	DWORD rx_buf_len = sizeof(rx_buf);
	RsproPDU_t *pdu_resp;
	long rc;

	LOGW(worker, "tpduModemToCard(%s)\n", osmo_hexdump_nospc(mdm2sim->data.buf, mdm2sim->data.size));

	if (worker->state != BW_ST_CONN_CLIENT_MAPPED_CARD) {
		LOGW(worker, "Unexpected tpduModemToCaard\n");
		return -104;
	}

	/* FIXME: Validate that toBankSlot / fromClientSlot match our expectations */

	rc = SCardTransmit(worker->reader.pcsc.hCard,
			   pioSendPci, mdm2sim->data.buf, mdm2sim->data.size,
			   &pioRecvPci, rx_buf, &rx_buf_len);
	PCSC_ERROR(worker, rc, "SCardTransmit");

	/* encode response PDU and send it */
	pdu_resp = rspro_gen_TpduCard2Modem(&mdm2sim->toBankSlot, &mdm2sim->fromClientSlot,
					    rx_buf, rx_buf_len);
	worker_send_rspro(worker, pdu_resp);

	return 0;
end:
	return rc;
}

/* handle one incoming RSPRO message from a client inside a worker thread */
static int worker_handle_rspro(struct bankd_worker *worker, const RsproPDU_t *pdu)
{
	int rc = -100;

	switch (pdu->msg.present) {
	case RsproPDUchoice_PR_connectClientReq:
		rc = worker_handle_connectClientReq(worker, pdu);
		break;
	case RsproPDUchoice_PR_tpduModemToCard:
		rc = worker_handle_tpduModemToCard(worker, pdu);
		break;
	case RsproPDUchoice_PR_clientSlotStatusInd:
		/* FIXME */
		break;
	default:
		rc = -101;
		break;
	}

	return rc; 
}

static int wait_for_fd_or_timeout(int fd, unsigned int timeout_secs)
{
	struct timeval tout = { timeout_secs, 0 };
	fd_set readset;

	FD_ZERO(&readset);
	FD_SET(fd, &readset);
	return select(fd + 1, &readset, NULL, NULL, timeout_secs ? &tout : NULL);
}

/* body of the main transceive loop */
static int worker_transceive_loop(struct bankd_worker *worker)
{
	struct ipaccess_head *hh;
	struct ipaccess_head_ext *hh_ext;
	uint8_t buf[65536]; /* maximum length expressed in 16bit length field */
	asn_dec_rval_t rval;
	int data_len, rc;
	RsproPDU_t *pdu = NULL;

restart_wait:
	rc = wait_for_fd_or_timeout(worker->client.fd, worker->timeout);
	if (rc == -1 && errno == EINTR) {
		if (worker->state == BW_ST_CONN_CLIENT_UNMAPPED)
			return -23;
		goto restart_wait;
	} else if (rc < 0)
		return rc;
	else if (rc == 0) {
		/* TIMEOUT case */
		switch (worker->state) {
		case BW_ST_CONN_CLIENT_WAIT_MAP:
			/* re-check if mapping exists meanwhile? */
			worker_try_slotmap(worker);
			break;
		case BW_ST_CONN_CLIENT_MAPPED:
			/* re-check if reader/card can be opened meanwhile? */
			worker_open_card(worker);
			break;
		default:
			OSMO_ASSERT(0);
		}
		/* return early, so we do another select rather than the blocking read below */
		return 0;
	};

	/* 1) blocking read of entire IPA message from the socket */
	rc = blocking_ipa_read(worker, buf, sizeof(buf));
	if (rc < 0)
		return rc;
	data_len = rc;

	hh = (struct ipaccess_head *) buf;
	if (hh->proto != IPAC_PROTO_OSMO && hh->proto != IPAC_PROTO_IPACCESS) {
		LOGW(worker, "Received unsupported IPA protocol != OSMO: 0x%02x\n", hh->proto);
		return -4;
	}

	if (hh->proto == IPAC_PROTO_IPACCESS) {
		switch (hh->data[0]) {
		case IPAC_MSGT_PING:
			return ipa_ccm_send_pong(worker->client.fd);
		default:
			LOGW(worker, "IPA CCM 0x%02x not implemented yet\n", hh->data[0]);
			break;
		}
		return 0;
	}

	hh_ext = (struct ipaccess_head_ext *) buf + sizeof(*hh);
	if (data_len < sizeof(*hh_ext)) {
		LOGW(worker, "Received short message\n");
		return -5;
	}
	data_len -= sizeof(*hh_ext);
	if (hh_ext->proto != IPAC_PROTO_EXT_RSPRO) {
		LOGW(worker, "Received unsupported IPA EXT protocol != RSPRO: 0x%02x\n", hh_ext->proto);
		return -6;
	}

	/* 2) ASN1 BER decode of the message */
	rval = ber_decode(NULL, &asn_DEF_RsproPDU, (void **) &pdu, hh_ext->data, data_len);
	if (rval.code != RC_OK) {
		LOGW(worker, "Error during BER decode of RSPRO\n");
		return -7;
	}

	/* 3) handling of the message, possibly resulting in PCSC commands */
	rc = worker_handle_rspro(worker, pdu);
	ASN_STRUCT_FREE(asn_DEF_RsproPDU, pdu);
	if (rc < 0) {
		LOGW(worker, "Error handling RSPRO\n");
		return rc;
	}

	/* everything OK if we reach here */
	return 0;
}

/* obtain an ascii representation of the client IP/port */
static int worker_client_addrstr(char *out, unsigned int outlen, const struct bankd_worker *worker)
{
	char hostbuf[32], portbuf[32];
	int rc;

	rc = getnameinfo((const struct sockaddr *)&worker->client.peer_addr,
			 worker->client.peer_addr_len, hostbuf, sizeof(hostbuf),
			 portbuf, sizeof(portbuf), NI_NUMERICHOST | NI_NUMERICSERV);
	if (rc != 0) {
		out[0] = '\0';
		return -1;
	}
	snprintf(out, outlen, "%s:%s", hostbuf, portbuf);
	return 0;
}

/* worker thread main function */
static void *worker_main(void *arg)
{
	void *top_ctx;
	int rc;

	g_worker = (struct bankd_worker *) arg;

	worker_set_state(g_worker, BW_ST_INIT);

	/* not permitted in multithreaded environment */
	talloc_disable_null_tracking();
	g_worker->tall_ctx = talloc_named_const(NULL, 0, "top");
	talloc_asn1_ctx = talloc_named_const(g_worker->tall_ctx, 0, "asn1");

	/* set the thread name */
	g_worker->name = talloc_asprintf(g_worker->tall_ctx, "bankd-worker(%u)", g_worker->num);
	pthread_setname_np(pthread_self(), g_worker->name);

	/* push cleanup helper */
	pthread_cleanup_push(&worker_cleanup, g_worker);

	/* we continuously perform the same loop here, recycling the worker thread
	 * once the client connection is gone or we have some trouble with the card/reader */
	while (1) {
		char buf[128];

		g_worker->client.peer_addr_len = sizeof(g_worker->client.peer_addr);

		worker_set_state(g_worker, BW_ST_ACCEPTING);
		/* first wait for an incoming TCP connection */
		rc = accept(g_worker->bankd->accept_fd, (struct sockaddr *) &g_worker->client.peer_addr,
			    &g_worker->client.peer_addr_len);
		if (rc < 0) {
			continue;
		}
		g_worker->client.fd = rc;
		worker_client_addrstr(buf, sizeof(buf), g_worker);
		LOGW(g_worker, "Accepted connection from %s\n", buf);
		worker_set_state(g_worker, BW_ST_CONN_WAIT_ID);

		/* run the main worker transceive loop body until there was some error */
		while (1) {
			rc = worker_transceive_loop(g_worker);
			if (rc < 0)
				break;
			if (g_worker->state == BW_ST_CONN_CLIENT_UNMAPPED)
				break;
		}

		LOGW(g_worker, "Error %d occurred: Cleaning up state\n", rc);

		/* clean-up: reset to sane state */
		if (g_worker->reader.pcsc.hCard) {
			SCardDisconnect(g_worker->reader.pcsc.hCard, SCARD_UNPOWER_CARD);
			g_worker->reader.pcsc.hCard = 0;
		}
		if (g_worker->reader.pcsc.hContext) {
			SCardReleaseContext(g_worker->reader.pcsc.hContext);
			g_worker->reader.pcsc.hContext = 0;
		}
		if (g_worker->reader.name)
			g_worker->reader.name = NULL;
		if (g_worker->client.fd >= 0)
			close(g_worker->client.fd);
		memset(&g_worker->client.peer_addr, 0, sizeof(g_worker->client.peer_addr));
		g_worker->client.fd = -1;
		g_worker->client.clslot.client_id = g_worker->client.clslot.slot_nr = 0;
	}

	pthread_cleanup_pop(1);
	talloc_free(top_ctx);
	pthread_exit(NULL);
}
